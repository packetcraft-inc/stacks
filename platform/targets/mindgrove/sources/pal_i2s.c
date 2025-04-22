/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      I2S driver implementation.
 *
 *  Copyright (c) 2019 Arm Ltd. All Rights Reserved.
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

#include "pal_i2s.h"
#include "nrf_gpio.h"
#include "nrfx_i2s.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#ifdef DEBUG

/*! \brief      Parameter check. */
#define PAL_I2S_PARAM_CHECK(expr)           { if (!(expr)) { palI2sCb.state = PAL_I2S_STATE_ERROR; return; } }

/*! \brief      Parameter check, with return value. */
#define PAL_I2S_PARAM_CHECK_RET(expr, rv)   { if (!(expr)) { palI2sCb.state = PAL_I2S_STATE_ERROR; return (rv); } }

#else

/*! \brief      Parameter check (disabled). */
#define PAL_I2S_PARAM_CHECK(expr)

/*! \brief      Parameter check, with return value (disabled). */
#define PAL_I2S_PARAM_CHECK_RET(expr, rv)

#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief Control block. */
struct
{
  PalI2sState_t      state:8;       /*!< Current state. */
  PalI2sCompCback_t  compCback;     /*!< Completion callback. */
  nrfx_i2s_buffers_t initBuf[2];    /*!< Initial buffer. */
  void               *pCtx;         /*!< Client operation context. */
} palI2sCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      I2S completion handler.
 *
 *  \param      pRelBuf     Released buffers.
 *  \param      status      Transfer status.
 */
/*************************************************************************************************/
static void palI2sCompHandler(nrfx_i2s_buffers_t const *pRelBuf, uint32_t status)
{
  if (palI2sCb.state != PAL_I2S_STATE_BUSY)
  {
    return;
  }

  if (status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)
  {
    if ((palI2sCb.initBuf[1].p_rx_buffer) ||
        (palI2sCb.initBuf[1].p_tx_buffer))
    {
      nrfx_err_t err;

      #ifndef DEBUG
        (void)err;
      #endif

      err = nrfx_i2s_next_buffers_set(&palI2sCb.initBuf[1]);
      PAL_I2S_PARAM_CHECK(err == NRFX_SUCCESS);

      palI2sCb.initBuf[1].p_tx_buffer = palI2sCb.initBuf[1].p_rx_buffer = NULL;
    }

    if (pRelBuf && (pRelBuf->p_rx_buffer || pRelBuf->p_tx_buffer))
    {
      palI2sCb.compCback(palI2sCb.pCtx);
    }
  }
}

/**************************************************************************************************
  Functions: Initialization
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize I2S resources.
 */
/*************************************************************************************************/
void PalI2sInit(void)
{
  PAL_I2S_PARAM_CHECK(palI2sCb.state == PAL_I2S_STATE_UNINIT);

  palI2sCb.state = PAL_I2S_STATE_IDLE;
}

/*************************************************************************************************/
/*!
 *  \brief      De-initialize I2S resource.
 */
/*************************************************************************************************/
void PalI2sDeInit(void)
{
  PAL_I2S_PARAM_CHECK(palI2sCb.state == PAL_I2S_STATE_IDLE);

  palI2sCb.state = PAL_I2S_STATE_UNINIT;
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
 *  Return the current state of the I2S driver.
 */
/*************************************************************************************************/
PalI2sState_t PalI2sGetState(void)
{
  return palI2sCb.state;
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize I2S configuration.
 *
 *  \param      pCfg         Pointer to I2s Configuration.
 */
/*************************************************************************************************/
void PalI2sConfig(PalI2sConfig_t *pCfg)
{
  PAL_I2S_PARAM_CHECK(palI2sCb.state == PAL_I2S_STATE_IDLE);
  PAL_I2S_PARAM_CHECK(pCfg->frmCback != NULL);

  palI2sCb.compCback = pCfg->frmCback;
  palI2sCb.pCtx = pCfg->pCtx;

  nrfx_i2s_config_t cfg = NRFX_I2S_DEFAULT_CONFIG;

  switch (pCfg->mode)
  {
  case PAL_I2S_MODE_MASTER:
    cfg.mode = NRF_I2S_MODE_MASTER;
    break;
  case PAL_I2S_MODE_SLAVE:
  default:
    cfg.mode = NRF_I2S_MODE_SLAVE;
    break;
  }

  switch (pCfg->bitDepth)
  {
  case 8:
    cfg.sample_width = NRF_I2S_SWIDTH_8BIT;
    break;
  case 16:
  default:
    cfg.sample_width = NRF_I2S_SWIDTH_16BIT;
    break;
  case 24:
    cfg.sample_width = NRF_I2S_SWIDTH_24BIT;
    break;
  }

  switch (pCfg->chan)
  {
  case PAL_I2S_CH_LEFT_BIT:
    cfg.channels = NRF_I2S_CHANNELS_LEFT;
    break;
  case PAL_I2S_CH_RIGHT_BIT:
    cfg.channels = NRF_I2S_CHANNELS_RIGHT;
    break;
  case PAL_I2S_CH_LEFT_BIT | PAL_I2S_CH_RIGHT_BIT:
  default:
    cfg.channels = NRF_I2S_CHANNELS_STEREO;
    break;
  }

  /* Supply codec with fastest MCLK possible: MCLK=4MHz. */
  cfg.mck_setup = NRF_I2S_MCK_32MDIV8;
  cfg.ratio = NRF_I2S_RATIO_48X;

  nrfx_err_t err;

  #ifndef DEBUG
    (void)err;
  #endif

  err = nrfx_i2s_init(&cfg, palI2sCompHandler);
  PAL_I2S_PARAM_CHECK(err == NRFX_SUCCESS);

  palI2sCb.state = PAL_I2S_STATE_READY;
}

/*************************************************************************************************/
/*!
 *  \brief      De-initialize I2S configuration.
 */
/*************************************************************************************************/
void PalI2sDeConfig(void)
{
  PAL_I2S_PARAM_CHECK(palI2sCb.state == PAL_I2S_STATE_READY);

  nrfx_i2s_uninit();

  palI2sCb.state = PAL_I2S_STATE_IDLE;
}

/**************************************************************************************************
  Functions: Data Transfer
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Read data from I2S interface.
 *
 *  \param      pData       Read buffer.
 *  \param      len         Number of bytes to write.
 */
/*************************************************************************************************/
void PalI2sReadData(uint8_t *pData, uint16_t len)
{
  PAL_I2S_PARAM_CHECK(len != 0);
  PAL_I2S_PARAM_CHECK(pData != NULL);

  nrfx_err_t err;

  #ifndef DEBUG
    (void)err;
  #endif

  switch (palI2sCb.state)
  {
    case PAL_I2S_STATE_BUSY:
    default:
    {
      nrfx_i2s_buffers_t i2sBuf =
      {
        .p_rx_buffer = (uint32_t *)pData,
        .p_tx_buffer = NULL
      };

      err = nrfx_i2s_next_buffers_set(&i2sBuf);
      PAL_I2S_PARAM_CHECK(err == NRFX_SUCCESS);
      break;
    }

    case PAL_I2S_STATE_READY:
    {
      if (!palI2sCb.initBuf[0].p_rx_buffer)
      {
        palI2sCb.initBuf[0].p_rx_buffer = (uint32_t *)pData;
      }
      else
      {
        palI2sCb.initBuf[1].p_rx_buffer = (uint32_t *)pData;

        err = nrfx_i2s_start(&palI2sCb.initBuf[0], len / sizeof(uint32_t), 0);  /* length in size of words */
        PAL_I2S_PARAM_CHECK(err == NRFX_SUCCESS);

        palI2sCb.initBuf[0].p_rx_buffer = NULL;

        palI2sCb.state = PAL_I2S_STATE_BUSY;
      }
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Write data to I2S interface.
 *
 *  \param      pData       Write buffer.
 *  \param      len         Number of bytes to write.
 */
/*************************************************************************************************/
void PalI2sWriteData(const uint8_t *pData, uint16_t len)
{
  PAL_I2S_PARAM_CHECK(len != 0);
  PAL_I2S_PARAM_CHECK(pData != NULL);

  nrfx_err_t err;

  #ifndef DEBUG
    (void)err;
  #endif

  switch (palI2sCb.state)
  {
    case PAL_I2S_STATE_BUSY:
    default:
    {
      nrfx_i2s_buffers_t i2sBuf =
      {
        .p_rx_buffer = NULL,
        .p_tx_buffer = (uint32_t *)pData
      };

      err = nrfx_i2s_next_buffers_set(&i2sBuf);
      PAL_I2S_PARAM_CHECK(err == NRFX_SUCCESS);
      break;
    }

    case PAL_I2S_STATE_READY:
    {
      if (!palI2sCb.initBuf[0].p_tx_buffer)
      {
        palI2sCb.initBuf[0].p_tx_buffer = (uint32_t *)pData;
      }
      else
      {
        palI2sCb.initBuf[1].p_tx_buffer = (uint32_t *)pData;

        err = nrfx_i2s_start(&palI2sCb.initBuf[0], len / sizeof(uint32_t), 0);  /* length in size of words */
        PAL_I2S_PARAM_CHECK(err == NRFX_SUCCESS);

        palI2sCb.initBuf[0].p_tx_buffer = NULL;

        palI2sCb.state = PAL_I2S_STATE_BUSY;
      }
      break;
    }
  }
}
