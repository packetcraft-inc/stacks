#include "io.h"
#include "log.h"

#ifdef __cplusplus
extern "c"{
#endif

#define ADC_BASE_ADDRESS  0x00032000

/* control register */
#define ADC_CONTROLREG    0x00
#define ADC_DISLVL        0x0001
#define ADC_INIT          0x0002
#define ADC_SOC           0x0020
#define ADC_FREE_RUN      0x0100
#define ADC_SINGLE_CONVERSION  0x0000
#define ADC_INTR_ENABLE   0x0200
#define ADC_INTR_DISABLE  0x0000
#define ADC_12BIT         0x00C0
#define ADC_10BIT         0x0080
#define ADC_8BIT          0x0040
#define ADC_6BIT          0x0000
#define ADC_CHANNEL_SEL(x)  x<<2

/* output register */
#define ADC_OUTPUTREG     0x04
#define ADC_EOC           0x1000
#define ADC_OUTPUT_MASK   0x0FFF

/**
 * @fn The function adc_init() initializes the ADC module and waits for the initialization to complete
 * before returning success.
 * 
 * @return The function `adc_init()` is returning the value `SUCCESS`.
 */
int ADC_Init();

/**
 * @fn The function int config_control_reg(int channel, int resolution, int en_freerun, int en_intr) configures the control register of an ADC with specified channel, 
 * resolution, free run, and interrupt enable settings.
 * 
 * @param channel The `channel` parameter in the `config_control_reg` function is used to select the
 * specific ADC channel that you want to configure. It is an integer value representing the channel
 * number.
 * @param resolution Resolution is the number of bits used to represent the analog signal in the
 * digital domain. It determines the level of detail or precision of the conversion. Common resolutions
 * for ADCs are 8-bit, 10-bit, 12-bit, etc.
 * @param en_freerun The `en_freerun` parameter in the `config_control_reg` function is used to enable
 * or disable the free-running mode for the ADC (Analog-to-Digital Converter). In free-running mode,
 * the ADC continuously samples the input without the need for external triggers, providing a
 * continuous stream of
 * @param en_intr The `en_intr` parameter in the `config_control_reg` function is used to enable or
 * disable interrupts for the ADC channel configuration. If `en_intr` is set to a non-zero value,
 * interrupts will be enabled; otherwise, interrupts will be disabled.
 * 
 * @return The function `config_control_reg` is returning the value `SUCCESS`.
 */
int ADC_CCR(int channel, int resolution, int en_freerun, int en_intr);

/**
 * @fn The function start_conversion() sets a control register bit to initiate an ADC conversion.
 * 
 * @return The function `start_conversion()` is returning the value `SUCCESS`.
 */
int ADC_StartConversion();

/**
 * @fn The function read_output() reads the output value from an ADC register after checking for end of
 * conversion and free run conditions.
 * 
 * @return The function `read_output` returns the value of the ADC output after waiting for the End of
 * Conversion (EOC) flag to be set and ensuring that the ADC is not in free run mode. The function
 * reads the output register of the ADC instance, masks it with `ADC_OUTPUT_MASK`, and returns the
 * result.
 */
int ADC_ReadOutput();

/**
 * @fn The function end_conversion() disables the start of conversion for an ADC instance and returns a
 * success status.
 * 
 * @return The function `end_conversion()` is returning the value `SUCCESS`.
 */
int ADC_EndConversion();


#ifdef __cplusplus
}
#endif