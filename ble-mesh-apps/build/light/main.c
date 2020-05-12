/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Main file for Light application.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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

#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_bufio.h"
#include "wsf_msg.h"
#include "wsf_nvm.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_heap.h"
#include "wsf_cs.h"
#include "wsf_timer.h"
#include "wsf_os.h"

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
#include "ll_init_api.h"
#endif

#include "pal_bb.h"
#include "pal_cfg.h"

#include "mesh_api.h"
#include "mesh_friend_api.h"

#include "light_api.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(BOARD_PCA10056)
#include "nrf.h"
#include "boards.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief UART TX buffer size */
#define PLATFORM_UART_TERMINAL_BUFFER_SIZE      2048U

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief  Pool runtime configuration */
static wsfBufPoolDesc_t mainPoolDesc[] =
{
  { 16,              16 },
  { 72,              16 },
  { 192,             16 },
  { 256,             16 },
  { 512,             16 }
};

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
  static LlRtCfg_t mainLlRtCfg;
#endif

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*! \brief  Stack configuration for application */
extern void StackInitCfgLight(void);

/*! \brief  Stack initialization for application */
extern void StackInitLight(void);

/*************************************************************************************************/
/*!
 *  \brief  Initialize WSF.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mainWsfInit(void)
{
#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
  /* +12 for message headroom, + 2 event header, +255 maximum parameter length. */
  const uint16_t maxRptBufSize = 12 + 2 + 255;

  /* +12 for message headroom, +4 for header. */
  const uint16_t aclBufSize = 12 + mainLlRtCfg.maxAclLen + 4 + BB_DATA_PDU_TAILROOM;

  /* Adjust buffer allocation based on platform configuration. */
  mainPoolDesc[2].len = maxRptBufSize;
  mainPoolDesc[2].num = mainLlRtCfg.maxAdvReports;
  mainPoolDesc[3].len = aclBufSize;
  mainPoolDesc[3].num = mainLlRtCfg.numTxBufs + mainLlRtCfg.numRxBufs;
#endif

  const uint8_t numPools = sizeof(mainPoolDesc) / sizeof(mainPoolDesc[0]);

  uint16_t memUsed;
  memUsed = WsfBufInit(numPools, mainPoolDesc);
  WsfHeapAlloc(memUsed);
  WsfOsInit();
  WsfTimerInit();
#if (WSF_TOKEN_ENABLED == TRUE) || (WSF_TRACE_ENABLED == TRUE)
  WsfTraceRegisterHandler(WsfBufIoWrite);
  WsfTraceEnable(TRUE);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Entry point for demo software.
 *
 *  \param  None.
 *
 *  \return None.
 */
/*************************************************************************************************/
int main(void)
{
  uint32_t memUsed;

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
  /* Configurations must be persistent. */
  static BbRtCfg_t mainBbRtCfg;

  PalBbLoadCfg((PalBbCfg_t *)&mainBbRtCfg);
  LlGetDefaultRunTimeCfg(&mainLlRtCfg);
#if (BT_VER >= LL_VER_BT_CORE_SPEC_5_0)
  /* Set 5.0 requirements. */
  mainLlRtCfg.btVer = LL_VER_BT_CORE_SPEC_5_0;
#endif
  PalCfgLoadData(PAL_CFG_ID_LL_PARAM, &mainLlRtCfg.maxAdvSets, sizeof(LlRtCfg_t) - 9);
#if (BT_VER >= LL_VER_BT_CORE_SPEC_5_0)
  PalCfgLoadData(PAL_CFG_ID_BLE_PHY, &mainLlRtCfg.phy2mSup, 4);
#endif
#endif

  memUsed = WsfBufIoUartInit(WsfHeapGetFreeStartAddress(), PLATFORM_UART_TERMINAL_BUFFER_SIZE);
  WsfHeapAlloc(memUsed);
  WsfNvmInit();

  mainWsfInit();

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
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

  memUsed = LlInit(&llCfg);
  WsfHeapAlloc(memUsed);

  LlSetAdvTxPower(0);

  bdAddr_t bdAddr;
  PalCfgLoadData(PAL_CFG_ID_BD_ADDR, bdAddr, sizeof(bdAddr_t));
  LlSetBdAddr((uint8_t *)&bdAddr);
  LlMathSetSeed((uint32_t *)&bdAddr);
#endif

  /* Configure Mesh App Task before intialization. */
  StackInitCfgLight();

  /* Initialize the Mesh App Task (handler id installation). */
  StackInitLight();

  /* Initialize Mesh Stack. */
  memUsed = MeshInit(WsfHeapGetFreeStartAddress(), WsfHeapCountAvailable());
  WsfHeapAlloc(memUsed);

  /* Initialize Mesh Friend. */
  memUsed = MeshFriendMemInit(WsfHeapGetFreeStartAddress(), WsfHeapCountAvailable());
  WsfHeapAlloc(memUsed);

  /* Start application. */
  LightStart();

  WsfOsEnterMainLoop();

  /* Does not return. */
  return 0;
}
