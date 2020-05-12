/*************************************************************************************************/
/*!
 *  \file   mesh_prv_sr_act.c
 *
 *  \brief  Mesh Provisioning Server state machine actions.
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
#include "wsf_buf.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "sec_api.h"
#include "util/bstream.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_types.h"
#include "mesh_local_config.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_main.h"
#include "mesh_prv_sr_api.h"
#include "mesh_prv_br_main.h"
#include "mesh_prv_common.h"
#include "mesh_security_toolbox.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Macros to identify the result of a Confirmation value computation */
#define CONFIRMATION_CBACK_ID_OWN    ((void*) 0)  /* Identifies result of computing own Confirmation */
#define CONFIRMATION_CBACK_ID_PEER   ((void*) 1)  /* Identifies result of computing peer Confirmation */

/*! Macros to identify the result of a Salt computation */
#define SALT_CBACK_ID_CONFIRMATION   ((void*) 0)  /* Identifies result of computing ConfirmationSalt */
#define SALT_CBACK_ID_PROVISIONING   ((void*) 1)  /* Identifies result of computing ProvisioningSalt */

/*! Macros to identify the result of a K1 computation */
#define K1_CBACK_ID_CONFIRMATION_KEY  ((void*) 0)  /* Identifies result of computing ConfirmationKey */
#define K1_CBACK_ID_SESSION_KEY       ((void*) 1)  /* Identifies result of computing SessionKey */
#define K1_CBACK_ID_SESSION_NONCE     ((void*) 2)  /* Identifies result of computing SessionNonce */
#define K1_CBACK_ID_DEVICE_KEY        ((void*) 3)  /* Identifies result of computing DeviceKey */

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! ECC keys generated internally */
static struct prvSrInternalEccKeys_tag
{
  uint8_t publicX[MESH_SEC_TOOL_ECC_KEY_SIZE];
  uint8_t publicY[MESH_SEC_TOOL_ECC_KEY_SIZE];
  uint8_t private[MESH_SEC_TOOL_ECC_KEY_SIZE];
} prvSrInternalEccKeys;

#if ((defined MESH_PRV_SAMPLE_BUILD) && (MESH_PRV_SAMPLE_BUILD == 1))
/*! Device Random: 55a2a2bca04cd32ff6f346bd0a0c1a3a */
static uint8_t sampleRandom[MESH_PRV_PDU_RANDOM_RANDOM_SIZE] = {
  0x55,0xa2,0xa2,0xbc,0xa0,0x4c,0xd3,0x2f,0xf6,0xf3,0x46,0xbd,0x0a,0x0c,0x1a,0x3a };

/*! Device Capabilities: NumOfElements = 1, Algorithms = FIPS_P256_EC, all other 0 */
static uint8_t sampleCapabNumOfElements = 1;
static uint16_t sampleCapabAlgorithms = MESH_PRV_ALGO_FIPS_P256_ELLIPTIC_CURVE;
static uint8_t sampleCapabPublicKeyType = 0;
static uint8_t sampleCapabStaticOobType = 0;
static uint8_t sampleCapabOutputOobSize = 0;
static uint16_t sampleCapabOutputOobAction = 0;
static uint8_t sampleCapabInputOobSize = 0;
static uint16_t sampleCapabInputOobAction = 0;
#endif

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
static void meshPrvSrConfirmationCback(const uint8_t *pCmacResult, void *pParam)
{
  /* Check session data is allocated */
  if (meshPrvSrCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR crypto callback!");
    return;
  }

  if (pParam == CONFIRMATION_CBACK_ID_OWN)
    /* Own confirmation has been computed */
  {
    MESH_TRACE_INFO0("MESH PRV SR: Own Confirmation value has been computed.");

    /* Send event  */
    meshPrvSrOwnConfirm_t *pMsg = WsfMsgAlloc(sizeof (meshPrvSrOwnConfirm_t));
    if (pMsg != NULL)
    {
      pMsg->hdr.event = PRV_SR_EVT_CONFIRMATION_READY;
      memcpy(pMsg->confirmation,
             pCmacResult,
             MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE);
      WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
    }
    /* Else should never happen if buffers are properly configured */
  }
  else if (pParam == CONFIRMATION_CBACK_ID_PEER)
    /* Peer confirmation has been computed */
  {
    /* Compare with the value received over the air */
    bool_t confirmationVerified = (memcmp(pCmacResult,
                                          meshPrvSrCb.pSessionData->authParams.peerConfirmation,
                                          MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE) == 0x00);

    if (confirmationVerified)
    {
      MESH_TRACE_INFO0("MESH PRV SR: Peer Confirmation value was verified.");

    }
    else
    {
      MESH_TRACE_INFO0("MESH PRV SR: Peer Confirmation value was not verified!.");
    }

    /* Send event  */
    wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
    if (pMsg != NULL)
    {
      if (confirmationVerified)
      {
        pMsg->event = PRV_SR_EVT_CONFIRMATION_VERIFIED;
      }
      else
      {
        pMsg->event = PRV_SR_EVT_CONFIRMATION_FAILED;
        pMsg->param = MESH_PRV_ERR_CONFIRMATION_FAILED;
      }
      WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
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
static void meshPrvSrK1Cback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  (void)resultSize;
  WSF_ASSERT(resultSize == MESH_SEC_TOOL_AES_BLOCK_SIZE);

  /* Check session data is allocated */
  if (meshPrvSrCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR crypto callback!");
    return;
  }

  if (pParam == K1_CBACK_ID_CONFIRMATION_KEY)
  {
    MESH_TRACE_INFO0("MESH PRV SR: ConfirmationKey has been computed.");

    /* Save ConfirmationKey value */
    memcpy(meshPrvSrCb.pSessionData->authParams.confirmationKey, pResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

#if ((defined MESH_PRV_SAMPLE_BUILD) && (MESH_PRV_SAMPLE_BUILD == 1))
    /* Use device Random from sample data */
    memcpy(meshPrvSrCb.pSessionData->authParams.tempRandomAndAuthValue, sampleRandom, MESH_PRV_PDU_RANDOM_RANDOM_SIZE);
#else
    /* Generate own Random */
    SecRand(meshPrvSrCb.pSessionData->authParams.tempRandomAndAuthValue, MESH_PRV_PDU_RANDOM_RANDOM_SIZE);
#endif /* MESH_PRV_SAMPLE_BUILD */

    /* Save a copy of own Random for Session Key calculation */
    memcpy(&meshPrvSrCb.pSessionData->authParams.confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE +
                                                                                 MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
           meshPrvSrCb.pSessionData->authParams.tempRandomAndAuthValue,
           MESH_PRV_PDU_RANDOM_RANDOM_SIZE);

    /* Compute own Confirmation */
    MeshSecToolCmacCalculate(meshPrvSrCb.pSessionData->authParams.confirmationKey,
                             meshPrvSrCb.pSessionData->authParams.tempRandomAndAuthValue,
                             MESH_PRV_PDU_RANDOM_RANDOM_SIZE + MESH_PRV_AUTH_VALUE_SIZE,
                             meshPrvSrConfirmationCback,
                             CONFIRMATION_CBACK_ID_OWN);
  }
  else if (pParam == K1_CBACK_ID_SESSION_KEY)
  {
    MESH_TRACE_INFO0("MESH PRV SR: SessionKey has been computed.");

    /* Save SessionKey value */
    memcpy(meshPrvSrCb.pSessionData->authParams.sessionKey, pResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Compute SessionNonce */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_SESSION_NONCE_TEMP,
                              sizeof(MESH_PRV_SESSION_NONCE_TEMP) - 1,
                              meshPrvSrCb.pSessionData->authParams.provisioningSalt,
                              meshPrvSrCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvSrK1Cback,
                              K1_CBACK_ID_SESSION_NONCE);
  }
  else if (pParam == K1_CBACK_ID_SESSION_NONCE)
  {
    MESH_TRACE_INFO0("MESH PRV SR: SessionNonce has been computed.");

    /* Save SessionNonce value - the 13 least significant octets of the result */
    memcpy(meshPrvSrCb.pSessionData->authParams.sessionNonce,
           pResult + MESH_SEC_TOOL_AES_BLOCK_SIZE - MESH_PRV_SESSION_NONCE_SIZE,
           MESH_PRV_SESSION_NONCE_SIZE);

    /* Compute device key */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_DEVICE_KEY_TEMP,
                              sizeof(MESH_PRV_DEVICE_KEY_TEMP) - 1,
                              meshPrvSrCb.pSessionData->authParams.provisioningSalt,
                              meshPrvSrCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvSrK1Cback,
                              K1_CBACK_ID_DEVICE_KEY);
  }
  else if (pParam == K1_CBACK_ID_DEVICE_KEY)
  {
    MESH_TRACE_INFO0("MESH PRV SR: DeviceKey has been computed.");

    /* Save device key */
    memcpy(meshPrvSrCb.pSessionData->deviceKey, pResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Send event  */
    wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
    if (pMsg != NULL)
    {
      pMsg->event = PRV_SR_EVT_SESSION_KEY_READY;
      WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
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
static void meshPrvSrSecToolEccKeyGenCback(const uint8_t *pPubX,
                                           const uint8_t *pPubY,
                                           const uint8_t *pPriv)
{
  MESH_TRACE_INFO0("MESH PRV SR: ECC keys have been generated.");

  /* Save keys */
  memcpy(prvSrInternalEccKeys.publicX, pPubX, MESH_SEC_TOOL_ECC_KEY_SIZE);
  memcpy(prvSrInternalEccKeys.publicY, pPubY, MESH_SEC_TOOL_ECC_KEY_SIZE);
  memcpy(prvSrInternalEccKeys.private, pPriv, MESH_SEC_TOOL_ECC_KEY_SIZE);

  /* Generate event */
  wsfMsgHdr_t* pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
  if (pEvtMsg != NULL)
  {
    pEvtMsg->event = PRV_SR_EVT_PUBLIC_KEY_GENERATED;
    WsfMsgSend(meshPrvSrCb.timer.handlerId, pEvtMsg);
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
static void meshPrvSrEcdhSecretCback(bool_t isValid, const uint8_t *pSharedSecret)
{
  MESH_TRACE_INFO0("MESH PRV SR: ECDH Secret has been computed.");

  /* Check session data is allocated */
  if (meshPrvSrCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR crypto callback!");
    return;
  }

  if (isValid)
  {
    MESH_TRACE_INFO0("MESH PRV SR: Peer's public key is valid.");
    /* Save ECDH Secret value */
    memcpy(meshPrvSrCb.pSessionData->ecdhSecret, pSharedSecret, MESH_SEC_TOOL_ECC_KEY_SIZE);
  }
  else
  {
    MESH_TRACE_INFO0("MESH PRV SR: Peer's public key is invalid.");
  }

  /* Send event */
  wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
  if (pMsg != NULL)
  {
    if (isValid)
    {
      pMsg->event = PRV_SR_EVT_PUBLIC_KEY_VALID;
    }
    else
    {
      pMsg->event = PRV_SR_EVT_PUBLIC_KEY_INVALID;
      pMsg->param = MESH_PRV_ERR_INVALID_FORMAT;
    }
    WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
  }
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
static void meshPrvSrSaltCback(const uint8_t *pCmacResult, void *pParam)
{
  /* Check session data is allocated */
  if (meshPrvSrCb.pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR crypto callback!");
    return;
  }

  if (pParam == SALT_CBACK_ID_CONFIRMATION)
  {
    MESH_TRACE_INFO0("MESH PRV SR: ConfirmationSalt has been computed.");

    /* Save ConfirmationSalt value */
    memcpy(meshPrvSrCb.pSessionData->authParams.confirmationSaltAndFinalRandoms, pCmacResult,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Compute ConfirmationKey */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_CONFIRMATION_KEY_TEMP,
                              sizeof(MESH_PRV_CONFIRMATION_KEY_TEMP) - 1,
                              meshPrvSrCb.pSessionData->authParams.confirmationSaltAndFinalRandoms,
                              meshPrvSrCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvSrK1Cback,
                              K1_CBACK_ID_CONFIRMATION_KEY);
  }
  else if (pParam == SALT_CBACK_ID_PROVISIONING)
  {
    MESH_TRACE_INFO0("MESH PRV SR: ProvisioningSalt has been computed.");

    /* Save ProvisioningSalt value */
    memcpy(meshPrvSrCb.pSessionData->authParams.provisioningSalt, pCmacResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Compute SessionKey */
    (void)MeshSecToolK1Derive((uint8_t*)MESH_PRV_SESSION_KEY_TEMP,
                              sizeof(MESH_PRV_SESSION_KEY_TEMP) - 1,
                              meshPrvSrCb.pSessionData->authParams.provisioningSalt,
                              meshPrvSrCb.pSessionData->ecdhSecret,
                              MESH_SEC_TOOL_ECC_KEY_SIZE,
                              meshPrvSrK1Cback,
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
static void meshPrvSrDataDecryptCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
{
  (void)pParam;

  if (pCcmResult->results.decryptResult.isAuthSuccess == TRUE)
  {
    MESH_TRACE_INFO0("MESH PRV SR: Provisioning data has been successfully decrypted.");
  }
  else
  {
    MESH_TRACE_INFO0("MESH PRV SR: Provisioning data could not be decrypted.");
  }

  /* Send event  */
  wsfMsgHdr_t *pMsg = WsfMsgAlloc(sizeof (wsfMsgHdr_t));
  if (pMsg != NULL)
  {
    if (pCcmResult->results.decryptResult.isAuthSuccess == TRUE)
    {
      pMsg->event = PRV_SR_EVT_DATA_DECRYPTED;
    }
    else
    {
      pMsg->event = PRV_SR_EVT_DATA_NOT_DECRYPTED;
      pMsg->param = MESH_PRV_ERR_DECRYPTION_FAILED;
    }
    WsfMsgSend(meshPrvSrCb.timer.handlerId, pMsg);
  }
  /* Else should never happen if buffers are properly configured */
}

/*************************************************************************************************/
/*!
 *  \brief  General cleanup when returning to IDLE.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvSrCleanup(void)
{
  /* Stop provisioning protocol timer */
  WsfTimerStop(&meshPrvSrCb.timer);

  /* Reset attention timer state for the primary element */
  MeshLocalCfgSetAttentionTimer(0, 0);

  /* Free session data buffer */
  if (meshPrvSrCb.pSessionData != NULL)
  {
    WsfBufFree(meshPrvSrCb.pSessionData);
    meshPrvSrCb.pSessionData = NULL;
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
void meshPrvSrActNone(void *pCcb, void *pMsg)
{
  (void)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] No action on state change.");
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
void meshPrvSrActLinkClosed(void *pCcb, void *pMsg)
{
  (void)pMsg;
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;

  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Terminate provisioning on link closing.");

  /* Perform general cleanup */
  meshPrvSrCleanup();

  /* Notify upper layer */
  meshPrvSrEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_SR_EVENT;
  evtMsg.hdr.param = MESH_PRV_SR_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_LINK_CLOSED_BY_PEER;
  pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     End provisioning when timeout has occurred.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActRecvTimeout(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Terminate provisioning on PDU receive timeout.");

  /* Close link silently. */
  MeshPrvBrCloseLinkSilent();

  /* Perform general cleanup */
  meshPrvSrCleanup();

  /* Notify upper layer */
  meshPrvSrEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_SR_EVENT;
  evtMsg.hdr.param = MESH_PRV_SR_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_RECEIVE_TIMEOUT;
  pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);
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
void meshPrvSrActSendTimeout(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Terminate provisioning when unable to send a Provisioning PDU.");

  /* Perform general cleanup */
  meshPrvSrCleanup();

  /* Notify upper layer */
  meshPrvSrEvtPrvFailed_t evtMsg;
  evtMsg.hdr.event = MESH_PRV_SR_EVENT;
  evtMsg.hdr.param = MESH_PRV_SR_PROVISIONING_FAILED_EVENT;
  evtMsg.hdr.status = MESH_FAILURE;
  evtMsg.reason = MESH_PRV_FAIL_SEND_TIMEOUT;
  pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);
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
void meshPrvSrActSuccess(void *pCcb, void *pMsg)
{
  (void)pCcb;
  switch (((wsfMsgHdr_t*)pMsg)->event)
  {
    case PRV_SR_EVT_SENT_COMPLETE: /* Fallthrough */
    case PRV_SR_EVT_LINK_CLOSED_SUCCESS:
      MESH_TRACE_INFO0("MESH PRV SR: [ACT] Provisioning completed successfully.");
      break;

    case PRV_SR_EVT_SEND_TIMEOUT:
      MESH_TRACE_INFO0("MESH PRV SR: [ACT] Provisioning completed, but Provisioner \
                        did not acknowledge the Provisioning Complete PDU.");
      break;

    default:
      MESH_TRACE_WARN1("MESH PRV SR: [ACT] Provisioning completed with an \
                        unexpected event (0x%02X).", ((wsfMsgHdr_t*)pMsg)->event);
      break;
  }

  /* Perform general cleanup */
  meshPrvSrCleanup();
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Link Opened event.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActWaitLink(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*) pCcb;
  meshPrvSrEnterPbAdv_t* pEnterPbAdv = (meshPrvSrEnterPbAdv_t*) pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Enable PB-ADV bearer and start waiting for link.");

  /* Enable PB-ADV Server */
  MeshPrvBrEnablePbAdvServer(pEnterPbAdv->ifId,
                             ((meshPrvSrEnterPbAdv_t *)pMsg)->beaconInterval,
                             pSr->pUpdInfo->pDeviceUuid,
                             pSr->pUpdInfo->oobInfoSrc,
                             (uint8_t*)pSr->pUpdInfo->pUriData,
                             pSr->pUpdInfo->uriLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Invite PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActWaitInvite(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*) pCcb;
  meshPrvSrSmMsg_t* pEvtMsg = (meshPrvSrSmMsg_t*) pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Start waiting for Provisioning Invite PDU.");

  if (pEvtMsg->hdr.event == PRV_SR_EVT_BEGIN_LINK_OPEN)
    /* We are running PB-GATT, need to perform session setup */
  {
    /* Enable PB-GATT Server */
    MeshPrvBrEnablePbGattServer(pEvtMsg->enterPbGatt.connId);
  }
  else
    /* We are running PB-ADV, server has been enabled */
  {
     /* Notify upper layer that the link has been opened */
    wsfMsgHdr_t evtMsg;
    evtMsg.event = MESH_PRV_SR_EVENT;
    evtMsg.param = MESH_PRV_SR_LINK_OPENED_EVENT;
    evtMsg.status = MESH_SUCCESS;
    pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);
  }

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pSr->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Capabilities PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActSendCapabilities(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*) pCcb;
  meshPrvSrRecvInvite_t* pInvite = (meshPrvSrRecvInvite_t*) pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send Provisioning Capabilities PDU.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Stop timer because Provisioning Invite PDU was received */
  WsfTimerStop(&pSr->timer);

  if (pInvite->attentionTimer > 0)
  {
    /* Set attention timer state for the primary element */
    MeshLocalCfgSetAttentionTimer(0, pInvite->attentionTimer);
  }

  /* Allocate buffer for the Provisioning Capabilities PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_CAPAB_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_CAPABILITIES;
#if ((defined MESH_PRV_SAMPLE_BUILD) && (MESH_PRV_SAMPLE_BUILD == 1))
    pBuf[MESH_PRV_PDU_CAPAB_NUM_ELEM_INDEX] = sampleCapabNumOfElements;
    UINT16_TO_BE_BUF(&pBuf[MESH_PRV_PDU_CAPAB_ALGORITHMS_INDEX], sampleCapabAlgorithms);
    pBuf[MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_INDEX] = sampleCapabPublicKeyType;
    pBuf[MESH_PRV_PDU_CAPAB_STATIC_OOB_INDEX] = sampleCapabStaticOobType;
    pBuf[MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_INDEX] = sampleCapabOutputOobSize;
    UINT16_TO_BE_BUF(&pBuf[MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_INDEX], sampleCapabOutputOobAction);
    pBuf[MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_INDEX] = sampleCapabInputOobSize;
    UINT16_TO_BE_BUF(&pBuf[MESH_PRV_PDU_CAPAB_IN_OOB_ACT_INDEX], sampleCapabInputOobAction);
#else
    pBuf[MESH_PRV_PDU_CAPAB_NUM_ELEM_INDEX] = pSr->pUpdInfo->pCapabilities->numOfElements;
    UINT16_TO_BE_BUF(&pBuf[MESH_PRV_PDU_CAPAB_ALGORITHMS_INDEX], pSr->pUpdInfo->pCapabilities->algorithms);
    pBuf[MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_INDEX] = pSr->pUpdInfo->pCapabilities->publicKeyType;
    pBuf[MESH_PRV_PDU_CAPAB_STATIC_OOB_INDEX] = pSr->pUpdInfo->pCapabilities->staticOobType;
    pBuf[MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_INDEX] = pSr->pUpdInfo->pCapabilities->outputOobSize;
    UINT16_TO_BE_BUF(&pBuf[MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_INDEX], pSr->pUpdInfo->pCapabilities->outputOobAction);
    pBuf[MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_INDEX] = pSr->pUpdInfo->pCapabilities->inputOobSize;
    UINT16_TO_BE_BUF(&pBuf[MESH_PRV_PDU_CAPAB_IN_OOB_ACT_INDEX], pSr->pUpdInfo->pCapabilities->inputOobAction);
#endif /* MESH_PRV_SAMPLE_BUILD */

    /* Copy parameters to the ConfirmationInputs */
    memcpy(&pSr->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE],
           &pBuf[MESH_PRV_PDU_PARAM_INDEX],
           MESH_PRV_PDU_CAPAB_PARAM_SIZE);

    (void) MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_CAPAB_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Start PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActWaitStart(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Start waiting for Provisioning Start PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pSr->timer, MESH_PRV_TRAN_TIMEOUT_MS);
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
void meshPrvSrActWaitPublicKey(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  meshPrvSrRecvStart_t* pStart = (meshPrvSrRecvStart_t*)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Start waiting for Provisioning Public Key PDU.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Mark peer confirmation as not received */
  pSr->pSessionData->authParams.peerConfirmationReceived = FALSE;

  /* Copy packed parameters to the ConfirmationInputs */
  memcpy(&pSr->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                           MESH_PRV_PDU_CAPAB_PARAM_SIZE],
         pStart->packedPduParam,
         MESH_PRV_PDU_START_PARAM_SIZE);

  /* Store relevant parameters from the Provisioning Start PDU */
  pSr->pSessionData->startParams.oobPublicKey = (bool_t)pStart->oobPubKeyUsed;
  pSr->pSessionData->startParams.authMethod = pStart->authMethod;
  pSr->pSessionData->startParams.authAction = pStart->authAction;
  pSr->pSessionData->startParams.authSize = pStart->authSize;
  if (pSr->pSessionData->startParams.authSize > MESH_PRV_MAX_OOB_SIZE)
  {
    /* Should never happen, but just in case */
    pSr->pSessionData->startParams.authSize = MESH_PRV_MAX_OOB_SIZE;
  }

  /* Check for invalid state */
  if ((pStart->oobPubKeyUsed == TRUE) && (pSr->pUpdInfo->pAppOobEccKeys == NULL))
  {
    meshPrvSrSmMsg_t *pNewMsg;

    pNewMsg = WsfMsgAlloc(sizeof (meshPrvSrSmMsg_t));

    if (pMsg != NULL)
    {
      pNewMsg->hdr.event = PRV_SR_EVT_RECV_BAD_PDU;
      pNewMsg->hdr.param = MESH_PRV_ERR_INVALID_FORMAT;
        WsfMsgSend(meshPrvSrCb.timer.handlerId, pNewMsg);
    }
  }
  else
  {
    /* Start transaction timer while waiting for a PDU */
    WsfTimerStartMs(&pSr->timer, MESH_PRV_TRAN_TIMEOUT_MS);
  }
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
void meshPrvSrActGeneratePublicKey(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  meshPrvSrRecvPubKey_t* pRecvPubKey = (meshPrvSrRecvPubKey_t*)pMsg;

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Stop timer because Provisioning Public Key PDU was received */
  WsfTimerStop(&pSr->timer);

  /* Copy peer public key to the ConfirmationInputs */
  memcpy(&pSr->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                           MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                           MESH_PRV_PDU_START_PARAM_SIZE],
                                                   /* This is the public key of the Provisioner,
                                                    * so it goes right after Start */
         &pRecvPubKey->pubKeyPdu[MESH_PRV_PDU_PARAM_INDEX],
         MESH_PRV_PDU_PUB_KEY_PARAM_SIZE);

  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Generate own Public Key.");

  if (pSr->pUpdInfo->pAppOobEccKeys == NULL)
    /* Application has not provided ECC keys */
  {
    /* Use the ECC keys generated by the stack */
    pSr->pSessionData->eccKeys.pPubKeyX = prvSrInternalEccKeys.publicX;
    pSr->pSessionData->eccKeys.pPubKeyY = prvSrInternalEccKeys.publicY;
    pSr->pSessionData->eccKeys.pPrivateKey = prvSrInternalEccKeys.private;

    /* Generate the keys */
    (void)MeshSecToolEccGenerateKey(meshPrvSrSecToolEccKeyGenCback);
  }
  else
    /* Application has provided ECC keys */
  {
    /* Use the ECC keys provided by the application */
    memcpy(&pSr->pSessionData->eccKeys, pSr->pUpdInfo->pAppOobEccKeys, sizeof(meshPrvEccKeys_t));

    /* Simulate that the Public Key has been generated */
    MESH_TRACE_INFO0("MESH PRV SR: Public Key provided by the application. Simulating PublicKeyGenerated event...");

    wsfMsgHdr_t* pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
    if (pEvtMsg != NULL)
    {
      pEvtMsg->event = PRV_SR_EVT_PUBLIC_KEY_GENERATED;
      WsfMsgSend(pSr->timer.handlerId, pEvtMsg);
    }
    /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Validate peer's Public Key by computing ECDH.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void meshPrvSrActValidatePublicKey(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Validate peer's Public Key by computing ECDH.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }
  /* Compute ECDH Secret */
  uint8_t *pPeerPubX, *pPeerPubY, *pLocalPriv;
  pPeerPubX = &meshPrvSrCb.pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                                       MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                                       MESH_PRV_PDU_START_PARAM_SIZE];
  pPeerPubY = &meshPrvSrCb.pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                                       MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                                       MESH_PRV_PDU_START_PARAM_SIZE +
                                                                       MESH_SEC_TOOL_ECC_KEY_SIZE];
  pLocalPriv = meshPrvSrCb.pSessionData->eccKeys.pPrivateKey;

  (void)MeshSecToolEccCompSharedSecret(pPeerPubX, pPeerPubY, pLocalPriv, meshPrvSrEcdhSecretCback);
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
void meshPrvSrActSendPublicKey(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send Public Key PDU.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  if (pSr->pSessionData->startParams.oobPublicKey == FALSE)
    /* Public Key of the Server is not available OOB at the Client side */
  {
    /* Allocate buffer for the Provisioning Public Key PDU */
    pBuf = WsfBufAlloc(MESH_PRV_PDU_PUB_KEY_PDU_SIZE);
    if (pBuf != NULL)
    {
      pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_PUB_KEY;
      memcpy(&pBuf[MESH_PRV_PDU_PUB_KEY_X_INDEX], pSr->pSessionData->eccKeys.pPubKeyX, MESH_PRV_PDU_PUB_KEY_X_SIZE);
      memcpy(&pBuf[MESH_PRV_PDU_PUB_KEY_Y_INDEX], pSr->pSessionData->eccKeys.pPubKeyY, MESH_PRV_PDU_PUB_KEY_Y_SIZE);

      /* Copy own public key to the ConfirmationInputs */
      memcpy(&pSr->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                               MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                               MESH_PRV_PDU_START_PARAM_SIZE +
                                                               MESH_PRV_PDU_PUB_KEY_PARAM_SIZE],
             &pBuf[MESH_PRV_PDU_PARAM_INDEX],
             MESH_PRV_PDU_PUB_KEY_PARAM_SIZE);

      (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_PUB_KEY_PDU_SIZE);
    }
    /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
  }
  else
    /* Public Key of the Server is available OOB at the Client side */
  {
    /* Simulate that the Public Key has been delivered */
    MESH_TRACE_INFO0("MESH PRV SR: Public Key available OOB at Client-side. Simulating SentPublicKey event...");

    uint8_t *pPubKeyX, *pPubKeyY;

    pPubKeyX = pSr->pSessionData->eccKeys.pPubKeyX;
    pPubKeyY = pSr->pSessionData->eccKeys.pPubKeyY;

    /* Copy own public key to the ConfirmationInputs */
    memcpy(&pSr->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                             MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                             MESH_PRV_PDU_START_PARAM_SIZE +
                                                             MESH_PRV_PDU_PUB_KEY_PARAM_SIZE],
           pPubKeyX,
           MESH_PRV_PDU_PUB_KEY_PARAM_SIZE / 2);
    memcpy(&pSr->pSessionData->authParams.confirmationInputs[MESH_PRV_PDU_INVITE_PARAM_SIZE +
                                                             MESH_PRV_PDU_CAPAB_PARAM_SIZE +
                                                             MESH_PRV_PDU_START_PARAM_SIZE +
                                                             MESH_PRV_PDU_PUB_KEY_PARAM_SIZE +
                                                             MESH_PRV_PDU_PUB_KEY_PARAM_SIZE / 2],
           pPubKeyY,
           MESH_PRV_PDU_PUB_KEY_PARAM_SIZE / 2);

    wsfMsgHdr_t* pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
    if (pEvtMsg != NULL)
    {
      pEvtMsg->event = PRV_SR_EVT_SENT_PUBLIC_KEY;
      WsfMsgSend(pSr->timer.handlerId, pEvtMsg);
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
void meshPrvSrActPrepareOobAction(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  wsfMsgHdr_t* pEvtMsg;
  meshPrvSrEvtOutputOob_t evtMsg;
  uint8_t *pOutput;
  uint32_t randomNumeric;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Prepare OOB Action.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  switch (pSr->pSessionData->startParams.authMethod)
  {
    case MESH_PRV_START_AUTH_METHOD_INPUT_OOB:
      /* Go to WAIT_INPUT state. */
      MESH_TRACE_INFO0("MESH PRV SR: Authentication method is Input OOB. Changing to WAIT_INPUT...");
      pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
      if (pEvtMsg != NULL)
      {
        pEvtMsg->event = PRV_SR_EVT_GOTO_INPUT;
        WsfMsgSend(pSr->timer.handlerId, pEvtMsg);
      }
      /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
      break;

    case MESH_PRV_START_AUTH_METHOD_OUTPUT_OOB:
      /* Generate random Output OOB data */
      MESH_TRACE_INFO0("MESH PRV SR: Authentication method is Output OOB. "
          "Generating random output and going to WAIT_CONFIRMATION...");
      if (pSr->pSessionData->startParams.authAction == MESH_PRV_START_OUT_OOB_ACTION_ALPHANUMERIC)
      {
        /* Generate array of alphanumeric values, right-padded with zeros */
        pOutput = &pSr->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE];
        meshPrvGenerateRandomAlphanumeric(pOutput, pSr->pSessionData->startParams.authSize);
        memset(&pSr->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE +
                                                                     pSr->pSessionData->startParams.authSize],
               0x00,
               MESH_PRV_AUTH_VALUE_SIZE - pSr->pSessionData->startParams.authSize);

        /* Copy to upper layer event parameter */
        evtMsg.outputOobSize = pSr->pSessionData->startParams.authSize;
        memcpy(evtMsg.outputOobData.alphanumericOob, pOutput, pSr->pSessionData->startParams.authSize);
        if (pSr->pSessionData->startParams.authSize < MESH_PRV_INOUT_OOB_MAX_SIZE)
        {
          memset(&evtMsg.outputOobData.alphanumericOob[pSr->pSessionData->startParams.authSize],
                 0x00,
                 MESH_PRV_INOUT_OOB_MAX_SIZE - pSr->pSessionData->startParams.authSize);
        }
      }
      else
      {
        /* Generate big-endian number, left-padded with zeros */
        pOutput = &pSr->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE +
                                                                        MESH_PRV_AUTH_VALUE_SIZE -
                                                                        MESH_PRV_NUMERIC_OOB_SIZE_OCTETS];

        randomNumeric = meshPrvGenerateRandomNumeric(pSr->pSessionData->startParams.authSize);
        UINT32_TO_BE_BUF(pOutput, randomNumeric);
        memset(&pSr->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
               0x00,
               MESH_PRV_AUTH_VALUE_SIZE - MESH_PRV_NUMERIC_OOB_SIZE_OCTETS);

        /* Copy to upper layer event parameter */
        evtMsg.outputOobSize = 0;
        evtMsg.outputOobData.numericOob = randomNumeric;
      }

      /* Notify upper layer to output the OOB data */
      evtMsg.hdr.event = MESH_PRV_SR_EVENT;
      evtMsg.hdr.param = MESH_PRV_SR_OUTPUT_OOB_EVENT;
      evtMsg.hdr.status = MESH_SUCCESS;
      evtMsg.outputOobAction = (meshPrvOutputOobAction_t) (1 << pSr->pSessionData->startParams.authAction);
      pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);

      /* Generate state machine event */
      pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
      if (pEvtMsg != NULL)
      {
        pEvtMsg->event = PRV_SR_EVT_GOTO_CONFIRMATION;
        WsfMsgSend(pSr->timer.handlerId, pEvtMsg);
      }
      /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
      break;

    case MESH_PRV_START_AUTH_METHOD_NO_OOB:
      /* Set OOB data to 0 */
      memset(&pSr->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
             0x00,
             MESH_PRV_AUTH_VALUE_SIZE);

      /* Go to WAIT_CONFIRMATION state. */
      MESH_TRACE_INFO0("MESH PRV SR: Authentication method is No OOB. Changing to WAIT_CONFIRMATION...");
      pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
      if (pEvtMsg != NULL)
      {
        if (pSr->pSessionData->authParams.peerConfirmationReceived == TRUE)
        {
          pEvtMsg->event = PRV_SR_EVT_RECV_CONFIRMATION;
        }
        else
        {
          pEvtMsg->event = PRV_SR_EVT_GOTO_CONFIRMATION;
        }
        WsfMsgSend(pSr->timer.handlerId, pEvtMsg);
      }
      /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
      break;

    case MESH_PRV_START_AUTH_METHOD_STATIC_OOB:
      /* Copy static OOB data */
      memcpy(&pSr->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
             pSr->pUpdInfo->pStaticOobData,
             MESH_PRV_AUTH_VALUE_SIZE);

      /* Go to WAIT_CONFIRMATION state. */
      MESH_TRACE_INFO0("MESH PRV SR: Authentication method is Static OOB. Changing to WAIT_CONFIRMATION...");
      pEvtMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t));
      if (pEvtMsg != NULL)
      {
        if (pSr->pSessionData->authParams.peerConfirmationReceived == TRUE)
        {
          pEvtMsg->event = PRV_SR_EVT_RECV_CONFIRMATION;
        }
        else
        {
          pEvtMsg->event = PRV_SR_EVT_GOTO_CONFIRMATION;
        }
        WsfMsgSend(pSr->timer.handlerId, pEvtMsg);
      }
      /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
      break;

    default:
      /* Should never get here; parameter check should catch this. Provisioning will fail on timeout. */
      break;
  }
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
void meshPrvSrActWaitInput(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  meshPrvSrEvtInputOob_t evtMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Wait for user OOB input.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  evtMsg.hdr.event = MESH_PRV_SR_EVENT;
  evtMsg.hdr.param = MESH_PRV_SR_INPUT_OOB_EVENT;
  evtMsg.hdr.status = MESH_SUCCESS;
  evtMsg.inputOobAction = (meshPrvInputOobAction_t) (1 << pSr->pSessionData->startParams.authAction);
  pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Input Complete PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActSendInputComplete(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  meshPrvSrInputOob_t* pOob = (meshPrvSrInputOob_t*)pMsg;
  uint8_t *pBuf;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send Provisioning Input Complete PDU.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Save Input OOB data */
  meshPrvPackInOutOobToAuthArray(
        &pSr->pSessionData->authParams.tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE],
        pOob->inputOobData,
        pOob->inputOobSize);

  /* Allocate buffer for the Provisioning Input Complete PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_INPUT_COMPLETE_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_INPUT_COMPLETE;
    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_INPUT_COMPLETE_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
  (void)pSr;
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
void meshPrvSrActWaitConfirmation(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Start waiting for Provisioning Confirmation PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pSr->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Store Peer the provisioning confirmation.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActSaveConfirmation(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  meshPrvSrRecvConfirm_t* pConfirm = (meshPrvSrRecvConfirm_t*)pMsg;
  wsfMsgHdr_t evtMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Save peer provisioning confirmation value.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Save peer Confirmation */
  memcpy(pSr->pSessionData->authParams.peerConfirmation, pConfirm->confirm, MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE);
  pSr->pSessionData->authParams.peerConfirmationReceived = TRUE;

  /* Notify upper layer to stop outputting OOB data, if applicable */
  if (pSr->pSessionData->startParams.authMethod == MESH_PRV_START_AUTH_METHOD_OUTPUT_OOB)
  {
    evtMsg.event = MESH_PRV_SR_EVENT;
    evtMsg.param = MESH_PRV_SR_OUTPUT_CONFIRMED_EVENT;
    evtMsg.status = MESH_SUCCESS;
    pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);
  }
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
void meshPrvSrActCalcConfirmation(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Calculate own provisioning confirmation value.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Stop timer because Provisioning Confirmation PDU was received */
  WsfTimerStop(&pSr->timer);

  /* Save peer Confirmation */
  if (pSr->pSessionData->authParams.peerConfirmationReceived == FALSE)
  {
      meshPrvSrActSaveConfirmation(pCcb, pMsg);
  }

  /* Calculate ConfirmationSalt = s1(ConfirmationInputs) */
  (void)MeshSecToolGenerateSalt(pSr->pSessionData->authParams.confirmationInputs,
                                MESH_PRV_CONFIRMATION_INPUTS_SIZE,
                                meshPrvSrSaltCback,
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
void meshPrvSrActSendConfirmation(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  meshPrvSrOwnConfirm_t* pConfirm = (meshPrvSrOwnConfirm_t*)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send Provisioning Confirmation PDU.");

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

  (void)pSr;
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
void meshPrvSrActWaitRandom(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;

  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Start waiting for Provisioning Random PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pSr->timer, MESH_PRV_TRAN_TIMEOUT_MS);
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
void meshPrvSrActCheckConfirmation(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  meshPrvSrRecvRandom_t* pRandom = (meshPrvSrRecvRandom_t*)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Check peer's provisioning confirmation.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Stop timer because Provisioning Random PDU was received */
  WsfTimerStop(&pSr->timer);

  /* Overwrite own Random with peer Random for peer Confirmation calculation */
  memcpy(pSr->pSessionData->authParams.tempRandomAndAuthValue, pRandom->random, MESH_PRV_PDU_RANDOM_RANDOM_SIZE);

  /* Save a copy of peer Random for Session Key calculation */
  memcpy(&pSr->pSessionData->authParams.confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE],
         pRandom->random,
         MESH_PRV_PDU_RANDOM_RANDOM_SIZE);

  /* Compute peer Confirmation */
  MeshSecToolCmacCalculate(pSr->pSessionData->authParams.confirmationKey,
                           pSr->pSessionData->authParams.tempRandomAndAuthValue,
                           MESH_PRV_PDU_RANDOM_RANDOM_SIZE + MESH_PRV_AUTH_VALUE_SIZE,
                           meshPrvSrConfirmationCback,
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
void meshPrvSrActCalcSessionKey(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;

  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Calculate Session Key.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Calculate ProvisioningSalt = s1(ConfirmationSalt||RandomP||RandomD) */
  (void)MeshSecToolGenerateSalt(pSr->pSessionData->authParams.confirmationSaltAndFinalRandoms,
                                MESH_PRV_CONFIRMATION_SALT_SIZE + 2 * MESH_PRV_PDU_RANDOM_RANDOM_SIZE,
                                meshPrvSrSaltCback,
                                SALT_CBACK_ID_PROVISIONING);
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
void meshPrvSrActSendRandom(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send Provisioning Random PDU.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Allocate buffer for the Provisioning Random PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_RANDOM_PDU_SIZE);
  if (pBuf != NULL)
  {
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_RANDOM;
    memcpy(&pBuf[MESH_PRV_PDU_RANDOM_RANDOM_INDEX],
           &(pSr->pSessionData->authParams.confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE +
                                                                           MESH_PRV_PDU_RANDOM_RANDOM_SIZE]),
           MESH_PRV_PDU_RANDOM_RANDOM_SIZE);
    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_RANDOM_PDU_SIZE);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Wait for Provisioning Data PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActWaitData(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Start waiting for Provisioning Data PDU.");

  /* Start transaction timer while waiting for a PDU */
  WsfTimerStartMs(&pSr->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Decrypt the provisioning data.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActDecryptData(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  meshPrvSrRecvData_t* pData = (meshPrvSrRecvData_t*)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Decrypt provisioning data.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Stop timer because Provisioning Data PDU was received */
  WsfTimerStop(&pSr->timer);

  /* Save encrypted data and MIC */
  memcpy(pSr->pSessionData->provisioningDataAndMic, pData->encryptedDataAndMic, MESH_PRV_PDU_DATA_PARAM_SIZE);

  /* CCM decryption parameters */
  meshSecToolCcmParams_t params;
  params.authDataLen = 0;
  params.pAuthData = NULL;
  params.cbcMacSize = MESH_PRV_PDU_DATA_MIC_SIZE;
  params.pCbcMac = &pSr->pSessionData->provisioningDataAndMic[MESH_PRV_PDU_DATA_ENC_DATA_SIZE];
  params.inputLen = MESH_PRV_PDU_DATA_ENC_DATA_SIZE;
  params.pIn = pSr->pSessionData->provisioningDataAndMic;
  params.pCcmKey = pSr->pSessionData->authParams.sessionKey;
  params.pNonce = pSr->pSessionData->authParams.sessionNonce;
  params.pOut = params.pIn; /* Overwrite the same location with plain data */

  (void)MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_DECRYPT,
                                     &params,
                                     meshPrvSrDataDecryptCback,
                                     NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Complete PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActSendComplete(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  (void)pMsg;
  uint8_t* pBuf;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send Provisioning Complete PDU.");

  /* Check session data is allocated */
  if (pSr->pSessionData == NULL)
  {
    MESH_TRACE_ERR0("MESH PRV SR: Session data not allocated during PRV SR SM action!");
    return;
  }

  /* Allocate buffer for the Provisioning Complete PDU */
  pBuf = WsfBufAlloc(MESH_PRV_PDU_COMPLETE_PDU_SIZE);
  if (pBuf != NULL)
  {
    /* Send the Provisioning Complete PDU */
    pBuf[MESH_PRV_PDU_OPCODE_INDEX] = MESH_PRV_PDU_COMPLETE;
    (void)MeshPrvBrSendProvisioningPdu(pBuf, MESH_PRV_PDU_COMPLETE_PDU_SIZE);

    /* Trigger application event */
    meshPrvSrEvtPrvComplete_t evtMsg;
    evtMsg.hdr.event = MESH_PRV_SR_EVENT;
    evtMsg.hdr.param = MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT;
    evtMsg.hdr.status = MESH_SUCCESS;
    memcpy(evtMsg.devKey, pSr->pSessionData->deviceKey, MESH_KEY_SIZE_128);
    memcpy(evtMsg.netKey,
           &pSr->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_NETKEY_INDEX],
           MESH_KEY_SIZE_128);
    BYTES_BE_TO_UINT16(evtMsg.netKeyIndex,
                       &pSr->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_NETKEYIDX_INDEX]);
    evtMsg.flags = pSr->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_FLAGS_INDEX];
    BYTES_BE_TO_UINT32(evtMsg.ivIndex,
                       &pSr->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_IVIDX_INDEX]);
    BYTES_BE_TO_UINT16(evtMsg.address,
                       &pSr->pSessionData->provisioningDataAndMic[MESH_PRV_DECRYPTED_DATA_ADDRESS_INDEX]);
    pSr->prvSrEvtNotifyCback((meshPrvSrEvt_t*)&evtMsg);
  }
  /* Else provisioning will fail on timeout - this should not happen if buffers are properly configured. */
}

/*************************************************************************************************/
/*!
 *  \brief     Send Provisioning Failed PDU with reason Unexpected PDU.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActSendUnexpectedPdu(void *pCcb, void *pMsg)
{
  (void)pCcb;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send failed PDU with reason Unexpected PDU.");

  /* This is an unexpected PDU */
  meshPrvSrSendFailedPdu(MESH_PRV_ERR_UNEXPECTED_PDU);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an error encountered in the protocol.
 *
 *  \param[in] pCcb  Control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrActHandleProtocolError(void *pCcb, void *pMsg)
{
  meshPrvSrCb_t* pSr = (meshPrvSrCb_t*)pCcb;
  wsfMsgHdr_t* pEvt = (wsfMsgHdr_t*)pMsg;
  MESH_TRACE_INFO0("MESH PRV SR: [ACT] Send failed PDU.");

  switch (pEvt->event)
  {
    case PRV_SR_EVT_PUBLIC_KEY_INVALID:
    case PRV_SR_EVT_CONFIRMATION_FAILED:
    case PRV_SR_EVT_DATA_NOT_DECRYPTED:
    case PRV_SR_EVT_RECV_BAD_PDU:
      /* This is either invalid opcode or invalid parameter or security error. */
      meshPrvSrSendFailedPdu((uint8_t) pEvt->param);
      break;
    default:
      /* This is an unexpected PDU */
      meshPrvSrSendFailedPdu(MESH_PRV_ERR_UNEXPECTED_PDU);
      break;
  }

  /* Start transaction timer while waiting for link to close. */
  WsfTimerStartMs(&pSr->timer, MESH_PRV_TRAN_TIMEOUT_MS);
}
