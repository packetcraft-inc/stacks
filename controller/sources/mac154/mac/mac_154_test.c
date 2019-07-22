/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 MAC test implementation file.
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

#include "wsf_types.h"
#include "wsf_buf.h"
#include "util/bstream.h"
#include "bb_api.h"
#include "bb_154_int.h"
#include "bb_154_api_op.h"
#include "bb_154.h"
#include "chci_154_int.h"
#include "mac_154_int.h"
#include "mac_154_defs.h"
#include "sch_api.h"
#include "util/prand.h"
#include "pal_bb.h"

#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      Test states. */
enum
{
  MAC_154_TEST_STATE_IDLE  = 0,    /*!< Disabled state */
  MAC_154_TEST_STATE_TX    = 1,    /*!< Tx enabled state */
  MAC_154_TEST_STATE_RX    = 2,    /*!< Rx enabled state */
  MAC_154_TEST_STATE_TERM  = 3,    /*!< Terminating state */
  MAC_154_TEST_STATE_RESET = 4     /*!< Reset state. */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      Test control block. */
static struct
{
  uint8_t  state;
  uint16_t numPkt;

  struct
  {
    uint32_t pktCount;
    uint32_t ackCount;
    uint32_t pktErrCount;
    uint32_t ackErrCount;
  } stats;
} mac154TestCb;

/*| \brief Test payload defined in Zigbee 14-0332-01 */
static const uint8_t mac154TestZBPayload[16] =
{
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
  0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

/*************************************************************************************************/
/*!
 *  \brief      Initialize for test operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154TestInit(void)
{
  memset(&mac154TestCb, 0, sizeof(mac154TestCb));
}

/*************************************************************************************************/
/*!
 *  \brief      Set network parameters for test operations.
 *
 *  \param      addr64      Extended address.
 *  \param      addr16      Short address.
 *  \param      panId       PAN ID.
 *
 *  \return     Status code.
 */
/*************************************************************************************************/
uint8_t Mac154TestSetNetParams(uint64_t addr64, uint16_t addr16, uint16_t panId)
{
  Mac154Pib_t *pPib = Mac154GetPIB();

  pPib->extAddr = addr64;
  pPib->shortAddr = addr16;
  pPib->panId  = panId;
  PalBb154FlushPIB();

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief  Tx operation completion callback.
 *
 *  \param  pOp     Tx operation descriptor.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mac154TestTxOpEndCback(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154TestTx_t * const pTx = &p154->op.testTx;

  /* All of the requested packets have been transmitted. */
  if (mac154TestCb.numPkt > 0)
  {
    uint32_t attempts = mac154TestCb.stats.pktCount + mac154TestCb.stats.pktErrCount;
    if (mac154TestCb.numPkt <= attempts)
    {
      mac154TestCb.state = MAC_154_TEST_STATE_TERM;
    }
  }

  if (mac154TestCb.state == MAC_154_TEST_STATE_TX)
  {
    /* Reschedule transmit. */
    SchInsertLateAsPossible(pOp, BbGetSchSetupDelayUs(), pTx->pktInterUsec);
  }
  else
  {
    WsfBufFree(pTx->pTxDesc);
    WsfBufFree(p154);
    WsfBufFree(pOp);

    BbStop(BB_PROT_15P4);

    Chci154TestSendTestEndInd(mac154TestCb.stats.pktCount, mac154TestCb.stats.pktErrCount,
        mac154TestCb.stats.ackCount, mac154TestCb.stats.ackErrCount);

    mac154TestCb.state = MAC_154_TEST_STATE_IDLE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Complete a transmit.
 *
 *  \param      pOp       Operation context.
 *  \param      ack       TRUE if ACK status.
 *  \param      success   TRUE if operation succeeded.
 *
 *  \return     TRUE if next receive should be set up.
 */
/*************************************************************************************************/
static bool_t mac154TestTxCback(BbOpDesc_t *pOp, bool_t ack, bool_t success)
{
  /* Unused parameters */
  (void)pOp;

  /* Update statistics. */
  if (ack)
  {
    mac154TestCb.stats.pktCount++;
    if (success)
    {
      mac154TestCb.stats.ackCount++;
    }
    else
    {
      mac154TestCb.stats.ackErrCount++;
      return FALSE;
    }
  }
  else
  {
    if (success)
    {
      mac154TestCb.stats.pktCount++;
    }
    else
    {
      mac154TestCb.stats.pktErrCount++;
      return FALSE;
    }
  }

  /* All of the requested packets have been transmitted. */
  if (mac154TestCb.numPkt > 0)
  {
    uint32_t attempts = mac154TestCb.stats.pktCount + mac154TestCb.stats.pktErrCount;
    if (mac154TestCb.numPkt <= attempts)
    {
      return FALSE;
    }
  }

  /* Test has ended. */
  if (mac154TestCb.state != MAC_154_TEST_STATE_TX)
  {
    return FALSE;
  }

  /* Continue transmitting next packet. */
  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      Start transmit test.
 *
 *  \param      chan            Channel.
 *  \param      power           Power (dBm).
 *  \param      len             Length of test data.
 *  \param      pydType         Test packet payload type.
 *  \param      numPkt          Auto terminate after number of packets, 0 for infinite.
 *  \param      interPktSpace   Interpacket space in microseconds.
 *  \param      rxAck           Receive ACK after transmit.
 *  \param      addrModes       Destination and source address modes.
 *  \param      dstAddr         Destination address.
 *  \param      dstPanId        Destination PAN ID.
 *
 *  \return     Status code.
 */
/*************************************************************************************************/
uint8_t Mac154TestTx(uint8_t chan, uint8_t power, uint8_t len, uint8_t pydType, uint16_t numPkt,
    uint32_t interPktSpace, bool_t rxAck, uint8_t addrModes, uint64_t dstAddr, uint16_t dstPanId)
{
  Mac154Pib_t *pPib = Mac154GetPIB();

  /* Unused parameters */
  (void)pydType;

  if (mac154TestCb.state != MAC_154_TEST_STATE_IDLE)
  {
    return 0xFF;
  }

  BbOpDesc_t *pOp;
  if ((pOp = WsfBufAlloc(sizeof(*pOp))) == NULL)
  {
    return 0xFF;
  }
  memset(pOp, 0, sizeof(*pOp));

  Bb154Data_t *p154;
  if ((p154 = WsfBufAlloc(sizeof(*p154))) == NULL)
  {
    WsfBufFree(pOp);
    return 0xFF;
  }
  memset(p154, 0, sizeof(*p154));

  p154->opType = BB_154_OP_TEST_TX;

  p154->chan.channel = chan;
  p154->chan.txPower = power;

  p154->opParam.flags = rxAck ? PAL_BB_154_FLAG_TX_AUTO_RX_ACK : 0;

  p154->opParam.psduMaxLength  = PHY_154_aMaxPHYPacketSize;

  p154->op.testTx.pktInterUsec = interPktSpace;
  p154->op.testTx.rxLen        = p154->opParam.psduMaxLength;
  p154->op.testTx.testCback    = mac154TestTxCback;

  if ((p154->op.testTx.pTxDesc = WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) + p154->opParam.psduMaxLength)) == NULL)
  {
    WsfBufFree(p154);
    WsfBufFree(pOp);
    return 0xFF;
  }

  uint8_t tmp;
  uint8_t *pFrame = PAL_BB_154_TX_FRAME_PTR(p154->op.testTx.pTxDesc);
  uint8_t dam = MAC_154_TEST_GET_DAM(addrModes);
  uint8_t sam = MAC_154_TEST_GET_SAM(addrModes);
  bool_t  panIdComp = (pPib->panId == dstPanId) && dam && sam;

  /* Frame control */
  tmp = MAC_154_FRAME_TYPE_DATA;
  tmp |= (rxAck ? MAC_154_FC_ACK_REQUEST_MASK : 0);
  tmp |= (panIdComp ? MAC_154_FC_PAN_ID_COMP_MASK : 0);
  UINT8_TO_BSTREAM(pFrame, tmp);
  tmp = dam << (MAC_154_FC_DST_ADDR_MODE_SHIFT - 8); /* Frame version 0 implied */
  tmp |= sam << (MAC_154_FC_SRC_ADDR_MODE_SHIFT - 8);
  UINT8_TO_BSTREAM(pFrame, tmp);

  /* Sequence number - fixed at 0 */
  UINT8_TO_BSTREAM(pFrame, 0);

  /* Destination address */
  if (dam)
  {
    UINT16_TO_BSTREAM(pFrame, dstPanId);
    if (dam == MAC_154_ADDR_MODE_EXTENDED)
    {
      UINT64_TO_BSTREAM(pFrame, dstAddr);
    }
    else
    {
      UINT16_TO_BSTREAM(pFrame, dstAddr);
    }
  }

  /* Source address */
  if (sam)
  {
    if (!panIdComp)
    {
      UINT16_TO_BSTREAM(pFrame, pPib->panId);
    }
    if (sam == MAC_154_ADDR_MODE_EXTENDED)
    {
      UINT64_TO_BSTREAM(pFrame, pPib->extAddr);
    }
    else
    {
      UINT16_TO_BSTREAM(pFrame, pPib->shortAddr);
    }
  }

  //pPib->rxOnWhenIdle = TRUE;

  /* Data */
  uint8_t hdrLen = pFrame - PAL_BB_154_TX_FRAME_PTR(p154->op.testTx.pTxDesc);
  if (len > p154->opParam.psduMaxLength - hdrLen)
  {
    len = p154->opParam.psduMaxLength - hdrLen;
  }

  if (len == 16)
  {
    /* If length is set to 16, use PHY tx payload defined in Zigbee 14-0332-01 */
    memcpy(pFrame, mac154TestZBPayload, len);
  }
  else
  {
    /* Otherwise just use a random payload */
    PrandGen(pFrame, len);
  }

  /* Add in header length */
  p154->op.testTx.pTxDesc->len = len + hdrLen;

  pOp->protId        = BB_PROT_15P4;
  pOp->prot.p154     = p154;
  pOp->endCback      = mac154TestTxOpEndCback;
  pOp->dueOffsetUsec = 0;

  mac154TestCb.state  = MAC_154_TEST_STATE_TX;
  mac154TestCb.numPkt = numPkt;
  memset(&mac154TestCb.stats, 0, sizeof(mac154TestCb.stats));

  BbStart(BB_PROT_15P4);
  SchInsertNextAvailable(pOp);

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief  Rx operation completion callback.
 *
 *  \param  pOp     Rx operation descriptor.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mac154TestRxOpEndCback(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;

  /* All of the requested packets have been received. */
  if (mac154TestCb.numPkt > 0)
  {
    uint32_t attempts = mac154TestCb.stats.pktCount + mac154TestCb.stats.pktErrCount;
    if (mac154TestCb.numPkt <= attempts)
    {
      mac154TestCb.state = MAC_154_TEST_STATE_TERM;
    }
  }

  if (mac154TestCb.state == MAC_154_TEST_STATE_RX)
  {
    /* Reschedule receive. */
    SchInsertNextAvailable(pOp);
  }
  else
  {
    WsfBufFree(p154);
    WsfBufFree(pOp);

    BbStop(BB_PROT_15P4);

    Chci154TestSendTestEndInd(mac154TestCb.stats.pktCount, mac154TestCb.stats.pktErrCount,
        mac154TestCb.stats.ackCount, mac154TestCb.stats.ackErrCount);

    mac154TestCb.state = MAC_154_TEST_STATE_IDLE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Complete a receive.
 *
 *  \param      pOp       Operation context.
 *  \param      ack       TRUE if ACK status.
 *  \param      success   TRUE if operation succeeded.
 *
 *  \return     TRUE if next receive should be set up.
 */
/*************************************************************************************************/
static bool_t mac154TestRxCback(BbOpDesc_t *pOp, bool_t ack, bool_t success)
{
  /* Unused parameters */
  (void)pOp;

  /* Update statistics. */
  if (ack)
  {
    mac154TestCb.stats.pktCount++;
    if (success)
    {
      mac154TestCb.stats.ackCount++;
    }
    else
    {
      mac154TestCb.stats.ackErrCount++;
      return FALSE;
    }
  }
  else
  {
    if (success)
    {
      mac154TestCb.stats.pktCount++;
    }
    else
    {
      mac154TestCb.stats.pktErrCount++;
      return FALSE;
    }
  }

  /* All of the requested packets have been received. */
  if (mac154TestCb.numPkt > 0)
  {
    uint32_t attempts = mac154TestCb.stats.pktCount + mac154TestCb.stats.pktErrCount;
    if (mac154TestCb.numPkt <= attempts)
    {
      return FALSE;
    }
  }

  /* Test has ended. */
  if (mac154TestCb.state != MAC_154_TEST_STATE_RX)
  {
    return FALSE;
  }

  /* Continue receiving next packet. */
  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      Start receive test.
 *
 *  \param      chan            Channel.
 *  \param      numPkt          Auto terminate after number of packets, 0 for infinite.
 *  \param      txAck           Transmit ACK after receive.
 *  \param      promiscuous     Promiscuous mode.
 *
 *  \return     Status error code.
 */
/*************************************************************************************************/
uint8_t Mac154TestRx(uint8_t chan, uint16_t numPkt, bool_t txAck,
    bool_t promiscuous)
{
  Mac154Pib_t *pPib = Mac154GetPIB();

  if (mac154TestCb.state != MAC_154_TEST_STATE_IDLE)
  {
    return 0xFF;
  }

  BbOpDesc_t *pOp;
  if ((pOp = WsfBufAlloc(sizeof(*pOp))) == NULL)
  {
    return 0xFF;
  }
  memset(pOp, 0, sizeof(*pOp));

  Bb154Data_t *p154;
  if ((p154 = WsfBufAlloc(sizeof(*p154))) == NULL)
  {
    WsfBufFree(pOp);
    return 0xFF;
  }
  memset(p154, 0, sizeof(*p154));

  pPib->promiscuousMode = (uint8_t)promiscuous;
  pPib->rxOnWhenIdle = TRUE;

  p154->opType = BB_154_OP_TEST_RX;

  p154->chan.channel = chan;
  p154->chan.txPower = 0;

  p154->opParam.flags         = txAck ? PAL_BB_154_FLAG_TX_RX_AUTO_ACK : PAL_BB_154_FLAG_TX_AUTO_RX_ACK;
  p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

  p154->op.testRx.rxLen     = p154->opParam.psduMaxLength;
  p154->op.testRx.testCback = mac154TestRxCback;

  pOp->protId        = BB_PROT_15P4;
  pOp->prot.p154     = p154;
  pOp->endCback      = mac154TestRxOpEndCback;
  pOp->dueOffsetUsec = 0;

  mac154TestCb.state  = MAC_154_TEST_STATE_RX;
  mac154TestCb.numPkt = numPkt;
  memset(&mac154TestCb.stats, 0, sizeof(mac154TestCb.stats));

  BbStart(BB_PROT_15P4);
  SchInsertNextAvailable(pOp);

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      Get packet statistics.
 *
 *  \param[out] pPktCount     Packet (frame) count.
 *  \param[out] pAckCount     Acknowledgement count.
 *  \param[out] pPktErrCount  Packet error count.
 *  \param[out] pAckErrCount  Acknowledgement error count.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154TestGetPktStats(uint32_t *pPktCount,
                           uint32_t *pAckCount,
                           uint32_t *pPktErrCount,
                           uint32_t *pAckErrCount)
{
  *pPktCount = mac154TestCb.stats.pktCount;
  *pAckCount = mac154TestCb.stats.ackCount;
  *pPktErrCount = mac154TestCb.stats.pktErrCount;
  *pAckErrCount = mac154TestCb.stats.ackErrCount;
}

/*************************************************************************************************/
/*!
 *  \brief      End test.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154TestEnd(void)
{
  if ((mac154TestCb.state == MAC_154_TEST_STATE_TX) || (mac154TestCb.state == MAC_154_TEST_STATE_RX))
  {
    mac154TestCb.state = MAC_154_TEST_STATE_TERM;
    BbCancelBod();
  }
}
