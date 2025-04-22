#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/*extern unsigned int wd_cycles;
extern unsigned short reset_cycles;
extern unsigned short control_reg;
*/

#define WATCHDOG_EN 			1<<0
#define RESET_INTERRUPT_MODE	1<<1
#define SOFT_RESET				1<<2


#define INTERRUPT_MODE			1
#define SOFT_RESET_MODE			7
#define RESET_MODE				3

#define WD_CYCLES_REG_ADDR		0x40400
#define CONTROL_REG_ADDR		0x40408
#define RESET_CYCLES_REG_ADDR	0x40410


void wdt_init(void);
void wdtimer_start(uint16_t mode, uint16_t rcycles, uint32_t wcycles);
void wdtimer_disable();
void wdtimer_reset();

#ifdef __cplusplus
}
#endif	
