

/***************************************************************************
 * Project               	    : shakti devt board
 * Name of the file	            : pwmv2.h
 * Brief Description of file    : Header file for PWM V2 Driver.
 * Name of Author    	        : Sathya Narayanan, Kotteeswaran
 * Email ID                     : kottee.off@gmail.com

 Copyright (C) 2021  IIT Madras. All rights reserved.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
***************************************************************************/
/**
 * @file pwmv2.c
 * @project shakti devt board
 * @brief Header file for PWM Driver.
 */


#ifdef __cplusplus
extern "C" {
#endif

#include<stdbool.h>
#include <stdint.h>


#define PERIOD_REGISTER_MAX   0xFFFFFFFF
#define DUTY_REGISTER_MAX         0xFFFFFFFF
#define CONTROL_REGISTER_MAX          0x0000FFFF
#define DEADBAND_DELAY_REGISTER_MAX   0x0000FFFF
#define PWM_MAX_COUNT 8

#define PWM_0 0
#define PWM_1 1 
#define PWM_2 2 
#define PWM_3 3
#define PWM_4 4 
#define PWM_5 5 
#define PWM_6 6
#define PWM_7 7

// Control Register Individual Bits
#define PWM_ENABLE                      0x00000001
#define PWM_START                       0x00000002
#define PWM_OUTPUT_ENABLE               0x00000004
#define PWM_OUTPUT_POLARITY             0x00000008
#define PWM_COUNTER_RESET               0x00000010
#define PWM_COMP_OUTPUT_ENABLE          0x00000020
#define PWM_HALFPERIOD_INTERRUPT_ENABLE 0x00000040
#define PWM_FALL_INTERRUPT_ENABLE       0x00000080
#define PWM_RISE_INTERRUPT_ENABLE       0x00000100
#define PWM_HALFPERIOD_INTERRUPT        0x00000200
#define PWM_FALL_INTERRUPT              0x00000400
#define PWM_RISE_INTERRUPT              0x00000800
#define PWM_UPDATE_ENABLE               0x00001000


typedef enum
{
  rise_interrupt,       //Enable interrupt only on rise
  fall_interrupt,       //Enable interrupt only on fall
  halfperiod_interrupt,     //Enable interrupt only on halfperiod
  rise_fall_interrupt,      //Enable interrupt on rise and fall
  fall_halfperiod_interrupt,    //Enable interrupt on fall and halfperiod
  rise_halfperiod_interrupt,    //Enable interrupt on rise and halfperiod
  rise_fall_halfperiod_interrupt,   //Enable interrupt on rise, fall and halfperiod
  no_interrupt        //Disable interrupts
}pwm_interrupt_modes;

typedef struct 
{
  int pwm_number;
  uint32_t duty;
  uint32_t period;
  uint32_t value;
  int pin_number;
  int val;
  bool update;
  pwm_interrupt_modes interrupt_mode;
  bool change_output_polarity;
  uint16_t prescalar_value;
  uint32_t deadband_delay;
}PWM_Config_t;


//function prototype

/** @fn  pwm_init
 * @brief Function to initialize all pwm modules
 * @details This function will be called to initialize all pwm modules
 * @param[in] No input parameter
 * @param[Out] No output parameter
 */
void PWM_Init(PWM_Config_t *config,int, ...);

/** @fn pwm_set_prescalar_value
 * @brief Function to set the prescalar value of a specific pwm cluster		
 * @details This function will set the prescalar value of a specific pwm cluster
 * @param[in] uint32_t (cluster_number-  the pwm cluster to be selected)
 *            uint32_t (prescalar_value-  value of prescalar values which is used to divide the clock frequency.)
 * @param[Out] No output parameter
 */
void PWM_Set_Prescalar_Value(PWM_Config_t *config,int, int[]);

/** @fn  pwm_configure
 * @brief Function to configure the pwm module with all the values required like config->pwm_number, period, duty, interrupt_mode, deadband_delay, change_output_polarity_
 * @details This function configure the pwm module
 * @param[in] uint32_t (config->pwm_number - the pwm module to be selected)
 * uint32_t(period - value of periodic cycle to be used. the signal resets after every count of the periodic cycle)
 * uint32_t(duty - value of duty cycle. It specifies how many cycles the signal is active out of the periodic cycle )
 * pwm_interrupt_modes(interrupt_mode - value of interrupt mode. It specifies the interrupt mode to be used)
 * uint32_t(deadband_delay - value of deadband_delay. It specifies the deadband_delay to be used.)
 * bool (change_output_polarity - value of change_output_polarity. It specifies if output polarity is to be changed.)
 * @param[Out] No output parameter
 */
void PWM_Configure(PWM_Config_t *config,int, int[]); 

/** @fn pwm_start
 * @brief Function to start a specific pwm module
 * @details This function will start the specific pwm module
 * @param[in] uint32_t (config->pwm_number-  the pwm module to be selected)
 * @param[Out] No output parameter
 */
void PWM_Start(PWM_Config_t *config,int, ...);

/** @fn  pwm_set_control
 * @brief Function to set the control register of the selected pwm module
 * @details This function will be called to set the value of the control register for the selected module
 * @param[in] uint32_t (config->pwm_number- specifies the pwm smodule to be selected)
 *            uint32_t (value - value to be set between 0x0000 to 0xffff.)
 * @param[Out] uint32_t (returns 1 on success, 0 on failure.)
 */
int PWM_Set_Control(PWM_Config_t *config, int count, int pwm[]);

/** @fn pwm_clear
 * @brief Function to clear all registers in a specific pwm module
 * @details This function will be called to clear all registers in a specific pwm module
 * @param[in] uint32_t (config->pwm_number- specifies the pwm module to be selected)
 * @param[Out] No output parameter
 */
void PWM_Clear(PWM_Config_t *config);

/** @fn  pwm_stop
 * @brief Function to stop a specific pwm module
 * @details This function will stop a specific pwm module
 * @param[in] uint32_t (config->pwm_number-  the pwm module to be selected)
 * @param[out] No output parameter 
 */
void PWM_Stop(PWM_Config_t *config,int, ...);

/** @fn  pwm_set_duty_cycle
* @brief Function to set the duty cycle value of a specific pwm module 
 * @details This function will set the duty cycles value of a specific pwm module
 * @param[in]uint32_t(config->pwm_number-  the pwm module to be selected )
 *           uint32_t(duty - value of duty cycles to be used to decide how many period cycles the pwm signal is set to 1.
 * @param[Out] No output parameter
 */
void PWM_Set_Duty_Cycle(PWM_Config_t *config);

/** @fn pwm_set_periodic_cycle
 * @brief Function to set the period cycles value of a specific pwm module
 * @details This function will set the period cycles value of a specific pwm module
 * @param[in] uint32_t (config->pwm_number-  the pwm module to be selected)
 *            uint32_t (clock_divisor-  value of period cycles which is used to further divide the frequency into fixed period cycles.)
 * @param[Out] No output parameter
 */
void PWM_Set_Periodic_Cycle(PWM_Config_t *config);

/** @fn  pwm_set_deadband_delay
 * @brief Function to set the deadband delay register of the selected pwm module
 * @details This function will be called to set the value of the deadband_delay register for the selected module
 * @param[in] uint32_t (config->pwm_number- specifies the pwm module to be selected)
 *            uint32_t (value - value to be set between 0x0000 to 0xff.)
 * @param[Out] uint32_t (returns 1 on success, 0 on failure.)
 */
int PWM_Set_Deadband_Delay(PWM_Config_t *config);

/** @fn   PWM_Config_Control_mode
 * @brief Function to set value of control register based on parameteres
 * @details This function will set value of control register based on parameters
 * @param[in]  bool          (update                      - specifies if the module is to be updated)
 *           interrupt_mode  (interrupt_mode              - it specifes the mode of the interrupt)
 *           bool            (change_output_polarity      - it specifies if the output polarity should be changed) 
 * @param[Out] uint32_t (returns value to be set in the control register.)
 */
int PWM_Config_Control(PWM_Config_t *config);

/** @fn pwm_reset_all
 * @brief Function to reset all pwm modules
 * @details This function will be called to reset all pwm modules
 * @param[in] No input parameter
 * @param[Out] No output parameter
 */
void PWM_RESET_All();

/** @fn  pwm_update
 * @brief Function to update the pwm module with all the values required like config->pwm_number, period, duty, interrupt_mode, deadband_delay, change_output_polarity
 * @details This function udate the pwm module
 * @param[in] uint32_t (config->pwm_number - the pwm module to be selected)
 *           uint32_t(period - value of periodic cycle to be used. the signal resets after every count of the periodic cycle)
 *           uint32_t(duty - value of duty cycle. It specifies how many cycles the signal is active out of the periodic cycle )
 *           bool (rise_interrupt - value of rise_interrupt. It specifies if rise_interrupt is to be used.)
 *           pwm_interrupt_modes(interrupt_mode - value of interrupt mode. It specifies the interrupt mode to be used)
 * @param[Out] No output parameter
 */
void PWM_Update(PWM_Config_t *config);

void PWM_Show_Values(PWM_Config_t *config);

/** @fn  pwm_clr_control
 * @brief Function to clear the control register of the selected pwm module
 * @details This function will be called to clear the bits from the control register for the selected module
 * @param[in] uint32_t (config->pwm_number- specifies the pwm smodule to be selected)
 *            uint32_t (value - value to be set between 0x0000 to 0xffff.)
 * @param[Out] uint32_t (returns 1 on success, 0 on failure.)
 */
int PWM_Clr_Control(PWM_Config_t *config);

void PWM_Show_Frequency(PWM_Config_t *config);

/** @fn  PWM_Isr_Handler0
 * @brief Function to handle isr
 * @details This function will printf logs which specify what interrupt has been triggered- rise,fall,halfperiod
 * @param[in] No Input parameter
 * @param[out] No output parameter 
 */
void PWM_Isr_Handler0();

/** @fn  PWM_Isr_Handler1
 * @brief Function to handle isr
 * @details This function will printf logs which specify what interrupt has been triggered- rise,fall,halfperiod
 * @param[in] No Input parameter
 * @param[out] No output parameter 
 */
void PWM_Isr_Handler1();

/** @fn  PWM_Isr_Handler2
 * @brief Function to handle isr
 * @details This function will printf logs which specify what interrupt has been triggered- rise,fall,halfperiod
 * @param[in] No Input parameter
 * @param[out] No output parameter 
 */
void PWM_Isr_Handler2();

/** @fn  PWM_Isr_Handler3
 * @brief Function to handle isr
 * @details This function will printf logs which specify what interrupt has been triggered- rise,fall,halfperiod
 * @param[in] No Input parameter
 * @param[out] No output parameter 
 */
void PWM_Isr_Handler3();

/** @fn  PWM_Isr_Handler4
 * @brief Function to handle isr
 * @details This function will printf logs which specify what interrupt has been triggered- rise,fall,halfperiod
 * @param[in] No Input parameter
 * @param[out] No output parameter 
 */
void PWM_Isr_Handler4();

/** @fn  PWM_Isr_Handler5
 * @brief Function to handle isr
 * @details This function will printf logs which specify what interrupt has been triggered- rise,fall,halfperiod
 * @param[in] No Input parameter
 * @param[out] No output parameter 
 */
void PWM_Isr_Handler5();

#ifdef __cplusplus
}
#endif
