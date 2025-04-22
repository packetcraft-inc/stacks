/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : gpio.h
 * Brief Description of file         : Header to Standard gpio driver
 * Name of Author                    : Kapil Shyam. M, Shri Mahaalakshmi S J
 * Email ID                          : kapilshyamm@gmail.com, mahaalakshmi@mindgrovetech.in 
 * 
 * @file gpio.h
 * @author Kapil Shyam. M (kapilshyamm@gmail.com)
 * @author Shri Mahaalakshmi S J (mahaalakshmi@mindgrovetech.in)
 * @brief This is a Baremetal GPIO Driver's Header file for Mindgrove Silicon's GPIO Peripheral
 * @version 0.2
 * @date 2024-09-23
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.
 * 
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "secure_iot.h"
#include "pinmux.h"
#include "log.h"
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_IN  0
#define GPIO_OUT 1
#define ALL_GPIO_PINS -1

/**
 * @fn GPIO_Config(bool direction, uint8_t no_of_pins, ...)
 *
 * @brief The function `GPIO_Config` initializes the GPIO instance and set the direction.
 * 
 * @details It is used to initializes the GPIO instance and set the direction of a GPIO pin 
 * which configure the GPIO port as input or output.  
 * 
 * @param direction The direction parameter is an bool value that specifies the direction of the
 * GPIO pin. It can be either 0 or 1, where 0 represents input and 1 represents output.
 * 
 * @param no_of_pins The `no_of_pins` parameter specifies the number of pins that you want to set in
 * the GPIO_DIRECTION register.
 */
uint8_t GPIO_Config(bool direction, uint8_t no_of_pins, ...);

/**
 * @fn GPIO_Pin_Set(uint8_t no_of_pins, ...)
 * 
 * @brief The function `GPIO_Pin_Set` sets multiple GPIO pins based on the input arguments provided.
 * 
 * @details It is used to set the value of a GPIO pin in a GPIO instance, it also sets for multiple 
 * GPIO pins.
 * 
 * @param no_of_pins The `no_of_pins` parameter specifies the number of pins that you want to set in
 * the GPIO_SET register.
 */
uint8_t GPIO_Pin_Set(uint8_t no_of_pins, ...);

/**
 * @fn GPIO_Pin_Clear(uint8_t no_of_pins, ...)
 * 
 * @brief The function `GPIO_Pin_Clear` clears multiple GPIO pins based on the input arguments provided.
 * 
 * @details It is used to clear the value of a GPIO pin in a GPIO instance, it also clears for multiple 
 * GPIO pins.
 * 
 * @param no_of_pins The `no_of_pins` parameter specifies the number of pins that you want to clear in
 * the GPIO_CLEAR register.
 */
uint8_t GPIO_Pin_Clear(uint8_t no_of_pins, ...);

/**
 * @fn GPIO_Pin_Toggle(uint8_t no_of_pins, ...)
 * 
 * @brief The function `GPIO_Pin_Toggle` toggles multiple GPIO pins based on the input arguments provided.
 * 
 * @details It is used to toggle the value of a GPIO pin in a GPIO instance, it also toggle for multiple 
 * GPIO pins.
 * 
 * @param no_of_pins The `no_of_pins` parameter specifies the number of pins that you want to toggle in
 * the GPIO_TOGGLE register.
 */
uint8_t GPIO_Pin_Toggle(uint8_t no_of_pins, ...);

/**
 * @fn GPIO_Interrupt_Config(uint8_t gpio_pin, uint8_t low_ena)
 *
 * @brief The function `GPIO_Interrupt_Config` configures the interrupt for a specific GPIO pin 
 * 
 * @details It is used to configure the interrupt for GPIO pins based on whether it is 
 * active low or active high.
 * 
 * @param gpio_pin The pin number for which the interrupt configuration is being set.
 *
 * @param low_ena This parameter is a boolean value that determines whether the interrupt is active low
 * or active high. If it is set to true (non-zero), the interrupt is active low, and if it is set to
 * false (zero), the interrupt is active high.
 */
uint8_t GPIO_Interrupt_Config(uint8_t gpio_pin, uint8_t low_ena);

/**
 * @fn GPIO_Read_Data()
 *
 * @brief The function `GPIO_Read_Data` returns the value of the data register in a GPIO instance.
 * 
 * @details It is used to return the value of the GPIO_DATA register
 * 
 * @return It returns the value of the `data` register of the `gpio_instance`. 
 */
uint32_t GPIO_Read_Data();

/**
 * @fn GPIO_Read_Pin_Status(uint32_t gpio_pin)
 * 
 * @brief The function `GPIO_Read_Pin_Status` reads the status of GPIO instance.
 * 
 * @details It is used to read the status of the specific pin in the GPIO instance
 * 
 * @param gpio_pin The pin number to read the status from.
 * 
 * @return It returns 1 if the pin is set (high), 0 if the pin is cleared (low).
 */
uint32_t GPIO_Read_Pin_Status(uint32_t gpio_pin);

/**
 * @fn GPIO_Write_Data(uint8_t data_word)
 *
 * @brief The function `GPIO_Write_Data` sets the data register of a GPIO instance to a given data.
 * 
 * @details It is used to sets the data register of a GPIO instance to a given data.
 * 
 * @param data_word It represents the data to be written to the data register of a GPIO module. The data register 
 * is a hardware register that holds the current state of the GPIO pins. Writing to this register sets the output
 * values of the GPIO pins.
 */
uint32_t GPIO_Write_Data(uint32_t data_word);

#ifdef __cplusplus
}
#endif

#endif