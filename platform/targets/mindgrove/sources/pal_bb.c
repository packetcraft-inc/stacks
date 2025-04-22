#include "pal_types.h"
#include "pal_bb.h"
#include <string.h>
#include <stdio.h>

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief  Driver enable count. */
static uint8_t palBbEnableCnt;

/*! \brief  Protocol ID. */
static uint8_t palBbProtId = BB_PROT_BLE;

/*! \brief  Table of timer IRQ callbacks. */
static bbDrvIrqCback_t palBbTimerIrqCbackTbl[BB_PROT_NUM];

/*! \brief  Table of radio IRQ callbacks. */
static bbDrvIrqCback_t palBbRadioIrqCbackTbl[BB_PROT_NUM];

/*************************************************************************************************/
/*!
 *  \brief      Initialize the baseband driver (stub).
 */
/*************************************************************************************************/
void PalBbInit(void)
{
  palBbEnableCnt = 0;
  memset(palBbTimerIrqCbackTbl, 0, sizeof(palBbTimerIrqCbackTbl));
  memset(palBbRadioIrqCbackTbl, 0, sizeof(palBbRadioIrqCbackTbl));
}

/*************************************************************************************************/
/*!
 *  \brief      Enable the BB hardware (stub).
 */
/*************************************************************************************************/
void PalBbEnable(void)
{
  palBbEnableCnt++;
}

/*************************************************************************************************/
/*!
 *  \brief      Disable the BB hardware (stub).
 */
/*************************************************************************************************/
void PalBbDisable(void)
{
  if (palBbEnableCnt)
  {
    palBbEnableCnt--;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Load BB timing configuration (stub).
 */
/*************************************************************************************************/
void PalBbLoadCfg(PalBbCfg_t *pCfg)
{
  pCfg->clkPpm = 20;
  pCfg->rfSetupDelayUsec  = BB_RF_SETUP_DELAY_US;
  pCfg->maxScanPeriodMsec = BB_MAX_SCAN_PERIOD_MS;
  pCfg->schSetupDelayUsec = BB_SCH_SETUP_DELAY_US;

#if (BB_CLK_RATE_HZ == 32768)
  pCfg->BbTimerBoundaryUsec = BB_RTC_MAX_VALUE_US;
#elif (BB_CLK_RATE_HZ == 8000000)
  pCfg->BbTimerBoundaryUsec = BB_TIMER_8MHZ_MAX_VALUE_US;
#elif (BB_CLK_RATE_HZ == 1000000)
  pCfg->BbTimerBoundaryUsec = BB_TIMER_1MHZ_MAX_VALUE_US;
#else
  #error "Unsupported platform clock rate."
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      Get current time in microseconds (stub).
 */
/*************************************************************************************************/
uint32_t PalBbGetCurrentTime(void)
{
  // Return a stub value; in real implementation, read hardware timer.
  return palBbEnableCnt ? 123456 : 0;
}

/*************************************************************************************************/
/*!
 *  \brief      Get timestamp (stub).
 */
/*************************************************************************************************/
bool_t PalBbGetTimestamp(uint32_t *pTime)
{
  if (palBbEnableCnt == 0 || pTime == NULL)
  {
    return FALSE;
  }

  *pTime = 123456;  // Stub time value.
  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      Register protocol IRQ callbacks.
 */
/*************************************************************************************************/
void PalBbRegisterProtIrq(uint8_t protId, bbDrvIrqCback_t timerCback, bbDrvIrqCback_t radioCback)
{
  if (protId < BB_PROT_NUM)
  {
    palBbTimerIrqCbackTbl[protId] = timerCback;
    palBbRadioIrqCbackTbl[protId] = radioCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Set active protocol ID.
 */
/*************************************************************************************************/
void PalBbSetProtId(uint8_t protId)
{
  palBbProtId = protId;
}

/*************************************************************************************************/
/*!
 *  \brief      Stub RADIO interrupt handler.
 */
/*************************************************************************************************/
void RADIO_IRQHandler(void)
{
  if (palBbRadioIrqCbackTbl[palBbProtId])
  {
    palBbRadioIrqCbackTbl[palBbProtId]();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Stub TIMER interrupt handler.
 */
/*************************************************************************************************/
void TIMER0_IRQHandler(void)
{
  if (palBbTimerIrqCbackTbl[palBbProtId])
  {
    palBbTimerIrqCbackTbl[palBbProtId]();
  }
}
