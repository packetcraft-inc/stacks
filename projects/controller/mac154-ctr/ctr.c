/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Controller module for library.
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

#include "mac_154_api.h"
#include "util/bstream.h"
#include "chci_api.h"
#include "chci_tr.h"
#include "pal_bb.h"
#include "pal_cfg.h"
#include "sch_api.h"
#include "util/prand.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_heap.h"
#include "wsf_timer.h"

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief  Persistent BB runtime configuration. */
static BbRtCfg_t mainBbRtCfg;

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Load runtime configuration.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mainLoadConfiguration(void)
{
  PalBbLoadCfg((PalBbCfg_t *)&mainBbRtCfg);
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize WSF.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mainWsfInit(void)
{
  wsfBufPoolDesc_t poolDesc[] =
  {
    { 16,  8 },
    { 32,  4 },
    { 128, 4 },
    { 256, 9 },       /* TODO packet buffers. */
    { 500, 4 }        /* TODO Need some larger buffers */
  };

  const uint8_t numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);

  /* Initial buffer configuration. */
  uint16_t memUsed;

  memUsed = WsfBufInit(numPools, poolDesc);
  WsfHeapAlloc(memUsed);

  WsfTimerInit();
}

/*************************************************************************************************/
/*!
 *  \brief  Controller initialization.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CtrInit(void)
{
  uint8_t cfgExtAddr[MAC_154_EXTENDED_ADDR_LEN];
  uint64_t extAddr;
  wsfHandlerId_t handlerId;

  mainLoadConfiguration();
  mainWsfInit();

  handlerId = WsfOsSetNextHandler(ChciTrHandler);
  ChciTrHandlerInit(handlerId);

  /* TODO: order may need reconsidering */
  /* Note this function may load radio configuration as well */
  PalCfgLoadData(PAL_CFG_ID_MAC_ADDR, cfgExtAddr, MAC_154_EXTENDED_ADDR_LEN);

  BbInitRunTimeCfg(&mainBbRtCfg);
  BbInit();

  handlerId = WsfOsSetNextHandler(SchHandler);
  SchHandlerInit(handlerId);

  Mac154Init(TRUE);
  PrandInit();
  BYTES_TO_UINT64(extAddr, cfgExtAddr);
  Mac154SetExtAddr(extAddr);
}

/*************************************************************************************************/
/*!
 *  \brief  Controller main processing.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CtrMain(bool_t sleep)
{
  WsfTimerSleepUpdate();

  wsfOsDispatcher();

  if (sleep)
  {
    bool_t serialPending = ChciTrService();

    if (!serialPending)
    {
      WsfTimerSleep();
    }
  }
  else
  {
    (void)ChciTrService();
  }
}
