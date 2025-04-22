/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : i2c_driver.h
 * Brief Description of file         : This is a Baremetal I2C Driver header file for Mindgrove Silicon's I2C Peripheral.
 * Name of Author                    : Vishwajith.N.S 
 * Email ID                          : vishwajith@mindgrovetech.in
 * 
 * 
 * 
 * @file i2c_driver.h
 * @author Vishwajith .N.S (vishwajith@mindgrovetech.in)
 * @brief This is a Baremetal I2C Driver header file for Mindgrove Silicon's I2C Peripheral.
 * @version 1.0
 * @date 2024-10-17
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.
 * 
 */
#ifndef I2C_H
#define I2C_H
#ifdef __cplusplus
extern "C" {
#endif
#include<stdint.h>
#include"errors.h"
#include "log.h"
#include"secure_iot.h"

#define START_BIT 1<<1
#define NO_START_BIT 0
#define STOP_BIT 1<<2
#define REPEATED_START 0

/**
 * @fn I2C_Init(uint8_t instance_number,uint32_t clock_frequency)
 * 
 * @brief Used to set clock frequency for I2C communication.
 * 
 * @param instance_number The parameter \a instance_number is an unsigned integer that represents the I2C instance number.
 * @param clock_frequency The parameter \a clock_frequency is an unsigned integer that represents the clock frequency of I2C communication.
 * 
 * @return SUCCESS when successfully initialised and ENODEV when device not found.
 */
uint32_t I2C_Init(uint8_t instance_number,uint32_t clock_frequency);

/**
 * @fn I2C_Transmit(uint32_t instance_number,uint8_t slave_address,uint8_t *data,uint8_t len)
 * 
 * @brief Used to send data to slave through I2C communication.
 * 
 * @param instance_number The parameter \a instance_number is an unsigned integer that represents the I2C instance number.
 * @param slave_address The parameter \a slave_address is an 7 bit address that represents the I2C slave address.
 * @param data The parameter \a data is an pointer to array which has data to be sent.
 * @param length The parameter \a length is the length for which has data to be sent from data array.
 * @param mode The parameter \a mode is used to set whether to send start bit and stop bit in transaction.
 * 
 * @return SUCCESS if successfull transmission, ENODEV when device not found and NOACK is no acknowledgement is recieved for data.
 */
uint32_t I2C_Transmit(uint32_t instance_number,uint8_t slave_address,uint8_t *data,uint8_t length,uint8_t mode);

/**
 * @fn I2C_Recieve(uint32_t instance_number,uint8_t slave_address,uint8_t *data,uint8_t len)
 * 
 * @brief Used to send data to slave through I2C communication.
 * 
 * @param instance_number The parameter \a instance_number is an unsigned integer that represents the I2C instance number.
 * @param slave_address The parameter \a slave_address is an 7 bit address that represents the I2C slave address.
 * @param data The parameter \a data is an pointer to array which has data to be sent.
 * @param length The parameter \a length is the length for which has data to be sent from data array.
 * @param mode The parameter \a mode is used to set whether to send start bit and stop bit in transaction.
 * 
 * @return SUCCESS if successfull transmission, ENODEV when device not found and NOACK is no acknowledgement is recieved for data.
 */
uint32_t I2C_Recieve(uint32_t instance_number,uint8_t slave_address,uint8_t *data,uint8_t length,uint8_t mode);

#ifdef __cplusplus
}
#endif
#endif

/**
 * \code{.c}
#include"i2c.h"
void main(){
    uint8_t data[8] = {[0 ... 7]=0x22};
    I2C_Init(0,100000);
    ///Only I2C Write Transaction
    I2C_Transmit(0,0x68,data,8,START_BIT|STOP_BIT);
    ///Only I2C Write and Read Transaction without repeated start.
    I2C_Transmit(0,0x68,data,8,START_BIT|STOP_BIT);
    I2C_Recieve(0,0x68,data,8,START_BIT|STOP_BIT);
     ///Only I2C Write and Read Transaction with repeated start.
    I2C_Transmit(0,0x68,data,8,START_BIT|REPEATED_START);
    I2C_Recieve(0,0x68,data,8,NO_START_BIT|I2C_STOP);
    
    while(1);
}
 \endcode
 * 
 */