/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Stub implementation of LED driver for Mindgrove platform.
 */
/*************************************************************************************************/

#include "pal_led.h"
#include <stdio.h>

/**************************************************************************************************
  Macros & Local Definitions
**************************************************************************************************/

// Define custom LED GPIOs or platform-specific identifiers here if needed.
#define LED_GPIO_COUNT 2

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the LED system.
 */
/*************************************************************************************************/
void PalLedInit(void)
{
  // Stub: Initialize GPIOs if applicable.
  // On real hardware, configure GPIOs as outputs and turn them off.
  printf("[pal_led] Init called\n");
}

/*************************************************************************************************/
/*!
 *  \brief  De-initialize the LED system.
 */
/*************************************************************************************************/
void PalLedDeInit(void)
{
  // Stub: Release LED resources if needed.
  printf("[pal_led] DeInit called\n");
}

/*************************************************************************************************/
/*!
 *  \brief  Turn on a specific LED by ID.
 *
 *  \param  id    LED ID to turn on.
 */
/*************************************************************************************************/
void PalLedOn(uint8_t id)
{
  // Stub: Replace with actual GPIO set low
  printf("[pal_led] LED ON: ID=%u\n", id);
}

/*************************************************************************************************/
/*!
 *  \brief  Turn off a specific LED by ID.
 *
 *  \param  id    LED ID to turn off.
 */
/*************************************************************************************************/
void PalLedOff(uint8_t id)
{
  // Stub: Replace with actual GPIO set high
  printf("[pal_led] LED OFF: ID=%u\n", id);
}
