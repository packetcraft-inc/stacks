/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : qspi_flash_driver.c
 * Brief Description of file         : This is a QSPI flash Driver header file for Mindgrove Silicon's QSPI Peripheral.
 * Name of Author                    : Vishwajith.N.S 
 * Email ID                          : vishwajith@mindgrovetech.in
 * 
 * 
 * 
 * @file qspi_flash_driver.c
 * @author Vishwajith .N.S (vishwajith@mindgrovetech.in)
 * @brief This is a QSPI flash Driver header file for Mindgrove Silicon's QSPI Peripheral.
 * @version 1.0
 * @date 2024-10-18
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 * 
 */
#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @fn uint32_t fastReadQuad(uint8_t qspinum,uint8_t* data,uint32_t address,uint8_t data_length)
 * 
 * @brief Used to read data from flash through all the four data lines and instruction through one address lines.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param data The parameter \a data is a pointer to array which contains data to be read from flash.
 * @param address The parameter \a address specifies from which start address data to be read from flash through QSPI.
 * @param data_length The parameter \a data_length specifies how many bytes of data should be read from flash.
 * 
 * @return SUCCESS if operation is successful,ENODEV if invalid instance number and ELENEXCEED is length of read and write parameters exceeded.
 */
uint32_t fastReadQuad(uint8_t qspinum,uint8_t* data,uint32_t address,uint8_t data_length);
/**
 * @fn uint32_t fastReadQuadIO(uint8_t qspinum,uint8_t *data,uint32_t address,uint8_t data_length)
 * 
 * @brief Used to write data into flash through all the four data lines and instruction through two address lines.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param data The parameter \a data is a pointer to array which contains data to written in flash.
 * @param address The parameter \a address specifies from which start address data to be read from flash through QSPI.
 * @param data_length The parameter \a data_length specifies how many bytes of data should be written to flash from array.
 * 
 * @return SUCCESS if operation is successful,ENODEV if invalid instance number and ELENEXCEED is length of read and write parameters exceeded.
 */
uint32_t fastReadQuadIO(uint8_t qspinum,uint8_t *data,uint32_t address,uint8_t data_length);
/**
 * @fn uint32_t fastReadSingle(uint8_t qspinum,uint8_t *data,uint32_t address,uint8_t data_length)
 * 
 * @brief Used to write data into flash through one data line and one instruction line.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param data The parameter \a data is a pointer to array which contains data read from flash.
 * @param address The parameter \a address specifies from which start address data should be read from flash.
 * @param data_length The parameter \a data_length specifies how many bytes of data should be read from flash.
 * 
 * @return SUCCESS if operation is successful,ENODEV if invalid instance number and ELENEXCEED is length of read and write parameters exceeded.
 */
uint32_t fastReadSingle(uint8_t qspinum,uint8_t *data,uint32_t address,uint8_t data_length);
/**
 * @fn uint32_t inputpageQuad(uint8_t qspinum,uint8_t* data,uint32_t address,uint8_t data_length)
 * 
 * @brief Used to write data to flash with one address line,one instruction line and four data lines.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param data The parameter \a data is a pointer to array which contains data to be read from flash.
 * @param address The parameter \a address specifies from which start address data should be read from flash.
 * @param data_length The parameter \a data_length specifies how many bytes of data should be read from flash.
 * 
 * @return SUCCESS if operation is successful,ENODEV if invalid instance number and ELENEXCEED is length of read and write parameters exceeded.
 */
uint32_t inputpageQuad(uint8_t qspinum,uint8_t* data,uint32_t address,uint8_t data_length);
/**
 * @fn uint32_t inputpageSingle(uint8_t qspinum,uint8_t* data,uint32_t address,uint8_t data_length)
 * 
 * @brief Used to write to flash through single line.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param data The parameter \a data is a pointer to array which contains data to be written to flash.
 * @param address The parameter \a address specifies from which start address data to be written to flash through QSPI.
 * @param data_length The parameter \a data_length specifies how many bytes of data should be written to flash from array.
 * 
 * @return SUCCESS if operation is successful,ENODEV if invalid instance number and ELENEXCEED is length of read and write parameters exceeded.
 */
uint32_t inputpageSingle(uint8_t qspinum,uint8_t* data,uint32_t address,uint8_t data_length);
/**
 * @fn void sector4KErase(uint8_t qspinum,uint32_t address)
 * 
 * @brief Used to erase QSPI flash for 4KB of space from mentioned starting address.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param address The parameter \a address specifies from which start address data should be erased from flash through QSPI.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t sector4KErase(uint8_t qspinum,uint32_t address);
/**
 * @fn void sector32KErase(uint8_t qspinum,uint32_t address)
 * 
 * @brief Used to erase QSPI flash for 32KB of space from mentioned starting address.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param address The parameter \a address specifies from which start address data should be erased from flash through QSPI.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t sector32KErase(uint8_t qspinum,uint32_t address);
/**
 * @fn void chipErase(uint8_t qspinum)
 * 
 * @brief Used to erase whole chip through QSPI.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t chipErase(uint8_t qspinum);
/**
 * @fn void writeEnable(uint8_t qspinum)
 * 
 * @brief Used to enable qspi write transactions.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeEnable(uint8_t qspinum);
/**
 * @fn void writeDisable(uint8_t qspinum)
 * 
 * @brief Used to disable qspi write transactions.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeDisable(uint8_t qspinum);
/**
 * @fn void suspend(uint8_t qspinum)
 * 
 * @brief Used to suspend current qspi transactions.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t suspend(uint8_t qspinum);
/**
 * @fn void resume(uint8_t qspinum)
 * 
 * @brief Used to resume suspended qspi transactions.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t resume(uint8_t qspinum);
/**
 * @fn uint32_t power_down(uint8_t qspinum)
 * 
 * @brief Used to enter power down mode.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t power_down(uint8_t qspinum);
/**
 * @fn uint32_t release_power_down(uint8_t qspinum)
 * 
 * @brief Used to exit power down mode.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t release_power_down(uint8_t qspinum);
/**
 * @fn uint8_t readStatusRegister1(uint8_t qspinum)
 * 
 * @brief Used to read value in Status register 1.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return Returns value in Status Register 1.
 */
uint8_t readStatusRegister1(uint8_t qspinum);
/**
 * @fn uint8_t readStatusRegister2(uint8_t qspinum)
 * 
 * @brief Used to read value in Status register 2.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return Returns value in Status Register 2.
 */
uint8_t readStatusRegister2(uint8_t qspinum);
/**
 * @fn uint8_t readStatusRegister3(uint8_t qspinum)
 * 
 * @brief Used to read value in Status register 3.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return Returns value in Status Register 3.
 */
uint8_t readStatusRegister3(uint8_t qspinum);
/**
 * @fn uint8_t readFlagStatusRegister(uint8_t qspinum)
 * 
 * @brief Used to read value in Flag Status Register.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return Returns value in Flag Status Register.
 */
uint8_t readFlagStatusRegister(uint8_t qspinum);
/**
 * @fn void writeEnableStatusRegister(uint8_t qspinum)
 * 
 * @brief Used to enable write operation to status register.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeEnableStatusRegister(uint8_t qspinum);
/**
 * @fn void writeStatusRegister1(uint8_t qspinum,uint8_t* statusData)
 * 
 * @brief Used to write value in Status Register 1.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param statusData The parameter \a statusData is value that has to be written to status register.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeStatusRegister1(uint8_t qspinum,uint8_t* statusData);
/**
 * @fn void writeStatusRegister2(uint8_t qspinum,uint8_t* statusData)
 * 
 * @brief Used to write value in Status Register 2.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param statusData The parameter \a statusData is value that has to be written to status register.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeStatusRegister2(uint8_t qspinum,uint8_t* statusData);
/**
 * @fn void writeStatusRegister3(uint8_t qspinum,uint8_t* statusData)
 * 
 * @brief Used to write value in Status Register 2.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param statusData The parameter \a statusData is value that has to be written to status register.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeStatusRegister3(uint8_t qspinum,uint8_t* statusData);
/**
 * @fn uint8_t readGlobalFreezeBit(uint8_t qspinum)
 *
 * @brief Used to read global freeze bit.
 *
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 *
 * @return 1 if global freeze bit is set otherwise 0.
 */
uint8_t readGlobalFreezeBit(uint8_t qspinum);
/**
 * @fn uint32_t writeGlobalFreezeBit(uint8_t qspinum)
 *
 * @brief Used to set or clear global freeze bit.
 *
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 *
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeGlobalFreezeBit(uint8_t qspinum,uint8_t *data);
/**
 * @fn uint8_t readFlashSFDP(uint8_t qspinum,uint32_t address)
 * 
 * @brief Used to read value of Serial Flash Discoverable Parameter.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param address The parameter \a address specifies from which address we have to read data.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint8_t readFlashSFDP(uint8_t qspinum,uint32_t address);
/**
 * @fn uint32_t readNVCR(uint8_t qspinum, uint8_t* data)
 * 
 * @brief Used to read NVCR(Non volatile configuration register)
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param data The parameter \a data specifies pointer to location which has NVCR contents.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t readNVCR(uint8_t qspinum, uint8_t* data);
/**
 * @fn uint32_t readJedecID(uint8_t qspinum, uint8_t *id)
 * 
 * @brief Used to write to NVCR(Non volatile configuration register)
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param id The parameter \a id specifies pointer which has pointer of variable which has JedecID.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t readJedecID(uint8_t qspinum, uint8_t *id);
/**
 * @fn uint32_t writeNVCR(uint8_t qspinum, uint8_t* data)
 * 
 * @brief Used to write to NVCR(Non volatile configuration register)
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param data The parameter \a data specifies pointer which has data to be written to NVCR.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t writeNVCR(uint8_t qspinum, uint8_t* data);
/**
 * @fn uint32_t flash_xip_init(uint8_t qspinum, int flash_size)
 * 
 * @brief Used to change QSPI mode to Excute In Place mode,used to run code from flash memory.
 * 
 * @param qspinum The parameter \a qspinum is an unsigned integer that represents the QSPI instance number.
 * @param flash_size The parameter \a flash_size specifies flash size.
 * 
 * @return SUCCESS if operation is successful and ENODEV if invalid instance number.
 */
uint32_t flash_xip_init(uint8_t qspinum, int flash_size);

uint32_t psram_init(uint8_t qspinum, int flash_size);

#ifdef __cplusplus
}
#endif

#endif // FLASH_DRIVER_H
