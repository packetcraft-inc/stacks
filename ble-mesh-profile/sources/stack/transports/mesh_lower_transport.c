/*************************************************************************************************/
/*!
 *  \file   mesh_lower_transport.c
 *
 *  \brief  Lower Transport implementation.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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
 *
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "mesh_defs.h"

#include "mesh_error_codes.h"

#include "mesh_api.h"
#include "mesh_seq_manager.h"
#include "mesh_network.h"
#include "mesh_lower_transport.h"
#include "mesh_sar_utils.h"
#include "mesh_sar_rx.h"
#include "mesh_sar_tx.h"
#include "mesh_replay_protection.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Lower Transport control block type definition */
typedef struct meshLtrCb_tag
{
  meshLtrAccRecvCback_t         ltrAccRecvCback;         /*!< LTR Access PDU received callback */
  meshLtrCtlRecvCback_t         ltrCtlRecvCback;         /*!< LTR Control PDU received callback */
  meshLtrEventNotifyCback_t     ltrEventCback;           /*!< LTR Event Notify callback */
  meshLtrFriendQueueAddCback_t  ltrFriendQueueAddCback;  /*!< LTR Friend Queue add callback */
} meshLtrCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Lower Transport control block */
static meshLtrCb_t ltrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Mesh Lower Transport Access PDU received empty callback.
 *
 *  \param[in] pLtrAccPduInfo  Pointer to the structure holding the received Upper Transport Access
 *                             PDU and other fields. See ::meshLtrAccPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLtrEmptyAccRecvCback(meshLtrAccPduInfo_t *pLtrAccPduInfo)
{
  (void)pLtrAccPduInfo;
  MESH_TRACE_WARN0("MESH LTR: Access PDU Receive callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Lower Transport Control PDU received empty callback.
 *
 *  \param[in] pLtrCtlPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Control PDU and other fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLtrEmptyCtlRecvCback(meshLtrCtlPduInfo_t *pLtrAccPduInfo)
{
  (void)pLtrAccPduInfo;
  MESH_TRACE_WARN0("MESH LTR: Control PDU Receive callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Lower Transport Friend Queue PDU add empty callback.
 *
 *  \param[in] pPduInfo  Pointer to PDU and information.
 *  \param[in] pduType   Type of the generic pPduInfo parameter. See ::meshFriendQueuePduTypes
 *
 *  \return    TRUE if the PDU is accepted in a Friend Queue, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshLtrEmptyFriendQueueAddCback(const void *pPduInfo, meshFriendQueuePduType_t pduType)
{
  (void)pPduInfo;
  (void)pduType;

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Lower Transport empty event notification callback.
 *
 *  \param[in] event  Reason the callback is being invoked. See ::meshLtrEvent_t
 *  \param[in] seqNo  Sequence number used to identify the Tx transaction.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLtrEmptyEventNotifyCback(meshLtrEvent_t event, meshSeqNumber_t seqNo)
{
  (void)event;
  (void)seqNo;
  MESH_TRACE_WARN0("MESH LTR: Notification callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network layer PDU received callback function pointer type.
 *
 *  \param[in] pNwkPduInfo  Pointer to the structure holding the received transport PDU and other
 *                          fields. See ::meshNwkPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkRecvCback(meshNwkPduRxInfo_t *pNwkPduRxInfo)
{
  uint8_t *pLtrPduInfo = NULL;
  meshSarTxBlockAck_t blockAck;
  uint16_t seqZero;
  bool_t oboFlag;
  uint8_t utrPduLen = 0;

  /* Check if the pointer is not NULL. */
  if (pNwkPduRxInfo == NULL)
  {
    WSF_ASSERT(pNwkPduRxInfo != NULL);

    return;
  }

  /* Check if the Upper Transport PDU pointer is NULL. */
  if (pNwkPduRxInfo->pLtrPdu == NULL)
  {
    WSF_ASSERT(pNwkPduRxInfo->pLtrPdu != NULL);

    return;
  }

  /* Check if the Upper Transport PDU length is 0. */
  if (pNwkPduRxInfo->pduLen == 0)
  {
    WSF_ASSERT(pNwkPduRxInfo->pduLen != 0);

    return;
  }

  if (pNwkPduRxInfo->ctl == 0)
  {
    /* Check if the message is Segmented or Unsegmented. */
    if (MESH_UTILS_BITMASK_CHK(pNwkPduRxInfo->pLtrPdu[0], MESH_SEG_MASK))
    {
      if (pNwkPduRxInfo->pduLen > MESH_LTR_SEG_HDR_LEN + MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN)
      {
        /* Maximum expected size exceeded. */
        WSF_ASSERT(pNwkPduRxInfo->pduLen <= MESH_LTR_SEG_HDR_LEN + MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN);

        return;
      }

      /* Segmented message. */
      MeshSarRxProcessSegment(pNwkPduRxInfo);
    }
    else
    {
      if (pNwkPduRxInfo->pduLen > MESH_LTR_UNSEG_HDR_LEN + MESH_LTR_MAX_UNSEG_UTR_ACC_PDU_LEN)
      {
        /* Maximum expected size exceeded. */
        WSF_ASSERT(pNwkPduRxInfo->pduLen <= MESH_LTR_UNSEG_HDR_LEN + MESH_LTR_MAX_UNSEG_UTR_ACC_PDU_LEN);

        return;
      }

      /* Check if PDU should go in Friend Queue. */
      if(ltrCb.ltrFriendQueueAddCback(pNwkPduRxInfo, MESH_FRIEND_QUEUE_NWK_PDU))
      {
        if(MESH_IS_ADDR_UNICAST(pNwkPduRxInfo->dst))
        {
          /* PDU consumed only by the Friend Queue, not needed anymore. */
          return;
        }
      }

      /* For Unsegmented PDU's the LTR header size is 1 byte. */
      utrPduLen = (pNwkPduRxInfo->pduLen) - 1;

      /* Allocate buffer for Lower Transport PDU Info and Upper Transport PDU. */
      pLtrPduInfo = WsfBufAlloc(sizeof(meshLtrAccPduInfo_t) + utrPduLen);

      if (pLtrPduInfo != NULL)
      {
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->src = pNwkPduRxInfo->src;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->dst = pNwkPduRxInfo->dst;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->netKeyIndex = pNwkPduRxInfo->netKeyIndex;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->friendLpnAddr = pNwkPduRxInfo->friendLpnAddr;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->ttl = pNwkPduRxInfo->ttl;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->seqNo = pNwkPduRxInfo->seqNo;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->gtSeqNo = pNwkPduRxInfo->seqNo;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->ivIndex = pNwkPduRxInfo->ivIndex;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->aid =
          MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[0], MESH_AID_SHIFT, MESH_AID_SIZE);
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->akf =
          MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[0], MESH_AKF_SHIFT, MESH_AKF_SIZE);

        /* Copy the UTR PDU. */
        memcpy((uint8_t *)pLtrPduInfo + sizeof(meshLtrAccPduInfo_t),
               &(pNwkPduRxInfo->pLtrPdu[1]),
               utrPduLen);

        /* Point to the start of the UTR PDU. */
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->pUtrAccPdu = (uint8_t*)((uint8_t *)pLtrPduInfo +
                                                                      sizeof(meshLtrAccPduInfo_t));
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->pduLen = utrPduLen;
        ((meshLtrAccPduInfo_t *)pLtrPduInfo)->szMic = 0;

        /* Unsegmented message. */
        ltrCb.ltrAccRecvCback((meshLtrAccPduInfo_t *)pLtrPduInfo);
      }
    }
  }
  else
  {
    /* Check if the message is Segmented or Unsegmented. */
    if (MESH_UTILS_BITMASK_CHK(pNwkPduRxInfo->pLtrPdu[0], MESH_SEG_MASK))
    {
      if (pNwkPduRxInfo->pduLen > MESH_LTR_SEG_HDR_LEN + MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN)
      {
        /* Maximum expected size exceeded. */
        WSF_ASSERT(pNwkPduRxInfo->pduLen <= MESH_LTR_SEG_HDR_LEN + MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN);

        return;
      }

      /* Segmented message. */
      MeshSarRxProcessSegment(pNwkPduRxInfo);
    }
    else
    {
      if (pNwkPduRxInfo->pduLen > MESH_LTR_UNSEG_HDR_LEN + MESH_LTR_MAX_UNSEG_UTR_CTL_PDU_LEN)
      {
        /* Maximum expected size exceeded. */
        WSF_ASSERT(pNwkPduRxInfo->pduLen <= MESH_LTR_UNSEG_HDR_LEN + MESH_LTR_MAX_UNSEG_UTR_CTL_PDU_LEN);

        return;
      }

      /* Check if the message is Segment Acknowledgement. */
      if (MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[0],
                            MESH_CTL_OPCODE_SHIFT,
                            MESH_CTL_OPCODE_SIZE) == MESH_SEG_ACK_OPCODE)
      {
        /* Check if the length matches the ACK Opcode. */
        if (pNwkPduRxInfo->pduLen != MESH_SEG_ACK_LENGTH)
        {
          /* Segmented ACK size not met. */
          WSF_ASSERT(pNwkPduRxInfo->pduLen == MESH_SEG_ACK_LENGTH);

          return;
        }

        /* Check if PDU should go in Friend Queue. */
        if(ltrCb.ltrFriendQueueAddCback(pNwkPduRxInfo, MESH_FRIEND_QUEUE_NWK_PDU))
        {
          if(MESH_IS_ADDR_UNICAST(pNwkPduRxInfo->dst))
          {
            /* PDU consumed only by the Friend Queue, not needed anymore. */
            return;
          }
        }

        /* Extract OBO field. */
        oboFlag = MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[1], MESH_OBO_SHIFT, MESH_OBO_SIZE);
        /* Extract SeqZero field. */
        seqZero = (((uint16_t)(MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[1],
                                                  MESH_SEQ_ZERO_H_SHIFT,
                                                  MESH_SEQ_ZERO_H_SIZE)) <<
                  MESH_SEQ_ZERO_L_SIZE) |
                  (uint8_t)(MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[2],
                                              MESH_SEQ_ZERO_L_SHIFT,
                                              MESH_SEQ_ZERO_L_SIZE)));
        /* Extract BlockAck field. */
        blockAck = (((meshSarTxBlockAck_t)pNwkPduRxInfo->pLtrPdu[3] << 24) |
                    ((meshSarTxBlockAck_t)pNwkPduRxInfo->pLtrPdu[4] << 16) |
                    ((meshSarTxBlockAck_t)pNwkPduRxInfo->pLtrPdu[5] << 8) |
                    ((meshSarTxBlockAck_t)pNwkPduRxInfo->pLtrPdu[6]));

        /* Update the Replay List with the SeqNo in the packet */
        MeshRpUpdateList(pNwkPduRxInfo->src, pNwkPduRxInfo->seqNo, pNwkPduRxInfo->ivIndex);

        /* Signal SAR-TX that a Segment ACK was received. */
        MeshSarTxProcessBlockAck(pNwkPduRxInfo->src, seqZero, oboFlag, blockAck);
      }
      else
      {
        /* Check if PDU should go in Friend Queue. */
        if(ltrCb.ltrFriendQueueAddCback(pNwkPduRxInfo, MESH_FRIEND_QUEUE_NWK_PDU))
        {
          if(MESH_IS_ADDR_UNICAST(pNwkPduRxInfo->dst))
          {
            /* PDU consumed only by the Friend Queue, not needed anymore. */
            return;
          }
        }

        /* For Unsegmented PDU's the LTR header size is 1 byte. */
        utrPduLen = (pNwkPduRxInfo->pduLen) - 1;

        /* Allocate queue element for Lower Transport PDU Info and Upper Transport PDU. */
        pLtrPduInfo = WsfBufAlloc(sizeof(meshLtrCtlPduInfo_t) + utrPduLen);

        if (pLtrPduInfo != NULL)
        {
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->src = pNwkPduRxInfo->src;
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->dst = pNwkPduRxInfo->dst;
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->netKeyIndex = pNwkPduRxInfo->netKeyIndex;
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->friendLpnAddr = pNwkPduRxInfo->friendLpnAddr;
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->ttl = pNwkPduRxInfo->ttl;
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->seqNo = pNwkPduRxInfo->seqNo;
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->opcode =
                                           MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[0],
                                                             MESH_CTL_OPCODE_SHIFT,
                                                             MESH_CTL_OPCODE_SIZE);

          /* Copy the UTR PDU. */
          memcpy((uint8_t *)pLtrPduInfo + sizeof(meshLtrCtlPduInfo_t),
                 &(pNwkPduRxInfo->pLtrPdu[1]),
                 utrPduLen);

          /* Point to the start of the UTR PDU. */
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->pUtrCtlPdu = (uint8_t*)((uint8_t *)pLtrPduInfo +
                                                                       sizeof(meshLtrCtlPduInfo_t));
          ((meshLtrCtlPduInfo_t *)pLtrPduInfo)->pduLen = utrPduLen;

          /* Update the Replay List with the SeqNo in the packet */
          MeshRpUpdateList(pNwkPduRxInfo->src, pNwkPduRxInfo->seqNo, pNwkPduRxInfo->ivIndex);

          /* Unsegmented message. */
          ltrCb.ltrCtlRecvCback((meshLtrCtlPduInfo_t *)pLtrPduInfo);
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network layer event notification callback function pointer type.
 *
 *  \param[in] event        Reason the callback is being invoked. See ::meshNwkEvent_t
 *  \param[in] pEventParam  Pointer to the event parameter passed to the function.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkEventNotifyCback(meshNwkEvent_t event, void *pEventParam)
{
  (void)event;
  (void)pEventParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Notifies the upper layer the outcome of a SAR Tx transaction.
 *
 *  \param[in] eventStatus  Event status value. See ::meshSarTxEventStatus
 *  \param[in] seqNo        Sequence number used to identify the SAR Tx transaction.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxNotifyCback(meshSarTxEventStatus_t eventStatus, meshAddress_t dst)
{
  (void)dst;

  if (eventStatus == MESH_SAR_TX_EVENT_SUCCESS)
  {
    ltrCb.ltrEventCback(MESH_LTR_SEND_SUCCESS, 0);
  }
  else if (eventStatus == MESH_SAR_TX_EVENT_TIMEOUT)
  {
    ltrCb.ltrEventCback(MESH_LTR_SEND_SAR_TX_TIMEOUT, 0);
  }
  else if (eventStatus == MESH_SAR_TX_EVENT_REJECTED)
  {
    ltrCb.ltrEventCback(MESH_LTR_SEND_SAR_TX_REJECTED, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR Rx reassembly complete callback.
 *
 *  \param[in] pduType       Type and format of the reassembled PDU.
 *  \param[in] pReasPduInfo  Pointer to reassembled PDU information.
 *                           \see meshSarRxReassembledPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarRxPduRecvCback(meshSarRxPduType_t pduType,
                                  const meshSarRxReassembledPduInfo_t *pReasPduInfo)
{
  /* Invalid SAR RX PDU Info. */
  WSF_ASSERT(pReasPduInfo != NULL);

  if (pduType == MESH_SAR_RX_TYPE_ACCESS)
  {
    /* Reassembled Access PDU and info received, pass to the Upper Transport Layer. */
    ltrCb.ltrAccRecvCback((meshLtrAccPduInfo_t *)pReasPduInfo);
  }
  else if (pduType == MESH_SAR_RX_TYPE_CTL)
  {
    ltrCb.ltrCtlRecvCback((meshLtrCtlPduInfo_t *)pReasPduInfo);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Mesh Upper Transport Access PDU.
 *
 *  \param[in] pLtrAccPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Access PDU and other fields. See ::meshLtrAccPduInfo_t
 *
 *  \return    Success or error reason. \see meshLtrRetVal_t
 */
/*************************************************************************************************/
static meshLtrRetVal_t meshLtrSendUtrAccPduInternal(meshLtrAccPduInfo_t *pLtrAccPduInfo)
{
  meshNwkPduTxInfo_t nwkPduInfo;
  meshLtrRetVal_t retVal = MESH_SUCCESS;

  /* Lower Transport Access PDU header for unsegmented PDU's. */
  uint8_t ltrHdr = 0;

  /* Check if PDU should go in Friend Queue. */
  if(ltrCb.ltrFriendQueueAddCback(pLtrAccPduInfo, MESH_FRIEND_QUEUE_LTR_ACC_PDU))
  {
    if(MESH_IS_ADDR_UNICAST(pLtrAccPduInfo->dst))
    {
      /* Free the Upper Access PDU buffer. */
      WsfBufFree(pLtrAccPduInfo);
      /* PDU consumed only by the Friend Queue, not needed anymore. */
      return MESH_SUCCESS;
    }
  }

  /* Check if Segmented or Acknowledged message. */
  if ((pLtrAccPduInfo->pduLen > MESH_LTR_MAX_UNSEG_UTR_ACC_PDU_LEN) ||
      (pLtrAccPduInfo->ackRequired == TRUE))
  {
    MeshSarTxStartSegAccTransaction(pLtrAccPduInfo);
  }
  else
  {
    /* Set AKF field. */
    MESH_UTILS_BF_SET(ltrHdr, pLtrAccPduInfo->akf, MESH_AKF_SHIFT, MESH_AKF_SIZE);
    /* Set AID field. */
    MESH_UTILS_BF_SET(ltrHdr, pLtrAccPduInfo->aid, MESH_AID_SHIFT, MESH_AID_SIZE);
    /* Populate the message for the Network Layer. */
    nwkPduInfo.src = pLtrAccPduInfo->src;
    nwkPduInfo.dst = pLtrAccPduInfo->dst;
    nwkPduInfo.netKeyIndex = pLtrAccPduInfo->netKeyIndex;
    nwkPduInfo.ctl = 0;
    nwkPduInfo.ttl = pLtrAccPduInfo->ttl;
    nwkPduInfo.seqNo = pLtrAccPduInfo->seqNo;
    nwkPduInfo.pLtrHdr = &ltrHdr;
    nwkPduInfo.ltrHdrLen = 1;
    nwkPduInfo.pUtrPdu = pLtrAccPduInfo->pUtrAccPdu;
    nwkPduInfo.utrPduLen = (uint8_t)pLtrAccPduInfo->pduLen;
    nwkPduInfo.prioritySend = FALSE;
    nwkPduInfo.friendLpnAddr = pLtrAccPduInfo->friendLpnAddr;
    nwkPduInfo.ifPassthr = FALSE;

    /* Send the message to the network layer. */
    retVal = MeshNwkSendLtrPdu(&nwkPduInfo);

    /* Free the Upper Access PDU buffer. */
    WsfBufFree(pLtrAccPduInfo);
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Mesh Upper Transport Control PDU.
 *
 *  \param[in] pLtrCtlPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Control PDU and other fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    Success or error reason. \see meshLtrRetVal_t
 */
/*************************************************************************************************/
static meshLtrRetVal_t meshLtrSendUtrCtlPduInternal(meshLtrCtlPduInfo_t *pLtrCtlPduInfo)
{
  meshNwkPduTxInfo_t nwkPduInfo;
  meshLtrRetVal_t retVal = MESH_SUCCESS;
  uint8_t ltrHdr = 0;  /* Lower Transport Control PDU header for unsegmented PDU's. */

  /* Check if PDU should go in Friend Queue. */
  if(ltrCb.ltrFriendQueueAddCback(pLtrCtlPduInfo, MESH_FRIEND_QUEUE_LTR_CTL_PDU))
  {
    if(MESH_IS_ADDR_UNICAST(pLtrCtlPduInfo->dst))
    {
      /* Free the Upper Control PDU buffer. */
      WsfBufFree(pLtrCtlPduInfo);
      /* PDU consumed only by the Friend Queue, not needed anymore. */
      return MESH_SUCCESS;
    }
  }
  /* Check if Segmented or Acknowledged message. */
  if ((pLtrCtlPduInfo->pduLen > MESH_LTR_MAX_UNSEG_UTR_CTL_PDU_LEN) ||
      (pLtrCtlPduInfo->ackRequired == TRUE))
  {
    MeshSarTxStartSegCtlTransaction(pLtrCtlPduInfo);
  }
  else
  {
    /* Set OPCODE field. */
    MESH_UTILS_BF_SET(ltrHdr, pLtrCtlPduInfo->opcode, MESH_CTL_OPCODE_SHIFT, MESH_CTL_OPCODE_SIZE);
    /* Populate the message for the Network Layer. */
    nwkPduInfo.src = pLtrCtlPduInfo->src;
    nwkPduInfo.dst = pLtrCtlPduInfo->dst;
    nwkPduInfo.netKeyIndex = pLtrCtlPduInfo->netKeyIndex;
    nwkPduInfo.ctl = 1;
    nwkPduInfo.ttl = pLtrCtlPduInfo->ttl;
    nwkPduInfo.seqNo = pLtrCtlPduInfo->seqNo;
    nwkPduInfo.pLtrHdr = &ltrHdr;
    nwkPduInfo.ltrHdrLen = 1;
    nwkPduInfo.pUtrPdu = pLtrCtlPduInfo->pUtrCtlPdu;
    nwkPduInfo.utrPduLen = (uint8_t)pLtrCtlPduInfo->pduLen;
    nwkPduInfo.prioritySend  = pLtrCtlPduInfo->prioritySend;
    nwkPduInfo.friendLpnAddr = pLtrCtlPduInfo->friendLpnAddr;
    nwkPduInfo.ifPassthr = pLtrCtlPduInfo->ifPassthr;

    /* Send the message to the network layer. */
    retVal = MeshNwkSendLtrPdu(&nwkPduInfo);

    /* Free the Upper Control PDU buffer. */
    WsfBufFree(pLtrCtlPduInfo);
  }

  return retVal;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Lower Transport layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshLtrInit(void)
{
  MESH_TRACE_INFO0("MESH LTR: init");

  /* Store empty callbacks into local structure. */
  ltrCb.ltrAccRecvCback = meshLtrEmptyAccRecvCback;
  ltrCb.ltrCtlRecvCback = meshLtrEmptyCtlRecvCback;
  ltrCb.ltrEventCback = meshLtrEmptyEventNotifyCback;
  ltrCb.ltrFriendQueueAddCback = meshLtrEmptyFriendQueueAddCback;

  /* Register Network layer callback for receive and notify. */
  MeshNwkRegister(meshNwkRecvCback, meshNwkEventNotifyCback);

  /* Initialize the SAR-TX layer and register the notify callback. */
  MeshSarTxInit();
  MeshSarTxRegister(meshSarTxNotifyCback);

  /* Initialize the SAR-RX layer and register the reassemble PDU received callback. */
  MeshSarRxInit();
  MeshSarRxRegister(meshSarRxPduRecvCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callbacks used by the Lower Transport.
 *
 *  \param[in] accRecvCback  Callback to be invoked when an Upper Transport Access PDU is received.
 *  \param[in] ctlRecvCback  Callback to be invoked when an Upper Transport Control PDU is received.
 *  \param[in] eventCback    Event notification callback for the Upper Transport layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLtrRegister(meshLtrAccRecvCback_t accRecvCback, meshLtrCtlRecvCback_t ctlRecvCback,
                     meshLtrEventNotifyCback_t eventCback)
{
  /* Validate function parameters. */
  if ((accRecvCback != NULL) && (ctlRecvCback == NULL) && (eventCback == NULL))
  {
    MESH_TRACE_ERR0("MESH LTR: Invalid callbacks registered!");
    return;
  }

  /* Store callbacks into local structure. */
  ltrCb.ltrAccRecvCback = accRecvCback;
  ltrCb.ltrCtlRecvCback = ctlRecvCback;
  ltrCb.ltrEventCback = eventCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Friend Queue add callback used by the Lower Transport.
 *
 *  \param[in] friendQueueAddCback  Callback to be invoked when sending or receiving a Lower
 *                                  Transport PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLtrRegisterFriend(meshLtrFriendQueueAddCback_t friendQueueAddCback)
{
  if(friendQueueAddCback != NULL)
  {
    /* Store new callback. */
    ltrCb.ltrFriendQueueAddCback = friendQueueAddCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Upper Transport Access PDU to Lower Transport Layer.
 *
 *  \param[in] pLtrAccPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Access PDU and other fields. See ::meshLtrAccPduInfo_t
 *
 *  \return    Success or error reason. \see meshLtrRetVal_t
 */
/*************************************************************************************************/
meshLtrRetVal_t MeshLtrSendUtrAccPdu(meshLtrAccPduInfo_t *pLtrAccPduInfo)
{
  /* Check if the pLtrAccPduInfo is NULL. */
  if (pLtrAccPduInfo == NULL)
  {
    return MESH_LTR_INVALID_PARAMS;
  }

  /* Check if the pointer to the PDU is valid. */
  if (pLtrAccPduInfo->pUtrAccPdu == NULL)
  {
    /* Free the allocated queue element passed as parameter. */
    WsfBufFree(pLtrAccPduInfo);

    return MESH_LTR_INVALID_PARAMS;
  }

  /* Check if the size of the PDU is in range. */
  if ((pLtrAccPduInfo->pduLen > MESH_LTR_MAX_ACC_PDU_LEN) ||
      (pLtrAccPduInfo->pduLen < MESH_LTR_MIN_ACC_PDU_LEN))
  {
    /* Free the allocated queue element passed as parameter. */
    WsfBufFree(pLtrAccPduInfo);

    return MESH_LTR_INVALID_PARAMS;
  }

  /* Call internal function to handle the request. */
  return meshLtrSendUtrAccPduInternal(pLtrAccPduInfo);
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Upper Transport Control PDU to Lower Transport Layer.
 *
 *  \param[in] pLtrCtlPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Control PDU and other fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    Success or error reason. \see meshLtrRetVal_t
 */
/*************************************************************************************************/
meshLtrRetVal_t MeshLtrSendUtrCtlPdu(meshLtrCtlPduInfo_t *pLtrCtlPduInfo)
{

  /* Check if the pLtrCtlPduInfo is NULL. */
  if (pLtrCtlPduInfo == NULL)
  {
    return MESH_LTR_INVALID_PARAMS;
  }

  /* Check if the pointer to the PDU is valid. */
  if (pLtrCtlPduInfo->pUtrCtlPdu == NULL)
  {
    /* Free on error. */
    WsfBufFree(pLtrCtlPduInfo);

    return MESH_LTR_INVALID_PARAMS;
  }

  /* Check if the size of the PDU is in range. */
  if (pLtrCtlPduInfo->pduLen > MESH_LTR_MAX_CTL_PDU_LEN)
  {
    /* Free on error. */
    WsfBufFree(pLtrCtlPduInfo);

    return MESH_LTR_INVALID_PARAMS;
  }

  /* Call internal function to handle the request. */
  return meshLtrSendUtrCtlPduInternal(pLtrCtlPduInfo);
}
