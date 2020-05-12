/*************************************************************************************************/
/*!
 *  \file   mesh_prv_cl_act.c
 *
 *  \brief  Mesh Provisioning Client state machine actions.
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
#include "wsf_buf.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "sec_api.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_types.h"
#include "mesh_local_config.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_cl_main.h"
#include "mesh_prv_cl_api.h"
#include "mesh_prv_br_main.h"
#include "mesh_prv_common.h"
#include "mesh_security_toolbox.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/
/*! Macros to identify the result of a Confirmation value computation */
#define CONFIRMATION_CBACK_ID_OWN    ((void*) 0)  /*! Identifies result of computing own Confirmation */
#define CONFIRMATION_CBACK_ID_PEER   ((void*) 1)  /*! Identifies result of computing peer Confirmation */

/*! Macros to identify the result of a Salt computation */
#define SALT_CBACK_ID_CONFIRMATION   ((void*) 0)  /*! Identifies result of computing ConfirmationSalt */
#define SALT_CBACK_ID_PROVISIONING   ((void*) 1)  /*! Identifies result of computing ProvisioningSalt */

/*! Macros to identify the result of a K1 computation */
#define K1_CBACK_ID_CONFIRMATION_KEY  ((void*) 0)  /*! Identifies result of computing ConfirmationKey */
#define K1_CBACK_ID_SESSION_KEY       ((void*) 1)  /*! Identifies result of computing SessionKey */
#define K1_CBACK_ID_SESSION_NONCE     ((void*) 2)  /*! Identifies result of computing SessionNonce */
#define K1_CBACK_ID_DEVICE_KEY        ((void*) 3)  /*! Identifies result of computing DeviceKey */

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! ECC keys generated internally */
static struct prvClInternalEccKeys_tag
{
  uint8_t publicX[MESH_SEC_TOOL_ECC_KEY_SIZE];
  uint8_t publicY[MESH_SEC_TOOL_ECC_KEY_SIZE];
  uint8_t private[MESH_SEC_TOOL_ECC_KEY_SIZE];
} prvClInternalEccKeys;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Callback invoked when a confirmation value has been computed.
 *
 *  \param[in] pCmacResult  Pointer to the result buffer.
 *  \param[in] pParam       Generic parameter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvClConfirmationCback(const uint8_t *pCmacResult, void *pParam)
{
  /* Check session data is allocated */
  if (meshPrvClCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL crypto callback!");
    return;
  }

  if (pParam == CONFIRMATION_CBACK_ID_OWN)
    /* Own confirmation has been computed */
  {
    MESH_TRACE_INFO0("MESH PRV CL: Own Confirmation value has been computed.");

    /* Send event  */
    meshPrvClOwnConfirm_t *pMsg = WsfMsgAlloc(sizeof (meshPrvClOwnConfirm_t));
    if (pMsg != NULL)
    {
      pMsg->hdr.event = PRV_CL_EVT_CONFIRMATION_READY;
      memcpy(pMsg->confirmation,
             pCmacResult,
             MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE);
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
    }
    /* Else should never happen if buffers are properly configured */
  }
  else if (pParam == CONFIRMATION_CBACK_ID_PEER)
    /* Peer confirmation has been computed */
  {
    /* Compare with the value received over the air */
    bool_t confirmationVerified = (memcmp(pCmacResult,
                                          meshPrvClCb.pSessionData->authParams.peerConfirmation,
                                          MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE) == 0x00);

    if (confirmationVerified)
    {
      MESH_TRACE_INFO0("MESH PRV CL: Peer Confirmation value was verified.");
    }
    else
    {
      MESH_TRACE_INFO0("MESH PRV CL: Peer Confirmation value was not verified!.");
    }

    /* Send event  */
    wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
    if (pMsg != NULL)
    {
      if (confirmationVerified)
      {
        pMsg->event = PRV_CL_EVT_CONFIRMATION_VERIFIED;
      }
      else
      {
        pMsg->event = PRV_CL_EVT_CONFIRMATION_FAILED;
      }
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
    }
    /* Else should never happen if buffers are properly configured */
  }
  /* Else ignore */
}

/*************************************************************************************************/
/*!
 *  \brief     Callback invoked when a K1 value has been computed.
 *
 *  \param[in] pResult     Pointer to the result buffer.
 *  \param[in] resultSize  Size of the result buffer.
 *  \param[in] pParam      Generic parameter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvClK1Cback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  (void)resultSize;
  WSF_ASSERT(resultSize == MESH_SEC_TOOL_AES_BLOCK_SIZE);

  /* Check session data is allocated */
  if (meshPrvClCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL crypto callback!");
    return;
  }

  if (pParam == K1_CBACK_ID_CONFIRMATION_KEY)
  {
    MESH_TRACE_INFO0("MESH PRV CL: ConfirmationKey has been computed.");

    /* Save ConfirmationKey value */
    memcpy(meshPrvClCb.pSessionData->authParams.confirmationKey, pResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Generate own Random */
    SecRand(meshPrvClCb.pSessionData->authParams.tempRandomAndAuthValue, MESH_PRV_PDU_RANDOM_RANDOM_SIZE);

    /* Save a copy of own Random for Session Key Calculation */
    memcpy(&meshPrvClCb.pSessionData->authParams.confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE],
           meshPrvClCb.pSessionData->authParams.tempRandomAndAuthValue,
           MESH_PRV_PDU_RANDOM_RANDOM_SIZE);


    /* Compute own Confirmation */
    MeshSecToolCmacCalculate(meshPrvClCb.pSessionData->authParams.confirmationKey,
                             meshPrvClCb.pSessionData->authParams.tempRandomAndAuthValue,
                             MESH_PRV_PDU_RANDOM_RANDOM_SIZE + MESH_PRV_AUTH_VALUE_SIZE,
                             meshPrvClConfirmationCback,
                             CONFIRMATION_CBACK_ID_OWN);
  }
  else if (pParam == K1_CBACK_ID_SESSION_KEY)
  {
    MESH_TRACE_INFO0("MESH PRV CL: SessionKey has been computed.");

    /* Save SessionKey value */
    memcpy(meshPrvClCb.pSessionData->authParams.sessionKey, pResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Compute SessionNonce */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_SESSION_NONCE_TEMP,
                              sizeof(MESH_PRV_SESSION_NONCE_TEMP) - 1,
                              meshPrvClCb.pSessionData->authParams.provisioningSalt,
                              meshPrvClCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvClK1Cback,
                              K1_CBACK_ID_SESSION_NONCE);
  }
  else if (pParam == K1_CBACK_ID_SESSION_NONCE)
  {
    MESH_TRACE_INFO0("MESH PRV CL: SessionNonce has been computed.");

    /* Save SessionNonce value - the 13 least significant octets of the result */
    memcpy(meshPrvClCb.pSessionData->authParams.sessionNonce,
           pResult + MESH_SEC_TOOL_AES_BLOCK_SIZE - MESH_PRV_SESSION_NONCE_SIZE,
           MESH_PRV_SESSION_NONCE_SIZE);

    /* Compute device key */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_DEVICE_KEY_TEMP,
                              sizeof(MESH_PRV_DEVICE_KEY_TEMP) - 1,
                              meshPrvClCb.pSessionData->authParams.provisioningSalt,
                              meshPrvClCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvClK1Cback,
                              K1_CBACK_ID_DEVICE_KEY);
  }
  else if (pParam == K1_CBACK_ID_DEVICE_KEY)
  {
    MESH_TRACE_INFO0("MESH PRV CL: DeviceKey has been computed.");

    /* Save device key */
    memcpy(meshPrvClCb.pSessionData->deviceKey, pResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Send event  */
    wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
    if (pMsg != NULL)
    {
      pMsg->event = PRV_CL_EVT_SESSION_KEY_READY;
      WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
    }
    /* Else should never happen if buffers are properly configured */
  }
  /* Else ignore */
}

/*************************************************************************************************/
/*!
 *  \brief     Callback invoked when the ECC keys have been generated.
 *
 *  \param[in] pEccKey  Pointer to structure containing the public/private key par.
 *                      See ::meshSecToolEccKey_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvClSecToolEccKeyGenCback(const uint8_t *pPubX,
                                           const uint8_t *pPubY,
                                           const uint8_t *pPriv)
{
  MESH_TRACE_INFO0("MESH PRV CL: ECC keys have been generated.");

  /* Save keys */
  memcpy(prvClInternalEccKeys.publicX, pPubX, MESH_SEC_TOOL_ECC_KEY_SIZE);
  memcpy(prvClInternalEccKeys.publicY, pPubY, MESH_SEC_TOOL_ECC_KEY_SIZE);
  memcpy(prvClInternalEccKeys.private, pPriv, MESH_SEC_TOOL_ECC_KEY_SIZE);

  /* Generate event */
  wsfMsgHdr_t* pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
  if (pEvtMsg != NULL)
  {
    pEvtMsg->event = PRV_CL_EVT_PUBLIC_KEY_GENERATED;
    WsfMsgSend(meshPrvClCb.timer.handlerId, pEvtMsg);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Callback invoked when the ECDH secret has been computed.
 *
 *  \param[in] isValid        TRUE if the peer ECC key is valid, FALSE otherwise.
 *  \param[in] pSharedSecret  Pointer to the result buffer containing the shared secret.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvClEcdhSecretCback(bool_t isValid, const uint8_t *pSharedSecret)
{
  MESH_TRACE_INFO0("MESH PRV CL: ECDH Secret has been computed.");

  /* Check session data is allocated */
  if (meshPrvClCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL crypto callback!");
    return;
  }

  if (isValid)
  {
    MESH_TRACE_INFO0("MESH PRV CL: Peer's public key is valid.");

    /* Save ECDH Secret value */
    memcpy(meshPrvClCb.pSessionData->ecdhSecret, pSharedSecret, MESH_SEC_TOOL_ECC_KEY_SIZE);
  }
  else
  {
    MESH_TRACE_INFO0("MESH PRV CL: Peer's public key is invalid.");
  }

  /* Send event  */
  wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
  if (pMsg != NULL)
  {
    if (isValid)
    {
      pMsg->event = PRV_CL_EVT_PUBLIC_KEY_VALID;
    }
    else
    {
      pMsg->event = PRV_CL_EVT_PUBLIC_KEY_INVALID;
    }
    WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
  }
  /* Else should never happen if buffers are properly configured */
}

/*************************************************************************************************/
/*!
 *  \brief     Callback invoked when a salt value has been computed.
 *
 *  \param[in] pCmacResult  Pointer to the result buffer.
 *  \param[in] pParam       Generic parameter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvClSaltCback(const uint8_t *pCmacResult, void *pParam)
{
  /* Check session data is allocated */
  if (meshPrvClCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL crypto callback!");
    return;
  }

  if (pParam == SALT_CBACK_ID_CONFIRMATION)
  {
    MESH_TRACE_INFO0("MESH PRV CL: ConfirmationSalt has been computed.");

    /* Save ConfirmationSalt value */
    memcpy(meshPrvClCb.pSessionData->authParams.confirmationSaltAndFinalRandoms, pCmacResult,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Compute ConfirmationKey */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_CONFIRMATION_KEY_TEMP,
                              sizeof(MESH_PRV_CONFIRMATION_KEY_TEMP) - 1,
                              meshPrvClCb.pSessionData->authParams.confirmationSaltAndFinalRandoms,
                              meshPrvClCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvClK1Cback,
                              K1_CBACK_ID_CONFIRMATION_KEY);
  }
  else if (pParam == SALT_CBACK_ID_PROVISIONING)
  {
    MESH_TRACE_INFO0("MESH PRV CL: ProvisioningSalt has been computed.");

    /* Save ProvisioningSalt value */
    memcpy(meshPrvClCb.pSessionData->authParams.provisioningSalt, pCmacResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Compute SessionKey */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_SESSION_KEY_TEMP,
                              sizeof(MESH_PRV_SESSION_KEY_TEMP) - 1,
                              meshPrvClCb.pSessionData->authParams.provisioningSalt,
                              meshPrvClCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvClK1Cback,
                              K1_CBACK_ID_SESSION_KEY);
  }
  /* Else ignore */
}

/*************************************************************************************************/
/*!
 *  \brief     Callback invoked when the provisioning data has been decrypted.
 *
 *  \param[in] pCcmResult  Pointer to the result buffer.
 *  \param[in] pParam      Generic parameter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvClDataEncryptCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
{
  (void)pCcmResult;
  (void)pParam;

  MESH_TRACE_INFO0("MESH PRV CL: Provisioning data has been successfully encrypted.");

  /* Send event  */
  wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
  if (pMsg != NULL)
  {
    pMsg->event = PRV_CL_EVT_DATA_ENCRYPTED;
    WsfMsgSend(meshPrvClCb.timer.handlerId, pMsg);
  }
  /* Else should never happen if buffers are properly configured */
}

/*************************************************************************************************/
/*!
 *  \brief     Returns the position of the only bit that is set in a 16-bit value.
 *
 *  \param[in] bitMask  16-bit value with one bit set.
 *
 *  \return    The position of the set bit.
 *
 *  \remarks   For a valid input, the function returns a value from 0 to 15.
 *             For an invalid input with no bit set, the function returns 16.
 *             For an invalid input with more than one bit set, the function returns the position
 *             of the least significant bit that is set.
 */
/*************************************************************************************************/
static uint8_t meshPrvClGetSetBitPosition(uint16_t bitMask)
{
  uint8_t j;

  for (j = 0; j < 16; j++)
  {
    if (bitMask & (1 << j))
    {
      break;
    }
  }

  return j;
}

/*************************************************************************************************/
/*!
 *  \brief  General cleanup when returning to IDLE.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvClCleanup(void)
{
  /* Stop provisioning protocol timer */
  WsfTimerStop(&meshPrvClCb.timer);

  /* Free session data buffer */
  if (meshPrvClCb.pSessionData != NULL)
  {
    WsfBufFree(meshPrvClCb.pSessionData);
    meshPrvClCb.pSessionData = NULL;
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     No action.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActNone(void *pCcb, void *pMsg)
{
  (void)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] No action on state change.");
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning when link opening failed.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActLinkFailed(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Terminate provisioning on link opening failed.");

  /* Notify upper layer */
  meshPrvClEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_LINK_NOT_ESTABLISHED;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning when link was closed.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActLinkClosed(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Terminate provisioning on link closing.");

  /* Notify upper layer */
  meshPrvClEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_LINK_CLOSED_BY_PEER;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning on protocol error.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActProtocolError(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Terminate provisioning on protocol error.");

  /* Notify upper layer */
  meshPrvClEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_PROTOCOL_ERROR;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Close link */
  MeshPrvBrCloseLink(MESH_PRV_BR_REASON_FAIL);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning when timeout has occured.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActRecvTimeout(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Terminate provisioning on PDU receive timeout.");

  /* Close bearer with Fail reason; the Timeout reason is only for PB-ADV Tx transactions */
  MeshPrvBrCloseLink(MESH_PRV_BR_REASON_FAIL);

  /* Notify upper layer */
  meshPrvClEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_RECEIVE_TIMEOUT;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning when unable to send a Provisioning PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSendTimeout(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Terminate provisioning when unable to send a Provisioning PDU.");

  /* Notify upper layer */
  meshPrvClEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_SEND_TIMEOUT;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning in success.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSuccess(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*) pCcb;

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  switch (((wsfMsgHdr_t*)pMsg)->event)
  {
    case PRV_CL_EVT_RECV_COMPLETE:
      MESH_TRACE_INFO0("MESH PRV CL: [ACT] Provisioning completed successfully.");
      break;

    case PRV_CL_EVT_LINK_CLOSED_SUCCESS:
      /* This should not happen, but even if it does, provisioning is successful. */
      MESH_TRACE_INFO0("MESH PRV CL: [ACT] Provisioning completed, but the device "
                       "unexpectedly closed the link with Success.");
      break;

    case PRV_CL_EVT_RECV_TIMEOUT:
      MESH_TRACE_INFO0("MESH PRV CL: [ACT] Provisioning completed, but Provisioner "
                       "did not receive the Provisioning Complete PDU.");
      break;

    default:
      MESH_TRACE_WARN1("MESH PRV CL: [ACT] Provisioning completed with an "
                       "unexpected event (0x%02X).", ((wsfMsgHdr_t*)pMsg)->event);
      break;
  }

  /* Close link */
  MeshPrvBrCloseLink(MESH_PRV_BR_REASON_SUCCESS);

  /* Trigger application event */
  meshPrvClEvtPrvComplete_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_COMPLETE_EVENT;
  evtMsg.hdr.status = MESH_SUCCESS;
  memcpy(evtMsg.uuid, pCl->pSessionInfo->pDeviceUuid, MESH_PRV_DEVICE_UUID_SIZE);
  evtMsg.address = pCl->pSessionInfo->pData->address;
  evtMsg.numOfElements = pCl->pSessionData->deviceCapab.numOfElements;
  memcpy(evtMsg.devKey, pCl->pSessionData->deviceKey, MESH_KEY_SIZE_128);
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     Open provisioning link.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActOpenLink(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*) pCcb;
  meshPrvClStartPbAdv_t* pEnterPbAdv = (meshPrvClStartPbAdv_t*) pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Enable PB-ADV bearer and open link.");

  /* Store session information pointer */
  pCl->pSessionInfo = pEnterPbAdv->pSessionInfo;

  /* Enable Provisioning Client */
  MeshPrvBrEnablePbAdvClient(pEnterPbAdv->ifId);

  /* Open link */
  MeshPrvBrOpenPbAdvLink(pCl->pSessionInfo->pDeviceUuid);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Invite PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSendInvite(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*) pCcb;
  meshPrvClSmMsg_t* pEvtMsg = (meshPrvClSmMsg_t*) pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Send Provisioning Invite PDU.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  if (pEvtMsg->hdr.event == PRV_CL_EVT_BEGIN_LINK_OPEN)
    /* We are running PB-GATT, need to perform session setup */
  {
    /* Store session information pointer */
    pCl->pSessionInfo = pEvtMsg->startPbGatt.pSessionInfo;

    /* Enable Provisioning Client */
    MeshPrvBrEnablePbGattClient(pEvtMsg->startPbGatt.connId);
  }
  else
    /* We are running PB-ADV, client has been enabled */
  {
    /* Notify upper layer that the link has been opened */
    wsfMsgHdr_t evtMsg;
    evtMsg.event = MESH_PRV_CL_EVENT;
    evtMsg.param = MESH_PRV_CL_LINK_OPENED_EVENT;
    evtMsg.status = MESH_SUCCESS;

    pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);
  }

  /* Allocate buffer for the Provisioning Invite PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_INVITE_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_INVITE;
    pBuf[MESH_PRV_PDU_INVITE_ATTENTION_INDEX] = pCl->pSessionInfo->attentionDuration;

    /* Copy parameters to the ConfirmationInputs */
    memcpy(pCl->pSessionData->authParams.confirmationInputs,
           &pBuf[MESH_PRV_PDU_PARAM_INDEX],
           MESH_PRV_PDU_INVITE_PARAM_SIZE);

    (void) MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_INVITE_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Capabilities PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitCapabilities(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*) pCcb;
  (void) pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Start waiting for Provisioning Capabilities PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pCl->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for user selection of authentication method.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitSelectAuth(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*) pCcb;
  meshPrvClRecvCapab_t* pRecvCapab = (meshPrvClRecvCapab_t*) pMsg;
  meshPrvClEvtRecvCapabilities_t recvCapabEvt;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Send capabilities to the application and wait for selection.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Copy parameters to the ConfirmationInputs */
  memcpy(&pCl->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE],
         &pRecvCapab->capabPdu[MESH_PRV_PDU_PARAM_INDEX],
         MESH_PRV_PDU_CAPAB_PARAM_SIZE);

  /* Unpack capabilities for upper layer event notification */
  recvCapabEvt.capabilities.numOfElements = pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_NUM_ELEM_INDEX];
  BYTES_BE_TO_UINT16(recvCapabEvt.capabilities.algorithms, &pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_ALGORITHMS_INDEX]);
  recvCapabEvt.capabilities.publicKeyType = pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_INDEX];
  recvCapabEvt.capabilities.staticOobType = pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_STATIC_OOB_INDEX];
  recvCapabEvt.capabilities.outputOobSize = pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_INDEX];
  BYTES_BE_TO_UINT16(recvCapabEvt.capabilities.outputOobAction, &pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_INDEX]);
  recvCapabEvt.capabilities.inputOobSize = pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_INDEX];
  BYTES_BE_TO_UINT16(recvCapabEvt.capabilities.inputOobAction, &pRecvCapab->capabPdu[MESH_PRV_PDU_CAPAB_IN_OOB_ACT_INDEX]);

  /* Save unpacked capabilities for later use */
  memcpy(&pCl->pSessionData->deviceCapab, &recvCapabEvt.capabilities, sizeof (meshPrvCapabilities_t));

  /* Notify upper layer that capabilities have been received */
  recvCapabEvt.hdr.event = MESH_PRV_CL_EVENT;
  recvCapabEvt.hdr.param = MESH_PRV_CL_RECV_CAPABILITIES_EVENT;
  recvCapabEvt.hdr.status = MESH_SUCCESS;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&recvCapabEvt);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Start PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSendStart(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  meshPrvClSelAuthParam_t* pSelectAuth = (meshPrvClSelAuthParam_t*)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Send Start PDU.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Save selected authentication parameters */
  memcpy(&meshPrvClCb.pSessionData->selectAuth,
         &pSelectAuth->selectAuthParams,
         sizeof (meshPrvClSelectAuth_t));

  /* Allocate buffer for the Provisioning Start PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_START_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_START;
    pBuf[MESH_PRV_PDU_START_ALGORITHM_INDEX] = MESH_PRV_START_ALGO_FIPS_P_256_EC;
    pBuf[MESH_PRV_PDU_START_PUB_KEY_INDEX] = pCl->pSessionData->selectAuth.useOobPublicKey ?
                                              MESH_PRV_START_PUB_KEY_OOB_AVAILABLE :
                                              MESH_PRV_START_PUB_KEY_OOB_NOT_AVAILABLE;
    pBuf[MESH_PRV_PDU_START_AUTH_METHOD_INDEX] = pCl->pSessionData->selectAuth.oobAuthMethod;
    switch (pCl->pSessionData->selectAuth.oobAuthMethod)
    {
      case MESH_PRV_CL_USE_INPUT_OOB:
        pBuf[MESH_PRV_PDU_START_AUTH_ACTION_INDEX] =
            meshPrvClGetSetBitPosition(pCl->pSessionData->selectAuth.oobAction.inputOobAction);
        break;

      case MESH_PRV_CL_USE_OUTPUT_OOB:
        pBuf[MESH_PRV_PDU_START_AUTH_ACTION_INDEX] =
            meshPrvClGetSetBitPosition(pCl->pSessionData->selectAuth.oobAction.outputOobAction);
        break;

      default:
        pBuf[MESH_PRV_PDU_START_AUTH_ACTION_INDEX] = 0x00;
        break;
    }
    pBuf[MESH_PRV_PDU_START_AUTH_SIZE_INDEX] = pCl->pSessionData->selectAuth.oobSize;

    /* Copy parameters to the ConfirmationInputs */
    memcpy(&pCl->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                             MESH_PRV_PDU_CAPAB_PARAM_SIZE],
           &pBuf[MESH_PRV_PDU_PARAM_INDEX],
           MESH_PRV_PDU_START_PARAM_SIZE);

    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_START_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Public Key PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSendPublicKey(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Send Public Key PDU.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Allocate buffer for the Provisioning Public Key PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_PUB_KEY_PDU_SIZE);
  if (pBuf != NULL)
  {
    uint8_t *pPubKeyX, *pPubKeyY;
    if (pCl->pSessionInfo->pAppEccKeys != NULL)
    {
      /* Use public keys provided by the application */
      pPubKeyX = pCl->pSessionInfo->pAppEccKeys->pPubKeyX;
      pPubKeyY = pCl->pSessionInfo->pAppEccKeys->pPubKeyY;
    }
    else
    {
      /* Use public keys generated internally */
      pPubKeyX = pCl->pSessionData->eccKeys.pPubKeyX;
      pPubKeyY = pCl->pSessionData->eccKeys.pPubKeyY;
    }

    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_PUB_KEY;
    memcpy(&pBuf[MESH_PRV_PDU_PUB_KEY_X_INDEX], pPubKeyX, MESH_PRV_PDU_PUB_KEY_X_SIZE);
    memcpy(&pBuf[MESH_PRV_PDU_PUB_KEY_Y_INDEX], pPubKeyY, MESH_PRV_PDU_PUB_KEY_Y_SIZE);

    /* Copy own public key to the ConfirmationInputs */
    memcpy(&pCl->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                             MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                             MESH_PRV_PDU_START_PARAM_SIZE],
           &pBuf[MESH_PRV_PDU_PARAM_INDEX],
           MESH_PRV_PDU_PUB_KEY_PARAM_SIZE);

    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_PUB_KEY_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Public Key PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitPublicKey(void *pCcb, void *pMsg)
{
  (void)pMsg;
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Start waiting for Provisioning Public Key PDU.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  if (!pCl->pSessionData->selectAuth.useOobPublicKey)
    /* Peer needs to send public key over-the-air */
  {
    /* Start transaction timer while waiting for a PDU */
    WsfTimerStartMs(&pCl->timer, MESH_PRV_TRAN_TIMEOUT_MS);
  }
  else
  {
    /* Peer's public key is available OOB. Simulate that the Public Key has been received. */
    MESH_TRACE_INFO0("MESH PRV CL: Public Key available OOB. Simulating Public Key Received event...");

    meshPrvClRecvPubKey_t* pEvtMsg = WsfMsgAlloc(sizeof(meshPrvClRecvPubKey_t));
    if (pEvtMsg != NULL)
    {
      pEvtMsg->hdr.event = PRV_CL_EVT_RECV_PUBLIC_KEY;
      memcpy(&pEvtMsg->pubKeyPdu[MESH_PRV_PDU_PUB_KEY_X_INDEX],
             pCl->pSessionInfo->pDevicePublicKey->pPubKeyX,
             MESH_PRV_PDU_PUB_KEY_X_SIZE);
      memcpy(&pEvtMsg->pubKeyPdu[MESH_PRV_PDU_PUB_KEY_Y_INDEX],
             pCl->pSessionInfo->pDevicePublicKey->pPubKeyY,
             MESH_PRV_PDU_PUB_KEY_Y_SIZE);
      WsfMsgSend(pCl->timer.handlerId, pEvtMsg);
    }
    /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Validate peer's Public Key by computing ECDH.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActValidatePublicKey(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  meshPrvClRecvPubKey_t* pRecvPubKey =(meshPrvClRecvPubKey_t*)pMsg;

  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Validate peer's Public Key by calculating ECDH.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Stop timer in case Provisioning Public Key PDU was received */
  WsfTimerStop(&pCl->timer);

  /* Copy peer public key to the ConfirmationInputs */
  memcpy(&pCl->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                           MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                           MESH_PRV_PDU_START_PARAM_SIZE +
                                                           MESH_PRV_PDU_PUB_KEY_PARAM_SIZE],
                                                    /* This is the public key of the Device,
                                                     * so it goes right after Provisioner's */
         &pRecvPubKey->pubKeyPdu[MESH_PRV_PDU_PARAM_INDEX],
         MESH_PRV_PDU_PUB_KEY_PARAM_SIZE);

  /* Compute ECDH Secret */
  uint8_t *pPeerPubX, *pPeerPubY, *pLocalPriv;
  pPeerPubX = &meshPrvClCb.pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                                       MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                                       MESH_PRV_PDU_START_PARAM_SIZE +
                                                                       MESH_PRV_PDU_PUB_KEY_PARAM_SIZE];
  pPeerPubY = &meshPrvClCb.pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                                       MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                                       MESH_PRV_PDU_START_PARAM_SIZE +
                                                                       MESH_PRV_PDU_PUB_KEY_PARAM_SIZE +
                                                                       MESH_SEC_TOOL_ECC_KEY_SIZE];
  if (meshPrvClCb.pSessionInfo->pAppEccKeys != NULL)
  {
    /* Use private key provided by the application */
    pLocalPriv = meshPrvClCb.pSessionInfo->pAppEccKeys->pPrivateKey;
  }
  else
  {
    /* Use private key generated internally */
    pLocalPriv = meshPrvClCb.pSessionData->eccKeys.pPrivateKey;
  }

  (void)MeshSecToolEccCompSharedSecret(pPeerPubX, pPeerPubY, pLocalPriv, meshPrvClEcdhSecretCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Generate own Public Key.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActGeneratePublicKey(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Generate own Public Key.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  if (pCl->pSessionInfo->pAppEccKeys == NULL)
    /* Application has not provided ECC keys */
  {
    /* Use the ECC keys generated by the stack */
    pCl->pSessionData->eccKeys.pPubKeyX = prvClInternalEccKeys.publicX;
    pCl->pSessionData->eccKeys.pPubKeyY = prvClInternalEccKeys.publicY;
    pCl->pSessionData->eccKeys.pPrivateKey = prvClInternalEccKeys.private;

    /* Generate the keys */
    (void)MeshSecToolEccGenerateKey(meshPrvClSecToolEccKeyGenCback);
  }
  else
    /* Application has provided ECC keys */
  {
    /* Use the ECC keys provided by the application */
    memcpy(&pCl->pSessionData->eccKeys, pCl->pSessionInfo->pAppEccKeys, sizeof(meshPrvEccKeys_t));

    /* Simulate that the Public Key has been generated */
    MESH_TRACE_INFO0("MESH PRV CL: Public Key provided by the application. Simulating PublicKeyGenerated event...");

    wsfMsgHdr_t* pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
    if (pEvtMsg != NULL)
    {
      pEvtMsg->event = PRV_CL_EVT_PUBLIC_KEY_GENERATED;
      WsfMsgSend(pCl->timer.handlerId, pEvtMsg);
    }
    /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Prepare OOB action.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActPrepareOobAction(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  wsfMsgHdr_t* pEvtMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Prepare OOB Action.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
  if (pEvtMsg == NULL)
  {
    return;
    /* Provisioning will fail on timeout - this should not happen if buffers are properly configured. */
  }

  switch (pCl->pSessionData->selectAuth.oobAuthMethod)
  {
    case MESH_PRV_START_AUTH_METHOD_OUTPUT_OOB:
      /* Go to WAIT_INPUT state. */
      MESH_TRACE_INFO0("MESH PRV CL: Authentication method is Output OOB. Changing to WAIT_INPUT...");
      pEvtMsg->event = PRV_CL_EVT_GOTO_WAIT_INPUT;
      WsfMsgSend(pCl->timer.handlerId, pEvtMsg);
      break;

    case MESH_PRV_START_AUTH_METHOD_INPUT_OOB:
      /* Go to WAIT_INPUT_COMPLETE */
      MESH_TRACE_INFO0("MESH PRV CL: Authentication method is Input OOB. Changing to WAIT_INPUT_COMPLETE...");
      pEvtMsg->event = PRV_CL_EVT_GOTO_WAIT_IC;
      WsfMsgSend(pCl->timer.handlerId, pEvtMsg);
      break;

    case MESH_PRV_START_AUTH_METHOD_NO_OOB:
      /* Set OOB data to 0 */
      memset(&pCl->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
             0x00,
             MESH_PRV_AUTH_VALUE_SIZE);

      /* Go to CALC_CONFIRMATION state. */
      MESH_TRACE_INFO0("MESH PRV CL: Authentication method is No OOB. Changing to CALC_CONFIRMATION...");
      pEvtMsg->event = PRV_CL_EVT_GOTO_CONFIRMATION;
      WsfMsgSend(pCl->timer.handlerId, pEvtMsg);
      break;

    case MESH_PRV_START_AUTH_METHOD_STATIC_OOB:
      if (pCl->pSessionInfo->pStaticOobData == NULL)
      {
        /* Should never get here; parameter check should catch this. Provisioning will fail on timeout. */
        WsfMsgFree(pEvtMsg);
        MESH_TRACE_ERR0("MESH PRV CL: Using Static OOB with a NULL OOB data pointer!");
      }
      else
      {
        /* Copy static OOB data */
        memcpy(&pCl->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
               pCl->pSessionInfo->pStaticOobData,
               MESH_PRV_AUTH_VALUE_SIZE);

        /* Go to CALC_CONFIRMATION state. */
        MESH_TRACE_INFO0("MESH PRV CL: Authentication method is Static OOB. Changing to CALC_CONFIRMATION...");
        pEvtMsg->event = PRV_CL_EVT_GOTO_CONFIRMATION;
        WsfMsgSend(pCl->timer.handlerId, pEvtMsg);
      }
      break;

    default:
      /* Should never get here; parameter check should catch this. Provisioning will fail on timeout. */
      WsfMsgFree(pEvtMsg);
      MESH_TRACE_ERR0("MESH PRV CL: Invalid authentication method!");
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning when peer's Public Key is invalid.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActPublicKeyInvalid(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] End provisioning when peer's Public Key is invalid.");

  /* Notify upper layer */
  meshPrvClEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_INVALID_PUBLIC_KEY;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Close link */
  MeshPrvBrCloseLink(MESH_PRV_BR_REASON_FAIL);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for user input.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitInput(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  meshPrvClEvt_t evtMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Wait for user input of Output OOB data.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Notify upper layer to input the Output OOB data */
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT;
  evtMsg.hdr.status = MESH_SUCCESS;
  evtMsg.enterOutputOob.outputOobAction = pCl->pSessionData->selectAuth.oobAction.outputOobAction;
  pCl->prvClEvtNotifyCback(&evtMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Input Complete PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitInputComplete(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  meshPrvClEvtDisplayInputOob_t evtMsg;
  uint8_t *pInput;
  uint32_t randomNumeric;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Display Input OOB data and wait for "
      "Provisioning Input Complete PDU.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Generate random Input OOB data */
  if (pCl->pSessionData->selectAuth.oobAction.inputOobAction == MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM)
  {
    /* Generate array of alphanumeric values, right-padded with zeros */
    pInput = &pCl->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE];
    meshPrvGenerateRandomAlphanumeric(pInput, pCl->pSessionData->selectAuth.oobSize);
    memset(pInput + pCl->pSessionData->selectAuth.oobSize,
           0x00,
           MESH_PRV_AUTH_VALUE_SIZE - pCl->pSessionData->selectAuth.oobSize);

    /* Copy to upper layer event parameter */
    evtMsg.inputOobSize = pCl->pSessionData->selectAuth.oobSize;
    memcpy(evtMsg.inputOobData.alphanumericOob, pInput, pCl->pSessionData->selectAuth.oobSize);
    if (pCl->pSessionData->selectAuth.oobSize < MESH_PRV_INOUT_OOB_MAX_SIZE)
    {
      memset(&evtMsg.inputOobData.alphanumericOob[pCl->pSessionData->selectAuth.oobSize],
             0x00,
             MESH_PRV_INOUT_OOB_MAX_SIZE - pCl->pSessionData->selectAuth.oobSize);
    }
  }
  else
  {
    /* Generate big-endian number, left-padded with zeros */
    pInput = &pCl->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE +
                                                                   MESH_PRV_AUTH_VALUE_SIZE -
                                                                   MESH_PRV_NUMERIC_OOB_SIZE_OCTETS];
    randomNumeric = meshPrvGenerateRandomNumeric(pCl->pSessionData->selectAuth.oobSize);
    UINT32_TO_BE_BUF(pInput, randomNumeric);
    memset(&pCl->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
           0x00,
           MESH_PRV_AUTH_VALUE_SIZE - MESH_PRV_NUMERIC_OOB_SIZE_OCTETS);

    /* Copy to upper layer event parameter */
    evtMsg.inputOobSize = 0;
    evtMsg.inputOobData.numericOob = randomNumeric;
  }

  /* Notify upper layer to display the Input OOB data */
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_DISPLAY_INPUT_OOB_EVENT;
  evtMsg.hdr.status = MESH_SUCCESS;
  evtMsg.inputOobAction = pCl->pSessionData->selectAuth.oobAction.inputOobAction;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pCl->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Calculate the provisioning confirmation.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActCalcConfirmation(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  meshPrvClEnterOob_t* pOob;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Calculate own provisioning confirmation value.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Stop timer in case Provisioning Input Complete PDU was received */
  WsfTimerStop(&pCl->timer);

  if (pCl->pSessionData->selectAuth.oobAuthMethod == MESH_PRV_CL_USE_OUTPUT_OOB)
  {
    pOob = (meshPrvClEnterOob_t*)pMsg;

    /* Save Output OOB data */
    meshPrvPackInOutOobToAuthArray(
        &pCl->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
        pOob->outputOobData,
        pOob->outputOobSize);
  }

  /* Calculate ConfirmationSalt = s1(ConfirmationInputs) */
  (void)MeshSecToolGenerateSalt(pCl->pSessionData->authParams.confirmationInputs,
                                MESH_PRV_CONFIRMATION_INPUTS_SIZE,
                                meshPrvClSaltCback,
                                SALT_CBACK_ID_CONFIRMATION);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Confirmation PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSendConfirmation(void *pCcb, void *pMsg)
{
  (void)pCcb;
  meshPrvClOwnConfirm_t* pConfirm = (meshPrvClOwnConfirm_t*)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Send Provisioning Confirmation PDU.");

  /* Allocate buffer for the Provisioning Confirmation PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_CONFIRM_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_CONFIRMATION;
    memcpy(&pBuf[MESH_PRV_PDU_CONFIRM_CONFIRM_INDEX], pConfirm->confirmation,
           MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE);
    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_CONFIRM_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Confirmation PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitConfirmation(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Start waiting for Provisioning Confirmation PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pCl->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Random PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSendRandom(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  meshPrvClRecvConfirm_t* pConfirm = (meshPrvClRecvConfirm_t*)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Send Provisioning Random PDU.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Stop timer because Provisioning Confirmation PDU was received */
  WsfTimerStop(&pCl->timer);

  /* Save peer Confirmation */
  memcpy(pCl->pSessionData->authParams.peerConfirmation, pConfirm->confirm, MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE);

  /* Allocate buffer for the Provisioning Random PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_RANDOM_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_RANDOM;
    memcpy(&pBuf[MESH_PRV_PDU_RANDOM_RANDOM_INDEX],
           &pCl->pSessionData->authParams.confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE],
           MESH_PRV_PDU_RANDOM_RANDOM_SIZE);
    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_RANDOM_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Random PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitRandom(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;

  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Start waiting for Provisioning Random PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pCl->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Check Confirmation.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActCheckConfirmation(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  meshPrvClRecvRandom_t* pRandom = (meshPrvClRecvRandom_t*)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Check peer's provisioning confirmation.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Stop timer because Provisioning Random PDU was received */
  WsfTimerStop(&pCl->timer);

  /* Overwrite own Random with peer Random for peer Confirmation calculation */
  memcpy(pCl->pSessionData->authParams.tempRandomAndAuthValue, pRandom->random, MESH_PRV_PDU_RANDOM_RANDOM_SIZE);

  /* Save a copy of peer Random for Session Key calculation */
  memcpy(&pCl->pSessionData->authParams.confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE +
                                                                        MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
         pRandom->random,
         MESH_PRV_PDU_RANDOM_RANDOM_SIZE);

  /* Compute peer Confirmation */
  MeshSecToolCmacCalculate(pCl->pSessionData->authParams.confirmationKey,
                           pCl->pSessionData->authParams.tempRandomAndAuthValue,
                           MESH_PRV_PDU_RANDOM_RANDOM_SIZE + MESH_PRV_AUTH_VALUE_SIZE,
                           meshPrvClConfirmationCback,
                           CONFIRMATION_CBACK_ID_PEER);
}

/*************************************************************************************************/
/*!
 *  \brief     Calculate Session Key.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActCalcSessionKey(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;

  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Calculate Session Key.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Calculate ProvisioningSalt = s1(ConfirmationSalt||RandomP||RandomD) */
  (void)MeshSecToolGenerateSalt(pCl->pSessionData->authParams.confirmationSaltAndFinalRandoms,
                                MESH_PRV_CONFIRMATION_SALT_SIZE + 2 * MESH_PRV_PDU_RANDOM_RANDOM_SIZE,
                                meshPrvClSaltCback,
                                SALT_CBACK_ID_PROVISIONING);
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning on confirmation failure.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActConfirmationFailed(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] End provisioning on confirmation failure");

  /* Notify upper layer */
  meshPrvClEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_CL_EVENT;
  evtMsg.hdr.param = MESH_PRV_CL_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_CONFIRMATION;
  pCl->prvClEvtNotifyCback((meshPrvClEvt_t*)&evtMsg);

  /* Close link */
  MeshPrvBrCloseLink(MESH_PRV_BR_REASON_FAIL);

  /* Perform general cleanup */
  meshPrvClCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     Encrypt the provisioning data.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActEncryptData(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Encrypt provisioning data.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Stop timer because Provisioning Data PDU was received */
  WsfTimerStop(&pCl->timer);

  /* Build encrypted data */
  memcpy(&pCl->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_NETKEY_INDEX],
         pCl->pSessionInfo->pData->pNetKey,
         MESH_KEY_SIZE_128);
  UINT16_TO_BE_BUF(&pCl->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_NETKEYIDX_INDEX],
                   pCl->pSessionInfo->pData->netKeyIndex);
  pCl->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_FLAGS_INDEX] = pCl->pSessionInfo->pData->flags;
  UINT32_TO_BE_BUF(&pCl->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_IVIDX_INDEX],
                   pCl->pSessionInfo->pData->ivIndex);
  UINT16_TO_BE_BUF(&pCl->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_ADDRESS_INDEX],
                   pCl->pSessionInfo->pData->address);

  /* CCM encryption parameters */
  meshSecToolCcmParams_t params;
  params.authDataLen = 0;
  params.pAuthData = NULL;
  params.cbcMacSize = MESH_PRV_PDU_DATA_MIC_SIZE;
  params.pCbcMac = &pCl->pSessionData->provisioningDataAndMic[MESH_PRV_PDU_DATA_ENC_DATA_SIZE];
  params.inputLen = MESH_PRV_PDU_DATA_ENC_DATA_SIZE;
  params.pIn = pCl->pSessionData->provisioningDataAndMic;
  params.pCcmKey = pCl->pSessionData->authParams.sessionKey;
  params.pNonce = pCl->pSessionData->authParams.sessionNonce;
  params.pOut = params.pIn; /* Overwrite the same location with encrypted data */

  (void)MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_ENCRYPT,
                                      &params,
                                      meshPrvClDataEncryptCback,
                                      NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Data PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActSendData(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Send Provisioning Data PDU.");

  /* Check session data is allocated */
  if (pCl->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV CL: Session data not allocated during PRV CL SM action!");
    return;
  }

  /* Allocate buffer for the Provisioning Data PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_DATA_PDU_SIZE);
  if (pBuf != NULL)
  {
    /* Send the Provisioning Data PDU */
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_DATA;
    memcpy(&pBuf[MESH_PRV_PDU_DATA_ENC_DATA_INDEX],
           &pCl->pSessionData->provisioningDataAndMic,
           MESH_PRV_PDU_DATA_PARAM_SIZE);
    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_DATA_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Complete PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClActWaitComplete(void *pCcb, void *pMsg)
{
  meshPrvClCb_t* pCl = (meshPrvClCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV CL: [ACT] Start waiting for Provisioning Complete PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pCl->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}
