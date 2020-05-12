/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      SPI driver implementation.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include "pal_spi.h"
#include "nrfx_spim.h"
#include <string.h>

#if defined(BOARD_PCA10056) | defined(BOARD_PCA10040)
#include "boards.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

#ifdef DEBUG

/*! \brief      Parameter check. */
#define PAL_SPI_PARAM_CHECK(expr)           { if (!(expr)) { palSpiCb.state = PAL_SPI_STATE_ERROR; return; } }

/*! \brief      Parameter check, with return value. */
#define PAL_SPI_PARAM_CHECK_RET(expr, rv)   { if (!(expr)) { palSpiCb.state = PAL_SPI_STATE_ERROR; return (rv); } }

#else

/*! \brief      Parameter check (disabled). */
#define PAL_SPI_PARAM_CHECK(expr)

/*! \brief      Parameter check, with return value (disabled). */
#define PAL_SPI_PARAM_CHECK_RET(expr, rv)

#endif

#if defined(BOARD_PCA10056)
#define SPI_SCK_PIN     SER_APP_SPIM0_SCK_PIN
#define SPI_MOSI_PIN    SER_APP_SPIM0_MOSI_PIN
#define SPI_MISO_PIN    SER_APP_SPIM0_MISO_PIN
#define SPI_SS_PIN      SER_APP_SPIM0_SS_PIN
#endif

#if defined(BOARD_PCA10040)
#define SPI_SCK_PIN     SPIM0_SCK_PIN
#define SPI_MOSI_PIN    SPIM0_MOSI_PIN
#define SPI_MISO_PIN    SPIM0_MISO_PIN
#define SPI_SS_PIN      SPIM0_SS_PIN
#endif

#if defined(BOARD_NRF6832)
#define SPI_SCK_PIN     11
#define SPI_MOSI_PIN    12
#define SPI_MISO_PIN    13
#define SPI_SS_PIN      14
#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      Driver control block. */
static struct
{
  PalSpiState_t state:8;            /*!< Current state. */
  PalSpiCompCback compCback;        /*!< Completion callback. */
} palSpiCb;

static const nrfx_spim_t palSpiInst = NRFX_SPIM_INSTANCE(1);

/**************************************************************************************************
  Functions: Initialization
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Control callback.
 *
 *  \param      event     Event parameters.
 */
/*************************************************************************************************/
static void palSpiCallback(nrfx_spim_evt_t const *pEvent, void *pContext)
{
  palSpiCb.state = PAL_SPI_STATE_READY;

  if (palSpiCb.compCback)
  {
    palSpiCb.compCback();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize SPI resources.
 */
/*************************************************************************************************/
void PalSpiInit(const PalSpiConfig_t *pCfg)
{
  memset(&palSpiCb, 0, sizeof(palSpiCb));

  palSpiCb.compCback = pCfg->compCback;

  nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
  spi_config.ss_pin         = SPI_SS_PIN;
  spi_config.miso_pin       = SPI_MISO_PIN;
  spi_config.mosi_pin       = SPI_MOSI_PIN;
  spi_config.sck_pin        = SPI_SCK_PIN;
  spi_config.ss_active_high = FALSE;

       if (pCfg->clkRateHz >= 8000000) spi_config.frequency = NRF_SPIM_FREQ_8M;
  else if (pCfg->clkRateHz >= 4000000) spi_config.frequency = NRF_SPIM_FREQ_4M;
  else if (pCfg->clkRateHz >= 2000000) spi_config.frequency = NRF_SPIM_FREQ_2M;
  else if (pCfg->clkRateHz >= 1000000) spi_config.frequency = NRF_SPIM_FREQ_1M;
  else if (pCfg->clkRateHz >=  500000) spi_config.frequency = NRF_SPIM_FREQ_500K;
  else if (pCfg->clkRateHz >=  250000) spi_config.frequency = NRF_SPIM_FREQ_250K;
  else if (pCfg->clkRateHz >=  125000) spi_config.frequency = NRF_SPIM_FREQ_125K;

  nrfx_err_t err = nrfx_spim_init(&palSpiInst, &spi_config, palSpiCallback, NULL);
  PAL_SPI_PARAM_CHECK(err == NRFX_SUCCESS);

  #ifndef DEBUG
    (void)err;
  #endif

  palSpiCb.state = PAL_SPI_STATE_READY;
}

/*************************************************************************************************/
/*!
 *  \brief      De-Initialize the SPI resources.
 */
/*************************************************************************************************/
void PalSpiDeInit(void)
{
  PAL_SPI_PARAM_CHECK(palSpiCb.state == PAL_SPI_STATE_READY);

  nrfx_spim_uninit(&palSpiInst);

  palSpiCb.state = PAL_SPI_STATE_UNINIT;
}

/**************************************************************************************************
  Functions: Control and Status
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Get the current state.
 *
 *  \return     Current state.
 *
 *  Return the current state of the TWI.
 */
/*************************************************************************************************/
PalSpiState_t PalSpiGetState(void)
{
  return palSpiCb.state;
}

/**************************************************************************************************
  Functions: Data Transfer
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Always start an operation before reading or writing on TWI interface.
 *
 *  \param      handle      Device handle.
 */
/*************************************************************************************************/
void PalSpiDataExchange(uint8_t *pRdData, uint16_t rdDataLen, const uint8_t *pWrData, uint16_t wrDataLen)
{
  PAL_SPI_PARAM_CHECK(palSpiCb.state == PAL_SPI_STATE_READY);

  palSpiCb.state = PAL_SPI_STATE_BUSY;

  nrfx_spim_xfer_desc_t desc = NRFX_SPIM_SINGLE_XFER(pWrData, wrDataLen, pRdData, rdDataLen);

  nrfx_err_t err = nrfx_spim_xfer(&palSpiInst, &desc, 0);
  PAL_SPI_PARAM_CHECK(err == NRFX_SUCCESS);
  #ifndef DEBUG
    (void)err;
  #endif
}
