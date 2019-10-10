/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 MAC: Main.
 *
 *  Copyright (c) 2013-2018 Arm Ltd.
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

#include "bb_154_int.h"
#include "mac_154_int.h"
#include "mac_154_defs.h"
#include "sch_api.h"
#include <string.h>

enum
{
  MAC_154_EVT_SCHED_DATA_RX        = (1 << 0),     /*!< Schedule data receive. */
};

enum
{
  MAC_154_EVT_TIMEOUT_TPT,
  MAC_154_EVT_TIMEOUT_RX_ENABLE,
  MAC_154_EVT_TIMEOUT_PARAM
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      MAC control block. */
/* TODO: May need to consider multiple MAC instances */
static struct
{
  uint8_t            state;
  wsfHandlerId_t     handlerId;
  Mac154RawFrameFn_t rawFrameCback;
  Mac154DataFn_t     dataCback;
  Mac154EvtFn_t      evtCback;
  Mac154Pib_t        pib;
  Mac154PhyPib_t     phyPib;
  wsfTimer_t         rxEnableTimer;
  bool_t             rxEnabled;
} mac154Cb;

static const Mac154Pib_t mac154PibDef =
{
  .coordShortAddr = MAC_154_PIB_COORD_SHORT_ADDRESS_DEF,
  .vsCRCOverride = 0,
  .vsFctlOverride = 0,
  .vsRawRx = 0,
  .panId = MAC_154_PIB_PAN_ID_DEF,
  .shortAddr = MAC_154_PIB_SHORT_ADDRESS_DEF,
  .transactionPersistenceTime = MAC_154_PIB_TRANSACTION_PERSISTENCE_TIME_DEF,
  .maxFrameTotalWaitTime = MAC_154_PIB_MAX_FRAME_TOTAL_WAIT_TIME_DEF,
  .deviceType = MAC_154_PIB_DEVICE_TYPE_DEF,
  .disableCCA = MAC_154_PIB_DISABLE_CCA_DEF,
  .ackWaitDuration = MAC_154_PIB_ACK_WAIT_DURATION_DEF,
  .associationPermit = MAC_154_PIB_ASSOCIATION_PERMIT_DEF,
  .associatedPanCoord = MAC_154_PIB_ASSOCIATED_PAN_COORD_DEF,
  .autoRequest = MAC_154_PIB_AUTO_REQUEST_DEF,
  .minBE = MAC_154_PIB_MIN_BE_DEF,
  .maxBE = MAC_154_PIB_MAX_BE_DEF,
  .maxCSMABackoffs = MAC_154_PIB_MAX_CSMA_BACKOFFS_DEF,
  .maxFrameRetries = MAC_154_PIB_MAX_FRAME_RETRIES_DEF,
  .promiscuousMode = MAC_154_PIB_PROMISCUOUS_MODE_DEF,
  .responseWaitTime = MAC_154_PIB_RESPONSE_WAIT_TIME_DEF,
  .rxOnWhenIdle = MAC_154_PIB_RX_ON_WHEN_IDLE_DEF,
  .securityEnabled = MAC_154_PIB_SECURITY_ENABLED_DEF
};

static const Mac154PhyPib_t mac154PhyPibDef =
{
  .chan = PHY_154_INVALID_CHANNEL,
  .txPower = 0
};

typedef struct
{
  uint8_t offset;
  uint8_t length;
} Mac154PibLUT_t;

/* TODO: Add in min, default and max. where relevant */
static const Mac154PibLUT_t pibLUT[MAC_154_PIB_ENUM_RANGE] = {
  {(uint8_t *)&mac154Cb.pib.ackWaitDuration - (uint8_t *)&mac154Cb.pib, 1},             /* 0x40 - macAckWaitDuration */
  {(uint8_t *)&mac154Cb.pib.associationPermit - (uint8_t *)&mac154Cb.pib, 1},           /* 0x41 - macAssociationPermit */
  {(uint8_t *)&mac154Cb.pib.autoRequest - (uint8_t *)&mac154Cb.pib, 1},                 /* 0x42 - macAutoRequest */
  {0, 0},                                                                               /* 0x43 - macBattLifeExt N/A */
  {0, 0},                                                                               /* 0x44 - macBattLifeExtPeriods N/A */
  {(uint8_t *)&mac154Cb.pib.beaconPayload - (uint8_t *)&mac154Cb.pib, 0xFF},            /* 0x45 - macBeaconPayload (explicit length) */
  {(uint8_t *)&mac154Cb.pib.beaconPayloadLength - (uint8_t *)&mac154Cb.pib, 1},         /* 0x46 - macBeaconPayloadLength*/
  {0, 0},                                                                               /* 0x47 - macBeaconOrder N/A */
  {0, 0},                                                                               /* 0x48 - macBeaconTxTime N/A */
  {(uint8_t *)&mac154Cb.pib.bsn - (uint8_t *)&mac154Cb.pib, 1},                         /* 0x49 - macBSN */
  {(uint8_t *)&mac154Cb.pib.coordExtAddr - (uint8_t *)&mac154Cb.pib, 8},                /* 0x4a - macCoordExtendedAddress */
  {(uint8_t *)&mac154Cb.pib.coordShortAddr - (uint8_t *)&mac154Cb.pib, 2},              /* 0x4b - macCoordShortAddress */
  {(uint8_t *)&mac154Cb.pib.dsn - (uint8_t *)&mac154Cb.pib, 1},                         /* 0x4c - macDSN */
  {0, 0},                                                                               /* 0x4d - macGTSPermit N/A */
  {(uint8_t *)&mac154Cb.pib.maxCSMABackoffs - (uint8_t *)&mac154Cb.pib, 1},             /* 0x4e - macMaxCSMABackoffs */
  {(uint8_t *)&mac154Cb.pib.minBE - (uint8_t *)&mac154Cb.pib, 1},                       /* 0x4f - macMinBE */
  {(uint8_t *)&mac154Cb.pib.panId - (uint8_t *)&mac154Cb.pib, 2},                       /* 0x50 - macPANId */
  {(uint8_t *)&mac154Cb.pib.promiscuousMode - (uint8_t *)&mac154Cb.pib, 1},             /* 0x51 - macPromiscuousMode */
  {(uint8_t *)&mac154Cb.pib.rxOnWhenIdle - (uint8_t *)&mac154Cb.pib, 1},                /* 0x52 - macRxOnWhenIdle */
  {(uint8_t *)&mac154Cb.pib.shortAddr - (uint8_t *)&mac154Cb.pib, 2},                   /* 0x53 - macShortAddress */
  {0, 0},                                                                               /* 0x54 - macSuperframeOrder N/A */
  {(uint8_t *)&mac154Cb.pib.transactionPersistenceTime - (uint8_t *)&mac154Cb.pib, 2},  /* 0x55 - macTransactionPersistenceTime */
  {(uint8_t *)&mac154Cb.pib.associatedPanCoord - (uint8_t *)&mac154Cb.pib, 1},          /* 0x56 - macAssociatedPANCoord */
  {(uint8_t *)&mac154Cb.pib.maxBE - (uint8_t *)&mac154Cb.pib, 1},                       /* 0x57 - macMaxBE */
  {(uint8_t *)&mac154Cb.pib.maxFrameTotalWaitTime - (uint8_t *)&mac154Cb.pib, 2},       /* 0x58 - macMaxFrameTotalWaitTime */
  {(uint8_t *)&mac154Cb.pib.maxFrameRetries - (uint8_t *)&mac154Cb.pib, 1},             /* 0x59 - macMaxFrameRetries */
  {(uint8_t *)&mac154Cb.pib.responseWaitTime - (uint8_t *)&mac154Cb.pib, 4},            /* 0x5a - macResponseWaitTime */ /* TODO: Size extended from 1 to 4 to accommodate long timeouts */
  {0, 0},                                                                               /* 0x5b - macSyncSymbolOffset N/A */
  {0, 0},                                                                               /* 0x5c - macTimestampSupported N/A */
  {(uint8_t *)&mac154Cb.pib.securityEnabled - (uint8_t *)&mac154Cb.pib, 1}              /* 0x5d - macSecurityEnabled */
};

static const Mac154PibLUT_t pibVsLUT[MAC_154_PIB_VS_ENUM_RANGE] = {
  {(uint8_t *)&mac154Cb.pib.extAddr - (uint8_t *)&mac154Cb.pib, 8},                     /* 0x80 - macVsExtAddr */
  {(uint8_t *)&mac154Cb.pib.deviceType - (uint8_t *)&mac154Cb.pib, 1},                  /* 0x81 - macVsDeviceType */
  {(uint8_t *)&mac154Cb.pib.disableCCA - (uint8_t *)&mac154Cb.pib, 1},                  /* 0x82 - macVsDisableCCA */
  {(uint8_t *)&mac154Cb.pib.vsCRCOverride - (uint8_t *)&mac154Cb.pib, 2},               /* 0x83 - macVsCRCOverride */
  {(uint8_t *)&mac154Cb.pib.vsFctlOverride - (uint8_t *)&mac154Cb.pib, 2},              /* 0x84 - macVsFctlOverride */
  {(uint8_t *)&mac154Cb.pib.vsRawRx - (uint8_t *)&mac154Cb.pib, 1}                      /* 0x85 - macVsRawRx */
};

static const Mac154PibLUT_t phyPibLUT[MAC_154_PHY_PIB_ENUM_RANGE] = {
  {(uint8_t *)&mac154Cb.phyPib.chan - (uint8_t *)&mac154Cb.phyPib, 1},                  /* 0x90 - phyCurrentChannel */
  {(uint8_t *)&mac154Cb.phyPib.txPower - (uint8_t *)&mac154Cb.phyPib, 1},               /* 0x91 - phyTransmitPower */
};

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC message dispatch handler.
 *
 *  \param      event       WSF event.
 *  \param      pMsg        WSF message.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mac154Handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  if (event & MAC_154_EVT_SCHED_DATA_RX)
  {
    /* Note: Can't really signal any error if it occurs */
    (void)Mac154DataRxStart();
  }
  /* Note: Timer events do not get signalled through the event parameter */
  if (pMsg != NULL)
  {
    if (pMsg->event == MAC_154_EVT_TIMEOUT_TPT)
    {
      Bb154HandleTPTTimeout(pMsg);
    }
    else if (pMsg->event == MAC_154_EVT_TIMEOUT_RX_ENABLE)
    {
      uint8_t flags = Mac154AssessRxEnable(MAC_154_RX_ASSESS_RXEN, FALSE);
      mac154Cb.rxEnabled = FALSE;
      Mac154ActionRx(flags);
    }
    else if (pMsg->event == MAC_154_EVT_TIMEOUT_PARAM)
    {
      Mac154ParamTimer_t *pParamTimer = MAC_154_PARAM_TIMER_FROM_MSG(pMsg);
      (*pParamTimer->cback)(pParamTimer->param);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize 802.15.4 MAC subsystem with task handler.
 *
 *  \param      None.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154HandlerInit(void)
{
  mac154Cb.handlerId = WsfOsSetNextHandler(mac154Handler);
}

/*************************************************************************************************/
/*!
 *  \brief      Register raw frame callback
 *
 *  \param      rawFrameCback   Raw frame callback handler
 *
 *  \return     None.
 *
 *  \note       Called at a low level, so passed frame must be dealt with expediently
 */
/*************************************************************************************************/
void Mac154RegisterRawFrameCback(Mac154RawFrameFn_t rawFrameCback)
{
  mac154Cb.rawFrameCback = rawFrameCback;
}

/*************************************************************************************************/
/*!
 *  \brief      Execute raw frame callback
 *
 *  \param      mpduLen       Length of MPDU
 *  \param      pMpdu         Pointer to MPDU
 *  \param      linkQuality   Link quality
 *  \param      timestamp     Timestamp
 *
 *  \return     None.
 *
 *  \note       It is recommended that the data is queued if it cannot be processed very quickly
 */
/*************************************************************************************************/
void Mac154ExecuteRawFrameCback(uint8_t mpduLen, uint8_t *pMpdu, uint8_t linkQuality, uint32_t timestamp)
{
  if (mac154Cb.rawFrameCback)
  {
    mac154Cb.rawFrameCback(mpduLen, pMpdu, linkQuality, timestamp);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Register Event callback.
 *
 *  \param      evtCback    Event callback handler
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154RegisterEvtCback(Mac154EvtFn_t evtCback)
{
  mac154Cb.evtCback = evtCback;
}

/*************************************************************************************************/
/*!
 *  \brief      Execute Event callback
 *
 *  \param      pBuf    Buffer formatted as per MAC API document.
 *
 *  \return     TRUE if no further handling should take place.
 *
 *  \note       It is recommended that the data is queued if it cannot be process very quickly
 */
/*************************************************************************************************/
bool_t Mac154ExecuteEvtCback(uint8_t *pBuf)
{
  if (mac154Cb.evtCback)
  {
    return mac154Cb.evtCback(pBuf);
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Register Data callback
 *
 *  \param      dataCback   Data callback handler
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154RegisterDataCback(Mac154DataFn_t dataCback)
{
  mac154Cb.dataCback = dataCback;
}

/*************************************************************************************************/
/*!
 *  \brief      Execute Data callback
 *
 *  \param      pBuf    Buffer formatted as per MAC API document.
 *
 *  \return     TRUE if no further handling should take place.
 *
 *  \note       It is recommended that the data is queued if it cannot be process very quickly
 */
/*************************************************************************************************/
bool_t Mac154ExecuteDataCback(uint8_t *pBuf)
{
  if (mac154Cb.dataCback)
  {
    return mac154Cb.dataCback(pBuf);
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Schedule data receive
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154ScheduleDataRx(void)
{
  /* Check we are actually receiving first */
  if (Bb154RxInProgress() == NULL)
  {
    /* Store the pending data transmit operation handle */
    WsfSetEvent(mac154Cb.handlerId, MAC_154_EVT_SCHED_DATA_RX);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Start transaction persistence timer
 *
 *  \param      pTimer    Handle of associated timer
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154StartTransactionPersistenceTimer(wsfTimer_t *pTimer)
{
  pTimer->handlerId = mac154Cb.handlerId;
  pTimer->msg.event = MAC_154_EVT_TIMEOUT_TPT;
  WsfTimerStartMs(pTimer, PAL_BB_154_TPT_TO_MS(mac154Cb.pib.transactionPersistenceTime));
}

/*************************************************************************************************/
/*!
 *  \brief      Start rx enable timer
 *
 *  \param      symDuration   Duration in symbols
 *
 *  \return     None.
 *
 *  \note       The use of a ms timer does not meet the accuracy requirements in 802.15.4 [108,16]
 */
/*************************************************************************************************/
void Mac154StartRxEnableTimer(uint32_t symDuration)
{
  bool_t nextRxEn = FALSE;
  uint8_t flags;

  /* Stop any running timer */
  WsfTimerStop(&mac154Cb.rxEnableTimer);

  /* Restart if symbol duration > 0 */
  if (symDuration > 0)
  {
    nextRxEn = TRUE;
    mac154Cb.rxEnableTimer.handlerId = mac154Cb.handlerId;
    mac154Cb.rxEnableTimer.msg.event = MAC_154_EVT_TIMEOUT_RX_ENABLE;
    WsfTimerStartMs(&mac154Cb.rxEnableTimer, PAL_BB_154_SYMB_TO_MS(symDuration));
  }

  flags = Mac154AssessRxEnable(MAC_154_RX_ASSESS_RXEN, nextRxEn);
  mac154Cb.rxEnabled = nextRxEn;
  Mac154ActionRx(flags);
}

/*************************************************************************************************/
/*!
 *  \brief      Start timer with parameter
 *
 *  \param      pParamTimer   Timer with parameter
 *  \param      pParam        Callback associated with timer
 *  \param      pParam        Parameter associated with timer
 *  \param      timeout       Timeout value
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154StartParamTimer(Mac154ParamTimer_t *pParamTimer,
                           Mac154ParamTimerFn_t cback,
                           void *param,
                           uint32_t timeout)
{
  pParamTimer->cback = cback;
  pParamTimer->param = param;
  pParamTimer->timer.handlerId = mac154Cb.handlerId;
  pParamTimer->timer.msg.event = MAC_154_EVT_TIMEOUT_PARAM;
  /* Store offset of timer message within Mac154ParamTimer_t for retrieval in the timeout callback */
  pParamTimer->timer.msg.param = (uint16_t)((uint8_t *)&pParamTimer->timer.msg - (uint8_t *)pParamTimer);
  WsfTimerStartMs(&pParamTimer->timer, timeout);
}

/*************************************************************************************************/
/*!
 *  \brief      Assess whether rx should be enabled or disabled
 *
 *  \param      assess      Basis to assess rx enabled/disabled
 *  \param      nextState   What the next state will be
 *
 *  \return     Start/stop flags
 */
/*************************************************************************************************/
uint8_t Mac154AssessRxEnable(Mac154RxAssess_t assess, bool_t nextState)
{
  uint8_t retflags = 0;
  uint8_t currRXWI = mac154Cb.pib.rxOnWhenIdle;
  bool_t currRxEn = mac154Cb.rxEnabled;
  uint8_t currProm = mac154Cb.pib.promiscuousMode;

  switch (assess)
  {
    case MAC_154_RX_ASSESS_RXWI:
      if (!currRxEn && (currProm == 0))
      {
        if ((currRXWI == 0) && nextState)
        {
          retflags |= MAC_154_RX_START;
        }
        else if ((currRXWI != 0) && !nextState)
        {
          retflags |= MAC_154_RX_STOP;
        }
      }
      break;
    case MAC_154_RX_ASSESS_RXEN:
      if ((currRXWI == 0) && (currProm == 0))
      {
        if (!currRxEn && nextState)
        {
          retflags |= MAC_154_RX_START;
        }
        else if (currRxEn && !nextState)
        {
          retflags |= MAC_154_RX_STOP;
        }
      }
      break;
    case MAC_154_RX_ASSESS_PROM:
      if (!currRxEn && (currRXWI == 0))
      {
        if ((currProm == 0) && (nextState != 0))
        {
          retflags |= MAC_154_RX_START;
        }
        else if ((currProm != 0) && (nextState == 0))
        {
          retflags |= MAC_154_RX_STOP;
        }
      }
      break;
    default:
      break;
  }

  return retflags;
}

/*************************************************************************************************/
/*!
 *  \brief      Take appropriate 15.4 receive action
 *
 *  \param      flags       Start or stop receive.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154StartRx(void)
{
  if (Mac154IsRxEnabled())
  {
    /* Start receiving */
    Mac154ScheduleDataRx();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Take appropriate 15.4 receive action
 *
 *  \param      flags       Start or stop receive.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154ActionRx(uint8_t flags)
{
  if (flags & MAC_154_RX_START)
  {
    /* Start receiving */
    Mac154StartRx();
  }
  else if (flags & MAC_154_RX_STOP)
  {
    BbOpDesc_t *pOp;
    /* Cancel background Rx when running. If BOD is in the future, it will be cancelled at loading time. */
    if ((pOp = Bb154RxInProgress()) != NULL)
    {
      BbCancelBod();
      BbTerminateBod();
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC initialize PIB
 *
 *  \param      None.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154InitPIB(void)
{
  mac154Cb.pib = mac154PibDef;
  mac154Cb.phyPib = mac154PhyPibDef;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get PIB.
 *
 *  \param      None.
 *
 *  \return     Pointer to PIB.
 */
/*************************************************************************************************/
Mac154Pib_t *Mac154GetPIB(void)
{
  return &mac154Cb.pib;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC set extended address.
 *
 *  \param      None.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154SetExtAddr(uint64_t extAddr)
{
  mac154Cb.pib.extAddr = extAddr;
}

/*************************************************************************************************/
/*!
 *  \brief      Determine whether rx is enabled
 *
 *  \param      None.
 *
 *  \return     TRUE if rx is enabled, FALSE otherwise
 *
 *  \note       Based on the two values: PIB rx on when idle and rx enable flag set through
 *              MLME-RX-ENABLE.req. Note PIB rx on when idle is considered a "conflicting
 *              responsibility" ([109,10-13])
 */
/*************************************************************************************************/
bool_t Mac154IsRxEnabled(void)
{
  if (mac154Cb.state != MAC_154_STATE_SCAN)
  {
    return mac154Cb.pib.rxOnWhenIdle || mac154Cb.rxEnabled || mac154Cb.pib.promiscuousMode;
  }
  /* Scan handles its own receive scheduling */
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get PHY PIB.
 *
 *  \param      None.
 *
 *  \return     Pointer to PIB.
 */
/*************************************************************************************************/
Mac154PhyPib_t *Mac154GetPhyPIB(void)
{
  return &mac154Cb.phyPib;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get PIB attribute
 *
 *  \param      attrEnum    Attribute enumeration as specified in 802.15.4-2006 Table 86.
 *  \param      pAttrLen    Attribute length in bytes (returned).
 *
 *  \return     Pointer to attribute as byte string
 */
/*************************************************************************************************/
uint8_t *Mac154PibGetAttr(uint8_t attrEnum, uint8_t *pAttrLen)
{
  *pAttrLen = pibLUT[attrEnum - MAC_154_PIB_ENUM_MIN].length;
  return (uint8_t *)&mac154Cb.pib + pibLUT[attrEnum - MAC_154_PIB_ENUM_MIN].offset;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get PIB vendor-specific attribute
 *
 *  \param      attrEnum    Attribute enumeration as specified in 802.15.4-2006 Table 86.
 *  \param      pAttrLen    Attribute length in bytes (returned).
 *
 *  \return     Pointer to attribute as byte string
 */
/*************************************************************************************************/
uint8_t *Mac154PibGetVsAttr(uint8_t attrEnum, uint8_t *pAttrLen)
{
  *pAttrLen = pibVsLUT[attrEnum - MAC_154_PIB_VS_ENUM_MIN].length;
  return (uint8_t *)&mac154Cb.pib + pibVsLUT[attrEnum - MAC_154_PIB_VS_ENUM_MIN].offset;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get PHY PIB attribute
 *
 *  \param      attrEnum    Attribute enumeration as specified in 802.15.4-2006 Table 86.
 *  \param      pAttrLen    Attribute length in bytes (returned).
 *
 *  \return     Pointer to attribute as byte string
 */
/*************************************************************************************************/
uint8_t *Mac154PhyPibGetAttr(uint8_t attrEnum, uint8_t *pAttrLen)
{
  *pAttrLen = phyPibLUT[attrEnum - MAC_154_PHY_PIB_ENUM_MIN].length;
  return (uint8_t *)&mac154Cb.phyPib + pibVsLUT[attrEnum - MAC_154_PHY_PIB_ENUM_MIN].offset;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC set PIB attribute
 *
 *  \param      attrEnum    Attribute enumeration as specified in 802.15.4-2006 Table 86.
 *  \param      attrLen     Attribute length in bytes.
 *  \param      pAttr       Pointer to attribute as byte string.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154PibSetAttr(uint8_t attrEnum, uint8_t attrLen, uint8_t *pAttr)
{
  if (attrEnum <= MAC_154_PIB_ENUM_MAX)
  {
    uint8_t index = attrEnum - MAC_154_PIB_ENUM_MIN;

    if (attrLen == pibLUT[index].length)
    {
      /* TODO - bounds checking. */
      memcpy((uint8_t *)&mac154Cb.pib + pibLUT[index].offset, pAttr, attrLen);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC set PIB vendor specific attribute
 *
 *  \param      attrEnum    Attribute enumeration as specified in 802.15.4-2006 Table 86.
 *  \param      attrLen     Attribute length in bytes.
 *  \param      pAttr       Pointer to attribute as byte string.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154PibSetVsAttr(uint8_t attrEnum, uint8_t attrLen, uint8_t *pAttr)
{
  if (attrEnum <= MAC_154_PIB_VS_ENUM_MAX)
  {
    uint8_t index = attrEnum - MAC_154_PIB_VS_ENUM_MIN;

    if (attrLen == pibVsLUT[index].length)
    {
      memcpy((uint8_t *)&mac154Cb.pib + pibVsLUT[index].offset, pAttr, attrLen);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC set PHY PIB attribute
 *
 *  \param      attrEnum    Attribute enumeration as specified in 802.15.4-2006 Table 86.
 *  \param      attrLen     Attribute length in bytes.
 *  \param      pAttr       Pointer to attribute as byte string.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154PhyPibSetAttr(uint8_t attrEnum, uint8_t attrLen, uint8_t *pAttr)
{
  if (attrEnum <= MAC_154_PHY_PIB_ENUM_MAX)
  {
    uint8_t index = attrEnum - MAC_154_PHY_PIB_ENUM_MIN;

    if (attrLen == phyPibLUT[index].length)
    {
      memcpy((uint8_t *)&mac154Cb.phyPib + phyPibLUT[index].offset, pAttr, attrLen);
    }
  }
}

/* TODO: Properly partition the MAC into submodules
 * with proper access to MAC module structures, including
 * control block
 */

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get DSN and increment
 *
 *  \param      None.
 *
 *  \return     DSN
 */
/*************************************************************************************************/
uint8_t Mac154GetDSNIncr(void)
{
  return mac154Cb.pib.dsn++;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get BSN and increment
 *
 *  \param      None.
 *
 *  \return     DSN
 */
/*************************************************************************************************/
uint8_t Mac154GetBSNIncr(void)
{
  return mac154Cb.pib.bsn++;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC get state
 *
 *  \param      None.
 *
 *  \return     MAC state.
 */
/*************************************************************************************************/
uint8_t Mac154GetState(void)
{
  return mac154Cb.state;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 MAC set state
 *
 *  \param      state   State to set the MAC to.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154SetState(uint8_t state)
{
  mac154Cb.state = state;
}

