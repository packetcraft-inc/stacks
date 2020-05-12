/*************************************************************************************************/
/*!
 *  \file   mesh_upper_transport.c
 *
 *  \brief  Upper Transport implementation.
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
 *
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_queue.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_security_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_utils.h"

#include "mesh_api.h"
#include "mesh_seq_manager.h"
#include "mesh_lower_transport.h"
#include "mesh_security.h"
#include "mesh_security_toolbox.h"
#include "mesh_network.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_upper_transport_heartbeat.h"
#include "mesh_replay_protection.h"

#include "mesh_upper_transport.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Transport control block for the Upper Transport Access PDU, containing all the information
 *  necessaru for delivery to Lower Transport
 */
typedef struct meshUtrAccPduTxTcb_tag
{
  meshLtrAccPduInfo_t ltrAccPduInfo; /*!< Lower Transport access PDU information. Should always
                                      *   be the first member of this struct so that LTR can
                                      *   easily free the whole structure
                                      */
  uint8_t             *pLabelUuid;   /*!< Label UUID address in case destination is virtual */
  uint16_t            appKeyIndex;   /*!< Application Key Index used to encrypt the Upper Transport
                                      *   PDU
                                      */
} meshUtrAccPduTxTcb_t;

/*! Upper Transport control block type definition */
typedef struct meshUtrCb_tag
{
  meshUtrAccRecvCback_t           accRecvCback;            /*!< UTR Access PDU received callback */
  meshUtrFriendshipCtlRecvCback_t friendshipCtlRecvCback;  /*!< UTR LPN Control PDU received
                                                            *   callback
                                                            */
  meshUtrEventNotifyCback_t       eventCback;              /*!< UTR Event Notify callback */
  wsfQueue_t                      utrAccTxQueue;           /*!< UTR Access PDU Queue for
                                                            *   transmitting
                                                            */
  wsfQueue_t                      utrAccRxQueue;           /*!< UTR Access PDU Queue for receiving
                                                            */
  bool_t                          utrEncryptInProgress;    /*!< UTR encryption in progress flag */
  bool_t                          utrDecryptInProgress;    /*!< UTR decryption in progress flag */
} meshUtrCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Upper Transport control block */
static meshUtrCb_t utrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Mesh Upper Transport Access PDU received empty callback.
 *
 *  \param[in] pLtrAccPduInfo  Pointer to the structure holding the received Upper Transport Access
 *                             PDU and other fields. See ::meshLtrAccPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrEmptyAccRecvCback(const meshUtrAccPduRxInfo_t *pAccPduInfo)
{
  (void)pAccPduInfo;
  MESH_TRACE_WARN0("MESH UTR: Access PDU Receive callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Friendship Control PDU received callback function pointer type.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrEmptyFriendshipCtlRecvCback(const meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  (void)pCtlPduInfo;
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Upper Transport empty event notification callback.
 *
 *  \param[in] event  Reason the callback is being invoked. See ::meshLtrEvent_t
 *  \param[in] seqNo  Sequence number used to identify the Tx transaction.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrEmptyEventNotifyCback(meshUtrEvent_t event, void *pEventParam)
{
  (void)event;
  (void)pEventParam;
  MESH_TRACE_WARN0("MESH UTR: Notification callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Triggers an Upper Transpor decrypt request.
 *
 *  \param[in] pAccPduInfo  Pointer to structure containing RX information.
 *  \param[in] secCback     Decrypt callback to be used with the security module.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshUtrRetVal_t meshUtrDecryptRequest(meshLtrAccPduInfo_t *pAccPduInfo,
                                             meshSecUtrDecryptCback_t secCback)
{
  meshSecUtrDecryptParams_t utrDecryptParams;
  uint8_t transMicSize;

  /* Get TransMic size. */
  transMicSize = MESH_SZMIC_TO_TRANSMIC(pAccPduInfo->szMic);

  utrDecryptParams.pEncAppPayload = pAccPduInfo->pUtrAccPdu;
  utrDecryptParams.pAppPayload    = pAccPduInfo->pUtrAccPdu;

  utrDecryptParams.pTransMic = (uint8_t*)(pAccPduInfo->pUtrAccPdu +
                                          pAccPduInfo->pduLen -
                                          transMicSize);

  utrDecryptParams.seqNo          = pAccPduInfo->seqNo;
  utrDecryptParams.recvIvIndex    = pAccPduInfo->ivIndex;
  utrDecryptParams.srcAddr        = pAccPduInfo->src;
  utrDecryptParams.dstAddr        = pAccPduInfo->dst;
  utrDecryptParams.netKeyIndex    = pAccPduInfo->netKeyIndex;
  utrDecryptParams.appPayloadSize = pAccPduInfo->pduLen - transMicSize;
  utrDecryptParams.transMicSize   = transMicSize;

  if (pAccPduInfo->akf == 0x00)
  {
    utrDecryptParams.aid          = MESH_SEC_DEVICE_KEY_AID;
  }
  else
  {
    utrDecryptParams.aid          = pAccPduInfo->aid;
  }

  return (meshUtrRetVal_t)MeshSecUtrDecrypt(&utrDecryptParams, secCback, pAccPduInfo);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Upper transport decryption complete callback.
 *
 *  \param[in] isDecryptSuccess  TRUE if decryption and authentication finished successfully.
 *  \param[in] pAppPayload       Pointer to decrypted application payload, provided in the request.
 *  \param[in] pLabelUuid        Pointer to label UUID for virtual destination addresses.
 *  \param[in] appPayloadSize    Size of the decrypted application payload.
 *  \param[in] appKeyIndex       Global application index that matched the AID in the request.
 *  \param[in] netKeyIndex       Global network key index associated to the application key index.
 *  \param[in] pParam            Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrDecryptCback(bool_t isDecryptSuccess, uint8_t *pAppPayload, uint8_t *pLabelUuid,
                                uint16_t appPayloadSize, uint16_t appKeyIndex, uint16_t netKeyIndex,
                                void *pParam)
{
  meshLtrAccPduInfo_t *pLtrAccPduInfo = NULL;
  meshUtrAccPduRxInfo_t utrAccPduInfo;

  if (pParam == NULL)
  {
    /* Critical error. Memory will remain allocated. */
    WSF_ASSERT(pParam != NULL);

    return;
  }

  if (!isDecryptSuccess)
  {
    /* Free the allocated queue element passed as pParam. */
    WsfBufFree(pParam);
  }
  else
  {
    utrAccPduInfo.src = ((meshLtrAccPduInfo_t *)pParam)->src;
    utrAccPduInfo.dst = ((meshLtrAccPduInfo_t *)pParam)->dst;
    utrAccPduInfo.pDstLabelUuid = pLabelUuid;
    utrAccPduInfo.appKeyIndex = appKeyIndex;
    utrAccPduInfo.netKeyIndex = netKeyIndex;
    utrAccPduInfo.ttl = ((meshLtrAccPduInfo_t *)pParam)->ttl;

    if (((meshLtrAccPduInfo_t *)pParam)->akf == 0)
    {
      utrAccPduInfo.devKeyUse = 1;
    }
    else
    {
      utrAccPduInfo.devKeyUse = 0;
    }

    utrAccPduInfo.pAccPdu = pAppPayload;
    utrAccPduInfo.pduLen = appPayloadSize;

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
    meshTestUtrAccPduRcvdInd_t utrAccPduRcvdInd;

  if (meshTestCb.listenMask & MESH_TEST_UTR_LISTEN)
  {
    utrAccPduRcvdInd.hdr.event = MESH_TEST_EVENT;
    utrAccPduRcvdInd.hdr.param = MESH_TEST_UTR_ACC_PDU_RCVD_IND;
    utrAccPduRcvdInd.hdr.status = MESH_SUCCESS;

    utrAccPduRcvdInd.src            = utrAccPduInfo.src;
    utrAccPduRcvdInd.dst            = utrAccPduInfo.dst;
    utrAccPduRcvdInd.pDstLabelUuid  = utrAccPduInfo.pDstLabelUuid;
    utrAccPduRcvdInd.appKeyIndex    = utrAccPduInfo.appKeyIndex;
    utrAccPduRcvdInd.netKeyIndex    = utrAccPduInfo.netKeyIndex;
    utrAccPduRcvdInd.ttl            = utrAccPduInfo.ttl;
    utrAccPduRcvdInd.devKeyUse      = utrAccPduInfo.devKeyUse;
    utrAccPduRcvdInd.pAccPdu        = utrAccPduInfo.pAccPdu;
    utrAccPduRcvdInd.pduLen         = utrAccPduInfo.pduLen;

    meshTestCb.testCback((meshTestEvt_t *)&utrAccPduRcvdInd);
  }
#endif

    /* Notify the upper layer that a packet has been received. */
    utrCb.accRecvCback(&utrAccPduInfo);

    /* Free the allocated queue element passed as pParam. */
    WsfBufFree(pParam);
  }

  /* Clear decrypt in progress flag. */
  utrCb.utrDecryptInProgress = FALSE;

  /* Run maintenance on the RX queue. */
  pLtrAccPduInfo = (meshLtrAccPduInfo_t *)WsfQueueDeq(&(utrCb.utrAccRxQueue));

  while (pLtrAccPduInfo != NULL)
  {
    /* Set decrypt in progress flag. */
    utrCb.utrDecryptInProgress = TRUE;

    /* Request decryption. */
    if (meshUtrDecryptRequest(pLtrAccPduInfo, meshUtrDecryptCback) != MESH_SUCCESS)
    {
      /* Free queue element if decryption request fails. */
      WsfBufFree(pLtrAccPduInfo);

      /* Clear decrypt in progress flag. */
      utrCb.utrDecryptInProgress = FALSE;
    }
    else
    {
      /* Break loop because request will be processed. */
      break;
    }

    /* Get next PDU. */
    pLtrAccPduInfo = (meshLtrAccPduInfo_t *)WsfQueueDeq(&(utrCb.utrAccRxQueue));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Upper Transport Access PDU receive function.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Access PDU and other
 *                          fields. See ::meshUtrAccPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrRecvAccPdu(meshLtrAccPduInfo_t *pAccPduInfo)
{
  /* Invalid pointers. Critical Error. */
  if (pAccPduInfo == NULL)
  {
    WSF_ASSERT(pAccPduInfo != NULL);

    return;
  }

  if (pAccPduInfo->pUtrAccPdu == NULL)
  {
    WSF_ASSERT(pAccPduInfo->pUtrAccPdu != NULL);

    return;
  }

  /* Update the Replay List with the greatest SeqNo in the assembled packet. */
  MeshRpUpdateList(pAccPduInfo->src, pAccPduInfo->gtSeqNo, pAccPduInfo->ivIndex);

  /* Check if decryption is in progress. */
  if (utrCb.utrDecryptInProgress)
  {
    /* Enqueue the PDU in the RX queue. */
    WsfQueueEnq(&(utrCb.utrAccRxQueue), (void *)pAccPduInfo);
  }
  else
  {
    /* Mark decrypt in progress. */
    utrCb.utrDecryptInProgress = TRUE;

    /* Request decryption of the PDU. */
    if (meshUtrDecryptRequest(pAccPduInfo, meshUtrDecryptCback) != MESH_SUCCESS)
    {
      /* Free queue element allocated in LTR. */
      WsfBufFree(pAccPduInfo);

      /* If request failed, clear decrypt in progress flag. */
      utrCb.utrDecryptInProgress = FALSE;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Upper Transport Control PDU receive function.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrRecvCtlPdu(meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  /* Invalid pointers. Critical Error. */
  /* coverity[var_compare_op] */
  WSF_ASSERT((pCtlPduInfo != NULL) && (pCtlPduInfo->pUtrCtlPdu != NULL));

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
    meshTestUtrCtlPduRcvdInd_t utrCtlPduRcvdInd;

  if (meshTestCb.listenMask & MESH_TEST_UTR_LISTEN)
  {
    utrCtlPduRcvdInd.hdr.event = MESH_TEST_EVENT;
    utrCtlPduRcvdInd.hdr.param = MESH_TEST_UTR_CTL_PDU_RCVD_IND;
    utrCtlPduRcvdInd.hdr.status = MESH_SUCCESS;

    /* coverity[var_deref_op] */
    utrCtlPduRcvdInd.src         = pCtlPduInfo->src;
    utrCtlPduRcvdInd.dst         = pCtlPduInfo->dst;
    utrCtlPduRcvdInd.netKeyIndex = pCtlPduInfo->netKeyIndex;
    utrCtlPduRcvdInd.ttl         = pCtlPduInfo->ttl;
    utrCtlPduRcvdInd.seqNo       = pCtlPduInfo->seqNo;
    utrCtlPduRcvdInd.opcode      = pCtlPduInfo->opcode;
    utrCtlPduRcvdInd.pUtrCtlPdu  = pCtlPduInfo->pUtrCtlPdu;
    utrCtlPduRcvdInd.pduLen      = pCtlPduInfo->pduLen;

    meshTestCb.testCback((meshTestEvt_t *)&utrCtlPduRcvdInd);
  }
#endif

  /* coverity[var_deref_op] */
  switch (pCtlPduInfo->opcode)
  {
    /* Friendship Opcodes. */
    case MESH_UTR_CTL_FRIEND_POLL_OPCODE:
    case MESH_UTR_CTL_FRIEND_REQUEST_OPCODE:
    case MESH_UTR_CTL_FRIEND_SUBSCR_LIST_ADD_OPCODE:
    case MESH_UTR_CTL_FRIEND_SUBSCR_LIST_RM_OPCODE:
    case MESH_UTR_CTL_FRIEND_CLEAR_OPCODE:
    case MESH_UTR_CTL_FRIEND_CLEAR_CNF_OPCODE:
    case MESH_UTR_CTL_FRIEND_UPDATE_OPCODE:
    case MESH_UTR_CTL_FRIEND_OFFER_OPCODE:
    case MESH_UTR_CTL_FRIEND_SUBSCR_LIST_CNF_OPCODE:
      utrCb.friendshipCtlRecvCback(pCtlPduInfo);
      break;

    /* Heartbeat Opcodes. */
    case MESH_UTR_CTL_HB_OPCODE:
      /* coverity[var_deref_model] */
      MeshHbProcessHb(pCtlPduInfo);
      break;

    default:
      /* Invalid OpCode - silently discard. */
      break;
  }

  /* Free queue element from LTR. */
  WsfBufFree((uint8_t *)pCtlPduInfo);
}

/*************************************************************************************************/
/*!
 *\brief       Mesh Upper Transport event notification callback function.
 *
 *  \param[in] event  Reason the callback is being invoked.See::meshLtrEvent_t
 *  \param[in] seqNo  Sequence number used to identify the Tx transaction.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrEvtHandler(meshLtrEvent_t event, meshSeqNumber_t seqNo)
{
  (void)event;
  (void)seqNo;
}

/*************************************************************************************************/
/*!
 *  \brief     Triggers an Upper Transpor encrypt request.
 *
 *  \param[in] pUtrAccTxInfo  Pointer to structure containing TX information.
 *  \param[in] secCback       Encrypt callback to be used with the security module.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshUtrRetVal_t meshUtrEncryptRequest(meshUtrAccPduTxTcb_t *pUtrAccTxInfo,
                                             meshSecUtrEncryptCback_t secCback)
{
  meshSecUtrEncryptParams_t utrEncryptParams;

  /* Get address of the Lower Transport Access PDU. */
  meshLtrAccPduInfo_t *pLtrAccPduInfo = &(pUtrAccTxInfo->ltrAccPduInfo);

  /* Set the values for security request. */
  utrEncryptParams.pAppPayload    = pLtrAccPduInfo->pUtrAccPdu;
  utrEncryptParams.pEncAppPayload = pLtrAccPduInfo->pUtrAccPdu;
  utrEncryptParams.pTransMic      = pLtrAccPduInfo->pUtrAccPdu + pLtrAccPduInfo->pduLen;
  utrEncryptParams.srcAddr        = pLtrAccPduInfo->src;
  utrEncryptParams.dstAddr        = pLtrAccPduInfo->dst;
  utrEncryptParams.pLabelUuid     = (uint8_t *)pUtrAccTxInfo->pLabelUuid;
  utrEncryptParams.appPayloadSize = pLtrAccPduInfo->pduLen;
  utrEncryptParams.seqNo          = pLtrAccPduInfo->seqNo;
  utrEncryptParams.transMicSize   = MESH_UTR_TRANSMIC_32BIT_SIZE;
  utrEncryptParams.netKeyIndex    = pLtrAccPduInfo->netKeyIndex;
  utrEncryptParams.appKeyIndex    = pUtrAccTxInfo->appKeyIndex;

  return (meshUtrRetVal_t) MeshSecUtrEncrypt(&utrEncryptParams, secCback, (void*)pUtrAccTxInfo);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Upper transport encryption complete callback.
 *
 *  \param[in] isEncryptSuccess  TRUE if encryption finished successfully.
 *  \param[in] pEncAppPayload    Pointer to encrypted application payload, provided in the request.
 *  \param[in] appPayloadSize    Size of the encrypted application payload.
 *  \param[in] pTransMic         Pointer to buffer, provided in the request, used to store
 *                               calculated transport MIC over the application payload.
 *  \param[in] transMicSize      Size of the transport MIC (4 or 8 bytes).
 *  \param[in] aid               Application identifier or ::MESH_SEC_DEVICE_AID if device key
 *                               is used for encryption.
 *  \param[in] pParam            Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrEncryptCback(bool_t isEncryptSuccess, uint8_t *pEncAppPayload,
                                uint16_t appPayloadSize, uint8_t *pTransMic, uint8_t transMicSize,
                                uint8_t aid, void *pParam)
{
  meshUtrAccPduTxTcb_t *pUtrAccTxInfo;
  meshUtrRetVal_t retVal = MESH_SUCCESS;

  if (pParam == NULL)
  {
    /* Critical error. Memory will remain allocated. */
    WSF_ASSERT(pParam != NULL);

    return;
  }

  /* Recover TX request. */
  pUtrAccTxInfo = (meshUtrAccPduTxTcb_t *)pParam;

  if (!isEncryptSuccess)
  {
    /* Free the queue element passed as pParam. */
    WsfBufFree(pParam);

    /* Notify upper layer */
    utrCb.eventCback(MESH_UTR_ENC_FAILED, NULL);
  }
  else
  {
    /* TransMIC is already placed after the PDU, just add the size. */
    pUtrAccTxInfo->ltrAccPduInfo.pduLen += transMicSize;

    /* Set AKF and AID. */
    pUtrAccTxInfo->ltrAccPduInfo.akf = ((aid == MESH_SEC_DEVICE_KEY_AID) ? FALSE : TRUE);
    pUtrAccTxInfo->ltrAccPduInfo.aid = ((aid == MESH_SEC_DEVICE_KEY_AID) ? 0 : aid);

    /* Send to Lower Transport. */
    retVal = MeshLtrSendUtrAccPdu(&(pUtrAccTxInfo->ltrAccPduInfo));

    if (retVal != MESH_SUCCESS)
    {
      /* Notify upper layer. */
      utrCb.eventCback(MESH_UTR_SEND_FAILED, NULL);
    }
  }

  /* Clear encrypt in progress flag. */
  utrCb.utrEncryptInProgress = FALSE;

  /* Run maintenance on encrypt queue. */
  pUtrAccTxInfo = WsfQueueDeq(&(utrCb.utrAccTxQueue));

  while(pUtrAccTxInfo != NULL)
  {
    /* Set encrypt in progress flag. */
    utrCb.utrEncryptInProgress = TRUE;

    /* Trigger encrypt request. */
    if (meshUtrEncryptRequest(pUtrAccTxInfo, meshUtrEncryptCback) != MESH_SUCCESS)
    {
      /* Free the allocated queue element. */
      WsfBufFree(pUtrAccTxInfo);

      /* Clear encrypt in progress flag. */
      utrCb.utrEncryptInProgress = FALSE;

      /* Notify upper layer. */
      utrCb.eventCback(MESH_UTR_ENC_FAILED, NULL);
    }
    else
    {
      /* Break loop. */
      break;
    }

    /* Dequeue next request. */
    pUtrAccTxInfo = WsfQueueDeq(&(utrCb.utrAccTxQueue));
  }

  /* Remove compiler warning. */
  (void)pEncAppPayload;
  (void)pTransMic;
  (void)appPayloadSize;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Mesh Access PDU transmission.
 *
 *  \param[in] pAccPduInfo  Pointer to the structure holding the received Access PDU and other
 *                          fields. See ::meshUtrAccPduInfo_t
 *
 *  \return    Success or error reason. \see meshUtrRetVal_t
 */
/*************************************************************************************************/
static meshUtrRetVal_t meshUtrSendAccPduInternal(const meshUtrAccPduTxInfo_t *pAccPduInfo)
{
  meshLtrAccPduInfo_t    *pLtrAccPduInfo;
  meshUtrAccPduTxTcb_t   *pUtrAccTxInfo = NULL;
  meshUtrRetVal_t        retVal         = MESH_SUCCESS;
  uint16_t txInfoLen;

  /* Calculate number of bytes to be allocated for UTR TX information. */
  txInfoLen = sizeof(meshUtrAccPduTxTcb_t) +
              pAccPduInfo->accPduOpcodeLen + pAccPduInfo->accPduParamLen +
              MESH_UTR_TRANSMIC_32BIT_SIZE;

  /* Reserve memory for Label UUID. */
  if (pAccPduInfo->pDstLabelUuid != NULL)
  {
    txInfoLen += MESH_LABEL_UUID_SIZE;
  }

  /* Allocate memory to store the LTR PDU info, PDU, TransMIC and security information. */
  pUtrAccTxInfo = WsfBufAlloc(txInfoLen);

  if (pUtrAccTxInfo != NULL)
  {
    pLtrAccPduInfo = &(pUtrAccTxInfo->ltrAccPduInfo);

    pLtrAccPduInfo->src = pAccPduInfo->src;
    pLtrAccPduInfo->dst = pAccPduInfo->dst;

    retVal = MeshSeqGetNumber(pAccPduInfo->src, &(pLtrAccPduInfo->seqNo), TRUE);

    if (retVal == MESH_SUCCESS)
    {
      pLtrAccPduInfo->netKeyIndex = pAccPduInfo->netKeyIndex;
      pLtrAccPduInfo->friendLpnAddr = pAccPduInfo->friendLpnAddr;
      pLtrAccPduInfo->ackRequired = pAccPduInfo->ackRequired;
      pLtrAccPduInfo->szMic = MESH_TRANSMIC_TO_SZMIC(MESH_UTR_TRANSMIC_32BIT_SIZE);

      /* Point to the start of the UTR PDU. */
      pLtrAccPduInfo->pUtrAccPdu = (uint8_t *)pUtrAccTxInfo + sizeof(meshUtrAccPduTxTcb_t);

      pLtrAccPduInfo->pduLen = pAccPduInfo->accPduOpcodeLen + pAccPduInfo->accPduParamLen;

      /* Copy the UTR PDU. */
      memcpy(pLtrAccPduInfo->pUtrAccPdu, pAccPduInfo->pAccPduOpcode, pAccPduInfo->accPduOpcodeLen);

      memcpy(pLtrAccPduInfo->pUtrAccPdu + pAccPduInfo->accPduOpcodeLen, pAccPduInfo->pAccPduParam,
             pAccPduInfo->accPduParamLen);

      /* Copy Label UUID if needed. */
      if (pAccPduInfo->pDstLabelUuid != NULL)
      {
        pUtrAccTxInfo->pLabelUuid = pLtrAccPduInfo->pUtrAccPdu + pLtrAccPduInfo->pduLen;

        memcpy(pUtrAccTxInfo->pLabelUuid, pAccPduInfo->pDstLabelUuid, MESH_LABEL_UUID_SIZE);
      }
      else
      {
        pUtrAccTxInfo->pLabelUuid = NULL;
      }

      /* Check if the TTL is set to default. */
      if (pAccPduInfo->ttl == MESH_USE_DEFAULT_TTL)
      {
        pLtrAccPduInfo->ttl = MeshLocalCfgGetDefaultTtl();
      }
      else
      {
        pLtrAccPduInfo->ttl = pAccPduInfo->ttl;
      }

      /* Set AppKey index in the request. */
      pUtrAccTxInfo->appKeyIndex = pAccPduInfo->appKeyIndex;

      /* Verify if another encryption is in progress. */
      if (utrCb.utrEncryptInProgress)
      {
        /* Enqueue for later. */
        WsfQueueEnq(&(utrCb.utrAccTxQueue), (void *)pUtrAccTxInfo);
      }
      else
      {
        /* Mark encryption in progress. */
        utrCb.utrEncryptInProgress = TRUE;

        /* Request encryption. */
        retVal = meshUtrEncryptRequest(pUtrAccTxInfo, meshUtrEncryptCback);

        /* Check if request is processed. */
        if (retVal != MESH_SUCCESS)
        {
          /* Reset encryption in progress. */
          utrCb.utrEncryptInProgress = FALSE;
        }
      }
    }

    if (retVal != MESH_SUCCESS)
    {
      /* If one of the functions failed, free the allocated queue element. */
      WsfBufFree(pLtrAccPduInfo);
    }
  }
  else
  {
    retVal = MESH_UTR_OUT_OF_MEMORY;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Mesh Control PDU transmission.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshUtrCtlPduInfo_t
 *
 *  \return    Success or error reason. \see meshUtrRetVal_t
 */
/*************************************************************************************************/
static meshUtrRetVal_t meshUtrSendCtlPduInternal(const meshUtrCtlPduInfo_t *pCtlPduInfo)
{
  meshLtrCtlPduInfo_t *pLtrCtlPduInfo = NULL;
  meshUtrRetVal_t retVal = MESH_SUCCESS;

  /* Allocate a buffer to store the LTR PDU info and PDU. */
  pLtrCtlPduInfo = WsfBufAlloc(sizeof(meshLtrCtlPduInfo_t) + pCtlPduInfo->pduLen);

  if (pLtrCtlPduInfo != NULL)
  {
    pLtrCtlPduInfo->src = pCtlPduInfo->src;
    pLtrCtlPduInfo->dst = pCtlPduInfo->dst;
    pLtrCtlPduInfo->netKeyIndex = pCtlPduInfo->netKeyIndex;
    pLtrCtlPduInfo->friendLpnAddr = pCtlPduInfo->friendLpnAddr;
    pLtrCtlPduInfo->ifPassthr = pCtlPduInfo->ifPassthr;
    pLtrCtlPduInfo->opcode = pCtlPduInfo->opcode;
    pLtrCtlPduInfo->ackRequired = pCtlPduInfo->ackRequired;
    pLtrCtlPduInfo->prioritySend = pCtlPduInfo->prioritySend;

    /* Copy the UTR PDU. */
    memcpy((uint8_t *)pLtrCtlPduInfo + sizeof(meshLtrCtlPduInfo_t),
           pCtlPduInfo->pCtlPdu,
           pCtlPduInfo->pduLen);

    /* Point to the start of the UTR PDU. */
    pLtrCtlPduInfo->pUtrCtlPdu = (uint8_t *)pLtrCtlPduInfo + sizeof(meshLtrCtlPduInfo_t);

    pLtrCtlPduInfo->pduLen = pCtlPduInfo->pduLen;

    /* Get the sequence number.*/
    retVal = MeshSeqGetNumber(pCtlPduInfo->src, &(pLtrCtlPduInfo->seqNo), FALSE);

    if (retVal == MESH_SUCCESS)
    {
      /* Check if the TTL is set to default. */
      if (pCtlPduInfo->ttl == MESH_USE_DEFAULT_TTL)
      {
        pLtrCtlPduInfo->ttl = MeshLocalCfgGetDefaultTtl();
      }
      else
      {
        pLtrCtlPduInfo->ttl = pCtlPduInfo->ttl;
      }

      /* Send PDU to the Lower Transport Layer. */
      retVal = MeshLtrSendUtrCtlPdu(pLtrCtlPduInfo);

      if (retVal == MESH_SUCCESS)
      {
        /* Increment SEQ Number. */
        MeshSeqIncNumber(pCtlPduInfo->src);
      }
    }
    else
    {
      /* Free if errors encountered prior to passing the PDU info to LTR. */
      WsfBufFree(pLtrCtlPduInfo);
    }
  }
  else
  {
    retVal = MESH_UTR_OUT_OF_MEMORY;
  }

  return retVal;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Upper Transport layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshUtrInit(void)
{
  MESH_TRACE_INFO0("MESH UTR: init");

  /* Store empty callbacks into local structure. */
  utrCb.accRecvCback = meshUtrEmptyAccRecvCback;
  utrCb.friendshipCtlRecvCback = meshUtrEmptyFriendshipCtlRecvCback;
  utrCb.eventCback = meshUtrEmptyEventNotifyCback;

  /* Initialize the Lower Transport Layer and register the callback for receive and notify. */
  MeshLtrRegister(meshUtrRecvAccPdu, meshUtrRecvCtlPdu, meshUtrEvtHandler);

  /* Initialize the Heartbeat module. */
  MeshHbInit();

  /* Reset TX and RX queues. */
  WSF_QUEUE_INIT(&(utrCb.utrAccTxQueue));
  WSF_QUEUE_INIT(&(utrCb.utrAccRxQueue));

  /* Reset crypto in progress flags. */
  utrCb.utrEncryptInProgress = FALSE;
  utrCb.utrDecryptInProgress = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callbacks used by the Upper Transport Layer.
 *
 *  \param[in] accRecvCback  Callback to be invoked when an Access PDU is received.
 *  \param[in] eventCback    Event notification callback for the upper layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshUtrRegister(meshUtrAccRecvCback_t accRecvCback, meshUtrEventNotifyCback_t eventCback)
{
  WSF_ASSERT((accRecvCback != NULL) && (eventCback != NULL));

  /* Store callbacks into local structure. */
  utrCb.accRecvCback = accRecvCback;
  utrCb.eventCback = eventCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Friendship required callback used by the Upper Transport Layer.
 *
 *  \param[in] ctlRecvCback  Callback to be invoked when a Friendship Control PDU is received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshUtrRegisterFriendship(meshUtrFriendshipCtlRecvCback_t ctlRecvCback)
{
  WSF_ASSERT(ctlRecvCback != NULL);

  /* Store callback into local structure. */
  utrCb.friendshipCtlRecvCback = ctlRecvCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Access PDU to Upper Transport Layer.
 *
 *  \param[in] pAccPduInfo  Pointer to the structure holding the received Access PDU and other
 *                          fields. See ::meshUtrAccPduInfo_t
 *
 *  \return    Success or error reason. \see meshUtrRetVal_t
 */
/*************************************************************************************************/
meshUtrRetVal_t MeshUtrSendAccPdu(const meshUtrAccPduTxInfo_t *pAccPduInfo)
{
  uint16_t pduLen;

  /* Check for invalid parameters. */
  if (pAccPduInfo == NULL)
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  if ((pAccPduInfo->pAccPduOpcode == NULL) ||
      ((pAccPduInfo->pAccPduParam == NULL) && (pAccPduInfo->accPduParamLen != 0)))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Validate source address. Only unicast address is allowed. */
  if (!MESH_IS_ADDR_UNICAST(pAccPduInfo->src))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Validate destination. */
  if (MESH_IS_ADDR_UNASSIGNED(pAccPduInfo->dst) || (MESH_IS_ADDR_RFU(pAccPduInfo->dst)) ||
      (MESH_IS_ADDR_VIRTUAL(pAccPduInfo->dst) && (pAccPduInfo->pDstLabelUuid == NULL)))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  if ((pAccPduInfo->ackRequired) && !MESH_IS_ADDR_UNICAST(pAccPduInfo->dst))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Check if the appKeyIndex is in valid range. */
  if ((!pAccPduInfo->devKeyUse) &&
      (MESH_SEC_KEY_INDEX_IS_VALID(pAccPduInfo->appKeyIndex) == FALSE))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  if((pAccPduInfo->devKeyUse) &&
     (pAccPduInfo->appKeyIndex != MESH_APPKEY_INDEX_LOCAL_DEV_KEY) &&
     (pAccPduInfo->appKeyIndex != MESH_APPKEY_INDEX_REMOTE_DEV_KEY))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Check if the netKeyIndex is in valid range. */
  if (MESH_SEC_KEY_INDEX_IS_VALID(pAccPduInfo->netKeyIndex) == FALSE)
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Check if TTL is in valid range. */
  if ((MESH_TTL_IS_VALID(pAccPduInfo->ttl) == FALSE) ||
       (pAccPduInfo->ttl == MESH_TX_TTL_FILTER_VALUE))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Calculate total length. */
  pduLen = pAccPduInfo->accPduOpcodeLen + pAccPduInfo->accPduParamLen;

  /* Check if valid PDU size is passed. */
  if ((pduLen > MESH_UTR_MAX_ACC_PDU_LEN) || (pduLen == 0))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  return meshUtrSendAccPduInternal(pAccPduInfo);
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Control PDU to Upper Transport Layer.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshUtrCtlPduInfo_t
 *
 *  \return    Success or error reason. \see meshUtrRetVal_t
 */
/*************************************************************************************************/
meshUtrRetVal_t MeshUtrSendCtlPdu(const meshUtrCtlPduInfo_t *pCtlPduInfo)
{
  /* Check for invalid parameters. */
  if (pCtlPduInfo == NULL)
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  if (pCtlPduInfo->pCtlPdu == NULL)
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Validate source address. Only unicast address is allowed. */
  if (!MESH_IS_ADDR_UNICAST(pCtlPduInfo->src))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  if (pCtlPduInfo->ackRequired == FALSE)
  {
    /* Validate destination address. Only unicast and group addresses are allowed. */
    if (!MESH_IS_ADDR_UNICAST(pCtlPduInfo->dst) &&
        !MESH_IS_ADDR_GROUP(pCtlPduInfo->dst))
    {
      return MESH_UTR_INVALID_PARAMS;
    }
  }
  else
  {
    /* Validate destination address. Only unicast address is allowed for reliable send. */
    if (!MESH_IS_ADDR_UNICAST(pCtlPduInfo->dst))
    {
      return MESH_UTR_INVALID_PARAMS;
    }
  }

  /* Check if the netKeyIndex is in valid range. */
  if (MESH_SEC_KEY_INDEX_IS_VALID(pCtlPduInfo->netKeyIndex) == FALSE)
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Check if the Opcode is in the valid range. */
  if (MESH_UTR_CTL_OPCODE_IN_RANGE(pCtlPduInfo->opcode) == FALSE)
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Check if TTL is in valid range. */
  if ((MESH_TTL_IS_VALID(pCtlPduInfo->ttl) == FALSE) ||
       (pCtlPduInfo->ttl == MESH_TX_TTL_FILTER_VALUE))
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  /* Check if the maximum PDU size is not exceeded. */
  if (pCtlPduInfo->pduLen > MESH_UTR_MAX_CTL_PDU_LEN)
  {
    return MESH_UTR_INVALID_PARAMS;
  }

  return meshUtrSendCtlPduInternal(pCtlPduInfo);
}
