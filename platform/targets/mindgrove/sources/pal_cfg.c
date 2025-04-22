/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  System configuration definition.
 *
 *  Copyright (c) 2013-2019 Arm Ltd. All Rights Reserved.
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

#include "pal_cfg.h"
#include <stdint.h>
#include <string.h>

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

typedef struct
{
  uint8_t  maxAdvSets;
  uint8_t  maxAdvReports;
  uint16_t maxExtAdvDataLen;
  uint8_t  defExtAdvDataFragLen;
  uint16_t auxDelayUsec;
  uint16_t auxPtrOffsetUsec;
  uint8_t  maxScanReqRcvdEvt;
  uint16_t maxExtScanDataLen;
  uint8_t  maxConn;
  uint8_t  numTxBufs;
  uint8_t  numRxBufs;
  uint16_t maxAclLen;
  int8_t   defTxPwrLvl;
  uint8_t  ceJitterUsec;
  uint8_t  numIsoTxBuf;
  uint8_t  numIsoRxBuf;
  uint16_t maxIsoSduLen;
  uint16_t maxIsoPduLen;
  uint8_t  maxCig;
  uint8_t  maxCis;
  uint16_t cisSubEvtSpaceDelay;
  uint8_t  maxBig;
  uint8_t  maxBis;
  uint16_t dtmRxSyncMs;
} PalCfgLl_t;

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define PAL_UINT32_TO_BSTREAM(p, n)   {*(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
                                       *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24);}

/**************************************************************************************************
  Stub Implementations
**************************************************************************************************/

void palCfgGetBlePhyFeatures(uint8_t *pPhy2mSup, uint8_t *pPhyCodedSup,
                             uint8_t *pStableModIdxTxSup, uint8_t *pStableModIdxRxSup)
{
  *pPhy2mSup          = 0;
  *pPhyCodedSup       = 0;
  *pStableModIdxTxSup = 1;
  *pStableModIdxRxSup = 1;
}

void palCfgLoadLlParams(uint8_t *pConfig)
{
  PalCfgLl_t *pCfg = (PalCfgLl_t *)pConfig;

  pCfg->maxAdvSets          = 1;
  pCfg->maxAdvReports       = 8;
  pCfg->maxExtAdvDataLen    = 256;
  pCfg->defExtAdvDataFragLen = 0;
  pCfg->auxDelayUsec        = 0;
  pCfg->auxPtrOffsetUsec    = 0;
  pCfg->maxScanReqRcvdEvt   = 4;
  pCfg->maxExtScanDataLen   = 256;
  pCfg->maxConn             = 1;
  pCfg->numTxBufs           = 16;
  pCfg->numRxBufs           = 8;
  pCfg->maxAclLen           = 256;
  pCfg->defTxPwrLvl         = 0;
  pCfg->ceJitterUsec        = 0;
  pCfg->numIsoTxBuf         = 0;
  pCfg->numIsoRxBuf         = 0;
  pCfg->maxIsoSduLen        = 0;
  pCfg->maxIsoPduLen        = 0;
  pCfg->maxCig              = 0;
  pCfg->maxCis              = 0;
  pCfg->cisSubEvtSpaceDelay = 0;
  pCfg->maxBig              = 0;
  pCfg->maxBis              = 0;
  pCfg->dtmRxSyncMs         = 0;
}

void palCfgLoadBdAddress(uint8_t *pDevAddr)
{
  memset(pDevAddr, 0xAA, 6);
  pDevAddr[5] |= 0xC0;
}

void palCfgLoadExtMac154Address(uint8_t *pDevAddr)
{
  memset(pDevAddr, 0xBB, 8);
}

void PalCfgSetDeviceUuid(uint8_t *pBuf)
{
  (void)pBuf;
}

void palCfgLoadDeviceUuid(uint8_t *pDevUuid)
{
  uint8_t *pPtr = pDevUuid;
  PAL_UINT32_TO_BSTREAM(pPtr, 0x12345678);
  PAL_UINT32_TO_BSTREAM(pPtr, 0x9ABCDEF0);
  PAL_UINT32_TO_BSTREAM(pPtr, 0x11223344);
  PAL_UINT32_TO_BSTREAM(pPtr, 0x55667788);
  pDevUuid[7] = ((pDevUuid[7] & 0x0F) | 0x40);
  pDevUuid[8] = ((pDevUuid[8] & 0x3F) | 0x80);
}

void PalCfgLoadData(uint8_t cfgId, void *pBuf, uint32_t len)
{
  (void)len;
  switch (cfgId)
  {
    case PAL_CFG_ID_BD_ADDR:
      palCfgLoadBdAddress(pBuf);
      break;

    case PAL_CFG_ID_BLE_PHY:
      palCfgGetBlePhyFeatures(pBuf, (uint8_t *)pBuf + 1, (uint8_t *)pBuf + 2, (uint8_t *)pBuf + 3);
      break;

    case PAL_CFG_ID_LL_PARAM:
      palCfgLoadLlParams(pBuf);
      break;

    case PAL_CFG_ID_MAC_ADDR:
      palCfgLoadExtMac154Address(pBuf);
      break;

    case PAL_CFG_ID_UUID:
      palCfgLoadDeviceUuid(pBuf);
      break;

    default:
      break;
  }
}
