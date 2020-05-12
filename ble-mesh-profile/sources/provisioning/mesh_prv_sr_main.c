/*************************************************************************************************/
/*!
 *  \file   mesh_prv_sr_main.c
 *
 *  \brief  Mesh Provisioning Server module implementation.
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
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_main.h"
#include "mesh_prv_sr_api.h"
#include "mesh_prv_br_main.h"
#include "mesh_prv_common.h"
#include "mesh_utils.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST == 1))
#include "mesh_test_api.h"
#endif

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Module initialization flag */
static bool_t prvSrInitialized = FALSE;

/*! Mesh Provisioning Server callback event length table */
static const uint16_t meshPrvSrEvtCbackLen[] =
{
  sizeof(wsfMsgHdr_t),                   /*!< MESH_PRV_SR_LINK_OPENED_EVENT */
  sizeof(meshPrvSrEvtOutputOob_t),       /*!< MESH_PRV_SR_OUTPUT_OOB_EVENT */
  sizeof(wsfMsgHdr_t),                   /*!< MESH_PRV_SR_OUTPUT_CONFIRMED_EVENT */
  sizeof(meshPrvSrEvtInputOob_t),        /*!< MESH_PRV_SR_INPUT_OOB_EVENT */
  sizeof(meshPrvSrEvtPrvComplete_t),     /*!< MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT */
  sizeof(meshPrvSrEvtPrvFailed_t),       /*!< MESH_PRV_SR_PROVISIONING_FAILED_EVENT */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Control block */
meshPrvSrCb_t meshPrvSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Validates parameters for the incoming Provisioning Start PDU.
 *
 *  \param[in] pParams  Pointer to the Provisioning Start PDU parameters.
 *
 *  \return    TRUE if parameters are valid, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshPrvSrValidateStartParams(meshPrvSrRecvStart_t *pParams)
{
  /* Range validation for Algorithm, Public Key and Authentication Method */
  if (pParams->algorithm >= MESH_PRV_START_ALGO_RFU_START ||
      pParams->oobPubKeyUsed >= MESH_PRV_START_PUB_KEY_PROHIBITED_START ||
      pParams->authMethod >= MESH_PRV_START_AUTH_METHOD_PROHIBITED_START)
  {
    return FALSE;
  }

  /* Range validation for Authentication Action and Authentication Size
   * when using Output OOB */
  if (pParams->authMethod == MESH_PRV_START_AUTH_METHOD_OUTPUT_OOB &&
       (pParams->authAction >= MESH_PRV_START_OUT_OOB_ACTION_RFU_START ||
        pParams->authSize == MESH_PRV_START_OOB_SIZE_PROHIBITED ||
        pParams->authSize >= MESH_PRV_START_OOB_SIZE_RFU_START))
  {
    return FALSE;
  }

  /* Range validation for Authentication Action and Authentication Size
   * when using Input OOB */
  if (pParams->authMethod == MESH_PRV_START_AUTH_METHOD_INPUT_OOB &&
       (pParams->authAction >= MESH_PRV_START_IN_OOB_ACTION_RFU_START ||
        pParams->authSize == MESH_PRV_START_OOB_SIZE_PROHIBITED ||
        pParams->authSize >= MESH_PRV_START_OOB_SIZE_RFU_START))
  {
    return FALSE;
  }

  /* Range validation for Authentication Action and Authentication Size
   * when using Static or No OOB */
  if ((pParams->authMethod == MESH_PRV_START_AUTH_METHOD_STATIC_OOB ||
       pParams->authMethod == MESH_PRV_START_AUTH_METHOD_NO_OOB) &&
      (pParams->authAction != MESH_PRV_START_OOB_NO_SIZE_NO_ACTION ||
       pParams->authSize != MESH_PRV_START_OOB_NO_SIZE_NO_ACTION))
  {
    return FALSE;
  }

  /* Public Key validation against capabilities */
  if (pParams->oobPubKeyUsed == MESH_PRV_START_PUB_KEY_OOB_AVAILABLE &&
      !(meshPrvSrCb.pUpdInfo->pCapabilities->publicKeyType & MESH_PRV_PUB_KEY_OOB))
  {
    return FALSE;
  }

  /* Authentication Method, Action and Size validation against capabilities
   * when using Output OOB */
  if (pParams->authMethod == MESH_PRV_START_AUTH_METHOD_OUTPUT_OOB &&
       (meshPrvSrCb.pUpdInfo->pCapabilities->outputOobSize == MESH_PRV_OUTPUT_OOB_NOT_SUPPORTED ||
        meshPrvSrCb.pUpdInfo->pCapabilities->outputOobSize < pParams->authSize ||
        !(meshPrvSrCb.pUpdInfo->pCapabilities->outputOobAction & (1 << pParams->authAction))))
  {
    return FALSE;
  }

  /* Authentication Method, Action and Size validation against capabilities
     * when using Input OOB */
  if (pParams->authMethod == MESH_PRV_START_AUTH_METHOD_INPUT_OOB &&
       (meshPrvSrCb.pUpdInfo->pCapabilities->inputOobSize == MESH_PRV_INPUT_OOB_NOT_SUPPORTED ||
        meshPrvSrCb.pUpdInfo->pCapabilities->inputOobSize < pParams->authSize ||
        !(meshPrvSrCb.pUpdInfo->pCapabilities->inputOobAction & (1 << pParams->authAction))))
  {
    return FALSE;
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer PDU received callback.
 *
 *  \param[in] pPdu    Pointer to the Provisioning Bearer PDU received.
 *  \param[in] pduLen  Size of the Provisioning Bearer PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvSrPduRecvCback(const uint8_t *pPrvPdu, uint8_t pduLen)
{
  meshPrvSrSmMsg_t *pMsg;

  if (meshPrvSrCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated when receiving PDU!");
    return;
  }

  if (pduLen < MESH_PRV_PDU_OPCODE_SIZE)
  {
    MESH_TRACE_ERR0("MESH PRV SR: No Opcode in Provisioning PDU!");
    return;
  }

  pMsg = WsfMsgAlloc(sizeof (meshPrvSrSmMsg_t));
  if (pMsg == NULL)
  {
    /* Should never happen if buffers are properly configured */
    return;
  }

  switch (pPrvPdu[MESH_PRV_PDU_OPCODE_INDEX])
  {
    case MESH_PRV_PDU_INVITE:
      if (pduLen != MESH_PRV_PDU_INVITE_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV SR: Received invalid Provisioning Invite PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
        pMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
      }
      else
      {
        /* Copy parameters to the ConfirmationInputs */
        memcpy(meshPrvSrCb.pSessionData->authParams.confirmationInputs,
               &pPrvPdu[MESH_PRV_PDU_PARAM_INDEX],
               MESH_PRV_PDU_INVITE_PARAM_SIZE);

        pMsg->recvInvite.hdr.event = PRV_SR_EVT_RECV_INVITE;
        pMsg->recvInvite.attentionTimer = pPrvPdu[MESH_PRV_PDU_INVITE_ATTENTION_INDEX];
      }
      break;

    case MESH_PRV_PDU_START:
      if (pduLen != MESH_PRV_PDU_START_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV SR: Received invalid Provisioning Start PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
        pMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
      }
      else
      {
        /* Unpack parameters */
        pMsg->recvStart.hdr.event = PRV_SR_EVT_RECV_START;
        pMsg->recvStart.algorithm = pPrvPdu[MESH_PRV_PDU_START_ALGORITHM_INDEX];
        pMsg->recvStart.oobPubKeyUsed = pPrvPdu[MESH_PRV_PDU_START_PUB_KEY_INDEX];
        pMsg->recvStart.authMethod = pPrvPdu[MESH_PRV_PDU_START_AUTH_METHOD_INDEX];
        pMsg->recvStart.authAction = pPrvPdu[MESH_PRV_PDU_START_AUTH_ACTION_INDEX];
        pMsg->recvStart.authSize = pPrvPdu[MESH_PRV_PDU_START_AUTH_SIZE_INDEX];

        /* Parameter validation */
        if (!meshPrvSrValidateStartParams(&pMsg->recvStart))
        {
          pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
          pMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
        }
        else
        {
          /* Copy packed parameters required by the ConfirmationInputs */
          memcpy(pMsg->recvStart.packedPduParam,
                 &pPrvPdu[MESH_PRV_PDU_PARAM_INDEX],
                 MESH_PRV_PDU_START_PARAM_SIZE);

        }
      }
      break;

    case MESH_PRV_PDU_PUB_KEY:
      if (pduLen != MESH_PRV_PDU_PUB_KEY_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV SR: Received invalid Provisioning Public Key PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
        pMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
      }
      else
      {
        pMsg->hdr.event = PRV_SR_EVT_RECV_PUBLIC_KEY;
        memcpy(pMsg->recvPubKey.pubKeyPdu,
               pPrvPdu,
               MESH_PRV_PDU_PUB_KEY_PDU_SIZE);
      }
      break;

    case MESH_PRV_PDU_CONFIRMATION:
      if (pduLen != MESH_PRV_PDU_CONFIRM_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV SR: Received invalid Provisioning Confirmation PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
        pMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
      }
      else
      {
        pMsg->recvConfirm.hdr.event = PRV_SR_EVT_RECV_CONFIRMATION;
        memcpy(pMsg->recvConfirm.confirm,
               &pPrvPdu[MESH_PRV_PDU_CONFIRM_CONFIRM_INDEX],
               MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE);
      }
      break;

    case MESH_PRV_PDU_RANDOM:
      if (pduLen != MESH_PRV_PDU_RANDOM_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV SR: Received invalid Provisioning Random PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
        pMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
      }
      else
      {
        pMsg->recvRandom.hdr.event = PRV_SR_EVT_RECV_RANDOM;
        memcpy(pMsg->recvRandom.random,
               &pPrvPdu[MESH_PRV_PDU_RANDOM_RANDOM_INDEX],
               MESH_PRV_PDU_RANDOM_RANDOM_SIZE);
      }
      break;

    case MESH_PRV_PDU_DATA:
      if (pduLen != MESH_PRV_PDU_DATA_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV SR: Received invalid Provisioning Data PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
        pMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
      }
      else
      {
        pMsg->recvData.hdr.event = PRV_SR_EVT_RECV_DATA;
        memcpy(pMsg->recvData.encryptedDataAndMic,
               &pPrvPdu[MESH_PRV_PDU_DATA_ENC_DATA_INDEX],
               MESH_PRV_PDU_DATA_PARAM_SIZE);
      }
      break;

    case MESH_PRV_PDU_CAPABILITIES: /* Fallthrough */
    case MESH_PRV_PDU_INPUT_COMPLETE: /* Fallthrough */
    case MESH_PRV_PDU_COMPLETE: /* Fallthrough */
    case MESH_PRV_PDU_FAILED:
      MESH_TRACE_WARN1("MESH PRV SR: Received unexpected Provisioning PDU type: 0x%02X", pPrvPdu[0]);
      pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
      pMsg->hdr.param = MESH_PRV_ERR_UNEXPECTED_PDU;
      break;

    default:
      MESH_TRACE_WARN1("MESH PRV SR: Received invalid Provisioning PDU type: 0x%02X", pPrvPdu[0]);
      pMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
      pMsg->hdr.param = MESH_PRV_ERR_INVALID_PDU;
      break;
  }

  WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer event notification callback.
 *
 *  \param[in] evt           Reason the callback is being invoked. See ::meshPrvBrEvent_t
 *  \param[in] pEventParams  Pointer to the event parameters structure passed to the function.
 *                           See ::meshPrvBrEventParams_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvSrBrEventNotifyCback(meshPrvBrEvent_t evt,
                                        const meshPrvBrEventParams_t *pEvtParams)
{
  wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
  if (pMsg == NULL)
  {
    /* Should never happen if buffers are properly configured */
    return;
  }

  switch (evt)
  {
    case MESH_PRV_BR_LINK_OPENED:
      pMsg->event = PRV_SR_EVT_LINK_OPENED;
      WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_LINK_CLOSED_BY_PEER:
      if (pEvtParams->linkCloseReason == MESH_PRV_BR_REASON_SUCCESS)
      {
        pMsg->event = PRV_SR_EVT_LINK_CLOSED_SUCCESS;
      }
      else
      {
        pMsg->event = PRV_SR_EVT_LINK_CLOSED_FAIL;
      }
      WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_CONN_CLOSED:
      /* This event is ignored in the IDLE state, so it will signal a failure
       * to the upper layer only if the connection is closed before provisioning
       * is complete (i.e., in a state different than IDLE). */
      pMsg->event = PRV_SR_EVT_LINK_CLOSED_FAIL;
      WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_SEND_TIMEOUT:
      pMsg->event = PRV_SR_EVT_SEND_TIMEOUT;
      WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_PDU_SENT:
      switch (pEvtParams->pduSentOpcode)
      {
        case MESH_PRV_PDU_FAILED:
          MESH_TRACE_INFO0("MESH PRV SR: Provisioning Failed PDU sent successfully.");
          /* No event needed after sending a Provisioning Failed PDU */
          WsfMsgFree(pMsg);
          break;

        case MESH_PRV_PDU_CAPABILITIES:
          pMsg->event = PRV_SR_EVT_SENT_CAPABILITIES;
          WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_PUB_KEY:
          pMsg->event = PRV_SR_EVT_SENT_PUBLIC_KEY;
          WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_INPUT_COMPLETE:
          pMsg->event = PRV_SR_EVT_SENT_INPUT_COMPLETE;
          WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_CONFIRMATION:
          pMsg->event = PRV_SR_EVT_SENT_CONFIRMATION;
          WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_RANDOM:
          pMsg->event = PRV_SR_EVT_SENT_RANDOM;
          WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_COMPLETE:
          pMsg->event = PRV_SR_EVT_SENT_COMPLETE;
          WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
          break;

        default:
          MESH_TRACE_WARN0("MESH PRV SR: Received PDU Sent event with invalid opcode.");
          WsfMsgFree(pMsg);
          break;
      }
      break;

    default:
      MESH_TRACE_WARN1("MESH PRV SR: Received PRV BR event with invalid type: %d.", evt);
      WsfMsgFree(pMsg);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Empty event notification callback.
 *
 *  \param[in] pEvent  Pointer to the event.
 *                     See ::meshPrvSrEvent_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvSrEventNotifyEmptyCback(meshPrvSrEvt_t *pEvent)
{
  (void)pEvent;
  MESH_TRACE_WARN0("MESH PRV SR: Event notification callback not installed!");
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes and configures the Provisioning Server.
 *
 *  \param[in] pUpdInfo  Sets the unprovisioned device information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrInit(const meshPrvSrUnprovisionedDeviceInfo_t* pUpdInfo)
{
  if (prvSrInitialized)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Attempting multiple initialization sequences!");
    return;
  }

  if (pUpdInfo == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Unprovisioned Device information parameter is NULL!");
    return;
  }

  if ((pUpdInfo->pCapabilities->publicKeyType == MESH_PRV_PUB_KEY_OOB) &&
      (pUpdInfo->pAppOobEccKeys == NULL))
  {
    MESH_TRACE_ERR0("MESH PRV SR: App OOB Key is NULL!");
    return;
  }

  /* Initialize timer event value */
  meshPrvSrCb.timer.msg.event = PRV_SR_EVT_RECV_TIMEOUT;

  /* Link state machine instance */
  meshPrvSrCb.pSm = &meshPrvSrSmIf;

  /* Store capabilities */
  meshPrvSrCb.pUpdInfo = pUpdInfo;

  /* Set empty callback */
  meshPrvSrCb.prvSrEvtNotifyCback = meshPrvSrEventNotifyEmptyCback;

  /* Initialize empty session data */
  meshPrvSrCb.pSessionData = NULL;

  /* Initialize the provisioning bearer module and register callbacks */
  MeshPrvBrInit();
  MeshPrvBrRegisterCback(meshPrvSrPduRecvCback, meshPrvSrBrEventNotifyCback);

  /* Set initial state */
  meshPrvSrCb.state = PRV_SR_ST_IDLE;

  /* Set flag */
  prvSrInitialized = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh Provisioning Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Store Handler ID */
  meshPrvSrCb.timer.handlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF event handler for Mesh Provisioning Server API.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  /* Handle messages */
  if (pMsg != NULL)
  {
    meshPrvSrSmExecute(&meshPrvSrCb, (meshPrvSrSmMsg_t*)pMsg);
  }
  /* Handle events */
  else if (event)
  {

  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Provisioning Server event callback function.
 *
 *  \param[in] eventCback  Pointer to the callback to be invoked on provisioning events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrRegister(meshPrvSrEvtNotifyCback_t eventCback)
{
  if (eventCback != NULL)
  {
    meshPrvSrCb.prvSrEvtNotifyCback = eventCback;
  }
  else
  {
    MESH_TRACE_ERR0("MESH PRV SR: Attempting to install NULL event notification callback!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Begins provisioning over PB-ADV by waiting for a PB-ADV link.
 *
 *  \param[in] ifId            ID of the PB-ADV bearer interface.
 *  \param[in] beaconInterval  Unprovisioned Device beacon interval in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrEnterPbAdvProvisioningMode(uint8_t ifId, uint32_t beaconInterval)
{
  meshPrvSrEnterPbAdv_t *pMsg;

  /* Check module is initialized */
  if (prvSrInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Mesh Provisioning Server not initialized!");
    return;
  }

  /* Check session data is not already allocated */
  if (meshPrvSrCb.pSessionData != NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data already allocated!");
    return;
  }

  /* Allocate session data */
  meshPrvSrCb.pSessionData = WsfBufAlloc(sizeof(meshPrvSrSessionData_t));
  if (meshPrvSrCb.pSessionData == NULL)
  {
    /* Should not happen if buffers are properly configured */
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshPrvSrEnterPbAdv_t))) != NULL)
  {
    /* Set event type. */
    pMsg->hdr.event = PRV_SR_EVT_BEGIN_NO_LINK;

    /* Copy parameters */
    pMsg->ifId = ifId;
    pMsg->beaconInterval = beaconInterval;

    /* Send Message. */
    WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
  }
  else
  {
    /* Should not happen if buffers are properly configured. */
    WsfBufFree(meshPrvSrCb.pSessionData);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Begins provisioning over PB-GATT.
 *
 *  \param[in] connId  ID of the GATT connection.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrEnterPbGattProvisioningMode(uint8_t connId)
{
  meshPrvSrEnterPbGatt_t *pMsg;

  /* Check module is initialized */
  if (prvSrInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Mesh Provisioning Server not initialized!");
    return;
  }

  /* Check session data is not already allocated */
  if (meshPrvSrCb.pSessionData != NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data already allocated!");
    return;
  }

  /* Allocate session data */
  meshPrvSrCb.pSessionData = WsfBufAlloc(sizeof(meshPrvSrSessionData_t));
  if (meshPrvSrCb.pSessionData == NULL)
  {
    /* Should not happen if buffers are properly configured */
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshPrvSrEnterPbGatt_t))) != NULL)
  {
    /* Set event type. */
    pMsg->hdr.event = PRV_SR_EVT_BEGIN_LINK_OPEN;

    /* Copy parameters */
    pMsg->connId = connId;

    /* Send Message. */
    WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
  }
  else
  {
    /* Should not happen if buffers are properly configured. */
    WsfBufFree(meshPrvSrCb.pSessionData);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Provisioner Server application calls this function when it obtains the OOB input
 *             numbers or characters from the user.
 *
 *  \param[in] inputOobSize  Size of alphanumeric Input OOB data, used only when the Input OOB
 *                           Action was MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM; otherwise,
 *                           the Input OOB data is numeric, and this parameter shall be set to 0.
 *
 *  \param[in] inputOobData  Array of inputOobSize octets containing the alphanumeric Input OOB
 *                           data, if the Input OOB action selected by the Provisioner was
 *                           MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM action; otherwise,
 *                           this is a numeric 8-octet value and inputOobSize is ignored.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrInputComplete(meshPrvInputOobSize_t inputOobSize, meshPrvInOutOobData_t inputOobData)
{
  meshPrvSrInputOob_t *pMsg;

  if (prvSrInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Mesh Provisioning Server not initialized!");
    return;
  }

  /* Validate parameters */
  if (inputOobSize >= MESH_PRV_INPUT_OOB_SIZE_RFU_START ||
      (inputOobSize > 0 &&
       !meshPrvIsAlphanumericArray(inputOobData.alphanumericOob, inputOobSize)))
  {
    MESH_TRACE_ERR0("MESH PRV CL: Invalid parameters in MeshPrvSrInputComplete!");
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshPrvSrInputOob_t))) != NULL)
  {
    /* Set event type. */
    pMsg->hdr.event = PRV_SR_EVT_INPUT_READY;
    pMsg->inputOobSize = inputOobSize;
    pMsg->inputOobData = inputOobData;

    /* Send Message. */
    WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
  }
  /* Else should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Provisioning Server callback event.
 *
 *  \param[in] pMeshPrvSrEvt  Mesh Provisioning Server callback event.
 *
 *  \return    Size of Mesh Provisioning Server callback event.
 */
/*************************************************************************************************/
uint16_t MeshPrvSrSizeOfEvt(meshPrvSrEvt_t *pMeshPrvSrEvt)
{
  uint16_t len;

  /* If a valid Provisioning Server event ID */
  if ((pMeshPrvSrEvt->hdr.event == MESH_PRV_SR_EVENT) &&
      (pMeshPrvSrEvt->hdr.param <= MESH_PRV_SR_MAX_EVENT))
  {
    len = meshPrvSrEvtCbackLen[pMeshPrvSrEvt->hdr.param];
  }
  else
  {
    len = sizeof(wsfMsgHdr_t);
  }

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Failed PDU.
 *
 *  \param[in] errorCode  Error code, see ::meshPrvErrorCodeValues_tag.
 *
 *  \return    None.
 *
 *  \remarks   The function can be called at any moment during the protocol; it does not affect the
 *             state machine.
 */
/*************************************************************************************************/
void meshPrvSrSendFailedPdu(uint8_t errorCode)
{
  WSF_ASSERT(errorCode != MESH_PRV_ERR_PROHIBITED && errorCode < MESH_PRV_ERR_RFU_START);

  /* Allocate buffer for the Provisioning Failed PDU */
  uint8_t *pBuf = WsfBufAlloc(MESH_PRV_PDU_FAILED_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_FAILED;
    pBuf[MESH_PRV_PDU_FAILED_ERROR_CODE_INDEX] = errorCode;

    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_FAILED_PDU_SIZE);
  }
}
