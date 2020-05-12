/*************************************************************************************************/
/*!
 *  \file   mesh_prv_cl_main.c
 *
 *  \brief  Mesh Provisioning Client module implementation.
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
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_cl_main.h"
#include "mesh_prv_cl_api.h"
#include "mesh_prv_br_main.h"
#include "mesh_prv_common.h"
#include "mesh_security_toolbox.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Module initialization flag */
static bool_t prvClInitialized = FALSE;

/*! Mesh Provisioning Client callback event length table */
static const uint16_t meshPrvClEvtCbackLen[] =
{
  sizeof(wsfMsgHdr_t),                        /*!< MESH_PRV_CL_LINK_OPENED_EVENT */
  sizeof(meshPrvClEvtRecvCapabilities_t),     /*!< MESH_PRV_CL_RECV_CAPABILITIES_EVENT */
  sizeof(meshPrvClEvtEnterOutputOob_t),       /*!< MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT */
  sizeof(meshPrvClEvtDisplayInputOob_t),      /*!< MESH_PRV_CL_DISPLAY_INPUT_OOB_EVENT */
  sizeof(meshPrvClEvtPrvComplete_t),          /*!< MESH_PRV_CL_PROVISIONING_COMPLETE_EVENT */
  sizeof(meshPrvClEvtPrvFailed_t),            /*!< MESH_PRV_CL_PROVISIONING_FAILED_EVENT */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Control block */
meshPrvClCb_t meshPrvClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Validates parameters for the MeshPrvClSelectAuthentication API.
 *
 *  \param[in] pParams  Pointer to the Select Authentication API parameters.
 *
 *  \return    TRUE if parameters are valid, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshPrvClValidateSelectAuthParams(meshPrvClSelectAuth_t *pParams)
{
  if (meshPrvClCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated when checking SelectAuth params!");
    return FALSE;
  }

  /* Check OOB authentication method enumeration */
  if ((uint8_t)pParams->oobAuthMethod >= (uint8_t)MESH_PRV_START_AUTH_METHOD_PROHIBITED_START)
  {
    return FALSE;
  }

  /* Check that OOB Public Key is used only when it has been provided by the application
   * and when it has been indicated by the device in the provisioning capabilities */
  if (pParams->useOobPublicKey == TRUE &&
      (meshPrvClCb.pSessionInfo->pDevicePublicKey == NULL ||
       (!(meshPrvClCb.pSessionData->deviceCapab.publicKeyType & MESH_PRV_PUB_KEY_OOB))))
  {
    return FALSE;
  }

  /* Check that Static OOB authentication is used only when it has been provided by the application
   * and when it has been indicated by the device in the provisioning capabilities */
  if (pParams->oobAuthMethod == MESH_PRV_CL_USE_STATIC_OOB &&
      (meshPrvClCb.pSessionInfo->pStaticOobData == NULL ||
       (!(meshPrvClCb.pSessionData->deviceCapab.staticOobType & MESH_PRV_STATIC_OOB_INFO_AVAILABLE))))
  {
    return FALSE;
  }

  /* Check parameter consistency for Output OOB authentication */
  if (pParams->oobAuthMethod == MESH_PRV_CL_USE_OUTPUT_OOB)
  {
    /* Check size is greater than 0 but not greater than what the device supports */
    if (pParams->oobSize == 0 ||
        pParams->oobSize > meshPrvClCb.pSessionData->deviceCapab.outputOobSize)
    {
      return FALSE;
    }

    /* Check exactly one action bit is set */
    if (!pParams->oobAction.outputOobAction ||
        ((pParams->oobAction.outputOobAction - 1) & (pParams->oobAction.outputOobAction)))
    {
      return FALSE;
    }

    /* Check action is supported by the device */
    if (!(pParams->oobAction.outputOobAction & meshPrvClCb.pSessionData->deviceCapab.outputOobAction))
    {
      return FALSE;
    }
  }

  /* Check parameter consistency for Input OOB authentication */
  if (pParams->oobAuthMethod == MESH_PRV_CL_USE_INPUT_OOB)
  {
    /* Check size is greater than 0 but not greater than what the device supports */
    if (pParams->oobSize == 0 ||
        pParams->oobSize > meshPrvClCb.pSessionData->deviceCapab.inputOobSize)
    {
      return FALSE;
    }

    /* Check exactly one action bit is set */
    if (!pParams->oobAction.inputOobAction ||
        ((pParams->oobAction.inputOobAction - 1) & (pParams->oobAction.inputOobAction)))
    {
      return FALSE;
    }

    /* Check action is supported by the device */
    if (!(pParams->oobAction.inputOobAction & meshPrvClCb.pSessionData->deviceCapab.inputOobAction))
    {
      return FALSE;
    }
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
static void meshPrvClPduRecvCback(const uint8_t *pPrvPdu, uint8_t pduLen)
{
  meshPrvClSmMsg_t *pMsg;

  if (pduLen < MESH_PRV_PDU_OPCODE_SIZE)
  {
    MESH_TRACE_ERR0("MESH PRV CL: No Opcode in Provisioning PDU!");
    return;
  }

  pMsg = WsfMsgAlloc(sizeof (meshPrvClSmMsg_t));
  if (pMsg == NULL)
  {
    /* Should never happen if buffers are properly configured */
    return;
  }

  switch (pPrvPdu[MESH_PRV_PDU_OPCODE_INDEX])
  {
    case MESH_PRV_PDU_CAPABILITIES:
      if (pduLen != MESH_PRV_PDU_CAPAB_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV CL: Received invalid Provisioning Capabilities PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      }
      else
      {
        /* Set event and parameters */
        pMsg->recvCapab.hdr.event = PRV_CL_EVT_RECV_CAPABILITIES;
        memcpy(pMsg->recvCapab.capabPdu,
               pPrvPdu,
               MESH_PRV_PDU_CAPAB_PDU_SIZE);
      }
      break;

    case MESH_PRV_PDU_PUB_KEY:
      if (pduLen != MESH_PRV_PDU_PUB_KEY_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV CL: Received invalid Provisioning Public Key PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      }
      else
      {
        pMsg->hdr.event = PRV_CL_EVT_RECV_PUBLIC_KEY;
        memcpy(pMsg->recvPubKey.pubKeyPdu,
               pPrvPdu,
               MESH_PRV_PDU_PUB_KEY_PDU_SIZE);
      }
      break;

    case MESH_PRV_PDU_INPUT_COMPLETE:
      if (pduLen != MESH_PRV_PDU_INPUT_COMPLETE_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV CL: Received invalid Provisioning Input Complete PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      }
      else
      {
        pMsg->hdr.event = PRV_CL_EVT_RECV_INPUT_COMPLETE;
      }
      break;

    case MESH_PRV_PDU_CONFIRMATION:
      if (pduLen != MESH_PRV_PDU_CONFIRM_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV CL: Received invalid Provisioning Confirmation PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      }
      else
      {
        pMsg->recvConfirm.hdr.event = PRV_CL_EVT_RECV_CONFIRMATION;
        memcpy(pMsg->recvConfirm.confirm,
               &pPrvPdu[MESH_PRV_PDU_CONFIRM_CONFIRM_INDEX],
               MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE);
      }
      break;

    case MESH_PRV_PDU_RANDOM:
      if (pduLen != MESH_PRV_PDU_RANDOM_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV CL: Received invalid Provisioning Random PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      }
      else
      {
        pMsg->recvRandom.hdr.event = PRV_CL_EVT_RECV_RANDOM;
        memcpy(pMsg->recvRandom.random,
               &pPrvPdu[MESH_PRV_PDU_RANDOM_RANDOM_INDEX],
               MESH_PRV_PDU_RANDOM_RANDOM_SIZE);
      }
      break;

    case MESH_PRV_PDU_COMPLETE:
      if (pduLen != MESH_PRV_PDU_COMPLETE_PDU_SIZE)
      {
        MESH_TRACE_WARN1("MESH PRV CL: Received invalid Provisioning Complete PDU length: %d", pduLen);
        pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      }
      else
      {
        pMsg->hdr.event = PRV_CL_EVT_RECV_COMPLETE;
      }
      break;


    case MESH_PRV_PDU_FAILED:
      MESH_TRACE_WARN1("MESH PRV CL: Received Provisioning Failed PDU type: 0x%02X", pPrvPdu[0]);
      pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      break;

    case MESH_PRV_PDU_INVITE: /* Fallthrough */
    case MESH_PRV_PDU_START: /* Fallthrough */
    case MESH_PRV_PDU_DATA:
      MESH_TRACE_WARN1("MESH PRV CL: Received unexpected Provisioning PDU type: 0x%02X", pPrvPdu[0]);
      pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      break;

    default:
      MESH_TRACE_WARN1("MESH PRV CL: Received invalid Provisioning PDU type: 0x%02X", pPrvPdu[0]);
      pMsg->hdr.event = PRV_CL_EVT_BAD_PDU;
      break;
  }

  WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
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
static void meshPrvClBrEventNotifyCback(meshPrvBrEvent_t evt,
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
      pMsg->event = PRV_CL_EVT_LINK_OPENED;
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_LINK_FAILED:
      pMsg->event = PRV_CL_EVT_LINK_FAILED;
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_LINK_CLOSED_BY_PEER:
      if (pEvtParams->linkCloseReason == MESH_PRV_BR_REASON_SUCCESS)
      {
        pMsg->event = PRV_CL_EVT_LINK_CLOSED_SUCCESS;
      }
      else
      {
        pMsg->event = PRV_CL_EVT_LINK_CLOSED_FAIL;
      }
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_SEND_TIMEOUT:
      pMsg->event = PRV_CL_EVT_SEND_TIMEOUT;
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_CONN_CLOSED:
      pMsg->event = PRV_CL_EVT_LINK_CLOSED_FAIL;
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
      break;

    case MESH_PRV_BR_PDU_SENT:
      switch (pEvtParams->pduSentOpcode)
      {
        case MESH_PRV_PDU_INVITE:
          pMsg->event = PRV_CL_EVT_SENT_INVITE;
          WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_START:
          pMsg->event = PRV_CL_EVT_SENT_START;
          WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_PUB_KEY:
          pMsg->event = PRV_CL_EVT_SENT_PUBLIC_KEY;
          WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_CONFIRMATION:
          pMsg->event = PRV_CL_EVT_SENT_CONFIRMATION;
          WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_RANDOM:
          pMsg->event = PRV_CL_EVT_SENT_RANDOM;
          WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
          break;

        case MESH_PRV_PDU_DATA:
          pMsg->event = PRV_CL_EVT_SENT_DATA;
          WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
          break;

        default:
          MESH_TRACE_WARN0("MESH PRV CL: Received PDU Sent event with invalid opcode.");
          WsfMsgFree(pMsg);
          break;
      }
      break;

    default:
      MESH_TRACE_WARN1("MESH PRV CL: Received PRV BR event with invalid type: %d.", evt);
      WsfMsgFree(pMsg);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Empty event notification callback.
 *
 *  \param[in] pEvent  Pointer to the event.
 *                     See ::meshPrvClEvent_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvClEventNotifyEmptyCback(meshPrvClEvt_t *pEvent)
{
  (void)pEvent;
  MESH_TRACE_WARN0("MESH PRV CL: Event notification callback not installed!");
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Client.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvClInit(void)
{
  if (prvClInitialized)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Attempting multiple initialization sequences!");
    return;
  }

  /* Initialize timer event value */
  meshPrvClCb.timer.msg.event = PRV_CL_EVT_RECV_TIMEOUT;

  /* Link state machine instance */
  meshPrvClCb.pSm = &meshPrvClSmIf;

  /* Set empty callback */
  meshPrvClCb.prvClEvtNotifyCback = meshPrvClEventNotifyEmptyCback;

  /* Initialize empty session data */
  meshPrvClCb.pSessionData = NULL;

  /* Initialize the provisioning bearer module and register callbacks */
  MeshPrvBrInit();
  MeshPrvBrRegisterCback(meshPrvClPduRecvCback, meshPrvClBrEventNotifyCback);

  /* Set initial state */
  meshPrvClCb.state = PRV_CL_ST_IDLE;

  /* Set flag */
  prvClInitialized = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the WSF handler for the Provisioning Client.
 *
 *  \param[in] handlerId  WSF handler ID of the application using this service.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Store Handler ID */
  meshPrvClCb.timer.handlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF event handler for Mesh Provisioning Client API.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  /* Handle messages */
  if (pMsg != NULL)
  {
    meshPrvClSmExecute(&meshPrvClCb, (meshPrvClSmMsg_t*)pMsg);
  }
  /* Handle events */
  else if (event)
  {

  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Provisioning Client event callback function.
 *
 *  \param[in] evtCback  Pointer to the callback to be invoked on provisioning events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClRegister(meshPrvClEvtNotifyCback_t eventCback)
{
  if (eventCback != NULL)
  {
    meshPrvClCb.prvClEvtNotifyCback = eventCback;
  }
  else
  {
    MESH_TRACE_ERR0("MESH PRV CL: Attempting to install NULL event notification callback!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Starts the provisioning procedure over PB-ADV for the device with a given UUID.
 *
 *  \param[in] ifId          Interface id.
 *  \param[in] pSessionInfo  Pointer to static provisioning session information.
 *
 *  \remarks   The structure pointed by pSessionInfo shall be static for the provisioning session.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClStartPbAdvProvisioning(uint8_t ifId,
                                     meshPrvClSessionInfo_t const *pSessionInfo)
{
  meshPrvClStartPbAdv_t *pMsg;

  /* Check module is initialized */
  if (prvClInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Mesh Provisioning Client not initialized!");
    return;
  }

  /* Validate parameters */
  if (pSessionInfo == NULL ||
      pSessionInfo->pDeviceUuid == NULL ||
      pSessionInfo->pData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Invalid parameters in MeshPrvClStartPbAdvProvisioning!");
    return;
  }

  /* Check session data is not already allocated */
  if (meshPrvClCb.pSessionData != NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data already allocated!");
    return;
  }

  /* Allocate session data */
  meshPrvClCb.pSessionData = WsfBufAlloc(sizeof(meshPrvClSessionData_t));
  if (meshPrvClCb.pSessionData == NULL)
  {
    /* Should not happen if buffers are properly configured */
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshPrvClStartPbAdv_t))) != NULL)
  {
    /* Set event type and parameters. */
    pMsg->hdr.event = PRV_CL_EVT_BEGIN_NO_LINK;
    pMsg->ifId = ifId;
    pMsg->pSessionInfo = pSessionInfo;

    /* Send Message. */
    WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
  }
  else
  {
    /* Should not happen if buffers are properly configured. */
    WsfBufFree(meshPrvClCb.pSessionData);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Starts the provisioning procedure over PB-GATT for the device with a given UUID.
 *
 *  \param[in] connId        GATT connection id.
 *  \param[in] pSessionInfo  Pointer to static provisioning session information.
 *
 *  \remarks   The structure pointed by pSessionInfo shall be static for the provisioning session.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClStartPbGattProvisioning(uint8_t connId,
                                      meshPrvClSessionInfo_t const *pSessionInfo)
{
  meshPrvClStartPbGatt_t *pMsg;

  /* Check module is initialized */
  if (prvClInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Mesh Provisioning Client not initialized!");
    return;
  }

  /* Validate parameters */
  if (pSessionInfo == NULL ||
      pSessionInfo->pDeviceUuid == NULL ||
      pSessionInfo->pData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Invalid parameters in MeshPrvClStartPbAdvProvisioning!");
    return;
  }

  /* Check session data is not already allocated */
  if (meshPrvClCb.pSessionData != NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data already allocated!");
    return;
  }

  /* Allocate session data */
  meshPrvClCb.pSessionData = WsfBufAlloc(sizeof(meshPrvClSessionData_t));
  if (meshPrvClCb.pSessionData == NULL)
  {
    /* Should not happen if buffers are properly configured */
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshPrvClStartPbGatt_t))) != NULL)
  {
    /* Set event type and parameters. */
    pMsg->hdr.event = PRV_CL_EVT_BEGIN_LINK_OPEN;
    pMsg->connId = connId;
    pMsg->pSessionInfo = pSessionInfo;

    /* Send Message. */
    WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
  }
  else
  {
    /* Should not happen if buffers are properly configured. */
    WsfBufFree(meshPrvClCb.pSessionData);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Cancels any on-going provisioning procedure.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvClCancel(void)
{
  wsfMsgHdr_t *pMsg;

  if (prvClInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Mesh Provisioning Client not initialized!");
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
  {
    /* Set event type and parameters. */
    pMsg->event = PRV_CL_EVT_CANCEL;

    /* Send Message. */
    WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
  }
  /* Else should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Selects the authentication parameters to continue provisioning.
 *
 *  \param[in] pSelectAuth  Pointer to selected authentication parameters.
 *
 *  \return    None.
 *
 *  \remarks   This API shall be called when the MESH_PRV_CL_RECV_CAPABILITIES_EVENT
 *             event has been generated. The authentication parameters shall be
 *             set to valid values based on the received capabilities and on the
 *             availability of OOB public key and OOB static data.
 *             See ::meshPrvClSelectAuth_t.
 *             If invalid parameters are provided, the API call will be ignored
 *             and provisioning will timeout.
 */
/*************************************************************************************************/
void MeshPrvClSelectAuthentication(meshPrvClSelectAuth_t *pSelectAuth)
{
  meshPrvClSelAuthParam_t *pMsg;

  if (prvClInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Mesh Provisioning Client not initialized!");
    return;
  }

  /* Validate parameters */
  if (pSelectAuth == NULL || FALSE == meshPrvClValidateSelectAuthParams(pSelectAuth))
  {
    MESH_TRACE_ERR0("MESH PRV CL: Invalid parameters in MeshPrvClSelectAuthentication!");
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshPrvClSelAuthParam_t))) != NULL)
  {
    /* Set event type and parameters. */
    pMsg->hdr.event = PRV_CL_EVT_AUTH_SELECTED;
    memcpy(&pMsg->selectAuthParams, pSelectAuth, sizeof (meshPrvClSelectAuth_t));

    /* Send Message. */
    WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
  }
  /* Else should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Provisioning Client application calls this function when the
 *             MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT event has been generated and the user
 *             has input the data displayed by the device.
 *
 *  \param[in] outputOobSize  Size of alphanumeric Output OOB data, used only when the Output OOB
 *                            Action was MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM; otherwise,
 *                            the Output OOB data is numeric, and this parameter shall be set to 0.
 *
 *  \param[in] outputOobData  Array of outputOobSize octets containing the alphanumeric Output OOB
 *                            data, if the Output OOB action selected by the Provisioner was
 *                            MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM action; otherwise,
 *                            this is a numeric 8-octet value and outputOobSize is ignored.
 *
 *  \return    None.
 *
 */
/*************************************************************************************************/
void MeshPrvClEnterOutputOob(meshPrvOutputOobSize_t outputOobSize,
                             meshPrvInOutOobData_t  outputOobData)
{
  meshPrvClEnterOob_t *pMsg;

  if (prvClInitialized == FALSE)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Mesh Provisioning Client not initialized!");
    return;
  }

  /* Validate parameters */
  if (outputOobSize >= MESH_PRV_OUTPUT_OOB_SIZE_RFU_START ||
      (outputOobSize > 0 &&
       !meshPrvIsAlphanumericArray(outputOobData.alphanumericOob, outputOobSize)))
  {
    MESH_TRACE_ERR0("MESH PRV CL: Invalid parameters in MeshPrvClEnterOutputOob!");
    return;
  }

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshPrvClEnterOob_t))) != NULL)
  {
    /* Set event type and parameters. */
    pMsg->hdr.event = PRV_CL_EVT_INPUT_READY;
    pMsg->outputOobSize = outputOobSize;
    pMsg->outputOobData = outputOobData;

    /* Send Message. */
    WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
  }
  /* Else should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Provisioning Client callback event.
 *
 *  \param[in] pMeshPrvClEvt  Mesh Provisioning Client callback event.
 *
 *  \return    Size of Mesh Provisioning Client callback event.
 */
/*************************************************************************************************/
uint16_t MeshPrvClSizeOfEvt(meshPrvClEvt_t *pMeshPrvClEvt)
{
  uint16_t len;

  /* If a valid Provisioning Client event ID */
  if ((pMeshPrvClEvt->hdr.event == MESH_PRV_CL_EVENT) &&
      (pMeshPrvClEvt->hdr.param <= MESH_PRV_CL_MAX_EVENT))
  {
    len = meshPrvClEvtCbackLen[pMeshPrvClEvt->hdr.param];
  }
  else
  {
    len = sizeof(wsfMsgHdr_t);
  }

  return len;
}
