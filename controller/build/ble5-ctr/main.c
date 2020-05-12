/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Main module.
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

#include "ll_init_api.h"
#include "chci_tr.h"
#include "lhci_api.h"
#include "hci_defs.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_heap.h"
#include "wsf_timer.h"
#include "wsf_trace.h"
#include "bb_ble_sniffer_api.h"
#include "pal_bb.h"
#include "pal_cfg.h"

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief  Persistent BB runtime configuration. */
static BbRtCfg_t mainBbRtCfg;

/*! \brief  Persistent LL runtime configuration. */
static LlRtCfg_t mainLlRtCfg;

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Load runtime configuration.
 */
/*************************************************************************************************/
static void mainLoadConfiguration(void)
{
  PalBbLoadCfg((PalBbCfg_t *)&mainBbRtCfg);

  LlGetDefaultRunTimeCfg(&mainLlRtCfg);
  PalCfgLoadData(PAL_CFG_ID_LL_PARAM, &mainLlRtCfg.maxAdvSets, sizeof(LlRtCfg_t) - 9);
  PalCfgLoadData(PAL_CFG_ID_BLE_PHY, &mainLlRtCfg.phy2mSup, 4);

  /* Set 5.2 requirements. */
  mainLlRtCfg.btVer = LL_VER_BT_CORE_SPEC_5_2;
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize WSF.
 */
/*************************************************************************************************/
static void mainWsfInit(void)
{
  /* +12 for message headroom, + 2 event header, +255 maximum parameter length. */
  const uint16_t maxRptBufSize = 12 + 2 + 255;

  /* +12 for message headroom, +ISO Data Load, +4 for header. */
  const uint16_t dataBufSize = 12 + HCI_ISO_DL_MAX_LEN + mainLlRtCfg.maxAclLen + 4 + BB_DATA_PDU_TAILROOM;

  /* Use single pool for data buffers. */
  WSF_ASSERT(mainLlRtCfg.maxAclLen == mainLlRtCfg.maxIsoSduLen);

  /* Ensure pool buffers are ordered correctly. */
  WSF_ASSERT(maxRptBufSize < dataBufSize);

  wsfBufPoolDesc_t poolDesc[] =
  {
    { 16,            8 },
    { 32,            4 },
    { 128,           mainLlRtCfg.maxAdvReports },
    { maxRptBufSize, mainLlRtCfg.maxAdvReports },       /* Extended reports. */
    { dataBufSize,   mainLlRtCfg.numTxBufs + mainLlRtCfg.numRxBufs + mainLlRtCfg.numIsoTxBuf + mainLlRtCfg.numIsoRxBuf }
  };

  const uint8_t numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);

  /* Initial buffer configuration. */
  uint16_t memUsed;
  memUsed = WsfBufInit(numPools, poolDesc);
  WsfHeapAlloc(memUsed);

  /* Initialize RTOS resources. */
  WsfOsInit();
  WsfTimerInit();
  #if (WSF_TOKEN_ENABLED == TRUE)
    WsfTraceRegisterHandler(LhciVsEncodeTraceMsgEvtPkt);
  #endif
}

#if (WSF_TOKEN_ENABLED == TRUE)
/*************************************************************************************************/
/*!
 *  \brief  Check and service tokens (Trace and sniffer).
 *
 *  \return TRUE if there is token pending.
 */
/*************************************************************************************************/
static bool_t mainCheckServiceTokens(void)
{
  bool_t eventPending = FALSE;

#if (WSF_TOKEN_ENABLED == TRUE) || (BB_SNIFFER_ENABLED == TRUE)
  eventPending = LhciIsEventPending();
#endif

#if WSF_TOKEN_ENABLED == TRUE
  /* Allow only a single token to be processed at a time. */
  if (!eventPending)
  {
    eventPending = WsfTokenService();
  }
#endif

#if (BB_SNIFFER_ENABLED == TRUE)
  /* Service one sniffer packet, if in the buffer. */
  if (!eventPending)
  {
    eventPending = LhciSnifferHandler();
  }
#endif

  return eventPending;
}
#endif

/*************************************************************************************************/
/*!
 *  \brief  Main entry point.
 */
/*************************************************************************************************/
int main(void)
{
  mainLoadConfiguration();
  mainWsfInit();

  LlInitRtCfg_t llCfg =
  {
    .pBbRtCfg     = &mainBbRtCfg,
    .wlSizeCfg    = 4,
    .rlSizeCfg    = 4,
    .plSizeCfg    = 4,
    .pLlRtCfg     = &mainLlRtCfg,
    .pFreeMem     = WsfHeapGetFreeStartAddress(),
    .freeMemAvail = WsfHeapCountAvailable()
  };

  uint32_t memUsed;

  memUsed = LlInitControllerInit(&llCfg);
  WsfHeapAlloc(memUsed);

  bdAddr_t bdAddr;
  PalCfgLoadData(PAL_CFG_ID_BD_ADDR, bdAddr, sizeof(bdAddr_t));
  /* Coverity[uninit_use_in_call] */
  LlSetBdAddr((uint8_t *)&bdAddr);
  LlMathSetSeed((uint32_t *)&bdAddr);

  #if (WSF_TOKEN_ENABLED == TRUE)
    WsfOsRegisterSleepCheckFunc(mainCheckServiceTokens);
  #endif
  WsfOsRegisterSleepCheckFunc(ChciTrService);
  WsfOsEnterMainLoop();

  /* Does not return. */
  return 0;
}
