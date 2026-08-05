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
  /* LED is ACTIVE-LOW and currently disabled: drive HIGH so it stays dark
     from reset. (CubeMX default is RESET, which would light it.) */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

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

  /* PB9 (IMU INT1): analog, and NEVER an internal pull -- the net already has
     an external 4.7k pull-up (R1) and an internal pull would fight it. */
  GPIO_InitStruct.Pin       = GPIO_PIN_9;
  GPIO_InitStruct.Mode      = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = 0;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ---- Unconnected pads: define a level, do not leave them floating ----
     On WB0 silicon a pad left Hi-Z with no pull drifts to mid-rail in
     DEEPSTOP and leaks. Enabling a PWR-domain pull-down on every unused pad
     is what took the floor from ~880 uA to ~130 uA.

     Pin sets copied verbatim from the 130 uA build -- note PA9/PB0 are in
     here: the .ioc calls them USART1 TX/RX, but main.c never calls
     MX_USART1_UART_Init(), so they are unconnected AF pads and must be
     pulled like any other unused pin.
     Excluded: PA2/PA3 (SWD), PA10 (BOOT, re-sampled on DEEPSTOP wake),
     PB3 (LED, pull-UP below), PB9 (INT1), PB10/PB11 (I2C), PB12/PB13 (LSE),
     OSCIN/OSCOUT (HSE). PB8 is set analog but gets NO pull -- external 4.7k. */
  GPIO_InitStruct.Pin   = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                        | GPIO_PIN_9|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                        | GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode  = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;          /* run-mode: input buffer off */
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin   = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5
                        | GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_14|GPIO_PIN_15;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* DEEPSTOP: PWR-domain pull-down holds each of those pads at a defined level.
     PB8 deliberately absent -- external 4.7k already defines it. */
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A,
        PWR_GPIO_BIT_4|PWR_GPIO_BIT_5|PWR_GPIO_BIT_6|PWR_GPIO_BIT_7|PWR_GPIO_BIT_8
      | PWR_GPIO_BIT_9|PWR_GPIO_BIT_11|PWR_GPIO_BIT_12|PWR_GPIO_BIT_13
      | PWR_GPIO_BIT_14|PWR_GPIO_BIT_15);
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B,
        PWR_GPIO_BIT_0|PWR_GPIO_BIT_1|PWR_GPIO_BIT_2|PWR_GPIO_BIT_4|PWR_GPIO_BIT_5
      | PWR_GPIO_BIT_6|PWR_GPIO_BIT_7|PWR_GPIO_BIT_14|PWR_GPIO_BIT_15);

  /* PB3 = LED, ACTIVE-LOW. Pull UP (not down) so the pad is held high = LED
     off through DEEPSTOP; a pull-down here would light it. */
  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_3);
  HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_3);

  /**/
  HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_2);

  /*RT DEBUG GPIO_Init */
  RT_DEBUG_GPIO_Init();


}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
