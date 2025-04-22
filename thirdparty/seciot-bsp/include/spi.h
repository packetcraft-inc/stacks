/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : spi.h
 * Brief Description of file         : Header to Standard spi driver
 * Name of Author                    : Kapil Shyam. M
 * Email ID                          : kapilshyamm@gmail.com
 * 
 * @file spi.h
 * @author Kapil Shyam. M (kapilshyamm@gmail.com)
 * @brief This is a Baremetal SSPI Driver's Header file for Mindgrove Silicon's SPI Peripheral
 * @version 0.2
 * @date 2023-07-20
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 * 
 */

#ifndef SPI_H
#define SPI_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "platform.h"
#include "log.h"
#include "errors.h"
#include "secure_iot.h"
#include"gptimer.h"

#define FIFO_DEPTH_8  32
#define FIFO_DEPTH_16 FIFO_DEPTH_8/2
#define FIFO_DEPTH_32 FIFO_DEPTH_8/4

#define MASTER 1
#define SLAVE 0

#define DISABLE 0
#define ENABLE 1

#define LSB_FIRST 1
#define MSB_FIRST 0

#define SIMPLEX_TX 0
#define SIMPLEX_RX 1
#define HALF_DUPLEX 2
#define FULL_DUPLEX 3

#define FAILURE -1
#define TIMEOUT1 -2

#define SPI0 0
#define SPI1 1
#define SPI2 2
#define SPI3 3

#define DATA_SIZE_8 8
#define DATA_SIZE_16 16
#define DATA_SIZE_32 32


typedef struct 
{
    int spi_number;
    int pol;
    int pha;
    int prescale;;
    int setup_time;
    int hold_time;
    uint32_t bit_rate;
    int master_mode;
    int lsb_first;
    int comm_mode;
    int spi_size;
    uint32_t value;
}SPI_Config_t;


struct spi_buf {
	void *buf;
	int len;
};
struct spi_buf_set {
	const struct spi_buf *buffers;
	int count;
};

//! Enable SPI_DEBUG Macro for Debugging purposes.
//#define SPI_DEBUG 1

/**
 * @fn int sspi_enable(SPI_Config config)
 * 
 * @brief The function enables a specified SPI instance.
 * 
 * @param config->spi_number The config->spi_number parameter is an integer that represents the SPI instance number. It
 * is used to select the specific SPI peripheral that needs to be enabled.
 */
int SPI_Enable(SPI_Config_t *config);

/**
 * @fn int sspi_disable(SPI_Config config)
 * 
 * @brief This function disables a specified SPI instance.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that represents the SPI 
 * instance number. It is used to identify the specific SPI peripheral that needs to be disabled.
 */
int SPI_Disable(SPI_Config_t *config);

/**
 * @fn int sspi_init(spi_config config)
 * @details The function initializes a SPI instance and returns its base address.
 * 
 * @param config->spi_number The SPI instance number that needs to be initialized. It should be an integer
 * value between 0 and SSPI_MAX_COUNT-1.
 * 
 * @return the base address of the SPI instance that was initialized. If the initialization fails due
 * to an invalid SPI instance number, it returns -1.
 */
int SPI_Init(SPI_Config_t *config);

/**
 * @fn int sspi_busy(SPI_Config config)
 * @brief The function waits until the SPI communication is busy.
 * @details It is used to select the specific SPI instance that needs to be checked for busy status.
 * @param config->spi_number The parameter \a config->spi_number is an integer that represents the SPI instance number.
 * 
 */
int SPI_Busy(SPI_Config_t *config);

/**
 * @fn int sclk_config( spi_config config, int config->pol, int config->pha, int prescale, int setup_time, int hold_time)
 * 
 * @details The function configures the clock control settings for a specified SPI instance.
 * 
 * @param config->spi_number The SPI (Serial Peripheral Interface) Instance number for which the configuration is being
 * applied to.
 * 
 * @param config->pol The config->polarity of the clock signal. It determines the idle state of the clock signal. If it
 * is set to 0, the clock signal is low when idle. If it is set to 1, the clock signal is high when
 * idle.
 * 
 * @param config->pha The config->pha parameter in the sclk_config function is used to set the clock config->phase of the SPI
 * (Serial Peripheral Interface) bus. It determines the timing relationship between the data and the
 * clock signal. A value of 0 indicates that data is captured on the leading edge of the clock signal,
 * while value of 1 indicates that data is captured on the trailing edge of the clock signal.
 * 
 * @param prescale The prescale parameter is used to set the clock frequency of the SPI bus. It
 * determines the division factor of the system clock frequency to generate the SPI clock frequency.
 * 
 * @param setup_time The setup time parameter in the sclk_config function is the delay between the
 * assertion of the chip select signal and the start of the clock signal. It is the time required for
 * the slave device to prepare for data transfer.
 * 
 * @param hold_time Hold time refers to the amount of time that the SPI clock signal should be held low
 * after the data has been transmitted. This is important to ensure that the receiving device has
 * enough time to sample the data correctly. The hold time is typically specified in terms of clock
 * cycles.
 * 
 * @note if \a config->pol is 0, then \a config->pha cannot be 1, as it makes sampling at Falling edge. Likewise, if \a config->pol is 1 
 * then \a config->pha cannot be 0 as it makes sampling at the Falling edge.
 */
int SCLK_Config(SPI_Config_t *config);

/**
 * @fn int sspi_comm_control_config(SPI_Config_t *config, int master_mode, int lsb_first, int comm_mode, int spi_size)
 * 
 * @details The function configures the communication control for a specified SPI Instance based on the given
 * parameters.
 * 
 * @param spi_number The SPI Instance number to be configured.
 * 
 * @param master_mode Specifies whether the SPI interface is operating in master or slave mode. It can
 * take the values MASTER or SLAVE which is defined in the \a sspi.h file.
 * 
 * @param lsb_first The lsb_first parameter is a flag that determines whether the least significant bit
 * (LSB) or the most significant bit (MSB) is transmitted first in the SPI communication. If lsb_first
 * is set to 1, the LSB is transmitted first, and if it is set to 0, MSB will be transmitted first.
 * 
 * @param comm_mode The communication mode of the SPI interface, which can be set to either 
 * \arg SPI_MODE_0 = SIMPLEX_TX,
 * \arg SPI_MODE_1 = SIMPLEX_RX,
 * \arg SPI_MODE_2 = HALF_DUPLEX, or
 * \arg SPI_MODE_3 = FULL_DUPLEX. 
 * These defines are included in the \a sspi.h .
 * 
 * @param spi_size spi_size refers to the total number of bits to be transmitted and received in each
 * SPI transaction. It is used to configure the SPI communication control register. Mostly, the values for 
 * the \a spi_size will be 8, 16. and 32.
 * 
 * @see sspi_busy function.
 */
int SPI_Comm_Control_Config(SPI_Config_t *config);

/**
 * @fn int sspi8_transmit_data(SPI_Config config, uint8_t tx_data[FIFO_DEPTH_8], uint8_t data_size)
 * 
 * @brief The function transmits data over SPI in 8-bit format.
 * @details This function checks if the TX FIFO is not full, and it writes the data to the tx register.
 * 
 * @param config->spi_number The SPI instance number to be used for data transmission.
 * @param tx_data An array of 8-bit data to be transmitted over SPI.
 * @param data_size The number of bytes of data to be transmitted.
 * 
 * @see sspi_check_tx_fifo_32 function and sspi_wait_till_tx_complete function.
 */
int SPI8_Transmit_Data(SPI_Config_t *config, uint8_t data);

/**
 * @fn int sspi16_transmit_data(SPI_Config config, uint16_t tx_data[FIFO_DEPTH_16], uint8_t data_size)
 * 
 * @brief The function transmits 16-bit data over SPI in chunks of a specified size.
 * @details This function checks if the TX FIFO contains atleast 2-bytes space, and it writes the data to the tx register.
 * 
 * @param config->spi_number The SPI instance number to be used for transmitting data.
 * @param tx_data An array of 16-bit data to be transmitted over SPI.
 * @param data_size The total number of data to be transmitted in bytes.
 * 
 * @see sspi_check_tx_fifo_30 function and sspi_wait_till_tx_complete function.
 *
 * @return an integer value. If the transmission is successful, it returns 0. If the TX FIFO does not
 * have enough space, it returns -1.
 */
int SPI16_Transmit_Data(SPI_Config_t *config, uint16_t data);

/**
 * @fn int sspi32_transmit_data(SPI_Config config, uint32_t tx_data[FIFO_DEPTH_32], uint8_t data_size)
 * 
 * @brief The function transmits 32-bit data over SPI in chunks of a specified size.
 * @details This function checks if the TX FIFO contains atleast 4-bytes space, and it writes the data to the tx register.
 * 
 * @param config->spi_number The SPI instance number to be used for transmitting data.
 * @param tx_data An array of 32-bit data to be transmitted over SPI.
 * @param data_size The total number of data to be transmitted in bytes.
 * 
 * @see sspi_check_tx_fifo_28 function and sspi_wait_till_tx_complete function.
 * 
 * @return an integer value. If the transmission is successful, it returns 0. If the TX FIFO does not
 * have enough space, it returns -1.
 */
int SPI32_Transmit_Data(SPI_Config_t *config, uint32_t data);

/**
 * @fn sspi_wait_till_tx_not_en(SPI_Config config)
 * 
 * @brief The function waits until the transmit enable flag of a specified SPI instance is cleared.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that specifies the SPI instance number.
 * It is used to select the SPI peripheral to be configured and operated on.
 */
int SPI_Wait_Till_Tx_Not_En(SPI_Config_t *config);

/**
 * @fn int sspi_wait_till_tx_complete(SPI_Config config)
 * 
 * @brief The function when called, waits till the TX transfer completes.
 * @details This function waits until the transmit FIFO of a specified SPI instance is empty.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that represents the SPI instance number.
 * It is used to select the specific SPI peripheral instance to operate on when accessing the SPI registers.
 * 
 * @see sspi_wait_till_tx_not_en function.
 */
int SPI_Wait_Till_Tx_Complete(SPI_Config_t *config);

/**
 * @fn int sspi_check_tx_fifo_32(SPI_Config config)
 * 
 * @brief This function when called will check if TX Buffer is Full.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that specifies the SPI instance number.
 * It is used to select the SPI peripheral to be configured and operated on.
 * 
 * @return an integer value. If the TX FIFO is full, it returns 1. If the TX FIFO is not full, it
 * returns 0.
 */
int SPI_Check_Tx_Fifo_32(SPI_Config_t *config);

/**
 * @fn int sspi_check_tx_fifo_30(SPI_Config config)
 * 
 * @brief This function when called will check if TX Buffer contains space for atleast 2 bytes 
 * and also transmit FIFO of a specified SPI instance has atmost 30 bytes.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that specifies the SPI instance number.
 * It is used to select the SPI peripheral to be configured and operated on.
 * 
 * @return an integer value. If the TX FIFO contains less than or equal to 30 bytes, it returns 0.
 * Otherwise, it returns 1.
 */
int SPI_Check_Tx_Fifo_30(SPI_Config_t *config);

/**
 * @fn int sspi_check_tx_fifo_28(SPI_Config config)
 * 
 * @brief This function when called will check if TX Buffer contains space for atleast 4 bytes.
 * and also transmit FIFO of a specified SPI instance has atmost 28 bytes.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that specifies the SPI instance number.
 * It is used to select the SPI peripheral to be configured and operated on.
 * 
 * @return an integer value. If the TX FIFO contains less than or equal to 28 bytes, it returns 0.
 * Otherwise, it returns 1.
 */
int SPI_Check_Tx_Fifo_28(SPI_Config_t *config);

/**
 * @fn int sspi_wait_till_rxfifo_not_empty(SPI_Config config)
 * 
 * @brief The function waits until the receive FIFO of a specified SPI instance is not empty.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that specifies the SPI instance number.
 * It is used to select the SPI peripheral to be configured and operated on.
 */
int SPI_Wait_Till_Rxfifo_Not_Empty(SPI_Config_t *config);

/**
 * @fn int sspi_wait_till_rxfifo_2(SPI_Config config)
 * 
 * @brief The function waits until the receive FIFO of a specified SPI instance has atleast 2 bytes.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that specifies the SPI instance number.
 * It is used to select the SPI peripheral to be configured and operated on.
 */
int SPI_Wait_Till_Rxfifo_2(SPI_Config_t *config);

/**
 * @fn int sspi_wait_till_rxfifo_2(SPI_Config config)
 * 
 * @brief The function waits until the receive FIFO of a specified SPI instance has atleast 4 bytes.
 * 
 * @param config->spi_number The parameter `config->spi_number` is an integer that specifies the SPI instance number.
 * It is used to select the SPI peripheral to be configured and operated on.
 */
int SPI_Wait_Till_Rxfifo_4(SPI_Config_t *config);

/**
 * @fn int sspi8_receive_data(SPI_Config config, uint8_t rx_data[FIFO_DEPTH_8], uint8_t data_size)
 * 
 * @brief The function receives 8-bit data from an SPI interface.
 * @details This function checks if the RX FIFO is not empty, and then it reads the data from the rx register.
 * 
 * @param config->spi_number The SPI instance number to be used for receiving data.
 * @param rx_data An array of 8-bit data to be received over SPI.
 * @param data_size The total number of data to be received in bytes.
 */
uint8_t SPI8_Receive_Data(SPI_Config_t *config);

/**
 * @fn int sspi16_receive_data(SPI_Config config, uint16_t rx_data[FIFO_DEPTH_16], uint8_t data_size)
 * 
 * @brief The function receives 16-bit data from an SPI interface.
 * @details This function checks if the RX FIFO contains atleast 2-bytes, and then it reads the data from the rx register.
 * 
 * @param config->spi_number The SPI instance number to be use for receiving data.
 * @param rx_data An array of 16-bit data to be received over SPI.
 * @param data_size The total number of data to be received in bytes.
 */
uint16_t SPI16_Receive_Data(SPI_Config_t *config);

/**
 * @fn int sspi32_receive_data(SPI_Config config, uint32_t rx_data[FIFO_DEPTH_32], uint8_t data_size)
 * 
 * @brief The function receives 32-bit data from an SPI interface.
 * @details This function checks if the RX FIFO contains atleast 4-bytes, and then it reads the data from the rx register.
 * 
 * @param config->spi_number The SPI instance number to be use for receiving data.
 * @param rx_data An array of 32-bit data to be received over SPI.
 * @param data_size The total number of data to be received in bytes.
 */
uint32_t SPI32_Receive_Data(SPI_Config_t *config);

int INTER_Enable_Config(SPI_Config_t *config);

/**
 * @fn The function SPI_Configure(SPI_Config_t *config) configures SPI communication based on the provided configuration
 * settings.
 * 
 * @param config The `config` parameter is a pointer to a structure of type `SPI_Config_t`. This
 * structure likely contains configuration settings for the SPI (Serial Peripheral Interface)
 * communication, such as baud rate, data format, clock polarity, and other relevant parameters needed
 * to set up and control the SPI communication.
 * 
 * @return the value `SUCCESS`.
 */
int SPI_Configure(SPI_Config_t *config);

/**
 * @fn The function SPI_Transceive(SPI_Config_t *config, const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs) 
 * handles SPI communication based on the specified configuration and buffer sets for full duplex, simplex transmit, simplex receive, 
 * and half duplex modes with different data sizes.
 * 
 * @param config The `config` parameter in the `SPI_Transceive` function is a pointer to a structure of
 * type `SPI_Config_t`. This structure likely contains configuration settings for the SPI
 * communication, such as communication mode (full duplex, simplex TX, simplex RX, half duplex) and
 * data size (8-bit
 * @param tx_bufs `tx_bufs` is a pointer to a struct `spi_buf_set` which contains information about the
 * transmit buffers for SPI communication. It likely includes details such as the buffer length, buffer
 * data, and any other necessary configuration for transmitting data over SPI.
 * @param rx_bufs The `rx_bufs` parameter in the `SPI_Transceive` function is a pointer to a struct
 * `spi_buf_set` which contains information about the receive buffers for the SPI communication. It
 * allows the function to read data from the SPI bus into the specified receive buffers.
 * 
 * @return The function `SPI_Transceive` is returning a value `SUCCESS`.
 */
int SPI_Transceive(SPI_Config_t *config, const struct spi_buf_set *tx_bufs,
			                    const struct spi_buf_set *rx_bufs);


#ifdef __cplusplus
}
#endif
#endif