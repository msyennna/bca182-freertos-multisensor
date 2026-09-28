#include "hardware.h"

#include "main.h"
#include "serial_log.h"

#include "FreeRTOS.h"
#include "task.h"

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim4;

namespace {

void MX_GPIO_Init(void);
void MX_ADC1_Init(void);
void MX_I2C1_Init(void);
void MX_TIM4_Init(void);
void MX_BuzzerPWM_Init(void);

} // namespace

bool Hardware_VectorTableOk(void)
{
    return SCB->VTOR == FLASH_BASE;
}

void Hardware_Init(void)
{
    /*
     * Keep the STM32F103 on its reset-default 8 MHz HSI clock.  Do not perform
     * an additional RCC clock-tree transition here.  Wokwi already models the
     * reset clock correctly, and leaving it alone is both lighter and more
     * robust than repeatedly switching clock sources during startup.
     */
    Serial_WriteRaw("[Boot] HAL_Init...\r\n");
    HAL_Init();
    Serial_WriteRaw("[Boot] HAL_Init OK\r\n");

    SCB->VTOR = FLASH_BASE;
    __DSB();
    __ISB();

    /* SystemCoreClock is 8 MHz at reset. Refresh the CMSIS value only. */
    SystemCoreClockUpdate();

    Serial_WriteRaw("[Boot] GPIO init...\r\n");
    MX_GPIO_Init();
    Serial_WriteRaw("[Boot] GPIO OK\r\n");

    Serial_WriteRaw("[Boot] ADC init...\r\n");
    MX_ADC1_Init();
    Serial_WriteRaw("[Boot] ADC OK\r\n");

    Serial_WriteRaw("[Boot] I2C init...\r\n");
    MX_I2C1_Init();
    Serial_WriteRaw("[Boot] I2C OK\r\n");

    Serial_WriteRaw("[Boot] DHT safe timer init...\r\n");
    MX_TIM4_Init();
    Serial_WriteRaw("[Boot] DHT safe timer OK\r\n");

    Serial_WriteRaw("[Boot] Buzzer PWM init...\r\n");
    MX_BuzzerPWM_Init();
    Buzzer_Set(false);
    Serial_WriteRaw("[Boot] Buzzer PWM OK\r\n");
}

void Buzzer_Set(bool enabled)
{
    /* TIM2 is the CubeMX-generated 1 MHz HAL time base. Channel 3 shares the
     * same counter and can therefore provide a 1 kHz PWM signal on PB10. */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, enabled ? 500U : 0U);
}

namespace {

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_TIM2_ENABLE();

    /* DHT22 data. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PIR sensor. */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Rotary encoder: CLK=PB12, DT=PB13. */
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB10 = TIM2_CH3 buzzer PWM. */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {};

    /* At 8 MHz HSI with ADC prescaler /2, ADC clock is 4 MHz. */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_ADCPRE, RCC_ADCPCLK2_DIV2);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;

    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}


void MX_TIM4_Init(void)
{
    /* Dedicated 1 MHz free-running timer for DHT22 pulse timing. */
    __HAL_RCC_TIM4_CLK_ENABLE();

    uint32_t timerClock = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) {
        timerClock *= 2U;
    }

    uint32_t prescaler = timerClock / 1000000U;
    if (prescaler == 0U) {
        prescaler = 1U;
    }

    TIM4->CR1 = 0U;
    TIM4->PSC = static_cast<uint16_t>(prescaler - 1U);
    TIM4->ARR = 0xFFFFU;
    TIM4->CNT = 0U;
    TIM4->EGR = TIM_EGR_UG;
    TIM4->SR = 0U;
    TIM4->CR1 = TIM_CR1_CEN;

    htim4.Instance = TIM4;
}

void MX_BuzzerPWM_Init(void)
{
    /* TIM2 is already running as the 1 MHz HAL time base with ARR=999. */
    MODIFY_REG(TIM2->CCMR2,
               TIM_CCMR2_CC3S | TIM_CCMR2_OC3M | TIM_CCMR2_OC3PE,
               TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE);
    TIM2->CCR3 = 0U;
    SET_BIT(TIM2->CCER, TIM_CCER_CC3E);
}

} // namespace

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        HAL_IncTick();
    }
}

extern "C" void Error_Handler(void)
{
    Serial_WriteRaw("[ERROR] Hardware initialization failed\r\n");
    __disable_irq();
    while (1) {
    }
}
