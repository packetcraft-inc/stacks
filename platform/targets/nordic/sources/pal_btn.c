/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Button driver implementation.
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

#include "pal_btn.h"
#include "nrfx_gpiote.h"
#include <string.h>

#if defined(BOARD_PCA10056) | defined(BOARD_PCA10040)
#include "boards.h"
#endif

#if AUDIO_CAPE
#include "pal_io_exp.h"
#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

#ifdef DEBUG

/*! \brief      Parameter check. */
#define PAL_BTN_PARAM_CHECK(expr)           { if (!(expr)) { palBtnCb.state = PAL_BTN_STATE_ERROR; return; } }

/*! \brief      Parameter check, with return value. */
#define PAL_BTN_PARAM_CHECK_RET(expr, rv)   { if (!(expr)) { palBtnCb.state = PAL_BTN_STATE_ERROR; return (rv); } }

#else

/*! \brief      Parameter check (disabled). */
#define PAL_BTN_PARAM_CHECK(expr)

/*! \brief      Parameter check, with return value (disabled). */
#define PAL_BTN_PARAM_CHECK_RET(expr, rv)

#endif

static const uint8_t palBtnPinMap[] =
{
#if defined(BOARD_PCA10056) | defined(BOARD_PCA10040)
  BUTTON_1,
  BUTTON_2,
  BUTTON_3,
  BUTTON_4
#endif
#if defined(BOARD_NRF6832)
  25, 8, 15, 9, 10, 16
#endif
};

/*! \brief      Device control block. */
struct
{
  PalBtnActionCback_t actionCback;          /*!< Action call back function. */
  PalBtnState_t state;                      /*!< Button driver state. */
} palBtnCb;

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*! \brief  Button press event handler.
 *
 *  \param  pin     Pin that triggered the event.
 *  \param  action  Action that triggered the event.
 */
/*************************************************************************************************/
static void palBtnEventHandler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  PalBtnPos_t state;

#if defined(BOARD_NRF6832)
  if (nrf_gpio_pin_read(pin) == 1)
#else
  if (nrf_gpio_pin_read(pin) == BUTTONS_ACTIVE_STATE)
#endif
  {
    state = PAL_BTN_POS_DOWN;
  }
  else
  {
    state = PAL_BTN_POS_UP;
  }

#if defined(BOARD_NRF6832)
  switch (pin)
  {
  case 16:
    palBtnCb.actionCback(PAL_BTN_AUDIO_PLAY, state);
    return;
  case 9:
    palBtnCb.actionCback(PAL_BTN_AUDIO_VOL_UP, state);
    return;
  case 10:
    palBtnCb.actionCback(PAL_BTN_AUDIO_VOL_DN, state);
    return;
  case 25:
    palBtnCb.actionCback(PAL_BTN_AUDIO_RWD, state);
    return;
  case 15:
    palBtnCb.actionCback(PAL_BTN_AUDIO_FWD, state);
    return;
  }
#endif

  for (size_t i = 0; i < sizeof(palBtnPinMap); i++)
  {
    if (pin == palBtnPinMap[i])
    {
      palBtnCb.actionCback(i, state);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Button ID to physical button mapping.
 *
 *  \param  btnId   Button ID.
 */
/*************************************************************************************************/
static int palBtnGetPinMap(uint8_t btnId)
{
#if defined(BOARD_NRF6832)
  switch (btnId)
  {
  /* Predefined */
  case PAL_BTN_AUDIO_PLAY:
    return 15;
  case PAL_BTN_AUDIO_VOL_UP:
    return 9;
  case PAL_BTN_AUDIO_VOL_DN:
    return 10;
  case PAL_BTN_AUDIO_MUTE:
    return 16;      /* Sound Clear */

  /* App defined */
  case 0:
    return 25;
  case 1:
    return 8;
  }
#endif

  if (btnId < sizeof(palBtnPinMap))
  {
    return palBtnPinMap[btnId];
  }

  return -1;
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize buttons.
 *
 *  \param      actCback    Button event callback (called in ISR context).
 *
 *  Initialize buttons corresponding to set ordinal bit position in btnMask.
 */
/*************************************************************************************************/
void PalBtnInit(PalBtnActionCback_t actCback)
{
  nrfx_err_t err;

  #ifndef DEBUG
    (void)err;
  #endif

  if (!nrfx_gpiote_is_init())
  {
    err = nrfx_gpiote_init();
    PAL_BTN_PARAM_CHECK(err == NRFX_SUCCESS);
  }

  palBtnCb.actionCback = actCback;

  nrfx_gpiote_in_config_t cfg = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);

  #if defined(BOARD_NRF6832)
    cfg.pull = NRF_GPIO_PIN_PULLDOWN;
  #else  /* Default */
    cfg.pull = BUTTON_PULL;
  #endif

  for (size_t i = 0; i < sizeof(palBtnPinMap); i++)
  {
    uint32_t pin = palBtnPinMap[i];

    err = nrfx_gpiote_in_init(pin, &cfg, palBtnEventHandler);
    PAL_BTN_PARAM_CHECK(err == NRFX_SUCCESS);

    nrfx_gpiote_in_event_enable(pin, true);
  }

  palBtnCb.state = PAL_BTN_STATE_READY;
}

/*************************************************************************************************/
/*!
 *  \brief      De-Initialize buttons.
 *
 *  De-Initialize all buttons.
 */
/*************************************************************************************************/
void PalBtnDeInit(void)
{
  nrfx_gpiote_uninit();

  palBtnCb.state = PAL_BTN_STATE_UNINIT;
}


/*************************************************************************************************/
/*!
 *  \brief      Get the current state.
 *
 *  \return     Current state.
 *
 *  Return the current state of the TWI.
 */
/*************************************************************************************************/
PalBtnState_t PalBtnGetState(void)
{
  return palBtnCb.state;
}

/*************************************************************************************************/
/*!
 *  \brief      Get button state.
 *
 *  \param      btnId           Button ID.
 *
 *  \return     Button state.
 *
 *  Get the current button state.
 */
/*************************************************************************************************/
PalBtnPos_t PalBtnGetPosition(uint8_t btnId)
{
  PAL_BTN_PARAM_CHECK_RET(palBtnCb.state == PAL_BTN_STATE_READY, PAL_BTN_POS_INVALID);

  int pin = palBtnGetPinMap(btnId);
  PAL_BTN_PARAM_CHECK_RET(pin < 0, PAL_BTN_POS_INVALID);

  if (nrf_gpio_pin_read(pin))
  {
    return PAL_BTN_POS_UP;
  }
  else
  {
    return PAL_BTN_POS_DOWN;
  }
}
