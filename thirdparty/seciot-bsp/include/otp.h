#ifndef OTP_H
#define OTP_H
#endif

#include "io.h"
#include "errors.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTP_BASE_ADDRESS  0x33000

#define OTP_CONTROLREG    0x00
#define OTP_START         1 << 0
#define OTP_R_W           1 << 1
/* READ  - 1
 * WRITE - 0
 */
#define OTP_STARTUP       1 << 2

#define OTP_STATUS        0x08
#define OTP_GET_OUTPUT    1 << 0
#define OTP_PROG_STATUS   1 << 1
#define OTP_FATAL         1 << 2
#define OTP_STARTUP_DONE  1 << 3

#define OTP_ADDR          0x10
#define OTP_READ_DATA     0x18
#define OTP_WRITE_DATA    0x20

typedef struct {
  uint32_t control;
  uint32_t reserved0;
  volatile uint32_t status;
  uint32_t reserved1;
  uint32_t addr;
  uint32_t reserved2;
  volatile uint32_t read_data;
  uint32_t reserved3;
  uint32_t write_data;
  uint32_t reserved4;
} otp_struct;

/**
 * @fn int otp_init 
 * @brief Function to initialize the otp.
 * @details This initialization is done based on the Initialization waveforms for dual voltage
 *          mode given in the databook. 
 * @return The object pointing to OTP base address.
 */
int OTP_Init();

/**
 * @fn uint8_t otp_read(uint32_t addr_in) 
 * @brief Function to read data from the OTP.
 * @details The control register is set accordingly and goes into while loop until the read
 *          data is ready. The OTP returns an 8 bit data for the address specified.
 * @param The address from where the data to be read. The address needs to be byte aligned.
 * @return The read value.
 */
uint8_t OTP_Read(uint32_t addr_in);

/**
 * @fn int otp_write(uint32_t addr_in, uint32_t data_in) 
 * @brief Function to write data into the OTP.
 * @details The control registers are set accordingly to do a write operation. The function
 *          will wait until the PROGRAMMIN_SUCCESS signal has been sent. In case of a 
 *          failure in programming, the a error message will be printed mentioning it has
 *          failed to program and returns.
 *          The algorithm used is the smart programming algo in the databook. The function
 *          writes to the OTP only when the data to be written is '1'.
 * @param The address from where the data to be writen. The address needs to be bit aligned.
 * @param The data to be written
 * @return The read value.
 */
int OTP_Write(uint32_t addr_in, uint32_t data_in);

/**
 * @fn uint8_t OTP_Read32bitData(uint32_t addr_in, uint8_t* data_out, uint8_t data_len)
 * @brief function to read 32-bit data
 * @details reads 32-bit data from a specified address in OTP memory and stores it in an output buffer.
 * @param addr_in The `addr_in` parameter is the input address from which the data will be read. It is
 * a 32-bit unsigned integer (`uint32_t`).
 * @param data_out The `data_out` parameter is a pointer to an array where the 32-bit data read from
 * the OTP (One-Time Programmable) memory will be stored.
 * @param data_len The `data_len` parameter in the `OTP_Read32bitData` function represents the length
 * of the data that you want to read from the OTP (One-Time Programmable) memory. It specifies the
 * number of bytes that you want to read and store in the `data_out` buffer.
 */
uint8_t OTP_Eead32bitData(uint32_t addr_in, uint8_t* data_out, uint8_t data_len);

#ifdef __cplusplus
}
#endif
