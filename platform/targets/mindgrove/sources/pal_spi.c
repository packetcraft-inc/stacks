/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Stubbed SPI driver implementation.
 *
 *  Copyright (c) 2025 Mindgrove Technologies.
 */
/*************************************************************************************************/

#include "pal_spi.h"
#include <string.h>

/**************************************************************************************************
  Local Types
**************************************************************************************************/

static struct
{
  PalSpiState_t state;
  PalSpiCompCback compCback;
} palSpiCb = {
  .state = PAL_SPI_STATE_UNINIT,
  .compCback = NULL
};

/*************************************************************************************************/
/*!
 *  \brief      Initialize SPI resources.
 */
/*************************************************************************************************/
void PalSpiInit(const PalSpiConfig_t *pCfg)
{
  palSpiCb.state = PAL_SPI_STATE_READY;
  palSpiCb.compCback = pCfg ? pCfg->compCback : NULL;
}

/*************************************************************************************************/
/*!
 *  \brief      De-Initialize the SPI resources.
 */
/*************************************************************************************************/
void PalSpiDeInit(void)
{
  palSpiCb.state = PAL_SPI_STATE_UNINIT;
}

/*************************************************************************************************/
/*!
 *  \brief      Get the current state.
 */
/*************************************************************************************************/
PalSpiState_t PalSpiGetState(void)
{
  return palSpiCb.state;
}

/*************************************************************************************************/
/*!
 *  \brief      Dummy exchange function.
 */
/*************************************************************************************************/
void PalSpiDataExchange(uint8_t *pRdData, uint16_t rdDataLen, const uint8_t *pWrData, uint16_t wrDataLen)
{
  (void)pWrData;
  (void)wrDataLen;

  if (pRdData && rdDataLen > 0)
  {
    memset(pRdData, 0xFF, rdDataLen);  // Stubbed response
  }

  if (palSpiCb.compCback)
  {
    palSpiCb.compCback();
  }
}
