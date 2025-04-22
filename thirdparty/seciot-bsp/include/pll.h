#ifndef PLL_H
#define PLL_H

#include <stdint.h>
#include "platform.h"

#define PLL_LOCK 1

typedef enum
{
    CLOCK_700,
    CLOCK_350,
    CLOCK_175,
    CLOCK_35
}pll_clk_source;

/*
    -Method to lock the AXI. Further attempts to change the clock source will be invalid
    -This method is one time callable 
*/

uint8_t pll_axi_lock(void); 
uint8_t pll_clk_select(pll_clk_source select_num, uint8_t pll_lock);
uint8_t pll_clock_check(void);

#endif
