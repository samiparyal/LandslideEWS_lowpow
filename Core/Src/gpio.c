/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
     PA2   ------> DEBUG_SWDIO
     OSCOUT   ------> RCC_OSC_OUT
     OSCIN   ------> RCC_OSC_IN
     PB13   ------> RCC_OSC32_IN
     PB12   ------> RCC_OSC32_OUT
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SWDIO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* IMU data-ready interrupt : PB9 (LSM6DSV16X INT1).
     Was left in its reset state (analog) while the .ioc already enabled
     GPIOB_IRQn -- configure it explicitly as a rising-edge EXTI input.
     INT1 is push-pull active-high on the sensor, so pull DOWN (not up) keeps
     the line defined while the sensor is idle and prevents spurious edges. */
  GPIO_InitStruct.Pin       = IMU_INT1_Pin;
  GPIO_InitStruct.Mode      = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = 0;
  HAL_GPIO_Init(IMU_INT1_GPIO_Port, &GPIO_InitStruct);

  /* Match the pad-level pull to the EXTI config (WB0 PWR keeps its own
     pull settings that survive low-power modes). */
  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_9);
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_9);

  HAL_NVIC_SetPriority(IMU_INT1_EXTI_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(IMU_INT1_EXTI_IRQn);

  /* EXTI logic is unpowered in low-power mode, so the
     NVIC/EXTI config above only covers Run/Idle -- PB9 must also be registered
     with the PWR block or DRDY will stop waking the device once CFG_LPM_SUPPORTED
     is turned on. */
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PB9, PWR_WUP_RISIEDG);

  /**/
  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_3);

  /**/
  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_3);

  /**/
  HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_2);

  /*RT DEBUG GPIO_Init */
  RT_DEBUG_GPIO_Init();


}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
