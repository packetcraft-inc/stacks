/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Audio codec driver implementation.
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

#include "pal_codec.h"
#include "pal_i2s.h"
#include "pal_led.h"
#include "hci_defs.h"
#include "wsf_trace.h"
#include <string.h>

#if (AUDIO_CAPE && CODEC_BLUEDROID)
#include "oi_codec_sbc.h"
#include "sbc_encoder.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

#ifndef AUDIO_HQ
/*! \brief  Enable high quality codec. */
#define AUDIO_HQ                TRUE
#endif

#if (AUDIO_HQ)
#define AUDIO_FRM_PER_PKT       5       /*!< Number of SBC frames per ISO packet. */
#define AUDIO_PERIOD_USEC       (AUDIO_FRM_PER_PKT * 2000) /*!< Audio block period (SDU Interval). */
#define AUDIO_SAMPLE_RATE       16000   /*!< Sample rate. */
#define AUDIO_BIT_DEPTH         16      /*!< Bit depth. */
#define AUDIO_NUM_BLOCK         12      /*!< Number of blocks (buffer size). */
#endif

/*! \brief  Number of streams. */
#define AUDIO_NUM_STREAM        2

/*! \brief  Number of samples (one channel). */
#define AUDIO_NUM_SAMPLES       (AUDIO_SAMPLE_RATE / 1000 * AUDIO_PERIOD_USEC / 1000)

/*! \brief  Block length in bytes (stereo channels). */
#define AUDIO_BLOCK_LEN         (AUDIO_NUM_SAMPLES * 2 * (AUDIO_BIT_DEPTH / 8))

/*! \brief  Get PCM block index and increment. */
#define AUDIO_GET_IDX(c)        ((c) % AUDIO_NUM_BLOCK)

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

#if (AUDIO_CAPE)

/*! \brief  Stream context. */
typedef struct
{
  bool_t enabled;               /*!< Stream enabled. */
  uint16_t id;                  /*!< Stream ID. */
  PalAudioDir_t dir:8;          /*!< Audio direction. */
  uint16_t chMask;              /*!< Channel mask. */
  uint32_t intervalUsec;        /*!< Interval in milliseconds. */
  PalCodecDataReady_t rdyCback; /*!< Input data ready callback, ignored for output. */

#if (CODEC_BLUEDROID)
  /* Codec */
  union
  {
    SBC_ENC_PARAMS enc;
    OI_CODEC_SBC_DECODER_CONTEXT dec;
  } sbc;
#endif

  /* PCM */
  int16_t pcm[AUDIO_NUM_BLOCK][AUDIO_NUM_SAMPLES * 2];
                                /*!< Stereo PCM buffer. */
  uint32_t prodCtr;             /*!< Producer frame counter. */
  uint32_t consCtr;             /*!< Consumer frame counter. */
  uint32_t numProd;             /*!< Number of consecutive output packets. */
} cs47x_Stream_t;

#endif

/**************************************************************************************************
  Globals
**************************************************************************************************/

#if (AUDIO_CAPE)

/*! \brief  Stream table. */
static cs47x_Stream_t palCodecStreamTbl[AUDIO_NUM_STREAM] = { 0 };

/*! \brief  Decoder scratch memory. */
uint32_t palCodecScratch[400];

#endif

/**************************************************************************************************
  Functions: Control and Status
**************************************************************************************************/

/*************************************************************************************************/
/*!
 * \brief   Read local supported codecs.
 *
 * \param   pNumStd     Input is size of \a stdCodecs and output is actual number of standard codecs.
 * \param   stdCodecs   Standard codec info.
 * \param   pNumVs      Input is size of \a vsCodecs and output is actual number of vendor specific codecs.
 * \param   vsCodecs    Vendor specific codec info.
 *
 * \return  None.
 */
/*************************************************************************************************/
void PalCodecReadLocalSupportedCodecs(uint8_t *pNumStd, AudioStdCodecInfo_t stdCodecs[],
                                      uint8_t *pNumVs, AudioVsCodecInfo_t vsCodecs[])
{
  if (*pNumStd > 1)
  {
    *pNumStd = 1;
    stdCodecs[0].codecId = HCI_ID_LC3;
  }

  /* No VS codecs. */
  *pNumVs = 0;
}

/*************************************************************************************************/
/*!
 * \brief   Read local supported codec capabilities.
 *
 * \param   codingFmt   Coding format.
 * \param   compId      Company ID.
 * \param   vsCodecId   Vendor specific codec ID.
 * \param   dir         Direction.
 *
 * \return  TRUE if valid, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t PalCodecReadLocalSupportedCodecCapabilities(uint8_t codingFmt, uint16_t compId, uint16_t vsCodecId, PalAudioDir_t dir)
{
  switch (codingFmt)
  {
  case HCI_ID_LC3:
    return TRUE;

  case HCI_ID_VS:
  default:
    return FALSE;
  }
}

/*************************************************************************************************/
/*!
 * \brief   Read local supported codecs.
 *
 * \param   codingFmt   Coding format.
 * \param   compId      Company ID.
 * \param   vsCodecId   Vendor specific codec ID.
 * \param   dir         Direction.
 *
 * \return  TRUE if valid, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t PalCodecReadLocalSupportedControllerDelay(uint8_t codingFmt, uint16_t compId, uint16_t vsCodecId, PalAudioDir_t dir,
                                                 uint32_t *pMinDly, uint32_t *pMaxDly)
{
  switch (codingFmt)
  {
  case HCI_ID_LC3:
    switch (dir)
    {
    case PAL_CODEC_DIR_INPUT:
      *pMinDly = 1000;
      *pMaxDly = 2000;
      break;
    case PAL_CODEC_DIR_OUTPUT:
      *pMinDly = 100;
      *pMaxDly = 200;
      break;
    default:
      return HCI_ERR_UNSUP_FEAT;
    }
    return TRUE;

  case HCI_ID_VS:
  default:
    return FALSE;
  }
}

/*************************************************************************************************/
/*!
 * \brief   Read local supported codecs.
 *
 * \param   dir         Direction.
 * \param   dataPathId  Data Path ID.
 *
 * \return  TRUE if valid, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t PalCodecConfigureDataPath(PalAudioDir_t dir, uint8_t dataPathId)
{
  /* TODO needs implementation. */
  return TRUE;
}

#if (AUDIO_CAPE && CODEC_BLUEDROID)

/**************************************************************************************************
  Functions: SBC Encoder/Decoder
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Open audio path.
 */
/*************************************************************************************************/
static void palCodecSbcOpen(cs47x_Stream_t *pStream)
{
  switch (pStream->dir)
  {
    case PAL_CODEC_DIR_INPUT:
    {
      pStream->sbc.enc.s16ChannelMode      = SBC_JOINT_STEREO;
      pStream->sbc.enc.s16NumOfChannels    = 2;
      pStream->sbc.enc.s16SamplingFreq     = SBC_sf16000;
      pStream->sbc.enc.s16NumOfBlocks      = 4;
      pStream->sbc.enc.s16NumOfSubBands    = 8;
      /* pStream->sbc.enc.s16BitPool=38 set by SBC_Encoder_Init(). */
      pStream->sbc.enc.u16BitRate          = 128; /* 128kbps */
      pStream->sbc.enc.s16AllocationMethod = SBC_LOUDNESS;
      pStream->sbc.enc.mSBCEnabled         = FALSE;

      SBC_Encoder_Init(&pStream->sbc.enc);
      break;
    }

    case PAL_CODEC_DIR_OUTPUT:
    {
      OI_CODEC_SBC_DecoderReset(&pStream->sbc.dec,
                                palCodecScratch, sizeof(palCodecScratch),
                                2, 2, FALSE);
      break;
    }

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Close audio path.
 */
/*************************************************************************************************/
static void palCodecSbcClose(cs47x_Stream_t *pStream)
{
  /* No action. */
}

/*************************************************************************************************/
/*!
 *  \brief      Read audio data from ADC and encode.
 *
 *  \param      pData         Pointer to the encoded packet.
 *
 *  \return     Number of bytes of the encoded packet.
 */
/*************************************************************************************************/
static uint16_t palCodecSbcEncode(cs47x_Stream_t *pStream, uint8_t *pSbcBuf, uint16_t len)
{
  int32_t numBufAvail = pStream->prodCtr - pStream->consCtr;

  if (numBufAvail < 0)
  {
    AUD_TRACE_WARN2("Input audio stream underrun, id=%u, pktCtr[15:0]=%u", pStream->id, pStream->consCtr);

    pStream->consCtr = pStream->prodCtr;

    /* No audio data available. */
    return 0;
  }
  else if (numBufAvail > AUDIO_NUM_BLOCK)
  {
    AUD_TRACE_WARN2("Input audio stream overrun, id=%u, pktCtr[15:0]=%u", pStream->id, pStream->consCtr);

    /* Skip oldest audio blocks. */
    pStream->consCtr = pStream->prodCtr - AUDIO_NUM_BLOCK;
  }

  PalLedOn(2);

  pStream->sbc.enc.pu8Packet           = pSbcBuf;
  pStream->sbc.enc.u8NumPacketToEncode = AUDIO_FRM_PER_PKT;
  pStream->sbc.enc.ps16PcmBuffer       = (SINT16 *)pStream->pcm[AUDIO_GET_IDX(pStream->consCtr)];

  /* Encode PCM block to SBC frame. */
  SBC_Encoder(&pStream->sbc.enc);

  PalLedOff(2);

  return AUDIO_FRM_PER_PKT * pStream->sbc.enc.u16PacketLength;
}

/*************************************************************************************************/
/*!
 *  \brief      Decode and write audio data to DAC.
 *
 *  \param      pData         Pointer to the packet to be decoded.
 *  \param      dataLen       Number of bytes.
 */
/*************************************************************************************************/
static bool_t palCodecSbcDecode(cs47x_Stream_t *pStream, const uint8_t *pSbcBuf, uint16_t len, uint32_t pktCtr)
{
  const uint32_t sbcPktLen = 32;
  const uint32_t blockSize = AUDIO_BLOCK_LEN / AUDIO_FRM_PER_PKT;

  if (len != (AUDIO_FRM_PER_PKT * sbcPktLen))
  {
    AUD_TRACE_WARN2("Invalid packet size, pktCtr[15:0]=%u, len=%u", pktCtr, len);
    return FALSE;
  }

  PalLedOn(2);

  for (size_t i = 0; i < AUDIO_FRM_PER_PKT; i++)
  {
    const OI_BYTE *pFrameData = pSbcBuf + (sbcPktLen * i);
    OI_UINT32 frameBytes = sbcPktLen;
    OI_INT16 *pPcmData = &pStream->pcm[AUDIO_GET_IDX(pktCtr)][(blockSize / sizeof(int16_t)) * i];
    OI_UINT32 pcmBytes = blockSize;

    OI_STATUS status;

    if ((status = OI_CODEC_SBC_DecodeFrame(&pStream->sbc.dec,
                                           &pFrameData, &frameBytes,
                                           pPcmData, &pcmBytes)) != OI_STATUS_SUCCESS)
    {
      AUD_TRACE_WARN2("SBC decode failed, pktCtr[15:0]=%u, status=%u", pktCtr, status);
    }
  }

  PalLedOff(2);

  return TRUE;
}

#endif

/**************************************************************************************************
  Functions: Data Path
**************************************************************************************************/

#if (AUDIO_CAPE)

/*************************************************************************************************/
/*!
 *  \brief          Audio input frame complete.
 *
 *  \param  pCtx    Stream context.
 */
/*************************************************************************************************/
static void palCodecAifInFrameComplete(void *pCtx)
{
  PalLedOn(3);

  cs47x_Stream_t *pStream = (cs47x_Stream_t *)pCtx;

  pStream->prodCtr++;
  pStream->numProd++;

  /* Double buffered, supply +2 buffer. */
  PalI2sReadData((uint8_t *)pStream->pcm[AUDIO_GET_IDX(pStream->prodCtr + 2)], AUDIO_BLOCK_LEN);

  if (pStream->rdyCback)
  {
    /* Consume buffer */
    pStream->rdyCback(pStream->id);
  }

  PalLedOff(3);
}

/*************************************************************************************************/
/*!
 *  \brief          Audio output frame complete.
 *
 *  \param  pCtx    Stream context.
 */
/*************************************************************************************************/
static void palCodecAifOutFrameComplete(void *pCtx)
{
  PalLedOn(3);

  cs47x_Stream_t *pStream = (cs47x_Stream_t *)pCtx;

  if (pStream->numProd < (AUDIO_NUM_BLOCK / 2))
  {
    /* Audio stream re-synchronizing. */
    return;
  }

  int32_t numBufAvail = pStream->prodCtr - pStream->consCtr;
  if (numBufAvail > 0)
  {
    PalI2sWriteData((uint8_t *)pStream->pcm[AUDIO_GET_IDX(pStream->consCtr)], AUDIO_BLOCK_LEN);
    pStream->consCtr++;
  }
  else
  {
    AUD_TRACE_WARN2("Output audio stream underrun, id=%u, pktCtr[15:0]=%u", pStream->id, pStream->consCtr);
  }

  PalLedOff(3);
}

/*************************************************************************************************/
/*!
 *  \brief      Start input path.
 */
/*************************************************************************************************/
static void palCodecAifStart(cs47x_Stream_t *pStream)
{
  /*** Audio interface ***/

  PalI2sConfig_t cfg =
  {
    .mode       = PAL_I2S_MODE_SLAVE,
    .rate       = 32000,
    .bitDepth   = 16,
    .pCtx       = pStream,
  };

  if (pStream->chMask & PAL_CODEC_CH_LEFT_BIT)  cfg.chan |= PAL_I2S_CH_LEFT_BIT;
  if (pStream->chMask & PAL_CODEC_CH_RIGHT_BIT) cfg.chan |= PAL_I2S_CH_RIGHT_BIT;

  switch (pStream->dir)
  {
    case PAL_CODEC_DIR_INPUT:
      cfg.frmCback = palCodecAifInFrameComplete;
      PalI2sConfig(&cfg);

      /* First produced audio block is synchronized with the expected packet counter. */
      pStream->prodCtr -= 1;
      PalI2sReadData((uint8_t *)pStream->pcm[AUDIO_GET_IDX(pStream->prodCtr + 1)], AUDIO_BLOCK_LEN);
      PalI2sReadData((uint8_t *)pStream->pcm[AUDIO_GET_IDX(pStream->prodCtr + 2)], AUDIO_BLOCK_LEN);
      break;

    case PAL_CODEC_DIR_OUTPUT:
      cfg.frmCback = palCodecAifOutFrameComplete;
      PalI2sConfig(&cfg);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Stop audio path.
 */
/*************************************************************************************************/
static void palCodecAifStop(cs47x_Stream_t *pStream)
{
  PalI2sDeConfig();
}

/*************************************************************************************************/
/*!
 *  \brief      Find stream context.
 *
 *  \param      id          Stream ID.
 *
 *  \return     Stream context.
 */
/*************************************************************************************************/
static cs47x_Stream_t *audioFindContext(uint16_t id)
{
  for (size_t i = 0; i < AUDIO_NUM_STREAM; i++)
  {
    if ((palCodecStreamTbl[i].enabled) && (palCodecStreamTbl[i].id == id))
    {
      return &palCodecStreamTbl[i];
    }
  }

  return NULL;
}

/*************************************************************************************************/
/*!
 * \brief   Initialize data path resources.
 *
 * \return  None.
 */
/*************************************************************************************************/
void PalCodecDataInit(void)
{
  PalI2sInit();
}

/*************************************************************************************************/
/*!
 *  \brief      Start audio stream.
 *
 *  \param      id      Stream ID.
 *  \param      pParam  Stream parameters.
 *
 *  \return     TRUE if successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t PalCodecDataStartStream(uint16_t id, PalCodecSreamParam_t *pParam)
{
  if (audioFindContext(id))
  {
    AUD_TRACE_WARN1("Stream already in use, id=%d", id);
    return FALSE;
  }

  for (size_t i = 0; i < AUDIO_NUM_STREAM; i++)
  {
    cs47x_Stream_t *pStream = &palCodecStreamTbl[i];

    if (!pStream->enabled)
    {
      /*** Stream context ***/

      pStream->enabled = TRUE;
      pStream->id = id;
      pStream->chMask = pParam->chMask;
      pStream->intervalUsec = pParam->intervalUsec;
      pStream->dir = pParam->dir;
      pStream->rdyCback = pParam->rdyCback;
      pStream->prodCtr = pStream->consCtr = pParam->pktCtr + 1;

      /*** Codec ***/

      #if (CODEC_BLUEDROID)
        palCodecSbcOpen(pStream);
      #endif
      palCodecAifStart(pStream);

      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Stop audio stream.
 */
/*************************************************************************************************/
void PalCodecDataStopStream(uint16_t id)
{
  cs47x_Stream_t *pStream;

  if ((pStream = audioFindContext(id)) != NULL)
  {
    palCodecAifStop(pStream);
    #if (CODEC_BLUEDROID)
      palCodecSbcClose(pStream);
    #endif

    pStream->enabled = FALSE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Get stream input.
 *
 *  \return     Length.
 */
/*************************************************************************************************/
uint16_t PalCodecDataStreamIn(uint16_t id, uint8_t *pBuf, uint16_t len, uint32_t *pPktCtr)
{
  cs47x_Stream_t *pStream;

  if ((pStream = audioFindContext(id)) == NULL)
  {
    AUD_TRACE_WARN1("Unknown stream id=%u", id);
    return 0;
  }

  uint16_t encLen = 0;

  if (pPktCtr)
  {
    *pPktCtr = pStream->consCtr;
  }

  #if (CODEC_BLUEDROID)
    if (pBuf)
    {
      encLen = palCodecSbcEncode(pStream, pBuf, len);
    }
  #endif

  pStream->consCtr++;

  return encLen;
}

/*************************************************************************************************/
/*!
 *  \brief      Output audio data.
 */
/*************************************************************************************************/
void PalCodecDataStreamOut(uint16_t id, const uint8_t *pBuf, uint16_t len, uint32_t pktCtr)
{
  cs47x_Stream_t *pStream;

  if ((pStream = audioFindContext(id)) == NULL)
  {
    AUD_TRACE_WARN1("Unknown stream id=%u", id);
    return;
  }

  /*** Decode ***/

  #if (CODEC_BLUEDROID)
    if (pBuf && !palCodecSbcDecode(pStream, pBuf, len, pktCtr))
    {
      AUD_TRACE_WARN2("Failed to decode packet, id=%u, pktCtr[15:0]=%u", pStream->id, pktCtr);

      /* Failed decoding is same as missed packet. */
      pBuf = NULL;
    }
  #else
    pBuf = NULL;
  #endif

  /*** PLC ***/

  if (pBuf == NULL)
  {
    AUD_TRACE_WARN2("Missed audio packet; conceal with mute packet, id=%u, pktCtr[15:0]=%u", pStream->id, pktCtr);
    memset(pStream->pcm[AUDIO_GET_IDX(pStream->prodCtr + 1)], 0, AUDIO_BLOCK_LEN);
  }

  if (pktCtr == (pStream->prodCtr + 1))    /* Consecutive packet. */
  {
    /*** Stream integrity ***/

    int32_t numBufAvail = pktCtr - pStream->consCtr;

    if (numBufAvail < 0)
    {
      AUD_TRACE_WARN2("Output audio stream underrun, id=%u, pktCtr[15:0]=%u", pStream->id, pStream->consCtr);

      /* Recovery handled in ISR. */
    }
    else if (numBufAvail > AUDIO_NUM_BLOCK)
    {
      AUD_TRACE_WARN2("Output audio stream overrun, id=%u, pktCtr[15:0]=%u", pStream->id, pStream->consCtr);

      /* Skip oldest audio blocks. */
      pStream->consCtr = pktCtr - AUDIO_NUM_BLOCK;
    }

    pStream->prodCtr = pktCtr;
    pStream->numProd++;

    /*** Stream buffering ***/

    if (pStream->numProd == (AUDIO_NUM_BLOCK / 2))
    {
      AUD_TRACE_WARN2("Output stream synchronized, id=%u pktCtr[15:0]=%u", id, pStream->prodCtr);

      PalI2sWriteData((uint8_t *)pStream->pcm[AUDIO_GET_IDX(pStream->consCtr)], AUDIO_BLOCK_LEN);
      pStream->consCtr++;
      PalI2sWriteData((uint8_t *)pStream->pcm[AUDIO_GET_IDX(pStream->consCtr)], AUDIO_BLOCK_LEN);
      pStream->consCtr++;
    }
  }
  else
  {
    AUD_TRACE_WARN2("Output stream synchronization initialized/lost, id=%u pktCtr[15:0]=%u", id, pktCtr);

    /* Resynchronize stream. */
    pStream->prodCtr = pStream->consCtr = pktCtr;
    pStream->numProd = 0;
  }
}

#endif /* AUDIO_CAPE */
