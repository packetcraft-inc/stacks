/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : gptimer.h
 * Brief Description of file         : Header to Standard gp_timer driver
 * Name of Author                    : Suneeth D, Shri Mahaalakshmi S J
 * Email ID                          : suneethdamodharan@gmail.com, mahaalakshmi@mindgrovetech.in
 * 
 * @file gptimer.h
 * @author Suneeth D (suneethdamodharan@gmail.com)
 * @author Shri Mahaalakshmi S J (mahaalakshmi@mindgrovetech.in)
 * @brief This is a Baremetal GP_TIMER Driver's header file for Mindgrove Silicon's General Purpose Timer module.
 * @version 0.2
 * @date 2024-09-20
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.
 * 
 */

#ifndef GPT_H
#define GPT_H

#include <stdint.h>
#include "secure_iot.h"
#include "plic.h"
#include "log.h"
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPTIMER_0 0
#define GPTIMER_1 1
#define GPTIMER_2 2
#define GPTIMER_3 3

#define GPT_CONTIN_COUNT_EN 1 // For Up counter, Down counter and Updown counter
#define GPT_CONTIN_COUNT 0 // For PWM operation

#define GPT_OUT_EN 1 // PWM operation
#define GPT_OUT 0 // Up counter, Down counter, Updown counter

#define GPT_PWM_MODE 0 // PWM mode
#define GPT_UP_COUNT 1 // Up counter
#define GPT_DOWM_COUNT 2 // Down counter
#define GPT_UPDOWN_COUNT 3 // Updown counter

#define GPT_INTR_EN 1 // Interrupt enable
#define GPT_INTR_DIS 0 // Interrupt disable

typedef struct 
{
    // The parameter gpt_num is an unsigned integer that represents the GPT instance number.
    // @note gpt_num value must be always less than GPT_MAX_COUNT.
    uint32_t gpt_num;

    // The parameter mode is an unsigned integer that sets tCONTIN_CNT_ENhe mode of gp_timer
    // @note mode value must be always in the range 0<=mode<4.
    uint32_t mode;

    // The paramter interrupt_en is an unsigned integer used as a bool to enable the interrupt
    uint32_t interrupt_en;

    // The parameter period is an unsigned integer that sets the period of gp_timer till which it needs to count
    uint32_t period;

    // The parameter prescalar is an unsigned integer that sets the prescalar value with which the clock gets divided
    uint32_t prescalar;

    // The parameter dutycycle is an unsigned integer that sets the duty cycle value for the pulse generated in PWM
    uint32_t dutycycle;

    // The paramter cnt_en is unsigned integer that enables continous count mode of the gptimer
    uint32_t cnt_en;

    // The parameter capture_val is an unsigned integer that sets the input value to be captured
    uint32_t capture_val;

    // The parameter output_en is an unsigned integer that sets the output_enable high in the control and status register
    uint32_t output_en;
}GPTIMER_Config_t;

/**
 * @fn GPT_Init(GPTIMER_Config_t *gptimer)
 * 
 * @brief The function `GPT_Init` initialises the gp_timer with different modes and configures necessary registers.
 * 
 * @details It is used to select the specific GPT instance, set the mode, set the prescalar, set the dutycycle, enable
 * continous count and set the input bit to be captured.
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Init` function is a struct that
 * contains several properties related to configuring a General Purpose Timer (GPT).It has the following properties: 
 * gpt_num, mode, interrupt_en, period, prescalar, dutycycle, cnt_en, capture_val, output_en.
 */
uint32_t GPT_Init(GPTIMER_Config_t *gptimer);

/**
 * @fn GPT_Update_Enable(GPTIMER_Config_t *gptimer)
 * 
 * @brief The function `GPT_Update_Enable` checks the clock control register for specified register
 * 
 * @details It is used to checks and updates the clock control register for a specified GPTIMER
 * configuration.
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Update_Enable` function is a struct that
 * contains gpt_num.
 */
uint32_t GPT_Update_Enable(GPTIMER_Config_t *gptimer);

/**
 * @fn GPT_Read_Counter_Val(GPTIMER_Config_t *gptimer)
 * 
 * @brief The function `GPT_Read_Counter_Val` reads the value of the counter.
 * 
 * @details It is used to read the value of counter of a specific gpt instance at any time during its 
 * counting process. 
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Read_Counter_Val` function is a struct that
 * contains gpt_num.
 *  
 * @return returns a 32-bit counter value that is currently stored in the counter register
 */
uint32_t GPT_Read_Counter_Val(GPTIMER_Config_t *gptimer);

/**
 * @fn GPT_Read_ReptdCount(GPTIMER_Config_t *gptimer)
 * 
 * @brief The function `GPT_Read_ReptdCount` reads the value of repeated counts.
 * 
 * @details It is used to read the value of repeated counts counted by the counter of specific 
 * GPT instance when continous count is enabled
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Read_ReptdCount` function is a struct that
 * contains gpt_num.
 * 
 * @return returns a 32-bit repeated count value that is currently stored in the Repeated_Count register
 */
uint32_t GPT_Read_ReptdCount(GPTIMER_Config_t *gptimer);

/**
 * @fn GPT_Read_Captured_Val(GPTIMER_Config_t *gptimer)
 * 
 * @brief The function `GPT_Read_Captured_Val` reads captured counter value
 * 
 * @details It is used to read the value of counter value captured when a desired input is captured
 *  at a gpio pin
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Read_Captured_Val` function is a struct that
 * contains gpt_num.
 * 
 * @return returns a 32-bit counter value that is captured and currently stored in Capture_register
 */
uint32_t GPT_Read_Captured_Val(GPTIMER_Config_t *gptimer);

/**
 * @fn GPT_Reset(GPTIMER_Config_t *gptimer)
 * 
 * @brief The function `GPT_Reset` resets the specific GPT instance
 * 
 * @details It is used to clear all the interrupts and counter values and resets the mode of the specific
 * GPT instance.
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Reset` function is a struct that
 * contains gpt_num.
 */
uint32_t GPT_Reset(GPTIMER_Config_t *gptimer);

/**
 * @fn GPT_Delay_Milliseconds(GPTIMER_Config_t *gptimer, uint32_t delay)
 * 
 * @brief The function `GPT_Delay_Milliseconds` generates hardware delay in milliseconds.
 * 
 * @details It initializes a general-purpose timer with a specified period and prescalar, 
 * then waits for a specified delay in milliseconds.
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Delay_Milliseconds` function is a struct that
 * contains gpt_num, mode, interrupt_en, period, prescalar, dutycycle, cnt_en, capture_val, output_en.
 * 
 * @param delay The `delay` parameter in the `GPT_Delay_Milliseconds` function represents the time
 * delay in milliseconds. This function calculates the necessary period and prescalar values based on the provided 
 * delay to configure the timer accordingly.
 */
uint32_t GPT_Delay_Milliseconds(GPTIMER_Config_t *gptimer, uint32_t delay);

/**
 * @fn GPT_Delay_Millisecs_H(uint32_t delay)
 * 
 * @brief The function `GPT_Delay_Millisecs_H` generates hardware delay in milliseconds
 * 
 * @details The function `GPT_Delay_Millisecs_H` initializes a general-purpose timer with a specified 
 * delay in milliseconds and waits until the timer reaches zero.
 * 
 * @param delay The `delay` parameter in the `GPT_Delay_Millisecs_H` function represents the time delay
 * in milliseconds that you want the function to wait for. This delay is achieved using a
 * general-purpose timer (GPT) configured with a specific period based on the provided delay value.
 */
uint32_t GPT_Delay_Millisecs_H(uint32_t delay); 

/**
 * @fn GPT_Delay_Microseconds(GPTIMER_Config_t *gptimer, uint32_t delay)
 * 
 * @brief The function `GPT_Delay_Microseconds` generates hardware delay in microseconds.
 * 
 * @details It initializes a general-purpose timer with a specified period and prescalar, 
 * then waits for a specified delay in microseconds.
 * 
 * @param GPTIMER_Config_t The `GPTIMER_Config_t` parameter in the `GPT_Delay_Microseconds` function is a struct that
 * contains gpt_num, mode, interrupt_en, period, prescalar, dutycycle, cnt_en, capture_val, output_en.
 * 
 * @param delay The `delay` parameter in the `GPT_Delay_Microseconds` function represents the time
 * delay in microseconds. This function calculates the necessary period and prescalar values based on the provided 
 * delay to configure the timer accordingly.
 */
uint32_t GPT_Delay_Microseconds(GPTIMER_Config_t *gptimer, uint32_t delay);

/**
 * @fn GPT_Delay_Microsecs_H(uint32_t delay)
 * 
 * @brief The function `GPT_Delay_Microsecs_H` generates hardware delay in microseconds
 * 
 * @details The function `GPT_Delay_Microsecs_H` initializes a general-purpose timer with a specified 
 * delay in microseconds and waits until the timer reaches zero.
 * 
 * @param delay The `delay` parameter in the `GPT_Delay_Microsecs_H` function represents the time delay
 * in microseconds that you want the function to wait for. This delay is achieved using a
 * general-purpose timer (GPT) configured with a specific period based on the provided delay value.
 */
uint32_t GPT_Delay_Microsecs_H(uint32_t delay);

#ifdef __cplusplus
}
#endif

#endif