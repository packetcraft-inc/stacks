/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Audio control driver for Maxim 9867.
 *
 *  Copyright (c) 2018-2019 Arm Ltd.
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

#include "pal_audio_amp.h"
#include "pal_twi.h"
#include "pal_types.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      Slave address. */
#define MAXIM9867_SLAVE_ADDRESS 0x18

/*! \brief      Default volume. */
#define AUDIO_AMP_VOLUME_DEFAULT 0x09

/*! \brief      Highest volume. */
#define AUDIO_AMP_VOLUME_HIGHEST 0x00

/*! \brief      Lowest volume. */
#define AUDIO_AMP_VOLUME_LOWEST 0x28

/*! \brief      Mute volume. */
#define AUDIO_AMP_VOLUME_MUTE 0x3F

#ifdef DEBUG

/*! \brief      Parameter check. */
#define AUDIO_AMP_PARAM_CHECK(expr)           { if (!(expr)) { audioAmpCb.state = PAL_AUDIO_AMP_STATE_ERROR; return; } }

/*! \brief      Parameter check, with return value. */
#define AUDIO_AMP_PARAM_CHECK_RET(expr, rv)   { if (!(expr)) { audioAmpCb.state = PAL_AUDIO_AMP_STATE_ERROR; return (rv); } }

#else

/*! \brief      Parameter check (disabled). */
#define AUDIO_AMP_PARAM_CHECK(expr)

/*! \brief      Parameter check, with return value (disabled). */
#define AUDIO_AMP_PARAM_CHECK_RET(expr, rv)

#endif

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

/*! \brief      Volume control type. */
typedef enum
{
  AUDIO_AMP_VOLUME_LEFT,        /*!< Left volume control. */
  AUDIO_AMP_VOLUME_RIGHT        /*!< Right volume control. */
} AudioVolumeControl_t;

/*! \brief Control block. */
typedef struct
{
  uint8_t                        handle;            /*!< Handle. */
  PalAudioAmpState_t             state;             /*!< State. */
  AudioVolumeControl_t           volType;           /*!< Volume type. */
  uint8_t                        volLevel;          /*!< Volume level. */
  uint8_t                        volLvlMute;        /*!< Volume mute level. */
  uint8_t                        leftVolAddr;       /*!< Left volume register address. */
  uint8_t                        rightVolAddr;      /*!< Left volume register address. */
  uint8_t                        initCmdCnt;        /*!< Initiale command count. */
} PalAudioAmpCb_t;

/*! \brief      Command struct. */
typedef struct
{
  uint8_t addr;                     /*!< Command address. */
  uint8_t value;                    /*!< Command. */
} PalAudioAmpInitCmd_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief      Driver control block. */
PalAudioAmpCb_t audioAmpCb = {0, PAL_AUDIO_AMP_STATE_UNINIT, AUDIO_AMP_VOLUME_LEFT, AUDIO_AMP_VOLUME_DEFAULT, AUDIO_AMP_VOLUME_DEFAULT, 0x10, 0x11, 0};

/*! \brief      Initial commands. */
const PalAudioAmpInitCmd_t audioAmpInitCmds[] =
{
  /* Configure default PCLK to 16MHz and LRCLK to 16KHz. (MCLK should be exact 16MHz as input.) */
  {0x05, 0x1D},
  /* Set Maxim device as I2S master role and invert LRCLK. */
  {0x08, 0xC0},
  /* Set BCLK to 48x LRCLK. (For 16 bit ADC/DAC, BCLK should be >= 32x LRCLK.) */
  {0x09, 0x02},
  /* Set audio input mixer for line input. */
  {0x14, 0xA0},
  /* Headphone mode, stereo single ended, clickless. */
  {0x16, 0x04},
  /* Set left line input gain to 0dB. */
  {0x0E, 0x0C},
  /* Set right line input gain to 0dB. */
  {0x0F, 0x0C},
  /* Power on device and enable all. */
  {0x17, 0xEF},
  /* Default left volume. */
  {0x10, AUDIO_AMP_VOLUME_DEFAULT},
  /* Default right volume. */
  {0x11, AUDIO_AMP_VOLUME_DEFAULT},
};

/**************************************************************************************************
  Functions: Initialization
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      TWI operation ready callback.
 *
 *  \param      devHandle     Device handle value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void audioReadyCback(uint8_t devHandle)
{
  if (audioAmpCb.handle != devHandle)
  {
    return;
  }

  if (audioAmpCb.initCmdCnt < sizeof(audioAmpInitCmds)/2)
  {
    uint8_t initCmd[2] = {audioAmpInitCmds[audioAmpCb.initCmdCnt].addr, audioAmpInitCmds[audioAmpCb.initCmdCnt].value};
    PalTwiWriteData(audioAmpCb.handle, initCmd, sizeof(initCmd));
    audioAmpCb.initCmdCnt++;
  }
  else
  {
    if (audioAmpCb.volType == AUDIO_AMP_VOLUME_LEFT)
    {
      uint8_t initLeftVolume[2] = {audioAmpCb.leftVolAddr, audioAmpCb.volLevel};
      PalTwiWriteData(audioAmpCb.handle, initLeftVolume, sizeof(initLeftVolume));
    }
    else
    {
      uint8_t initRightVolume[2] = {audioAmpCb.rightVolAddr, audioAmpCb.volLevel};
      PalTwiWriteData(audioAmpCb.handle, initRightVolume, sizeof(initRightVolume));
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Write complete callback.
 *
 *  \param      devHandle     Device handle value.
 *  \param      success       Result.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void audioWrCompCback(uint8_t devHandle, bool_t success)
{
  if (audioAmpCb.handle != devHandle)
  {
    return;
  }

  PalTwiStopOperation(audioAmpCb.handle);

  if (!success)
  {
    /* If it is NACK, return. */
    return;
  }

  if (audioAmpCb.initCmdCnt < sizeof(audioAmpInitCmds)/2)
  {
    audioAmpCb.volType = AUDIO_AMP_VOLUME_RIGHT;
    PalTwiStartOperation(audioAmpCb.handle);
  }
  else if (audioAmpCb.state == PAL_AUDIO_AMP_STATE_UNINIT)
  {
    audioAmpCb.state = PAL_AUDIO_AMP_STATE_READY;
  }
  else
  {
    if (audioAmpCb.volType == AUDIO_AMP_VOLUME_LEFT)
    {
      audioAmpCb.volType = AUDIO_AMP_VOLUME_RIGHT;
      PalTwiStartOperation(audioAmpCb.handle);
    }
    else
    {
      audioAmpCb.state = PAL_AUDIO_AMP_STATE_READY;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize the audio device.
 *
 *  \return     None.
 *
 */
/*************************************************************************************************/
void PalAudioAmpInit(void)
{
  PalTwiInit();

  PalTwiDevConfig_t devCfg =
  {
    .devAddr = MAXIM9867_SLAVE_ADDRESS,
    .opReadyCback = audioReadyCback,
    .wrCback = audioWrCompCback,
    .rdCback = NULL
  };

  audioAmpCb.handle = PalTwiRegisterDevice(&devCfg);

  audioAmpCb.initCmdCnt = 0;
  audioAmpCb.state = PAL_AUDIO_AMP_STATE_UNINIT;

  PalTwiStartOperation(audioAmpCb.handle);
}

/*************************************************************************************************/
/*!
 *  \brief      De-Initialize the flash device.
 *
 *  \return     None.
 *
 *  Cleanup flash resources then put it into deep sleep.
 */
/*************************************************************************************************/
void PalAudioAmpDeInit(void)
{
  audioAmpCb.volType = AUDIO_AMP_VOLUME_LEFT;
  audioAmpCb.state = PAL_AUDIO_AMP_STATE_UNINIT;
}

/**************************************************************************************************
  Functions: Control and status
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Get the current state.
 *
*  \return      Current state.
 *
 *  Return the current state.
 */
/*************************************************************************************************/
PalAudioAmpState_t PalAudioAmpGetState(void)
{
  return audioAmpCb.state;
}

/*************************************************************************************************/
/*!
 *  \brief      Get the current volume value.
 *
*  \return      Current volume value.
 *
 *  Return the current volume value.
 */
/*************************************************************************************************/
uint8_t PalAudioAmpGetVol(void)
{
  return audioAmpCb.volLevel;
}

/*************************************************************************************************/
/*!
 *  \brief      Increase the audio device volume.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalAudioAmpSetVolUp(void)
{
  AUDIO_AMP_PARAM_CHECK(audioAmpCb.state == PAL_AUDIO_AMP_STATE_READY);

  if (audioAmpCb.volLevel != AUDIO_AMP_VOLUME_HIGHEST)
  {
    audioAmpCb.volLevel++;
  }

  audioAmpCb.volType = AUDIO_AMP_VOLUME_LEFT;
  audioAmpCb.state = PAL_AUDIO_AMP_STATE_BUSY;

  PalTwiStartOperation(audioAmpCb.handle);
}

/*************************************************************************************************/
/*!
 *  \brief      Decrease the audio device volume.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalAudioAmpSetVolDown(void)
{
  AUDIO_AMP_PARAM_CHECK(audioAmpCb.state == PAL_AUDIO_AMP_STATE_READY);

  if (audioAmpCb.volLevel != AUDIO_AMP_VOLUME_LOWEST)
  {
    audioAmpCb.volLevel--;
  }

  audioAmpCb.volType = AUDIO_AMP_VOLUME_LEFT;
  audioAmpCb.state = PAL_AUDIO_AMP_STATE_BUSY;

  PalTwiStartOperation(audioAmpCb.handle);
}

/*************************************************************************************************/
/*!
 *  \brief      Mute the audio device.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalAudioAmpMute(void)
{
  AUDIO_AMP_PARAM_CHECK(audioAmpCb.state == PAL_AUDIO_AMP_STATE_READY);

  audioAmpCb.volLvlMute = audioAmpCb.volLevel;
  audioAmpCb.volLevel = AUDIO_AMP_VOLUME_MUTE;

  audioAmpCb.volType = AUDIO_AMP_VOLUME_LEFT;
  audioAmpCb.state = PAL_AUDIO_AMP_STATE_BUSY;

  PalTwiStartOperation(audioAmpCb.handle);
}

/*************************************************************************************************/
/*!
 *  \brief      Unmute the audio device.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalAudioAmpUnmute(void)
{
  AUDIO_AMP_PARAM_CHECK(audioAmpCb.state == PAL_AUDIO_AMP_STATE_READY);

  audioAmpCb.volLevel = audioAmpCb.volLvlMute;

  audioAmpCb.volType = AUDIO_AMP_VOLUME_LEFT;
  audioAmpCb.state = PAL_AUDIO_AMP_STATE_BUSY;
  PalTwiStartOperation(audioAmpCb.handle);
}
