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

  /* IMU INT1 -> PB9. Two configurations:
       TRAINING   : rising-edge EXTI + PWR wakeup, DRDY paces sampling.
       PRODUCTION : ANALOG + pull-down, exactly as in the build that reached
                    130 uA. Sampling is virtual-timer driven there, so the pin
                    is not needed and a live EXTI input on it is what keeps the
                    device off the DEEPSTOP floor. */
  GPIO_InitStruct.Pin       = IMU_INT1_Pin;
#if TRAINING_MODE_ENABLED
  GPIO_InitStruct.Mode      = GPIO_MODE_IT_RISING;
#else
  GPIO_InitStruct.Mode      = GPIO_MODE_ANALOG;
#endif
  /* NO internal pull on PB9. The board already defines this net with an
     external 4.7k pull-up (R1, IMU_INT1). An internal pull-down here fights
     it continuously -- that divider costs ~700 uA and is why PB8/PB9/PB10/PB11
     are carved out of the "pull every floating pad" rule that took the floor
     from 880 uA to 130 uA. Floating-pad leakage does not apply to these four:
     they are externally defined. */
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = 0;
  HAL_GPIO_Init(IMU_INT1_GPIO_Port, &GPIO_InitStruct);

  /* Leave both PWR pad pulls OFF on PB9, for the same reason. */
  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_9);
  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_9);

#if TRAINING_MODE_ENABLED
  HAL_NVIC_SetPriority(IMU_INT1_EXTI_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(IMU_INT1_EXTI_IRQn);

  /* EXTI is unpowered in DEEPSTOP, so PB9 must also be a PWR wakeup source. */
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PB9, PWR_WUP_RISIEDG);
#else
  /* Nothing may wake the CPU from this pin in production. */
  HAL_NVIC_DisableIRQ(IMU_INT1_EXTI_IRQn);
  HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PB9);
#endif

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
