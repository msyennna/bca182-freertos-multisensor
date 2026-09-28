#pragma once

#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart1;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim4;

#ifdef __cplusplus
extern "C" {
#endif
extern TIM_HandleTypeDef htim2;
#ifdef __cplusplus
}
#endif

void Hardware_Init(void);
bool Hardware_VectorTableOk(void);
void Buzzer_Set(bool enabled);
