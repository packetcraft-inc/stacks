/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: Test.
 *
 *  Copyright (c) 2016-2018 Arm Ltd.
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
#include "util/bstream.h"

#include "chci_154_int.h"
#include "mac_154_int.h"
#include "bb_154.h"
#include "bb_154_drv_vs.h"

/*************************************************************************************************/
/*!
 *  \brief      Send set network parameters confirm.
 *
 *  \param      status    Set parameter status.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void Chci154TestSendSetNetParamsCnf(uint8_t status)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + CHCI_154_LEN_TEST_SET_NET_PARAMS_CNF)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    UINT8_TO_BSTREAM (pBuf, CHCI_154_CMD_TEST_SET_NET_PARAMS_CNF);
    UINT16_TO_BSTREAM(pBuf, CHCI_154_LEN_TEST_SET_NET_PARAMS_CNF);
    UINT8_TO_BSTREAM (pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send get RSSI confirm.
 *
 *  \param      rssi    RSSI value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void Chci154TestSendGetPktStatsCnf(uint32_t pktCount,
                                                 uint32_t ackCount,
                                                 uint32_t pktErrCount,
                                                 uint32_t ackErrCount)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + CHCI_154_LEN_TEST_GET_PKT_STATS_CNF)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    UINT8_TO_BSTREAM (pBuf, CHCI_154_CMD_TEST_GET_PKT_STATS_CNF);
    UINT16_TO_BSTREAM(pBuf, CHCI_154_LEN_TEST_GET_PKT_STATS_CNF);
    UINT32_TO_BSTREAM (pBuf, pktCount);
    UINT32_TO_BSTREAM (pBuf, ackCount);
    UINT32_TO_BSTREAM (pBuf, pktErrCount);
    UINT32_TO_BSTREAM (pBuf, ackErrCount);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send get RSSI confirm.
 *
 *  \param      rssi    RSSI value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void Chci154TestSendGetLastRssiCnf(uint8_t *pRssi, uint8_t lqi)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + CHCI_154_LEN_TEST_GET_LAST_RSSI_CNF)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    UINT8_TO_BSTREAM (pBuf, CHCI_154_CMD_TEST_GET_LAST_RSSI_CNF);
    UINT16_TO_BSTREAM(pBuf, CHCI_154_LEN_TEST_GET_LAST_RSSI_CNF);
    UINT8_TO_BSTREAM (pBuf, pRssi[0]);
    UINT8_TO_BSTREAM (pBuf, pRssi[1]);
    UINT8_TO_BSTREAM (pBuf, pRssi[2]);
    UINT8_TO_BSTREAM (pBuf, pRssi[3]);
    UINT8_TO_BSTREAM (pBuf, lqi);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send get RSSI confirm.
 *
 *  \param      rssi    RSSI value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void Chci154TestSendGetBBStatsCnf(Bb154DrvStats_t *stats)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + CHCI_154_LEN_TEST_GET_BB_STATS_CNF)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    UINT8_TO_BSTREAM (pBuf, CHCI_154_CMD_TEST_GET_PKT_STATS_CNF);
    UINT16_TO_BSTREAM(pBuf, CHCI_154_LEN_TEST_GET_PKT_STATS_CNF);
    UINT32_TO_BSTREAM(pBuf, stats->txSchMiss);
    UINT32_TO_BSTREAM(pBuf, stats->rxSchMiss);
    UINT32_TO_BSTREAM(pBuf, stats->txPkt);
    UINT32_TO_BSTREAM(pBuf, stats->txDmaFail);
    UINT32_TO_BSTREAM(pBuf, stats->rxPkt);
    UINT32_TO_BSTREAM(pBuf, stats->rxPktTimeout);
    UINT32_TO_BSTREAM(pBuf, stats->rxFilterFail);
    UINT32_TO_BSTREAM(pBuf, stats->rxCrcFail);
    UINT32_TO_BSTREAM(pBuf, stats->rxDmaFail);
    UINT32_TO_BSTREAM(pBuf, stats->ed);
    UINT32_TO_BSTREAM(pBuf, stats->edSchMiss);
    UINT32_TO_BSTREAM(pBuf, stats->ccaSchMiss);
    UINT32_TO_BSTREAM(pBuf, stats->txReq);
    UINT32_TO_BSTREAM(pBuf, stats->rxReq);

    Chci154SendEvent(pMsg);
  }
}


/*************************************************************************************************/
/*!
 *  \brief      Test command handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/
static bool_t chci154TestCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  switch (pHdr->code)
  {
    case CHCI_154_CMD_TEST_GET_BB_STATS_REQ:
    {
      Bb154DrvStats_t stats;
      Bb154DrvGetStats(&stats);
      Chci154TestSendGetBBStatsCnf(&stats);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_CONTINOUS_STOP:
    {
      PalBb154ContinuousStop();
      return TRUE;
    }
    case CHCI_154_CMD_TEST_CONTINOUS_TX:
    {
      PalBb154ContinuousTx(pBuf[0], pBuf[1], pBuf[2], pBuf[3]);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_CONTINOUS_RX:
    {
      PalBb154ContinuousRx(pBuf[0], pBuf[1]);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_GET_PKT_STATS_REQ:
    {
      uint32_t pktCount, ackCount, pktErrCount, ackErrCount;

      Mac154TestGetPktStats(&pktCount, &ackCount, &pktErrCount, &ackErrCount);
      Chci154TestSendGetPktStatsCnf(pktCount, ackCount, pktErrCount, ackErrCount);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_GET_LAST_RSSI_REQ:
    {
      uint8_t rssi[4];
      uint8_t lqi;

      PalBb154GetLastRssi(rssi);
      lqi = PalBb154RssiToLqi(rssi[0]);
      Chci154TestSendGetLastRssiCnf(rssi, lqi);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_SET_NET_PARAMS_REQ:
    {
      uint64_t addr64;
      uint16_t addr16;
      uint16_t panId;
      uint8_t  status;

      BSTREAM_TO_UINT64(addr64, pBuf);
      BSTREAM_TO_UINT16(addr16, pBuf);
      BSTREAM_TO_UINT16(panId,  pBuf);

      status = Mac154TestSetNetParams(addr64, addr16, panId);
      Chci154TestSendSetNetParamsCnf(status);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_TX:
    {
      uint8_t  chan;
      uint8_t  power;
      uint8_t  len;
      uint8_t  pktType;
      uint16_t numPkt;
      uint32_t interPktSpace;
      uint8_t  rxAck;
      uint8_t  addrModes;
      uint64_t dstAddr;
      uint16_t dstPanId;

      BSTREAM_TO_UINT8 (chan,          pBuf);
      BSTREAM_TO_UINT8 (power,         pBuf);
      BSTREAM_TO_UINT8 (len,           pBuf);
      BSTREAM_TO_UINT8 (pktType,       pBuf);
      BSTREAM_TO_UINT16(numPkt,        pBuf);
      BSTREAM_TO_UINT32(interPktSpace, pBuf);
      BSTREAM_TO_UINT8 (rxAck,         pBuf);
      BSTREAM_TO_UINT8 (addrModes,     pBuf);
      BSTREAM_TO_UINT64(dstAddr,       pBuf);
      BSTREAM_TO_UINT16(dstPanId,      pBuf);

      Mac154TestTx(chan, power, len, pktType, numPkt, interPktSpace, rxAck, addrModes, dstAddr, dstPanId);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_RX:
    {
      uint8_t  chan;
      uint16_t numPkt;
      uint8_t  txAck;
      bool_t   promiscuous;

      BSTREAM_TO_UINT8 (chan,          pBuf);
      BSTREAM_TO_UINT16(numPkt,        pBuf);
      BSTREAM_TO_UINT8 (txAck,         pBuf);
      BSTREAM_TO_UINT8 (promiscuous,   pBuf);
      Mac154TestRx(chan, numPkt, txAck, promiscuous);
      return TRUE;
    }
    case CHCI_154_CMD_TEST_END:
    {
      Mac154TestEnd();
      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for test operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154TestInit(void)
{
  Chci154RegisterCmdHandler(chci154TestCmdHandler);
}

/*************************************************************************************************/
/*!
 *  \brief      Send test end indication.
 *
 *  \param      pktCount    Packet count.
 *  \param      pktErrCount Packet error count.
 *  \param      ackCount    ACK count.
 *  \param      ackErrCount ACK error count.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154TestSendTestEndInd(uint32_t pktCount, uint32_t pktErrCount, uint32_t ackCount, uint32_t ackErrCount)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + CHCI_154_LEN_TEST_END_IND)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    UINT8_TO_BSTREAM (pBuf, CHCI_154_CMD_TEST_END_IND);
    UINT16_TO_BSTREAM(pBuf, CHCI_154_LEN_TEST_END_IND);
    UINT32_TO_BSTREAM(pBuf, pktCount);
    UINT32_TO_BSTREAM(pBuf, pktErrCount);
    UINT32_TO_BSTREAM(pBuf, ackCount);
    UINT32_TO_BSTREAM(pBuf, ackErrCount);

    Chci154SendEvent(pMsg);
  }
}
