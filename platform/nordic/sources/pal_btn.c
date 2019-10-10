/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Button driver implementation.
 *
 *  Copyright (c) 2013-2019 Arm Ltd.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
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
#include "boards.h"
#include "pal_io_exp.h"
#include "nrf_gpiote.h"
#include "audio_board.h"
#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      IO expander button mask. */
#define IO_EXP_BTN_MASK                  0xE0

/*! \brief      IO expander button trigger. */
#define IO_EXP_BUTTON_TRIG               NRF_GPIO_PIN_MAP(1, 15)

/*! \brief      Button list. */
#if (AUDIO_CAPE == 1)
#define AUDIO_BUTTON_PLAY    (IO_EXP_BTN_MASK | 0)
#define AUDIO_BUTTON_PAUSE   (IO_EXP_BTN_MASK | 1)
#define AUDIO_BUTTON_FWD     (IO_EXP_BTN_MASK | 2)
#define AUDIO_BUTTON_BACK    (IO_EXP_BTN_MASK | 3)
#define AUDIO_BUTTON_MUTE    (IO_EXP_BTN_MASK | 4)
#define AUDIO_BUTTON_VOLDN   (IO_EXP_BTN_MASK | 5)
#define AUDIO_BUTTON_VOLUP   (IO_EXP_BTN_MASK | 6)

#define PAL_BTN_LIST { BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, AUDIO_BUTTON_PLAY, AUDIO_BUTTON_PAUSE, AUDIO_BUTTON_FWD, AUDIO_BUTTON_BACK, AUDIO_BUTTON_MUTE, AUDIO_BUTTON_VOLDN, AUDIO_BUTTON_VOLUP }
#else
#define PAL_BTN_LIST { BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4 }
#endif

/*! \brief      Button Max number. */
#if (AUDIO_CAPE == 1)
#define PAL_BTN_MAX                        11
#else
#define PAL_BTN_MAX                        4
#endif

/*! \brief I/O Expander definitions */
enum
{
  PAL_BTN_IO_EXP_SUB_ADDR        = 0x07
};

/*! \brief I/O Expander accessories */
enum
{
  PAL_BTN_IO_EXP_CONFIG          = 0xFF,      /*!< Defined as inputs.*/
};

/*! \brief I/O Expander polarity */
enum
{
  PAL_BTN_IO_EXP_POLARITY_NORMAL     = 0x00,      /*!< No inversion. */
  PAL_BTN_IO_EXP_POLARITY_INVERT     = 0xFF       /*!< With inversion. */
};

#ifdef DEBUG

/*! \brief      Parameter check. */
#define PAL_BTN_PARAM_CHECK(expr)           { if (!(expr)) {  return; } }

/*! \brief      Parameter check, with return value. */
#define PAL_BTN_PARAM_CHECK_RET(expr, rv)   { if (!(expr)) { return (rv); } }

#else

/*! \brief      Parameter check (disabled). */
#define PAL_BTN_PARAM_CHECK(expr)

/*! \brief      Parameter check, with return value (disabled). */
#define PAL_BTN_PARAM_CHECK_RET(expr, rv)

#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      Device control block. */
struct
{
  uint8_t             devHandle;                             /*!< IO expander TWI handle. */
  bool_t              ioDetected;                            /*!< IO expander detected. */
  PalBtnActionCback_t actionCback;                           /*!< Action call back functions. */
} palBtnCb;

/*! \brief      Button pin mappings. */
const uint8_t palBtnList[PAL_BTN_MAX] = PAL_BTN_LIST;

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      IO expander Button trigger event handler.
 *
 *  \param      result         Button read is successful or not.
 *  \param      portValue      Button value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void palBtnIoExpHandler(bool_t result, uint8_t portValue)
{
  if (!result)
  {
    return;
  }

  for (uint8_t  btnId = 0; btnId < PAL_BTN_MAX; btnId++)
  {
    if ((palBtnList[btnId] & IO_EXP_BTN_MASK) == IO_EXP_BTN_MASK)
    {
      if (portValue & (1 << (palBtnList[btnId] & 0x0F)))
      {
        if (palBtnCb.actionCback)
        {
          palBtnCb.actionCback(btnId, PAL_BTN_STATE_DOWN);
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize buttons.
 *
 *  \return     None.
 *
 *  Initialize buttons corresponding to set ordinal bit position in btnMask.
 */
/*************************************************************************************************/
void PalBtnInit(void)
{
  memset(&palBtnCb, 0, sizeof(palBtnCb));

  NVIC_ClearPendingIRQ(GPIOTE_IRQn);
  NVIC_SetPriority(GPIOTE_IRQn, 0xFF);  /* lowest priority */

  for (unsigned int btnId = 0; btnId < PAL_BTN_MAX; btnId++)
  {
    if ((palBtnList[btnId] & IO_EXP_BTN_MASK) == IO_EXP_BTN_MASK)
    {
      palBtnCb.ioDetected = TRUE;
    }
    else
    {
      nrf_gpio_cfg_input(palBtnList[btnId], BUTTON_PULL);
    }
  }

  /* Disable all GPIOTE interrupts */
  NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;

  for (unsigned int btnId = 0; btnId < PAL_BTN_MAX; btnId++)
  {
    if ((palBtnList[btnId] & IO_EXP_BTN_MASK) != IO_EXP_BTN_MASK)
    {
      uint32_t inMask =nrf_gpio_pin_read(palBtnList[btnId]);
      nrf_gpio_cfg_sense_set(palBtnList[btnId], (inMask  != 0) ? GPIO_PIN_CNF_SENSE_Low : GPIO_PIN_CNF_SENSE_High);
    }
  }

  if (palBtnCb.ioDetected)
  {
    PalIoExpInit();

    palBtnCb.devHandle = PalIoExpRegisterDevice(PAL_BTN_IO_EXP_SUB_ADDR);
    PalIoExpRegisterCback(palBtnCb.devHandle, palBtnIoExpHandler, NULL);

    /* Configure port(all pins) no inversion. */
    PalIoExpWrite(palBtnCb.devHandle, PAL_IO_EXP_OP_POL_INV, PAL_BTN_IO_EXP_POLARITY_INVERT);

    /* button trigger is used to read IO expander buttons. */
    nrf_gpio_cfg_input(IO_EXP_BUTTON_TRIG, BUTTON_PULL);
    uint32_t inMask =nrf_gpio_pin_read(IO_EXP_BUTTON_TRIG);
    nrf_gpio_cfg_sense_set(IO_EXP_BUTTON_TRIG, (inMask  != 0) ? GPIO_PIN_CNF_SENSE_Low : GPIO_PIN_CNF_SENSE_High);
  }

  /* Clear any pending event. */
  nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);

  /* Set interrupt for port event from any pin */
  NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;

  NVIC_EnableIRQ(GPIOTE_IRQn);
}

/*************************************************************************************************/
/*!
 *  \brief      De-Initialize buttons.
 *
 *  \return     None.
 *
 *  De-Initialize all buttons.
 */
/*************************************************************************************************/
void PalBtnDeInit(void)
{
  /* Disable all GPIOTE interrupts */
  NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;

  if (palBtnCb.ioDetected)
  {
    PalIoExpDeInit();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Register action callback.
 *
 *  \param      actCback    Action callback.
 *
 *  \return     None.
 *
 *  Register an action callback function for button. The actCback is called when
 *  a button changes state. This may be returned in interrupt context.
 */
/*************************************************************************************************/
void PalBtnRegister(PalBtnActionCback_t actCback)
{
  palBtnCb.actionCback = actCback;
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
PalBtnState_t PalBtnGetState(uint8_t btnId)
{
  PalBtnState_t state = PAL_BTN_STATE_UP;

  PAL_BTN_PARAM_CHECK_RET(btnId < PAL_BTN_MAX, PAL_BTN_STATE_UP);

  if (!(palBtnList[btnId] & IO_EXP_BTN_MASK))
  {
    state = (PalBtnState_t)nrf_gpio_pin_read(palBtnList[btnId]);
  }

  return state;
}

/*************************************************************************************************/
/*!
 *  \brief  Function for handling the GPIOTE interrupt.
 *
 *  \param  none
 *
 *  \return none
 */
/*************************************************************************************************/
void GPIOTE_IRQHandler(void)
{
  /* Clear event. */
  nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);

  uint32_t btnId, p0Mask;

  p0Mask = (~NRF_P0->IN);

#if defined (NRF52840_XXAA)
  uint32_t p1Mask = (~NRF_P1->IN);
#endif

  for (btnId = 0; btnId < PAL_BTN_MAX; btnId++)
  {
    if (!(palBtnList[btnId] & IO_EXP_BTN_MASK))
    {
      if (palBtnList[btnId] < P0_PIN_NUM)
      {
        if ((1 << palBtnList[btnId]) & p0Mask)
        {
          break;
        }
      }
    }
#if defined (NRF52840_XXAA)
    else
    {
      if (1 << (palBtnList[btnId] & (P0_PIN_NUM - 1)) & p1Mask)
      {
        break;
      }
    }
#endif
  }

#if defined (NRF52840_XXAA)
  if (IO_EXP_BUTTON_TRIG & p1Mask)
  {
    /* Start read IO buttons . */
    if (palBtnCb.ioDetected)
    {
      PalIoExpRead(palBtnCb.devHandle, PAL_IO_EXP_OP_INPUT);
    }
  }
#endif

  if ((palBtnCb.actionCback) && (btnId != PAL_BTN_MAX)
          && (!(palBtnList[btnId] & IO_EXP_BTN_MASK)))
  {
    palBtnCb.actionCback(btnId, PAL_BTN_STATE_DOWN);
  }
}
