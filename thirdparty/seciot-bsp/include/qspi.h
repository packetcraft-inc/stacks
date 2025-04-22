/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : qspi_driver.c
 * Brief Description of file         : This is a Baremetal QSPI Driver header file for Mindgrove Silicon's I2C Peripheral.
 * Name of Author                    : Vishwajith.N.S 
 * Email ID                          : vishwajith@mindgrovetech.in
 * 
 * 
 * 
 * @file qspi_driver.c
 * @author Vishwajith .N.S (vishwajith@mindgrovetech.in)
 * @brief This is a Baremetal QSPI Driver header file for Mindgrove Silicon's QSPI Peripheral.
 * @version 1.0
 * @date 2023-07-08
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 * 
 */
#ifndef QSPI_H
#define QSPI_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "platform.h"
#include "log.h"
#include "secure_iot.h"
#include "errors.h"
#define WRITE 0
#define READ 1

#define FIFO_FULL 16
#define FIFO_EMPTY 0

/*Macros for Instruction MODE*/
#define CCR_IMODE_NIL             0x0
#define CCR_IMODE_SINGLE_LINE     0x1
#define CCR_IMODE_TWO_LINE        0x2
#define CCR_IMODE_FOUR_LINE       0x3
/*Macros for Address MODE*/
#define CCR_ADMODE_NIL            0x0
#define CCR_ADMODE_SINGLE_LINE    0x1
#define CCR_ADMODE_TWO_LINE       0x2
#define CCR_ADMODE_FOUR_LINE      0x3
/*Macros for Address Size*/
#define CCR_ADSIZE_8_BIT          0x0
#define CCR_ADSIZE_16_BIT         0x1
#define CCR_ADSIZE_24_BIT         0x2
#define CCR_ADSIZE_32_BIT         0x3
/*Macros for Alternate Byte mode*/
#define CCR_ABMODE_NIL            0x0
#define CCR_ABMODE_SINGLE_LINE    0x1
#define CCR_ABMODE_TW0_LINE       0x2
#define CCR_ABMODE_FOUR_LINE      0x3
/*Macros for Alternate Byte size*/
#define CCR_ABSIZE_8_BIT          0x0
#define CCR_ABSIZE_16_BIT         0x1
#define CCR_ABSIZE_24_BIT         0x2
#define CCR_ABSIZE_32_BIT         0x3
/*Macros for Data mode*/
#define CCR_DMODE_NO_DATA         0x0
#define CCR_DMODE_SINGLE_LINE     0x1
#define CCR_DMODE_TWO_LINE        0x2
#define CCR_DMODE_FOUR_LINE       0x3
/*Macros for Functional mode*/
#define CCR_FMODE_INDIRECT_WRITE  0x0
#define CCR_FMODE_INDIRECT_READ   0x1
#define CCR_FMODE_APM             0x2
#define CCR_FMODE_MMM             0x3
/*Macros for Memory map mode*/
#define CCR_MM_MODE_XIP           0x0
#define CCR_MM_MODE_RAM           0x1
/*Macros for OR and AND PMM*/
#define PMM_AND 0
#define PMM_OR  1



#define CR_PRESCALER(x)   (x<<18)//8bit
#define CR_PMM(x)         (x<<17)
#define CR_APMS(x)        (x<<16)
#define CR_TOIE(x)        (x<<15)//1bit
#define CR_SMIE(x)        (x<<14)
#define CR_FTIE(x)        (x<<13)
#define CR_TCIE(x)        (x<<12)
#define CR_TEIE(x)        (x<<11)
#define CR_FTHRES(x)      (x<<7 )//4bit
#define CR_FSEL(x)        (x<<6 )//Not used
#define CR_DFM(x)         (x<<5 )//Not used
#define CR_SSHIF(x)       (x<<4 )//Not used 1bit
#define CR_TCEN(x)        (x<<3 )
#define CR_DMAEN(x)       (x<<2 )//Not used
#define CR_ABORT(x)       (x<<1 )
#define CR_EN(x)          (x<<0 )

//Bit vectors for DCR 
#define DCR_MODE_BYTE(x)   (x<<21)
#define DCR_FSIZE(x)       (x<<16)//5bit 
#define DCR_CSHT(x)        (x<<8 )//3bit Not used
#define DCR_CKMODE(x)        (x)//1bit 

//Bit vectors for status register
#define SR_FLEVEL(x)      (x<<8)//5bit
#define SR_BUSY           (1<<5)//1bit
#define SR_TOF            (1<<4)
#define SR_SMF            (1<<3)
#define SR_FTF            (1<<2)
#define SR_TCF            (1<<1)
#define SR_TEF            (1<<0)

//Bit vectors for flag clear register 
#define FCR_CTOF (1<<4)
#define FCR_CSMF (1<<3)
#define FCR_CTCF (1<<1)//1bit
#define FCR_CTEF (1<<0)

//Bit vectors for CCR
#define CCR_DDRM(x)                (x<<31) 
/* #define CCR_DHHC(x)                (x<<30)//Not used */
#define CCR_MM_MODE(x)             (x<<30) //memory Map mode XIP=0;RAM=1;
#define CCR_DUMMY_BIT(x)           (x<<29) // Needed by Micron Flash Memories
#define CCR_SIOO(x)                (x<<28)
#define CCR_FMODE(x)               (x<<26)
#define CCR_DMODE(x)               (x<<24)
#define CCR_DUMMY_CONFIRMATION(x)  (x<<23) // Needed by Micron Flash Memories
#define CCR_DCYC(x)                (x<<18)
#define CCR_ABSIZE(x)              (x<<16)
#define CCR_ABMODE(x)              (x<<14)
#define CCR_ADSIZE(x)              (x<<12)
#define CCR_ADMODE(x)              (x<<10)
#define CCR_IMODE(x)               (x<<8 )
#define CCR_INSTRUCTION(x)         (x<<0 )

typedef struct{
    uint8_t functional_mode;              /**< Functional mode                                                           */
    uint8_t instruction;                  /**< Instruction                                                               */
    uint8_t instruction_mode;             /**< Instruction mode                                                          */
    uint8_t address_mode;                 /**< Address mode                                                              */
    uint8_t address_size;                 /**< Address size                                                              */
    uint32_t address;                     /**< Address                                                                   */
    uint8_t alternate_byte_mode;          /**< Alternate byte mode                                                       */
    uint32_t alternate_byte;               /**< Alternate byte                                                            */
    uint8_t alternate_byte_size;
    uint8_t dummy_mode:1;                 /**< Dummy mode                                                                */
    uint8_t dummy_bit:1;                  /**< Dummy bit                                                                 */
    uint8_t dummy_cycles:5;               /**< Dummy Cycles                                                              */
    uint8_t sioo:1;                       /**< Send instruction only once                                                */
    uint8_t mm_mode:1;                    /**< Memory map mode enable                                                    */
    uint8_t data_mode;                    /**< Data mode                                                                 */
    uint32_t length;                      /**< Data length                                                               */
    uint8_t *data_buffer;                 /**< Pointer to data buffer                                                    */
    uint8_t FMEM_SIZE;                    /**< Flash memory size                                                         */
    uint8_t CLK_MODE:1;                   /**< Clock mode                                                                */
    uint8_t fthresh;
    uint8_t csht;
      uint32_t TCEN       : 1;            /**< Timeout counter enable                                                    */
      uint32_t TEIE       : 1;            /**< Transfer error interrupt enable                                           */
      uint32_t TCIE       : 1;            /**< Transfer complete interrupt enable                                        */
      uint32_t FTIE       : 1;            /**< FIFO threshold interrupt enable                                           */
      uint32_t SMIE       : 1;            /**< Status match interrupt enable                                             */
      uint32_t TOIE       : 1;            /**< TimeOut interrupt enable                                                  */
      uint32_t APMS       : 1;            /**< Automatic poll mode stop                                                  */
      uint32_t PMM        : 1;            /**< Polling match mode                                                        */
      uint32_t PRESCALER  : 14;            /**< Clock prescaler                                                           */
      uint32_t status_mask;
      uint32_t status_match;
}qspi_msg;
/**
 * @fn QSPI_Transaction(uint32_t instance_number,qspi_msg *msg)
 * 
 * @brief Used to perfom QSPI transaction with necessary setting.
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param msg The parameter \a msg is a structure which has all settings to initiate a transaction.
 * 
 * @return SUCCESS if operation is successful,ENODEV if invalid instance number and ELENEXCEED is length of read and write parameters exceeded.
 */
uint32_t QSPI_Transaction(uint32_t instance_number,qspi_msg *msg);

#ifdef __cplusplus
}
#endif
#endif