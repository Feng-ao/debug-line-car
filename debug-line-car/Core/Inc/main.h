/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY1_Pin GPIO_PIN_0
#define KEY1_GPIO_Port GPIOA
#define KEY2_Pin GPIO_PIN_1
#define KEY2_GPIO_Port GPIOA
#define KEY3_Pin GPIO_PIN_2
#define KEY3_GPIO_Port GPIOA
#define KEY4_Pin GPIO_PIN_3
#define KEY4_GPIO_Port GPIOA
#define BUZZER_Pin GPIO_PIN_4
#define BUZZER_GPIO_Port GPIOA
#define AIN1_Pin GPIO_PIN_5
#define AIN1_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_1
#define LED2_GPIO_Port GPIOB
#define X7_Pin GPIO_PIN_10
#define X7_GPIO_Port GPIOB
#define X1_Pin GPIO_PIN_12
#define X1_GPIO_Port GPIOB
#define X2_Pin GPIO_PIN_13
#define X2_GPIO_Port GPIOB
#define X3_Pin GPIO_PIN_14
#define X3_GPIO_Port GPIOB
#define X4_Pin GPIO_PIN_15
#define X4_GPIO_Port GPIOB
#define AIN2_Pin GPIO_PIN_8
#define AIN2_GPIO_Port GPIOA
#define BIN1_Pin GPIO_PIN_11
#define BIN1_GPIO_Port GPIOA
#define BIN2_Pin GPIO_PIN_12
#define BIN2_GPIO_Port GPIOA
#define X8_Pin GPIO_PIN_5
#define X8_GPIO_Port GPIOB
#define X5_Pin GPIO_PIN_8
#define X5_GPIO_Port GPIOB
#define X6_Pin GPIO_PIN_9
#define X6_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
