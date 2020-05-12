/*************************************************************************************************/
/*!
 *  \file   mesh_br_br_main.c
 *
 *  \brief  Mesh Provisioning Bearer module implementation.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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
 *
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_math.h"
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "util/fcs.h"

#include "sec_api.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_utils.h"
#include "mesh_error_codes.h"

#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_bearer.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_br_main.h"
#include "mesh_prv_beacon.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

#define PROVISONER

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Extracts the opcode value from the first octet of a Control PDU */
#define EXTRACT_OPCODE(byte) ((byte) >> MESH_PRV_GPCF_SIZE)

/*! Sets the opcode value from the first octet of a Control PDU */
#define SET_OPCODE(byte, opcode) ((byte) = (byte) | ((opcode) << MESH_PRV_GPCF_SIZE))

/*! Extracts the padding value from the first octet of a Control PDU */
#define EXTRACT_PADDING(byte) ((byte) >> MESH_PRV_GPCF_SIZE)

/*! Extracts the SegN or SegIndex value from the first octet of a Control PDU */
#define EXTRACT_SEGX(byte) ((byte) >> MESH_PRV_GPCF_SIZE)

/*! Sets the SegN or SegIndex value from the first octet of a Control PDU */
#define SET_SEGX(byte, segX) ((byte) = (byte) | ((segX) << MESH_PRV_GPCF_SIZE))

/*! Extracts the GPCF value from an octet */
#define GPCF(byte) ((byte) & MESH_PRV_GPCF_MASK)

/*! Validate link ID with the current session info for a Transaction */
#define VALIDATE_LINK(linkId) ((prvBrCb.pbAdvSessionInfo.linkOpened) && \
                               (prvBrCb.pbAdvSessionInfo.linkId == (linkId)))

/*! Validate link ID and transaction number with the current session info for an ACK */
#define VALIDATE_ACK(linkId, tranNum) ((prvBrCb.pbAdvSessionInfo.linkOpened) && \
                                        (prvBrCb.pbAdvSessionInfo.linkId == (linkId) && \
                                         prvBrCb.pbAdvSessionInfo.localTranNum == (tranNum)))

/*! Increment the transaction number for a Provisioner Server */
#define PRV_SR_INC_TRAN_NUM(x) ((x) = ((x) != MESH_PRV_SR_TRAN_NUM_WRAP) ? ((x) + 1) : \
                                  MESH_PRV_SR_TRAN_NUM_START)

/*! Increment the transaction number for a Provisioner Client */
#define PRV_CL_INC_TRAN_NUM(x) ((x) = ((x) != MESH_PRV_CL_TRAN_NUM_WRAP) ? ((x) + 1) : \
                                  MESH_PRV_CL_TRAN_NUM_START)

/*! Mark segX as received in the segments mask */
#define MASK_MARK_SEG(segX) ((segX < 32) ? (prvBrCb.pbAdvSessionInfo.rxSegMask[1] |= (1 << (segX))) : \
                                  (prvBrCb.pbAdvSessionInfo.rxSegMask[0] |= (1 << ((segX) - 32))))

/*! Check segX is received in the segments mask */
#define MASK_CHECK_SEG(segX) ((segX < 32) ? (prvBrCb.pbAdvSessionInfo.rxSegMask[1] & (1 << (segX))) : \
                                      (prvBrCb.pbAdvSessionInfo.rxSegMask[0] & (1 << ((segX) - 32))))

/*! Invalid value for the Provisioning PDU opcode, used to detect new transactions */
#define MESH_PRV_BR_INVALID_OPCODE    (0xFF)

/*! Set the PDU retry count to the timer parameters */
#define SET_RETRY_COUNT(param, count) ((param) = ((param) & 0x00FF) | ((count) << 8))

/*! Get the PDU retry count from the timer parameters */
#define GET_RETRY_COUNT(param)        ((param) >> 8)

/*! Get the PDU retry opcode from the timer parameters */
#define GET_RETRY_OPCODE(param)       ((param) & 0xFF)

/*! Retry count for Provisioning Control PDUs */
#define PRV_CTL_PDU_RETRY_COUNT       3

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Provisioning Bearer WSF message events */
enum meshPrvBrWsfMsgEvents
{
  MESH_PRV_BR_MSG_TX_TMR_EXPIRED        = MESH_PRV_BR_MSG_START, /*!< Tx timer expired event */
  MESH_PRV_BR_MSG_TRAN_ACK_TMR_EXPIRED,                          /*!< Ack timer expired event */
  MESH_PRV_BR_MSG_LINK_TMR_EXPIRED,                              /*!< PB-ADV Link Establishment timer
                                                                  *   expired event
                                                                  */
  MESH_PRV_BR_MSG_RETRY_TMR_EXPIRED                              /*!< PB-ADV Control PDU retry timer
                                                                  *   expired event
                                                                  */
};

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer Control PDU received function pointer type.
 *
 *  \param[in] pCtlPdu  Pointer to the Provisioning Bearer Control PDU received.
 *  \param[in] pduLen   Size of the Provisioning Bearer Control PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshPrvBrCtlPduCback_t)(uint32_t linkId, const uint8_t *pCtlPdu, uint8_t pduLen);

/*! PB-ADV session information type definition */
typedef struct meshPrvBrSessionInfo_tag
{
  uint8_t       *pTxPrvPdu;                        /*!< Transmitted Prv PDU buffer. */
  uint32_t      txTranTimeoutMs;                   /*!< Tx transaction timeout in ms. */
  uint16_t      txTotalLength;                     /*!< Tx transaction PDU total length. */
  uint8_t       txNextSegmentIndex;                /*!< Tx transaction next segment index. */
  uint8_t       txSegN;                            /*!< Tx transaction SegN value. */

  uint8_t       *pRxPrvPdu;                        /*!< Received Prv PDU buffer. */
  uint32_t      rxSegMask[MESH_PRV_SEG_MASK_SIZE]; /*!< PB Control PDU received callback. */
  uint16_t      rxTotalLength;                     /*!< Rx transaction PDU total length. */
  uint8_t       rxSegN;                            /*!< Rx transaction SegN value. */
  uint8_t       rxFcs;                             /*!< Rx transaction FCS value. */
  bool_t        rxAckSent;                         /*!< Rx transaction has been acked. */
  uint8_t       rxLastReceivedOpcode;              /*!< Last received Provisioning PDU opcode. */

  uint8_t       receivedTranNum;                   /*!< Received transaction number on the
                                                    *   PB-ADV link.
                                                    */
  uint8_t       localTranNum;                      /*!< Local transaction number used on the
                                                    *   PB-ADV link.
                                                    */
  uint32_t      linkId;                            /*!< PB-ADV link indentifier. */
  uint8_t       *pDeviceUuid;                      /*!< PB-ADV Device UUID. Used by Provisioning
                                                    *   Client to repeat the link open procedure.
                                                    */
  wsfTimer_t    linkTimer;                         /*!< Link Establishment timer. Used by either
                                                    *   Provisioning Client or Server
                                                    */
  wsfTimer_t    ctlPduRetryTimer;                  /*!< Provisioning Bearer Control PDU retry timer.
                                                    *   Used for Link Ack and Link Close
                                                    */
  bool_t        linkOpened;                        /*!< TRUE if PB-ADV link is opened,
                                                    *   FALSE otherwise.
                                                    */
} meshPrvBrSessionInfo_t;

/*! Provisioning Bearer Control Block type definition */
typedef struct meshPrvBrCb_tag
{
  meshPrvBrCtlPduCback_t      brPrvCtlPduCback;   /*!< PB Control PDU received callback. */
  meshPrvBrEventNotifyCback_t brPrvEventCback;    /*!< PB event notification callback. */
  meshPrvBrPduRecvCback_t     brPrvPduRecvCback;  /*!< PB PDU received callback. */
  wsfTimer_t                  txTmr;              /*!< Tx timer */
  wsfTimer_t                  ackTmr;             /*!< Ack Transaction timer */
  meshPrvBrSessionInfo_t      pbAdvSessionInfo;   /*!< PB-ADV session information. */
  meshBrInterfaceId_t         advIfId;            /*!< PB-ADV interface identifier. */
  meshBrInterfaceId_t         gattIfId;           /*!< PB-GATT interface identifier. */
  meshPrvType_t               prvType;            /*!< Provisioner type. See ::meshPrvType. */
} meshPrvBrCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Provisioning Bearer Control Block */
static meshPrvBrCb_t  prvBrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming events from the bearer layer.
 *
 *  \param[in] brIfId        Unique identifier of the interface.
 *  \param[in] event         Type of event that is notified.
 *  \param[in] pEventParams  Event parameters associated to the event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrEventNotificationCback(meshBrInterfaceId_t brIfId, meshBrEvent_t event,
                                         const meshBrEventParams_t *pEventParams)
{
  meshPrvBrEventParams_t msgParams;

  /* Should never happen as these are validated by the bearer. */
  WSF_ASSERT(pEventParams != NULL);

  /* Handle events by type. */
  switch (event)
  {
    case MESH_BR_INTERFACE_CLOSED_EVT:
      if (brIfId == prvBrCb.gattIfId)
      {
        prvBrCb.gattIfId = MESH_BR_INVALID_INTERFACE_ID;

        /* Notify Provisioning Protocol of the closed connection */
        prvBrCb.brPrvEventCback(MESH_PRV_BR_CONN_CLOSED, NULL);
      }
      else if (brIfId == prvBrCb.advIfId)
      {
        prvBrCb.advIfId = MESH_BR_INVALID_INTERFACE_ID;
      }
      break;

    case MESH_BR_INTERFACE_PACKET_SENT_EVT:
      /* Should never happen as these are validated by the bearer. */
      WSF_ASSERT(pEventParams->brPduStatus.pPdu != NULL);
      /* Only on PB-GATT because there is no Link ACK */
      if (brIfId == prvBrCb.gattIfId)
      {
        /* Set opcode in message parameters */
        msgParams.pduSentOpcode = pEventParams->brPduStatus.pPdu[MESH_PRV_PDU_OPCODE_INDEX];

        /* Notify upper layer */
        prvBrCb.brPrvEventCback(MESH_PRV_BR_PDU_SENT, &msgParams);
      }
      /* Free buffer for PDU sent over-the-air */
      WsfBufFree(pEventParams->brPduStatus.pPdu);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Sends a Provisioning Bearer PDU to the Provisioning Bearer interface.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrSendPduToBearer(void)
{
  uint8_t *pPbAdvPdu, *pPduOffset;
  uint16_t txPduLen, txPduOffset;

  /* Calculate the Generic Prov PDU length for this segment and offset in the Prov PDU */
  if (prvBrCb.pbAdvSessionInfo.txNextSegmentIndex == 0)
  {
    txPduOffset = 0;
    txPduLen = WSF_MIN(prvBrCb.pbAdvSessionInfo.txTotalLength, MESH_PRV_MAX_SEG0_PB_PDU_SIZE);
  }
  else
  {
    txPduOffset = ((prvBrCb.pbAdvSessionInfo.txNextSegmentIndex - 1) * MESH_PRV_MAX_SEGX_PB_PDU_SIZE) +
                  MESH_PRV_MAX_SEG0_PB_PDU_SIZE;

    txPduLen = WSF_MIN(prvBrCb.pbAdvSessionInfo.txTotalLength - txPduOffset,
                       MESH_PRV_MAX_SEGX_PB_PDU_SIZE);
   }

  pPbAdvPdu = WsfBufAlloc(txPduLen + MESH_PRV_PB_ADV_GEN_PDU_OFFSET);

  if (pPbAdvPdu == NULL)
  {
    return;
  }

  /* Store Pdu Offset */
  pPduOffset = pPbAdvPdu;

  /* Fill PB-ADV Header */
  UINT32_TO_BE_BSTREAM(pPduOffset, prvBrCb.pbAdvSessionInfo.linkId);
  UINT8_TO_BSTREAM(pPduOffset,prvBrCb.pbAdvSessionInfo.localTranNum);

  /* Fill Generic Provisioning PDU */
  if (prvBrCb.pbAdvSessionInfo.txNextSegmentIndex == 0)
  {
    /* Set Start, SegN, Total Length and FCS */
    UINT8_TO_BSTREAM(pPduOffset, MESH_PRV_GPCF_START);
    SET_SEGX(pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET], prvBrCb.pbAdvSessionInfo.txSegN);
    UINT16_TO_BE_BSTREAM(pPduOffset, prvBrCb.pbAdvSessionInfo.txTotalLength);
    UINT8_TO_BSTREAM(pPduOffset,
                     FcsCalc(prvBrCb.pbAdvSessionInfo.txTotalLength, prvBrCb.pbAdvSessionInfo.pTxPrvPdu));
  }
  else
  {
    /* Set Continuation and SegmentIndex */
    UINT8_TO_BSTREAM(pPduOffset, MESH_PRV_GPCF_CONTINUATION);
    SET_SEGX(pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET], prvBrCb.pbAdvSessionInfo.txNextSegmentIndex);
  }

  /* Copy PDU Data */
  memcpy(pPduOffset, &prvBrCb.pbAdvSessionInfo.pTxPrvPdu[txPduOffset], txPduLen);

  MESH_TRACE_INFO2("MESH PRV BR: TX TRAN=0x%x SEG=0x%X ", prvBrCb.pbAdvSessionInfo.localTranNum,
                   pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET]);

  /* Send PDU to ADV Bearer */
  if (MeshBrSendPrvPdu(prvBrCb.advIfId, pPbAdvPdu, (uint8_t)((uint8_t)(pPduOffset - pPbAdvPdu) + txPduLen)))
  {
    /* Move on transmission to the next segment */
    prvBrCb.pbAdvSessionInfo.txNextSegmentIndex++;
  }
  else
  {
    /* Free buffer since the bearer can't accept the PDU */
    WsfBufFree(pPbAdvPdu);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Sends a Link Ack message to the Provisioning Client.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrSendLinkAck(void)
{
  uint8_t *pPbAdvPdu;

  pPbAdvPdu = WsfBufAlloc(1 + MESH_PRV_PB_ADV_GEN_PDU_OFFSET);

  if (pPbAdvPdu == NULL)
  {
    return;
  }

  /* Fill PB-ADV PDU */
  UINT32_TO_BE_BUF(pPbAdvPdu, prvBrCb.pbAdvSessionInfo.linkId);
  pPbAdvPdu[MESH_PRV_PB_ADV_TRAN_NUM_OFFSET] = 0;
  pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET] = MESH_PRV_GPCF_CONTROL;
  SET_OPCODE(pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET], MESH_PRV_LINK_ACK_OPCODE);

  MESH_TRACE_INFO0("MESH PRV BR: Sending Link Ack");

  /* Send ACK to Provisioning Client */
  MeshBrSendPrvPdu(prvBrCb.advIfId, pPbAdvPdu, 1 + MESH_PRV_PB_ADV_GEN_PDU_OFFSET);
}

/*************************************************************************************************/
/*!
 *  \brief  Prepare for sending repeated Link Ack messages to the Provisioning Client.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrPrepareLinkAck(void)
{
  uint8_t txDelayInMs;

  /* Store retry message type */
  prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.param = MESH_PRV_LINK_ACK_OPCODE;

  /* Store retry count */
  SET_RETRY_COUNT(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.param, PRV_CTL_PDU_RETRY_COUNT);

  SecRand(&txDelayInMs, sizeof(uint8_t));
  txDelayInMs = MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS + (txDelayInMs %
                (MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS - MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS));

  /* Start Retry timer */
  WsfTimerStartMs(&(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer), txDelayInMs);
}

/*************************************************************************************************/
/*!
 *  \brief  Sends a Link Close message to the Provisioning Client or Server.
 *
 *  \param[in] reason       Reason for closing the interface. See ::meshPrvBrReasonTypes.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrSendLinkClose(meshPrvBrReason_t reason)
{
  uint8_t *pPbAdvPdu;

  pPbAdvPdu = WsfBufAlloc(MESH_PRV_LINK_CLOSE_PDU_SIZE + MESH_PRV_PB_ADV_GEN_PDU_OFFSET);

  if (pPbAdvPdu == NULL)
  {
    return;
  }

  /* Fill PB-ADV PDU */
  UINT32_TO_BE_BUF(pPbAdvPdu, prvBrCb.pbAdvSessionInfo.linkId);
  pPbAdvPdu[MESH_PRV_PB_ADV_TRAN_NUM_OFFSET] = 0;
  pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET] = MESH_PRV_GPCF_CONTROL;
  SET_OPCODE(pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET], MESH_PRV_LINK_CLOSE_OPCODE);
  pPbAdvPdu[MESH_PRV_PB_ADV_GEN_DATA_OFFSET] = reason;

  /* Send Link Close PDU to Provisioning Client */
  MeshBrSendPrvPdu(prvBrCb.advIfId, pPbAdvPdu, MESH_PRV_LINK_CLOSE_PDU_SIZE +
                   MESH_PRV_PB_ADV_GEN_PDU_OFFSET);

  MESH_TRACE_INFO0("MESH PRV BR: Sending Link Close");
}

/*************************************************************************************************/
/*!
 *  \brief  Prepare for sending Link Close messages to the Provisioning Client or Server.
 *
 *  \param[in] reason       Reason for closing the interface. See ::meshPrvBrReasonTypes.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrPrepareLinkClose(meshPrvBrReason_t reason)
{
  wsfTimerTicks_t txDelayInMs;

  /* Store retry message type */
  prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.param = MESH_PRV_LINK_CLOSE_OPCODE;

  /* Store retry count */
  SET_RETRY_COUNT(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.param, PRV_CTL_PDU_RETRY_COUNT);

  /* Store reason */
  prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.status = reason;

  SecRand((uint8_t *)(&txDelayInMs), sizeof(wsfTimerTicks_t));
  txDelayInMs = MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS + (txDelayInMs %
                (MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS - MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS));

  /* Check if a transaction ACK is pending */
  if(prvBrCb.ackTmr.isStarted)
  {
    /* Offset TX delay to have the anchor point the transaction ACK PDU. */
    txDelayInMs += WSF_MS_PER_TICK * prvBrCb.ackTmr.ticks;
  }

  /* Start Retry timer */
  WsfTimerStartMs(&(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer), txDelayInMs);
}

/*************************************************************************************************/
/*!
 *  \brief  Sends a Transaction Ack message to acknowledge the transaction.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrAckTransaction(void)
{
  uint8_t *pPbAdvPdu;

  pPbAdvPdu = WsfBufAlloc(1 + MESH_PRV_PB_ADV_GEN_PDU_OFFSET);

  if (pPbAdvPdu == NULL)
  {
    return;
  }

  /* Fill PB-ADV PDU */
  UINT32_TO_BE_BUF(pPbAdvPdu, prvBrCb.pbAdvSessionInfo.linkId);
  pPbAdvPdu[MESH_PRV_PB_ADV_TRAN_NUM_OFFSET] = prvBrCb.pbAdvSessionInfo.receivedTranNum;
  pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET] = MESH_PRV_GPCF_ACK;

  MESH_TRACE_INFO1("MESH PRV BR: Ack TRAN=0x%X", prvBrCb.pbAdvSessionInfo.receivedTranNum);

  /* Send ACK to peer */
  MeshBrSendPrvPdu(prvBrCb.advIfId, pPbAdvPdu, 1 + MESH_PRV_PB_ADV_GEN_PDU_OFFSET);
}

/*************************************************************************************************/
/*!
 *  \brief  Prepare for sending Transaction Ack message to the peer.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrPrepareAckTransaction(void)
{
  uint8_t txDelayInMs;

  SecRand(&txDelayInMs, sizeof(uint8_t));
  txDelayInMs = MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS + (txDelayInMs %
                (MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS - MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS));

  /* Start transaction ACK delay timer */
  WsfTimerStartMs(&(prvBrCb.ackTmr), txDelayInMs);
}

/*************************************************************************************************/
/*!
 *  \brief     Checks the Transaction Rx Mask to verify if all segments have been received.
 *
 *  \param[in] segN  Last segment number expected for the pending transaction.
 *
 *  \return    TRUE if all segments have been received, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshPrvBrCheckRxMask(uint8_t segN)
{
  uint8_t segX = 0;

  /* Start checking from segment 0 to segN or 32 */
  while (segX <= WSF_MIN(segN, 31))
  {
    if ((prvBrCb.pbAdvSessionInfo.rxSegMask[1] & (1 << segX)) == 0)
    {
      /* Mask not set for this segX */
      return FALSE;
    }
    segX++;
  }

  if (segX > 31)
  {
    while (segX <= WSF_MIN(segN, 63))
    {
      if ((prvBrCb.pbAdvSessionInfo.rxSegMask[0] & (1 << (segX - 32))) == 0)
      {
        /* Mask not set for this segX */
        return FALSE;
      }
      segX++;
    }
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Processes a Start Transaction message and starts a Transaction Rx process. Allocates
 *             enough memory to receive all following segments.
 *
 *  \param[in] pGenPdu  Pointer to the Generic Provisioning PDU for the Start Transaction message.
 *  \param[in] pduLen   Generic Provisioning PDU length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrStartRxTransaction(const uint8_t *pGenPdu, uint8_t pduLen)
{
  /* Check if multi-segment transaction is active */
  if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu != NULL)
  {
    /* Check if this is the start segment of the same transaction */
    if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu[MESH_PRV_PDU_OPCODE_INDEX] == pGenPdu[MESH_PRV_PDU_OPCODE_INDEX])
    {
      /* Ignore the start segment of the same transaction */
      return;
    }
    else
    {
      /* This is a new transaction - free the buffer for the old transaction */
      WsfBufFree(prvBrCb.pbAdvSessionInfo.pRxPrvPdu);
      prvBrCb.pbAdvSessionInfo.pRxPrvPdu = NULL;
    }
  }

  /* Unpack Transaction Start PDU */
  prvBrCb.pbAdvSessionInfo.rxSegN = EXTRACT_SEGX(pGenPdu[0]);
  pGenPdu++;
  BSTREAM_BE_TO_UINT16(prvBrCb.pbAdvSessionInfo.rxTotalLength, pGenPdu);
  BSTREAM_TO_UINT8(prvBrCb.pbAdvSessionInfo.rxFcs, pGenPdu);

  /* Check number of segments */
  if (prvBrCb.pbAdvSessionInfo.rxSegN == 0)
  {
    /* The Prov PDU has only one segment. Check Fcs, Total Length */
    if ((prvBrCb.pbAdvSessionInfo.rxTotalLength == (pduLen - MESH_PRV_MAX_SEG0_PB_HDR_SIZE) &&
         FcsCalc(prvBrCb.pbAdvSessionInfo.rxTotalLength, pGenPdu) == prvBrCb.pbAdvSessionInfo.rxFcs))
    {
      /* Ack transaction */
      meshPrvBrPrepareAckTransaction();

      /* Mark transaction as Acked */
      prvBrCb.pbAdvSessionInfo.rxAckSent = TRUE;

      /* Save last received PDU opcode */
      prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode = pGenPdu[MESH_PRV_PDU_OPCODE_INDEX];

      /* The Provisioning Server received a PDU. Cancel Link timer, if started. */
      if (prvBrCb.pbAdvSessionInfo.linkTimer.isStarted)
      {
        /* Close link timer */
        WsfTimerStop(&(prvBrCb.pbAdvSessionInfo.linkTimer));
      }

      /* Send Provisioning PDU to the Upper Layer */
      prvBrCb.brPrvPduRecvCback(pGenPdu, (uint8_t)prvBrCb.pbAdvSessionInfo.rxTotalLength);
    }
  }
  else
  {
    /* This is a multi-segment Prov PDU. Allocate buffer for receiving it */
    prvBrCb.pbAdvSessionInfo.pRxPrvPdu = WsfBufAlloc(prvBrCb.pbAdvSessionInfo.rxTotalLength);

    /* Check if memory was allocated */
    if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu == NULL)
    {
      return;
    }

    MESH_TRACE_INFO1("MESH PRV BR: Recv SEG=0x%X ", prvBrCb.pbAdvSessionInfo.rxSegN);

    /* Reset received segments mask */
    memset(prvBrCb.pbAdvSessionInfo.rxSegMask, 0, sizeof(uint32_t) * MESH_PRV_SEG_MASK_SIZE);

    /* Copy Start segment data */
    memcpy(prvBrCb.pbAdvSessionInfo.pRxPrvPdu, pGenPdu, pduLen - MESH_PRV_MAX_SEG0_PB_HDR_SIZE);

    /* Mark Segment 0 as received */
    uint8_t recvSegmentIndex = 0;
    MASK_MARK_SEG(recvSegmentIndex);

    /* Reset Rx Ack Sent flag */
    prvBrCb.pbAdvSessionInfo.rxAckSent = FALSE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Checks whether an incoming Provisioning PDU is new.
 *
 *  \param[in] pGenPdu  Pointer to the Generic Provisioning PDU for the Start Transaction message.
 *  \param[in] pduLen   Generic Provisioning PDU length.
 *
 *  \return    TRUE if the PDU is new, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshPrvBrCheckNewPdu(const uint8_t *pGenPdu, uint8_t pduLen, uint8_t tranNum)
{
  (void)pduLen;

  /* Check for retransmitted start segment */
  if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu != NULL &&
      !prvBrCb.pbAdvSessionInfo.rxAckSent)
  {
    return FALSE;
  }

  /* Check if any PDU has been received so far */
  if (prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode == MESH_PRV_BR_INVALID_OPCODE)
  {
    /* This is the first PDU we are receiving - accept anything */
    return TRUE;
  }

  /* Check if Transaction Number is greater than the last received one */
  return (tranNum > prvBrCb.pbAdvSessionInfo.receivedTranNum);
}

/*************************************************************************************************/
/*!
 *  \brief     Processes a Continue Transaction message and continues the Transaction Rx process.
 *             Updates the allocated buffer with the segment data. If all segments are received,
 *             it signals the upper layer.
 *
 *  \param[in] pGenPdu  Pointer to the Generic Provisioning PDU for the Continue Transaction
 *                      message.
 *  \param[in] pduLen   Generic Provisioning PDU length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrContinueRxTransaction(const uint8_t *pGenPdu, uint8_t pduLen)
{
  uint8_t segX;
  uint16_t rxProvPduOffset;

  /* Do not process any continuation before start segment because we don't know the length */
  if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu == NULL)
  {
    return;
  }

  /* Unpack Transaction Continuation PDU */
  segX = EXTRACT_SEGX(pGenPdu[0]);

  /* Cross-check with SegN in Start segment */
  if (segX > prvBrCb.pbAdvSessionInfo.rxSegN)
  {
    return;
  }

  /* Check received mask */
  if (MASK_CHECK_SEG(segX) != 0)
  {
    /* Segment has been received before */
    if (prvBrCb.pbAdvSessionInfo.rxAckSent)
    {
      /* This transaction has been complete, but peer has missed the Ack - resend it */
      meshPrvBrPrepareAckTransaction();
    }
    return;
  }

  /* Calculate PDU offset for this segment */
  rxProvPduOffset = MESH_PRV_MAX_SEG0_PB_PDU_SIZE + (MESH_PRV_MAX_SEGX_PB_PDU_SIZE * (segX - 1));

  /* Copy segment data into received Prov PDU buffer and mark as received */
  MASK_MARK_SEG(segX);
  memcpy(&prvBrCb.pbAdvSessionInfo.pRxPrvPdu[rxProvPduOffset], &pGenPdu[1], pduLen - 1);

  if (meshPrvBrCheckRxMask(prvBrCb.pbAdvSessionInfo.rxSegN))
  {
    /* This is the last needed segment. Check Fcs, Total Length */
    if (FcsCalc(prvBrCb.pbAdvSessionInfo.rxTotalLength, prvBrCb.pbAdvSessionInfo.pRxPrvPdu) ==
        prvBrCb.pbAdvSessionInfo.rxFcs)
    {
      /* Ack transaction */
      meshPrvBrPrepareAckTransaction();

      /* Save last received PDU opcode */
      prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode = prvBrCb.pbAdvSessionInfo.pRxPrvPdu[MESH_PRV_PDU_OPCODE_INDEX];

      /* Link timer is started only on a Provisioning Server that didn't receive a PDU*/
      if (prvBrCb.pbAdvSessionInfo.linkTimer.isStarted)
      {
        /* Close link timer */
        WsfTimerStop(&(prvBrCb.pbAdvSessionInfo.linkTimer));
      }

      /* Send Provisioning PDU to the Upper Layer */
      prvBrCb.brPrvPduRecvCback(prvBrCb.pbAdvSessionInfo.pRxPrvPdu, (uint8_t)prvBrCb.pbAdvSessionInfo.rxTotalLength);

      /* Mark transaction as Acked */
      prvBrCb.pbAdvSessionInfo.rxAckSent = TRUE;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Closes a Tx transaction.
 *
 *  \return The opcode of the PDU that was being sent.
 */
/*************************************************************************************************/
static uint8_t meshPrvBrEndTxTransaction(void)
{
  uint8_t opcode = MESH_PRV_PDU_RFU_START;

  if (prvBrCb.pbAdvSessionInfo.pTxPrvPdu != NULL)
  {
    opcode = prvBrCb.pbAdvSessionInfo.pTxPrvPdu[0];

    /* Free buffer containing the Provisioning PDU */
    WsfBufFree(prvBrCb.pbAdvSessionInfo.pTxPrvPdu);
    prvBrCb.pbAdvSessionInfo.pTxPrvPdu = NULL;
  }

  /* Stop Tx timer */
  WsfTimerStop((&prvBrCb.txTmr));

  /* Increase the local transaction number */
  if (prvBrCb.prvType == MESH_PRV_SERVER)
  {
    PRV_SR_INC_TRAN_NUM(prvBrCb.pbAdvSessionInfo.localTranNum);
  }
  else
  {
    PRV_CL_INC_TRAN_NUM(prvBrCb.pbAdvSessionInfo.localTranNum);
  }

  return opcode;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer Control PDU received on a Provisioner Server callback.
 *
 *  \param[in] linkId   LinkId.
 *  \param[in] pPrvPdu  Provisioning Bearer Control PDU received.
 *  \param[in] pduLen   Size of the Provisioning Bearer Control PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrSrProcessCtlPdu(uint32_t linkId, const uint8_t *pCtlPdu, uint8_t pduLen)
{
  meshPrvBrEventParams_t eventParams;

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  meshTestPbLinkClosedInd_t    pbLinkClosedInd;
  meshTestPbInvalidOpcodeInd_t pbInvalidOpcodeInd;
#endif

  /* Extract opcode value */
  switch (EXTRACT_OPCODE(pCtlPdu[0]))
  {
    case MESH_PRV_LINK_OPEN_OPCODE:
      /* Verify PDU length and UUID */
      if ((pduLen == MESH_PRV_LINK_OPEN_PDU_SIZE) && (MeshPrvBeaconMatch(&pCtlPdu[1])))
      {
        if (!prvBrCb.pbAdvSessionInfo.linkOpened)
          /* Link not opened - stop beacons and initialize link information */
        {
          /* Stop sending unprovisioned beacons */
          MeshPrvBeaconStop();

          /* Open Link. Transaction ID shall be set to 0 */
          prvBrCb.pbAdvSessionInfo.linkId = linkId;
          prvBrCb.pbAdvSessionInfo.linkOpened = TRUE;

          /* Start Link timer. */
          WsfTimerStartMs(&(prvBrCb.pbAdvSessionInfo.linkTimer), MESH_PRV_LINK_TIMEOUT_MS);

          /* Set local transaction number */
          prvBrCb.pbAdvSessionInfo.localTranNum = MESH_PRV_SR_TRAN_NUM_START;

          /* Set last received opcode to an invalid value */
          prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode = MESH_PRV_BR_INVALID_OPCODE;

          /* Send Link ACK */
          meshPrvBrPrepareLinkAck();

          /* Notify Provisioning Protocol of the opened link */
          prvBrCb.brPrvEventCback(MESH_PRV_BR_LINK_OPENED, &eventParams);
        }
        else if ((prvBrCb.pbAdvSessionInfo.linkId == linkId) &&
                (prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode == MESH_PRV_BR_INVALID_OPCODE))
        {
          /* Send Link ACK even if link is already open and no PDU was received. Peer may have
           * missed the Link ACK
           */
          meshPrvBrPrepareLinkAck();
        }
      }
      break;
    case MESH_PRV_LINK_CLOSE_OPCODE:
      if (VALIDATE_LINK(linkId) && (pduLen == MESH_PRV_LINK_CLOSE_PDU_SIZE))
      {
        /* Link is closed. Stop Link timer and Transaction Ack timer. */
        WsfTimerStop(&(prvBrCb.pbAdvSessionInfo.linkTimer));
        WsfTimerStop(&(prvBrCb.ackTmr));

        /* Close pending Tx transaction */
        (void)meshPrvBrEndTxTransaction();

        /* Free Rx transaction buffer */
        if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu != NULL)
        {
          WsfBufFree(prvBrCb.pbAdvSessionInfo.pRxPrvPdu);
          prvBrCb.pbAdvSessionInfo.pRxPrvPdu = NULL;
        }

        /* Close Link */
        prvBrCb.pbAdvSessionInfo.linkOpened = FALSE;
        prvBrCb.pbAdvSessionInfo.localTranNum = 0;

        /* Notify Provisioning Protocol of the closed link */
        eventParams.linkCloseReason = pCtlPdu[1];
        prvBrCb.brPrvEventCback(MESH_PRV_BR_LINK_CLOSED_BY_PEER, &eventParams);

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
        if (meshTestCb.listenMask & MESH_TEST_PRVBR_LISTEN)
        {
          pbLinkClosedInd.hdr.event = MESH_TEST_EVENT;
          pbLinkClosedInd.hdr.param = MESH_TEST_PB_LINK_CLOSED_IND;
          pbLinkClosedInd.hdr.status = MESH_SUCCESS;

          meshTestCb.testCback((meshTestEvt_t *)&pbLinkClosedInd);
        }
#endif
      }
      break;
    default:
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
      if (meshTestCb.listenMask & MESH_TEST_PRVBR_LISTEN)
      {
        pbInvalidOpcodeInd.hdr.event = MESH_TEST_EVENT;
        pbInvalidOpcodeInd.hdr.param = MESH_TEST_PB_INVALID_OPCODE_IND;
        pbInvalidOpcodeInd.hdr.status = MESH_SUCCESS;
        pbInvalidOpcodeInd.opcode = EXTRACT_OPCODE(pCtlPdu[0]);
        meshTestCb.testCback((meshTestEvt_t *)&pbInvalidOpcodeInd);
      }
#endif
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer Control PDU received on a Provisioner Client callback.
 *
 *  \param[in] linkId   LinkId.
 *  \param[in] pPrvPdu  Provisioning Bearer Control PDU received.
 *  \param[in] pduLen   Size of the Provisioning Bearer Control PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrClProcessCtlPdu(uint32_t linkId, const uint8_t *pCtlPdu, uint8_t pduLen)
{
  meshPrvBrEventParams_t eventParams;

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  meshTestPbInvalidOpcodeInd_t pbInvalidOpcodeInd;
#endif

  /* Extract opcode value */
  switch (EXTRACT_OPCODE(pCtlPdu[0]))
  {
    case MESH_PRV_LINK_ACK_OPCODE:
      if (!prvBrCb.pbAdvSessionInfo.linkOpened && (prvBrCb.pbAdvSessionInfo.linkId == linkId) &&
          (pduLen == MESH_PRV_LINK_ACK_PDU_SIZE))
      {
        /* Mark link as opened. Reset device UUID. */
        prvBrCb.pbAdvSessionInfo.linkOpened = TRUE;
        prvBrCb.pbAdvSessionInfo.pDeviceUuid = NULL;

        /* Link is opened. Stop Link timer. */
        WsfTimerStop(&(prvBrCb.pbAdvSessionInfo.linkTimer));

        /* Set local transaction number */
        prvBrCb.pbAdvSessionInfo.localTranNum = MESH_PRV_CL_TRAN_NUM_START;

        /* Set last received opcode to an invalid value */
        prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode = MESH_PRV_BR_INVALID_OPCODE;

        /* Notify Provisioning Protocol of the opened link */
        prvBrCb.brPrvEventCback(MESH_PRV_BR_LINK_OPENED, &eventParams);
      }
      break;
    case MESH_PRV_LINK_CLOSE_OPCODE:
      if (VALIDATE_LINK(linkId) && (pduLen == MESH_PRV_LINK_CLOSE_PDU_SIZE))
      {
        /* Close pending Tx transaction */
        (void)meshPrvBrEndTxTransaction();

        /* Free Rx transaction buffer */
        if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu != NULL)
        {
          WsfBufFree(prvBrCb.pbAdvSessionInfo.pRxPrvPdu);
          prvBrCb.pbAdvSessionInfo.pRxPrvPdu = NULL;
        }

        /* Close Link */
        prvBrCb.pbAdvSessionInfo.linkOpened = FALSE;
        prvBrCb.pbAdvSessionInfo.pDeviceUuid = NULL;
        prvBrCb.pbAdvSessionInfo.localTranNum = 0;

        /* Link is opened. Stop Link timer and Transaction Ack timer. */
        WsfTimerStop(&(prvBrCb.pbAdvSessionInfo.linkTimer));
        WsfTimerStop(&(prvBrCb.ackTmr));

        /* Notify Provisioning Protocol of the closed link */
        eventParams.linkCloseReason = pCtlPdu[1];
        prvBrCb.brPrvEventCback(MESH_PRV_BR_LINK_CLOSED_BY_PEER, &eventParams);
      }
      break;
    default:
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
      if (meshTestCb.listenMask & MESH_TEST_PRVBR_LISTEN)
      {
        pbInvalidOpcodeInd.hdr.event = MESH_TEST_EVENT;
        pbInvalidOpcodeInd.hdr.param = MESH_TEST_PB_INVALID_OPCODE_IND;
        pbInvalidOpcodeInd.hdr.status = MESH_SUCCESS;
        pbInvalidOpcodeInd.opcode = EXTRACT_OPCODE(pCtlPdu[0]);
        meshTestCb.testCback((meshTestEvt_t *)&pbInvalidOpcodeInd);
      }
#endif
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer empty event notification callback
 *
 *  \param[in] event         Reason the callback is being invoked. See ::meshPrvBrEvent_t
 *  \param[in] pEventParams  Pointer to the event parameters structure passed to the function.
 *                           See ::meshPrvBrEventParams_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrEmptyPrvEventCback(meshPrvBrEvent_t event,
                                        const meshPrvBrEventParams_t *pEventParams)
{
  (void)event;
  (void)pEventParams;

  MESH_TRACE_INFO0("MESH PRV BR: Provisioning Event callback not set!");
}


/*************************************************************************************************/
/*!
 *  \brief     Empty callback for Mesh Provisioning Bearer Control PDU received.
 *
 *  \param[in] linkId   Link ID.
 *  \param[in] pCtlPdu  Pointer to the Provisioning Bearer Control PDU received.
 *  \param[in] pduLen   Size of the Provisioning Bearer Control PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrEmptySrProcessCtlPdu(uint32_t linkId, const uint8_t *pCtlPdu, uint8_t pduLen)
{
  (void)linkId;
  (void)pCtlPdu;
  (void)pduLen;

  MESH_TRACE_INFO0("MESH PRV BR: Process Control PDUs callback not set!");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty callback for Mesh Provisioning Bearer PDU received.
 *
 *  \param[in] pPrvPdu  Pointer to the Provisioning Bearer PDU received.
 *  \param[in] pduLen   Size of the Provisioning Bearer PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrEmptyPduRecvCback(const uint8_t *pPrvPdu, uint8_t pduLen)
{
  (void)pPrvPdu;
  (void)pduLen;

  MESH_TRACE_INFO0("MESH PRV BR: Process PDUs callback not set!");
}

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming PB_ADV or PB_GATT PDUs from the bearer.
 *
 *  \param[in] brIfId  Unique identifier of the interface.
 *  \param[in] pPbPdu  Pointer to the Provisioning PDU.
 *  \param[in] pduLen  Length of the Provisioning PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrPrvPduRecvCback(meshBrInterfaceId_t brIfId, const uint8_t *pPbPdu, uint8_t pduLen)
{
  uint32_t linkId;
  uint8_t tranNum;
  meshPrvBrEventParams_t msgParams;

  /* Should never happen since bearer validates this. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
  WSF_ASSERT(pPbPdu != NULL);
  WSF_ASSERT(pduLen > 0);

  /* Validate interface ID is registered. */
  if (prvBrCb.advIfId != brIfId && prvBrCb.gattIfId != brIfId)
  {
    return;
  }

  /* For PB-GATT, send it directly to the Provisioning protocol */
  if (MESH_BR_GET_BR_TYPE(brIfId) == MESH_GATT_BEARER)
  {
    /* Save last received PDU opcode */
    prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode = pPbPdu[MESH_PRV_PDU_OPCODE_INDEX];

    prvBrCb.brPrvPduRecvCback(pPbPdu, pduLen);
    return;
  }

  /* Validate PDU length for PB-ADV PDU */
  if (pduLen < MESH_PRV_MIN_PB_ADV_PDU_SIZE)
  {
    return;
  }

  /* Received PB-ADV PDU. Extract GPCF */
  BYTES_BE_TO_UINT32(linkId, pPbPdu);
  tranNum = pPbPdu[MESH_PRV_PB_ADV_TRAN_NUM_OFFSET];

  MESH_TRACE_INFO2("MESH PRV BR: RX TRAN=0x%x SEG=0x%X ", tranNum,
                    pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET]);

  /* Extract GPCF from Generic Provisioning PDU*/
  switch (GPCF(pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET]))
  {
    case MESH_PRV_GPCF_START:
      if (VALIDATE_LINK(linkId))
      {
        /* Check if peer is sending a new PDU */
        if (TRUE == meshPrvBrCheckNewPdu(&pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET],
                                         pduLen - MESH_PRV_PB_ADV_GEN_PDU_OFFSET, tranNum))
        {
          /* Check if Tx transaction is in progress and consider it complete.
             This is necessary because we may have lost the ACK from the peer. */
          if (prvBrCb.pbAdvSessionInfo.linkOpened && prvBrCb.pbAdvSessionInfo.pTxPrvPdu != NULL)
          {
            /* End current Tx transaction */
            (void)meshPrvBrEndTxTransaction();
          }
        }
        else if ((pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET + MESH_PRV_MAX_SEG0_PB_HDR_SIZE +
                        MESH_PRV_PDU_OPCODE_INDEX] ==
                 prvBrCb.pbAdvSessionInfo.rxLastReceivedOpcode) &&
            (prvBrCb.pbAdvSessionInfo.receivedTranNum == tranNum))
        {
          /* Ack only the first old PDU */
          meshPrvBrPrepareAckTransaction();
          return;
        }

        prvBrCb.pbAdvSessionInfo.receivedTranNum = tranNum;
        meshPrvBrStartRxTransaction(&pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET],
                                    pduLen - MESH_PRV_PB_ADV_GEN_PDU_OFFSET);
      }
      break;

    case MESH_PRV_GPCF_ACK:
      if (VALIDATE_ACK(linkId, tranNum) &&
          EXTRACT_PADDING(pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET]) == 0)
      {
        /* If the prohibited padding is all zeros, then accept the ACK */
        msgParams.pduSentOpcode = meshPrvBrEndTxTransaction();

        /* Notify upper layer */
        prvBrCb.brPrvEventCback(MESH_PRV_BR_PDU_SENT, &msgParams);
      }
      break;

    case MESH_PRV_GPCF_CONTINUATION:
      if (VALIDATE_LINK(linkId) && (pduLen > MESH_PRV_MIN_PB_ADV_PDU_SIZE))
      {
        /* If the packet contains data, then accept the Continuation */
        meshPrvBrContinueRxTransaction(&pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET],
                                       pduLen - MESH_PRV_PB_ADV_GEN_PDU_OFFSET);
      }
      break;

    case MESH_PRV_GPCF_CONTROL:
      /* Process Control PDU */
      prvBrCb.brPrvCtlPduCback(linkId, &pPbPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET],
                                pduLen - MESH_PRV_PB_ADV_GEN_PDU_OFFSET);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Sends a Link Open to the Provisioning Server. Used only by a Provisioning Client.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrSendLinkOpen(void)
{
  uint8_t *pPbAdvPdu;

  pPbAdvPdu = WsfBufAlloc(MESH_PRV_PB_ADV_GEN_PDU_OFFSET + MESH_PRV_LINK_OPEN_PDU_SIZE);

  if (pPbAdvPdu == NULL)
  {
    return;
  }

  /* Fill PB-ADV PDU */
  UINT32_TO_BE_BUF(pPbAdvPdu, prvBrCb.pbAdvSessionInfo.linkId);
  pPbAdvPdu[MESH_PRV_PB_ADV_TRAN_NUM_OFFSET] = 0x00;
  pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET] = MESH_PRV_GPCF_CONTROL;
  SET_OPCODE(pPbAdvPdu[MESH_PRV_PB_ADV_GEN_PDU_OFFSET], MESH_PRV_LINK_OPEN_OPCODE);
  memcpy(&pPbAdvPdu[MESH_PRV_PB_ADV_GEN_DATA_OFFSET], prvBrCb.pbAdvSessionInfo.pDeviceUuid,
         MESH_PRV_DEVICE_UUID_SIZE);

  MESH_TRACE_INFO0("MESH PRV BR: Sending Link Open");

  /* Send ACK to Provisioning Client */
  MeshBrSendPrvPdu(prvBrCb.advIfId, pPbAdvPdu, MESH_PRV_PB_ADV_GEN_PDU_OFFSET +
                   MESH_PRV_LINK_OPEN_PDU_SIZE);
}

/*************************************************************************************************/
/*!
 *  \brief     Closes the provisioning link.
 *
 *  \param[in] reason       Reason for closing the interface. See ::meshPrvBrReasonTypes.
 *  \param[in] silentClose  TRUE if no Link Close message is sent, FALSE otherwise.
 *                          (Used for ADV bearer only)
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrCloseLink(meshPrvBrReason_t reason, bool_t silentClose)
{
  /* Reason should be valid at this stage */
  WSF_ASSERT(reason <= MESH_PRV_BR_REASON_FAIL);

  /* Check if ADV BR session is closed and GATT IF ID is invalid. */
  if((prvBrCb.pbAdvSessionInfo.linkOpened == 0) &&
     (prvBrCb.gattIfId == MESH_BR_INVALID_INTERFACE_ID))
  {
    return;
  }

  /* Free Rx transaction buffer */
  if (prvBrCb.pbAdvSessionInfo.pRxPrvPdu != NULL)
  {
    WsfBufFree(prvBrCb.pbAdvSessionInfo.pRxPrvPdu);
    prvBrCb.pbAdvSessionInfo.pRxPrvPdu = NULL;
  }

  if (prvBrCb.pbAdvSessionInfo.linkOpened)
    /* PB-ADV link */
  {
    /* End TX transaction. */
    meshPrvBrEndTxTransaction();

    /* Decide if Link Close needs to be sent */
    if(!silentClose)
    {
      meshPrvBrPrepareLinkClose(reason);
    }

    /* Close Link */
    prvBrCb.pbAdvSessionInfo.linkOpened = FALSE;
    prvBrCb.pbAdvSessionInfo.localTranNum = 0;
  }
  else if (prvBrCb.gattIfId != MESH_BR_INVALID_INTERFACE_ID)
  /* PB-GATT link */
  {
    /* Close GATT interface */
    MeshBrCloseIf(prvBrCb.gattIfId);

    /* Mark PB-GATT link closed */
    prvBrCb.gattIfId = MESH_BR_INVALID_INTERFACE_ID;
  }
}

/*************************************************************************************************/
/*!
 * \brief  Mesh Provisioning Bearer Tx Timer callback.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrTxTmrCback(void)
{
  uint8_t txDelayInMs;

  if (prvBrCb.pbAdvSessionInfo.txTranTimeoutMs == 0)
  {
    /* Close PB-ADV link */
    MeshPrvBrCloseLink(MESH_PRV_BR_REASON_TIMEOUT);

    /* Notify Provisioning Protocol of the timeout */
    prvBrCb.brPrvEventCback(MESH_PRV_BR_SEND_TIMEOUT, NULL);
    return;
  }

  /* All fragments sent. ACK timeout */
  if (prvBrCb.pbAdvSessionInfo.txNextSegmentIndex > prvBrCb.pbAdvSessionInfo.txSegN)
  {
    /* Transaction is not finished. ACK was not received. Re-send all fragments. */
    prvBrCb.pbAdvSessionInfo.txNextSegmentIndex = 0;
  }

  /* Send the PDU to the bearer */
  meshPrvBrSendPduToBearer();

  /* Check if fragments need to be sent */
  if (prvBrCb.pbAdvSessionInfo.txNextSegmentIndex > prvBrCb.pbAdvSessionInfo.txSegN)
  {
    /* Wait for Ack. Start timer. */
    WsfTimerStartMs(&(prvBrCb.txTmr), 2 * MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS);
    txDelayInMs = 2 * MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS;
  }
  else
  {
    /* Set Random Tx Delay. Read random number and normalize it. */
    SecRand(&txDelayInMs, sizeof(uint8_t));
    txDelayInMs = MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS + (txDelayInMs %
                  (MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS - MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS));

    /* Start timer. */
    WsfTimerStartMs(&(prvBrCb.txTmr), txDelayInMs);
  }

  if (prvBrCb.pbAdvSessionInfo.txTranTimeoutMs < txDelayInMs)
  {
    /* Will timeout transaction on next timeout callback */
    prvBrCb.pbAdvSessionInfo.txTranTimeoutMs = 0;
  }
  else
  {
    prvBrCb.pbAdvSessionInfo.txTranTimeoutMs -= txDelayInMs;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Mesh Provisioning Bearer TransactionAck Timer callback.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrTranAckTmrCback(void)
{
  meshPrvBrAckTransaction();
}

/*************************************************************************************************/
/*!
 *  \brief  Mesh Provisioning Bearer Link Timer callback.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrLinkTmrCback(void)
{
  if ((prvBrCb.pbAdvSessionInfo.linkOpened == TRUE))
  {
    /* Close link on 60s timeout */
    meshPrvBrCloseLink(MESH_PRV_BR_REASON_TIMEOUT, FALSE);
  }
  else
  {
    /* Notify Provisioning Protocol of the failed link establishment */
    prvBrCb.brPrvEventCback(MESH_PRV_BR_LINK_FAILED, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Mesh Provisioning Bearer Control PDU Retry Timer callback.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBrCtlPduRetryTmrCback(void)
{
  uint8_t retryCount, txDelayInMs, opcode;

  /* Extract timer parameters */
  opcode = GET_RETRY_OPCODE(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.param);
  retryCount = GET_RETRY_COUNT(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.param);

  if (opcode == MESH_PRV_LINK_ACK_OPCODE)
  {
    meshPrvBrSendLinkAck();
  }
  else
  {
    /* Close reason */
    meshPrvBrSendLinkClose(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.status);
  }

  /* Check number of retries */
  if (retryCount > 0)
  {
    /* Decrement number of retries left*/
    retryCount--;
    SET_RETRY_COUNT(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.param, retryCount);

    SecRand(&txDelayInMs, sizeof(uint8_t));
    txDelayInMs = MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS + (txDelayInMs %
                  (MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS - MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS));

    /* Start Retry timer */
    WsfTimerStartMs(&(prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer), txDelayInMs);
  }
}
/*************************************************************************************************/
/*!
 *  \brief     WSF message handler callback.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvBrWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type to handle timer expiration. */
  switch(pMsg->event)
  {
    case MESH_PRV_BR_MSG_TX_TMR_EXPIRED:
      meshPrvBrTxTmrCback();
      break;
    case MESH_PRV_BR_MSG_TRAN_ACK_TMR_EXPIRED:
      meshPrvBrTranAckTmrCback();
      break;
    case MESH_PRV_BR_MSG_LINK_TMR_EXPIRED:
      meshPrvBrLinkTmrCback();
      break;
    case MESH_PRV_BR_MSG_RETRY_TMR_EXPIRED:
      meshPrvBrCtlPduRetryTmrCback();
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming Unprovisioned Beacon PDUs from the bearer.
 *
 *  \param[in] brIfId      Unique identifier of the interface.
 *  \param[in] pBeaconPdu  Pointer to the Unprovisioned Beacon PDU.
 *  \param[in] pduLen      Length of the Unprovisioned Beacon PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrBeaconPduRecvCback(meshBrInterfaceId_t brIfId, const uint8_t *pBeaconPdu,
                                     uint8_t pduLen)
{
  /* Should never happen since bearer validates this. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
  WSF_ASSERT(MESH_BR_GET_BR_TYPE(brIfId) != MESH_GATT_BEARER);
  WSF_ASSERT(pBeaconPdu != NULL);
  WSF_ASSERT(pduLen > 0);

  /* Validate interface ID is registered and beacon is of correct length. */
  if ((prvBrCb.advIfId != brIfId) ||
      ((pduLen != MESH_PRV_MAX_NO_URI_BEACON_SIZE) && (pduLen != MESH_PRV_MAX_BEACON_SIZE)))
  {
    return;
  }

  /* Check link is not opened and the Device UUID matches the one the client wants. */
  if (!prvBrCb.pbAdvSessionInfo.linkOpened &&
      (prvBrCb.pbAdvSessionInfo.pDeviceUuid != NULL) &&
      (!memcmp(&pBeaconPdu[MESH_PRV_BEACON_DEVICE_UUID_OFFSET], prvBrCb.pbAdvSessionInfo.pDeviceUuid,
          MESH_PRV_DEVICE_UUID_SIZE)))
  {
    /* Send a Link Open. */
    meshPrvBrSendLinkOpen();
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Bearer functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvBrInit(void)
{
  MESH_TRACE_INFO0("MESH PRV BR: init");

  /* Set bearer inteface as invalid */
  prvBrCb.advIfId = MESH_BR_INVALID_INTERFACE_ID;
  prvBrCb.gattIfId = MESH_BR_INVALID_INTERFACE_ID;

  /* Set empty event callbacks */
  prvBrCb.brPrvPduRecvCback = meshPrvBrEmptyPduRecvCback;
  prvBrCb.brPrvCtlPduCback = meshPrvBrEmptySrProcessCtlPdu;
  prvBrCb.brPrvEventCback = meshPrvBrEmptyPrvEventCback;

  /* Register WSF message handler callback. */
  meshCb.prvBrMsgCback = meshPrvBrWsfMsgHandlerCback;

  /* Initialize Tx timer, Control PDU retry timer and Link timer. */
  prvBrCb.txTmr.msg.event = MESH_PRV_BR_MSG_TX_TMR_EXPIRED;
  prvBrCb.txTmr.handlerId = meshCb.handlerId;
  prvBrCb.ackTmr.msg.event = MESH_PRV_BR_MSG_TRAN_ACK_TMR_EXPIRED;
  prvBrCb.ackTmr.handlerId = meshCb.handlerId;
  prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.msg.event = MESH_PRV_BR_MSG_RETRY_TMR_EXPIRED;
  prvBrCb.pbAdvSessionInfo.ctlPduRetryTimer.handlerId = meshCb.handlerId;
  prvBrCb.pbAdvSessionInfo.linkTimer.msg.event = MESH_PRV_BR_MSG_LINK_TMR_EXPIRED;
  prvBrCb.pbAdvSessionInfo.linkTimer.handlerId = meshCb.handlerId;

  /* Initialize the provisioning beacon module */
  MeshPrvBeaconInit();

  /* Register bearer callback */
  MeshBrRegisterPb(meshBrEventNotificationCback, meshBrPrvPduRecvCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callbacks.
 *
 *  \param[in] prvPduRecvCback      Callback to be invoked when a Provisioning PDU is received.
 *  \param[in] prvEventNotifyCback  Event notification callback for the upper layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrRegisterCback(meshPrvBrPduRecvCback_t prvPduRecvCback,
                            meshPrvBrEventNotifyCback_t prvEventNotifyCback)
{
  /* Check for valid callback */
  if (prvPduRecvCback != NULL && prvEventNotifyCback != NULL)
  {
    /* Set event callbacks */
    prvBrCb.brPrvPduRecvCback = prvPduRecvCback;
    prvBrCb.brPrvEventCback = prvEventNotifyCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-ADV Server functionality.
 *
 *  \param[in] advIfId         Advertising bearer interface ID.
 *  \param[in] beaconInterval  Unprovisioned Device beacon interval in ms.
 *  \param[in] pUuid           16 bytes of UUID data.
 *  \param[in] oobInfoSrc      OOB information indicating the availability of OOB data.
 *  \param[in] pUriData        Uniform Resource Identifier (URI) data.
 *  \param[in] uriLen          URI data length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbAdvServer(uint8_t advIfId, uint32_t periodInMs, const uint8_t *pUuid,
                               uint16_t oobInfoSrc, const uint8_t *pUriData, uint8_t uriLen)

{
  WSF_ASSERT(advIfId != MESH_BR_INVALID_INTERFACE_ID);

  MESH_TRACE_INFO0("MESH PRV BR: PB-ADV Enabled for Provisioning Server");

  /* Set provisioner type */
  prvBrCb.prvType = MESH_PRV_SERVER;

  /* Reset Link Opened Flag and Device UUID */
  prvBrCb.pbAdvSessionInfo.linkOpened = FALSE;
  prvBrCb.pbAdvSessionInfo.pDeviceUuid = NULL;

  /* Set function pointer for Provisioning Bearer Server */
  prvBrCb.brPrvCtlPduCback = meshPrvBrSrProcessCtlPdu;

  /* Set Advertising interface ID. */
  prvBrCb.advIfId = MESH_BR_ADV_IF_TO_BR_IF(advIfId);

  /* Start sending unprovisioned beacons */
  MeshPrvBeaconStart(prvBrCb.advIfId, periodInMs, pUuid, oobInfoSrc, pUriData, uriLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-GATT Server functionality.
 *
 *  \param[in] connId  GATT bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbGattServer(uint8_t connId)

{
  WSF_ASSERT(connId != MESH_BR_INVALID_INTERFACE_ID);

  MESH_TRACE_INFO0("MESH PRV BR: PB-GATT Enabled for Provisioning Server");

  /* Stop beacons in case PB-ADV Server has been started */
  MeshPrvBeaconStop();

  /* Set provisioner type */
  prvBrCb.prvType = MESH_PRV_SERVER;

  /* Set function pointer for Provisioning Bearer Server */
  prvBrCb.brPrvCtlPduCback = meshPrvBrSrProcessCtlPdu;

  /* Set interface ID. */
  prvBrCb.gattIfId = MESH_BR_CONN_ID_TO_BR_IF(connId);
}

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-ADV Client functionality.
 *
 *  \param[in] advIfId  Advertising bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbAdvClient(uint8_t advIfId)
{
  MESH_TRACE_INFO0("MESH PRV BR: PB-ADV Enabled for Provisioning Client");

  /* Set provisioner type */
  prvBrCb.prvType = MESH_PRV_CLIENT;

  /* Reset Link Opened Flag and Device UUID */
  prvBrCb.pbAdvSessionInfo.linkOpened = FALSE;
  prvBrCb.pbAdvSessionInfo.pDeviceUuid = NULL;

  /* Set function pointer for Provisioning Bearer Client */
  prvBrCb.brPrvCtlPduCback = meshPrvBrClProcessCtlPdu;

  /* Set PB-ADV interface ID */
  prvBrCb.advIfId = advIfId;

  /* Register beacon bearer callbacks. */
  MeshBrRegisterPbBeacon(meshBrEventNotificationCback, meshBrBeaconPduRecvCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-GATT Client functionality.
 *
 *  \param[in] connId  GATT bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbGattClient(uint8_t connId)
{
  MESH_TRACE_INFO0("MESH PRV BR: PB-GATT Enabled for Provisioning Client");

  /* Set provisioner type */
  prvBrCb.prvType = MESH_PRV_CLIENT;

  /* Set function pointer for Provisioning Bearer Client */
  prvBrCb.brPrvCtlPduCback = meshPrvBrClProcessCtlPdu;

  /* Set PB-GATT interface ID */
  prvBrCb.gattIfId = MESH_BR_CONN_ID_TO_BR_IF(connId);
}

/*************************************************************************************************/
/*!
 *  \brief     Closes the provisioning link. Can be used by both Provisioning Client and Server.
 *
 *  \param[in] reason  Reason for closing the interface. See ::meshPrvBrReasonTypes.
 *
 *  \remarks   Calling this function will NOT generate the MESH_PRV_BR_LINK_CLOSED_BY_PEER event,
 *             because the upper layer is already aware of the link closure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrCloseLink(meshPrvBrReason_t reason)
{
  /* Call common API to close the link. */
  meshPrvBrCloseLink(reason, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief   Closes the provisioning link, but without sending Link Close on the ADV bearer.
 *           Can be used by both Provisioning Client and Server.
 *
 *  \remarks Calling this function will NOT generate the MESH_PRV_BR_LINK_CLOSED event,
 *           because the upper layer is already aware of the link closure.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshPrvBrCloseLinkSilent(void)
{
  /* Silent close of the link. Use a dummy reason. */
  meshPrvBrCloseLink(MESH_PRV_BR_REASON_FAIL, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Opens a PB-ADV link with a Provisioning Server on the already enabled advertising
 *             interface. Stores device UUID and generates link ID. The Link Open message is sent
 *             after receiving an unprovisioned beacon with a matching UUID. Used only by a
 *             Provisioning Client.
 *
 *  \param[in] pUuid  Device UUID value of the Node.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrOpenPbAdvLink(const uint8_t *pUuid)
{
  /* Save Device UUID set by the upper layer. */
  prvBrCb.pbAdvSessionInfo.pDeviceUuid = (uint8_t *)pUuid;

  /* Generate random Link Id */
  SecRand((uint8_t *)&prvBrCb.pbAdvSessionInfo.linkId, sizeof(uint32_t));

  /* Start Link timer on the Provisioning client. */
  WsfTimerStartMs(&(prvBrCb.pbAdvSessionInfo.linkTimer), MESH_PRV_LINK_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Provisioning PDU on the already enabled Provisioning Bearer interface.
 *
 *  \param[in] pPrvPdu  Pointer to the Provisioning PDU.
 *  \param[in] pduLen   Provisioning PDU length.
 *
 *  \return    TRUE if the PDU can be send, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshPrvBrSendProvisioningPdu(uint8_t* pPrvPdu, uint8_t pduLen)
{
  uint8_t randomTxDelayInMs = 0;
  uint8_t seg0PduLen;

  WSF_ASSERT(pPrvPdu != NULL);
  WSF_ASSERT(pduLen > 0);

  /* First check for PB-GATT interface */
  if (prvBrCb.gattIfId != MESH_BR_INVALID_INTERFACE_ID)
  {
    /* Send PDU to the GATT bearer. */
    if (!MeshBrSendPrvPdu(prvBrCb.gattIfId, pPrvPdu, pduLen))
    {
      /* Free buffer */
      WsfBufFree(pPrvPdu);
      return FALSE;
    }
    return TRUE;
  }

  /* Abort if we don't have a link open */
  if (!prvBrCb.pbAdvSessionInfo.linkOpened)
  {
    /* Free buffer */
    WsfBufFree(pPrvPdu);
    return FALSE;
  }

  /* Check for any ongoing Tx transaction */
  if (prvBrCb.pbAdvSessionInfo.pTxPrvPdu != NULL)
  {
    /* Any existing transaction should be canceled,
     * except when trying to send a Provisioning Failed PDU */
    if (pPrvPdu[MESH_PRV_PDU_OPCODE_INDEX] == MESH_PRV_PDU_FAILED)
    {
      /* Cannot send Provisioning Failed PDU at this moment */
      WsfBufFree(pPrvPdu);
      return FALSE;
    }
    else
    {
      /* New transaction needs to take priority - end the old one and continue */
      (void)meshPrvBrEndTxTransaction();
    }
  }

  /* Start Transaction */
  prvBrCb.pbAdvSessionInfo.txNextSegmentIndex = 0;
  prvBrCb.pbAdvSessionInfo.txTranTimeoutMs = MESH_PRV_TRAN_TIMEOUT_MS;

  /* Use buffer allocated by the Provisioning Protocol */
  prvBrCb.pbAdvSessionInfo.pTxPrvPdu = pPrvPdu;
  prvBrCb.pbAdvSessionInfo.txTotalLength = pduLen;

  /* Calculate SegN */
  seg0PduLen = (uint8_t)WSF_MIN(prvBrCb.pbAdvSessionInfo.txTotalLength, MESH_PRV_MAX_SEG0_PB_PDU_SIZE);
  prvBrCb.pbAdvSessionInfo.txSegN = (uint8_t)
        ((prvBrCb.pbAdvSessionInfo.txTotalLength - seg0PduLen) / MESH_PRV_MAX_GEN_PB_PDU_SIZE) +
        (((prvBrCb.pbAdvSessionInfo.txTotalLength - seg0PduLen) % MESH_PRV_MAX_GEN_PB_PDU_SIZE) > 0);

  /* Set Random Tx Delay. Read random number and normalize it. */
  SecRand(&randomTxDelayInMs, sizeof(randomTxDelayInMs));
  randomTxDelayInMs = MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS + (randomTxDelayInMs %
                  (MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS - MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS + 1));

  /* Start timer. */
  WsfTimerStartMs(&(prvBrCb.txTmr), randomTxDelayInMs);
  prvBrCb.pbAdvSessionInfo.txTranTimeoutMs -= randomTxDelayInMs;

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Closes the PB-ADV link with failure.
 *
 *  \return None.
 */
/*************************************************************************************************/
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
void MeshTestPrvBrTriggerLinkClose(void)
{
  meshPrvBrEventParams_t eventParams;

  if (prvBrCb.pbAdvSessionInfo.linkOpened)
  {
    /* Send Link Close with failure. */
    MeshPrvBrCloseLink(MESH_PRV_BR_REASON_FAIL);
  }

  /* Notify Upper Layer */
  eventParams.linkCloseReason = MESH_PRV_BR_REASON_FAIL;
  prvBrCb.brPrvEventCback(MESH_PRV_BR_LINK_CLOSED_BY_PEER, &eventParams);
}
#endif
