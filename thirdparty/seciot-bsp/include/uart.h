/**
 * Project                               : Secure IoT SoC
 * Name of the file                      : uart_driver.c
 * Brief Description of file             : This is a Baremetal UART Driver file for Mindgrove Silicon's UART Peripheral.
 * Name of Author                        : Kapil Shyam. M, Santhosh Pavan. S, Vignesh Kumar. J 
 * Email ID                              : <kapilshyamm@gmail.com>  <santhosh@mindgrovetech.in>  <vigneshkumar@mindgrovetech.in>
 */
/**
 * @file uart.h
 * @brief Header file for uart
 * @details this is the header file for uart_driver.c 
 * @version 1.0
 * @date 2024-10-08
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.  
 */

 #ifndef UART_H
 #define UART_H
 #include <stdint.h>
 #include "secure_iot.h"
 #include "plic.h"
 #include "errors.h"
 #include "pinmux.h"
 
 #define MAX_UART_COUNT 5
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /*! UART Interrupt Enable bits description */
 #define ENABLE_RX_THRESHOLD	1 << 8
 #define ENABLE_BREAK_ERROR      1 << 7
 #define ENABLE_FRAME_ERROR      1 << 6
 #define ENABLE_OVERRUN          1 << 5
 #define ENABLE_PARITY_ERROR     1 << 4
 #define ENABLE_RX_FULL 		1 << 3
 #define ENABLE_RX_NOT_EMPTY 	1 << 2
 #define ENABLE_TX_FULL 		1 << 1
 #define ENABLE_TX_EMPTY 	1 << 0
 #define UARTX_BUFFER_SIZE       100
 
 
 typedef enum {
	 UART_0,
	 UART_1,
	 UART_2,
	 UART_3,
	 UART_4
 } UART_NUM;
 
 // Stopbits Values
 typedef enum {
	 STOP_BIT_1,
	 STOP_BIT_1_5,
	 STOP_BIT_2
 } ;
 
 //Parity values
 typedef enum {
	 NO_PARITY  ,
	 ODD_PARITY ,
	 EVEN_PARITY
 } ;
 
 #define DATA_SIZE_8  0x01
 #define DATA_SIZE_16 0x02
 #define DATA_SIZE_32 0x03
 
 struct uart_buf {
 
	 void *uart_data;
 
	 int len;
 };
 
 typedef struct 
 {
	 
	 uint8_t uart_num ;/*An Integer type parameter, which gets the uart number 
	 * from the user needed to be initialized. This is usually, 1 and 2, as
	 * the 0th instance is occupied for the serial communication.*/
 
	 uint32_t baudrate; //The desired baud rate for the UART communication.
 
	 uint8_t stop_bits : 2; //An integer type, which is of 2 bits. The value can be from 0-2, and 3 is undefined.
 
	 uint8_t parity : 2; //An integer type, which is again of 2 bits. The value can be from 0-2, and 3 is undefined.
 
	 uint8_t char_size : 2; //An integer type, which is of 2 bits. The values to be given are 5,6,7 and 8.
 
	 uint16_t delay; // Delay value
 
	 uint8_t rxthreshold;/*The value of the receive (RX) threshold for a UART instance. This 
							   * value is used to set the RX Threshold value of RX FIFO.*/
 
	 uint8_t transfer_mode : 3;
 
	 uint8_t receive_mode : 3 ;
 
	 uint8_t pullup : 1;
 
 }UART_Config_t;
 
 /**
  * @fn uint8_t UART_Available(UART_Config_t *uart_config)
  * 
  * @brief Check if the UART instance is available.
  * 
  * @details Checks the STATUS_REG register and the RX_BUFFER. If the buffer is empty
  * then the UART is labeled as available.
  *
  * @param  uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  *
  * @return It returns either 1, if available or 0, if not available.
  */
 
 uint8_t UART_Available(UART_Config_t *uart_config);
 
 /**
  * @fn uint8_t UART_Init(UART_Config_t *uart_config)
  * 
  * @brief Initialise specific UART. ANd sets the specified baudrate
  * 
  * @details Initialises the UART instance which is specified by 
  * the user as \a uart_num, sets the baudrate \a baudrate and cofigure the stopbits,
  * parity, charcter size from \a stopbits \a parity \a char_size where these 
  * parameters will be present the UART_Config_t structure, while calling this function.
  *
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Init(UART_Config_t const *uart_config);
 
 /**
  * @fn uint8_t UART_Set_Baudrate(UART_Config_t *uart_config)
  * 
  * @brief The function sets the baud rate of a UART instance based on a given baud rate value.
  * 
  * @details This function will be called to initialize a specific UART by passing baudrate value(through the structure)
  * which in turn used to calculate the baud_count to set the baudrate.
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Set_Baudrate(UART_Config_t *uart_config);
 
 
 /**
  * @fn uint8_t UART_Config( UART_Config_t *uart_config)
  * 
  * @brief Function to configure uart with no. of stop bits, type of parity and character size.
  * 
  * @details This function will be called to configure control register with the user-defined \a stop_bits, \a parity and \a char_size in the structure.
  *
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Config( UART_Config_t *uart_config);
 
 /**
  * @fn uint8_t UART_Write(UART_Config_t *uart_config , uint8_t * ptr_string, int uart_len)
  * 
  * @brief Function to transmit a string to a specific UART instance based on the \a uart_num in the UART_Config_t.
  * 
  * @details This function will be called to transmit a string to the specified UART instance.
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @param string_data A pointer to the string that needs to be transmitted via UART.
  * 
  * @param uart_len A integer value that specifies the length of the string that need to be transmitted
  * 
  * @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Write(UART_Config_t *uart_config , struct uart_buf *rx_bufs);
 
 /**
  * @fn uint8_t UART_Buffer_Read_Character(UART_Config_t *uart_config, uint8_t *data)
  * 
  * @brief This function reads a character from a UART instance and returns it through a pointer.
  * 
  * @details This function will be called to read a character from a specific uart instance by passing the
  * character pointer to store the character in the software buffer.
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @param data A pointer to a char variable where the read character will be stored.
  * 
  * @return It returns 0 which means operation is success.
  */
 
 
 /**
  * @fn void UART_Interrupt_Enable(UART_Config_t *uart_config,unsigned short interrupt)
  * 
  * @brief This function enables UART interrupts for a given instance.
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @param interrupt The "interrupt" parameter is an unsigned short that represents the specific UART
  * interrupt(s) to be enabled. It is likely a bitfield where each bit corresponds to a specific
  * interrupt source. By setting the appropriate bits in this parameter, the corresponding interrupts
  * will be enabled for the UART instance.
  * 
  * @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Interrupt_Enable(UART_Config_t *uart_config,uint16_t interrupt);
 
 /**
  * @fn void UART_RX_Threshold(UART_Config_t *uart_config, unsigned char rxthreshold)
  * 
  * @brief The function sets the receive threshold for a UART instance.
  * 
  * @details This function has a register called RX_THRESHOLD which will contain 
  * the information regarding whether the RX FIFO is Full or not. The depth of the UART RX buffer is 16.
  * When the 80% (i.e 12) of the blocks gets filled in the RX FIFO, then an interrupt is generated.
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @param rxthreshold The value of the receive (RX) threshold for a UART instance. This 
  * value is used to set the RX Threshold value of RX FIFO.
  * 
  * @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_RX_Threshold(UART_Config_t *uart_config, uint8_t rxthreshold);
 
 /**
  * @fn void UART_Flush(UART_Config_t  uart_config)
  * 
  * @brief The function flushes the receive buffer (i.e RX FIFO) of a UART instance.
  * 
  * @details This function will be called to flush the previous values stored in the 
  * UART RX FIFO for the specified UART instance.
  *  
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  *
  *  @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Flush(UART_Config_t *uart_config);
 
 /**
  * @fn uint8_t UART_Rx_Polling(UART_Config_t  uart_config)
  * 
  * @brief Check if the UART instance has received any data.
  * 
  * @details Checks the STATUS_REG register and the RX_BUFFER to see if any data has been received. 
  *
  * @param  uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  *
  * @return It returns either 1, if received or 0, if not received.
  */
 
 uint8_t UART_Rx_Polling(UART_Config_t *uart_config);
 
 /**
  * @fn int UART_Read_Character(UART_Config_t *uart_config, uint8_t *data)
  * 
  * @brief This function reads a character from a UART instance and returns it through a pointer.
  * 
  * @details This function will be called to read a character from a specific uart instance by passing the
  * character pointer to store the character in the RX register.
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @param data A pointer to a char variable where the read character will be stored.
  * 
  * @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Read_Character(UART_Config_t *uart_config , struct uart_buf *rx_bufs);
 
 /**
  * @fn uint8_t UART_Read_String(UART_Config_t *uart_config , struct uart_buf *rx_bufs)
  * 
  * @brief The function reads a string from a UART instance and returns the length of the string.
  * 
  * @details This function will be called to read a string, one character at a time from a
  * specific uart instance by passing the array in which to store the string by reference using pointers.
  * The values will be stored in the RX FIFO. And '\r' should be  appended at the end of transmitted 
  * string to exit from this function
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @param ptr_string A pointer to a character array where the received UART string will be stored.
  * 
  * @return @return It returns 0 which means operation is success.
  */
 
 uint8_t UART_Read_String(UART_Config_t *uart_config , struct uart_buf *rx_bufs);
 
 /**
  * @fn uint8_t UART_Write_Wait(UART_Config_t *uart_config)
  * 
  * @brief The function waits till the TX buffer is empty
  * 
  * @details This function will be called to wait till the transmission completes
  * 
  * @param uart_config A struct which consists of \a uart_num \a baudrate \a stop_bits \a parity \a char_size \a delay 
  * 
  * @return It returns 0 for there is nothing TX buff.
  */
 
 uint8_t UART_Write_Wait(UART_Config_t *uart_config);
 
 
 #undef putchar
 
 /**
  * @fn int putchar(int ch)
  * 
  * @brief The function sends a character over UART communication.
  * 
  * @details This function will be called to printf a single character to the stdout by passing
  * character as an integer.
  * 
  * @param ch The parameter "ch" is of type int and represents the character to be transmitted over UART
  * (Universal Asynchronous Receiver/Transmitter).
  * 
  * @return An integer value of 0.
  */
 
 
 
 int putchar(int ch);
 
 #undef getchar
 
 /**
  * @fn int getchar()
  * 
  * @brief Function to read a single character from the standard input device.
  * 
  * @details The function reads a character from a UART instance and waits until a character is available.
  * 
  * @return A character received through UART communication.
  */
 
 int getchar(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif