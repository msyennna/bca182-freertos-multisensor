/*
 * FreeRTOS Kernel V10.3.1 - STM32F103/Wokwi hybrid scheduler port
 *
 * Design used for this laboratory build:
 * - Native FreeRTOS kernel/APIs remain unchanged.
 * - TIM3 provides the 20 Hz RTOS/HAL time base.
 * - TIM3 never performs a task context switch inside the ISR.
 * - Blocking/yielding FreeRTOS calls switch tasks directly from Thread mode.
 * - The Idle hook sleeps with WFI and yields after each timer wake-up.
 *
 * This avoids the SVC/PendSV path that failed in the local Wokwi STM32F103
 * simulation, while also avoiding the CPU-heavy polling loop used in earlier
 * compatibility builds.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"

extern void * volatile pxCurrentTCB;
extern __IO uint32_t uwTick;

static UBaseType_t uxCriticalNesting = 0U;

/*
 * Set only when a TIM3 tick actually makes a higher-priority task Ready.
 * This prevents the Idle hook from performing an expensive direct context
 * switch on every pass through the idle loop.
 */
static volatile BaseType_t xTickYieldPending = pdFALSE;

static void prvTaskExitError(void) __attribute__((used, noinline));
static void prvTaskBootstrap(void) __attribute__((naked));
static void prvStartFirstTask(void) __attribute__((naked));
static void prvSelectNextTask(void) __attribute__((used, noinline));

/*
 * Software context frame, 10 words / 40 bytes (8-byte aligned):
 *   [0..7]  r4-r11
 *   [8]     resume address
 *   [9]     alignment pad
 *
 * For a never-run task:
 *   r4 = task entry
 *   r5 = task parameter
 *   LR = prvTaskBootstrap
 */
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                   TaskFunction_t pxCode,
                                   void *pvParameters)
{
    pxTopOfStack -= 10;

    pxTopOfStack[0] = (StackType_t)pxCode;
    pxTopOfStack[1] = (StackType_t)pvParameters;
    pxTopOfStack[2] = 0x06060606UL;
    pxTopOfStack[3] = 0x07070707UL;
    pxTopOfStack[4] = 0x08080808UL;
    pxTopOfStack[5] = 0x09090909UL;
    pxTopOfStack[6] = 0x10101010UL;
    pxTopOfStack[7] = 0x11111111UL;
    pxTopOfStack[8] = ((StackType_t)prvTaskBootstrap) | 1UL;
    pxTopOfStack[9] = 0U;

    return pxTopOfStack;
}

static void prvTaskExitError(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

static void prvTaskBootstrap(void)
{
    __asm volatile(
        " mov r0, r5                 \n"
        " blx r4                     \n"
        " bl prvTaskExitError        \n"
        " b .                        \n"
    );
}

static void prvSelectNextTask(void)
{
    vTaskSwitchContext();
}

/*
 * Direct Thread-mode context switch.
 * This is invoked only from normal FreeRTOS blocking/yield paths, never from
 * TIM3_IRQHandler().
 */
void vPortYieldDirect(void) __attribute__((naked));
void vPortYieldDirect(void)
{
    __asm volatile(
        " mrs r0, psp                         \n"
        " sub r0, r0, #40                    \n"
        " stmia r0, {r4-r11}                 \n"
        " str lr, [r0, #32]                  \n"
        " movs r1, #0                        \n"
        " str r1, [r0, #36]                  \n"
        " msr psp, r0                        \n"
        " ldr r3, =pxCurrentTCB              \n"
        " ldr r2, [r3]                       \n"
        " str r0, [r2]                       \n"
        " cpsid i                            \n"
        " bl prvSelectNextTask               \n"
        " cpsie i                            \n"
        " ldr r3, =pxCurrentTCB              \n"
        " ldr r2, [r3]                       \n"
        " ldr r0, [r2]                       \n"
        " ldmia r0!, {r4-r11}                \n"
        " ldr lr, [r0, #0]                   \n"
        " adds r0, r0, #8                    \n"
        " msr psp, r0                        \n"
        " isb                                \n"
        " bx lr                              \n"
    );
}

static void prvStartFirstTask(void)
{
    __asm volatile(
        " ldr r3, =pxCurrentTCB              \n"
        " ldr r2, [r3]                       \n"
        " ldr r0, [r2]                       \n"
        " ldmia r0!, {r4-r11}                \n"
        " ldr lr, [r0, #0]                   \n"
        " adds r0, r0, #8                    \n"
        " msr psp, r0                        \n"
        " movs r0, #2                        \n"
        " msr control, r0                    \n"
        " isb                                \n"
        " movs r0, #0                        \n"
        " msr basepri, r0                    \n"
        " cpsie i                            \n"
        " cpsie f                            \n"
        " bx lr                              \n"
    );
}

static void prvSetupTickTimer(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();

    TIM3->CR1 = 0U;

    /* Derive a 10 kHz TIM3 counter from the actual APB1 timer clock. */
    uint32_t timerClock = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) {
        timerClock *= 2U;
    }

    uint32_t prescaler = timerClock / 10000U;
    if (prescaler == 0U) {
        prescaler = 1U;
    }

    TIM3->PSC = (uint16_t)(prescaler - 1U);
    TIM3->ARR = (uint16_t)((10000U / configTICK_RATE_HZ) - 1U);
    TIM3->CNT = 0U;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR = 0U;
    TIM3->DIER = TIM_DIER_UIE;

    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM3_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(TIM3_IRQn);

    TIM3->CR1 = TIM_CR1_CEN;
}

/*
 * TIM3 advances FreeRTOS time only.  It deliberately does not switch task
 * context in interrupt mode.  Once the ISR returns, the Idle hook or the next
 * blocking FreeRTOS call performs a normal direct yield.
 */
void TIM3_IRQHandler(void)
{
    if ((TIM3->SR & TIM_SR_UIF) != 0U) {
        TIM3->SR &= ~TIM_SR_UIF;

        portDISABLE_INTERRUPTS();
        const BaseType_t switchRequired = xTaskIncrementTick();
        if (switchRequired != pdFALSE) {
            xTickYieldPending = pdTRUE;
        }
        portENABLE_INTERRUPTS();

        uwTick += (1000U / configTICK_RATE_HZ);
    }
}

/*
 * Called from the Idle hook in Thread mode.
 *
 * The old compatibility build called taskYIELD() on every Idle-hook pass.
 * If WFI returns immediately in the simulator, that becomes a tight loop of
 * full register saves/restores and can drive Wokwi down to ~1% speed.
 *
 * Consume the request only when a timer tick actually woke a task.
 */
BaseType_t xPortConsumeTickYield(void)
{
    BaseType_t pending;

    portDISABLE_INTERRUPTS();
    pending = xTickYieldPending;
    xTickYieldPending = pdFALSE;
    portENABLE_INTERRUPTS();

    return pending;
}

BaseType_t xPortStartScheduler(void)
{
    uxCriticalNesting = 0U;

    /* CubeMX TIM2 was used for HAL startup timing.  Keep TIM2 itself running
       because channel 3 drives the buzzer, but stop its 1 kHz IRQ. */
    TIM2->DIER &= ~TIM_DIER_UIE;
    NVIC_DisableIRQ(TIM2_IRQn);
    NVIC_ClearPendingIRQ(TIM2_IRQn);

    /* Cortex-M SysTick/SVC/PendSV are not used by this compatibility build. */
    SysTick->CTRL = 0U;

    prvSetupTickTimer();
    prvStartFirstTask();

    return pdFALSE;
}

void vPortEndScheduler(void)
{
}

void vPortEnterCritical(void)
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;
}

void vPortExitCritical(void)
{
    configASSERT(uxCriticalNesting > 0U);

    uxCriticalNesting--;
    if (uxCriticalNesting == 0U) {
        portENABLE_INTERRUPTS();
    }
}

/* Strong symbols retained because FreeRTOSConfig maps CMSIS exception names
   to these functions. They are intentionally unused in this Wokwi build. */
void vPortSVCHandler(void)
{
}

void xPortPendSVHandler(void)
{
}

void xPortSysTickHandler(void)
{
}

#if (configASSERT_DEFINED == 1)
void vPortValidateInterruptPriority(void)
{
    /* No application ISR calls FreeRTOS FromISR APIs in this laboratory. */
}
#endif
