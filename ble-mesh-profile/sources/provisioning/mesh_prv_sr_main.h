/*************************************************************************************************/
/*!
 *  \file   mesh_prv_sr_main.h
 *
 *  \brief  Mesh Provisioning Server module interface.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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

#ifndef MESH_PRV_SR_MAIN_H
#define MESH_PRV_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_types.h"
#include "wsf_os.h"
#include "wsf_timer.h"
#include "mesh_security_toolbox.h"
#include "mesh_prv_defs.h"
#include "mesh_prv_sr_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Number of columns in state table */
#define MESH_PRV_SR_SM_NUM_COLS     3

/*! Compiler switch to enable sample data testing for provisioning.
    Default value: 0 (disabled).
    When enabled, the device uses the following values from the sample data:
    -- Device Random (normally generated randomly when computing Device Confirmation)
    -- Public and Private Key (normally generated randomly at initialization or supplied by the application)
    -- The device capabilities are overwritten to indicate no OOB support and one element .*/
#define MESH_PRV_SAMPLE_BUILD                     0

/*! State machine states - enumeration values */
enum meshPrvSrSmStateValues
{
  PRV_SR_ST_IDLE,                       /* Idle */
  PRV_SR_ST_WAIT_LINK,                  /* Waiting for Link Opened event */
  PRV_SR_ST_WAIT_INVITE,                /* (1)  Waiting for Provisioning Invite PDU */
  PRV_SR_ST_SEND_CAPABILITIES,          /* (1)  Sending Provisioning Capabilities PDU */
  PRV_SR_ST_WAIT_START,                 /* (2)  Waiting for Provisioning Start PDU */
  PRV_SR_ST_WAIT_PUBLIC_KEY,            /* (2)  Waiting for Provisioning Public Key PDU */
  PRV_SR_ST_GENERATE_PUBLIC_KEY,        /* (2)  Generate own Provisioning Public Key */
  PRV_SR_ST_VALIDATE_PUBLIC_KEY,        /* (2)  Validate received Provisioning Public Key */
  PRV_SR_ST_SEND_PUBLIC_KEY,            /* (2a) Sending Provisioning Public Key PDU */
  PRV_SR_ST_PREPARE_OOB_ACTION,         /* (3)  Preparing OOB action */
  PRV_SR_ST_WAIT_INPUT,                 /* (3b) Waiting for user input */
  PRV_SR_ST_SEND_INPUT_COMPLETE,        /* (3b) Sending Provisioning Input Complete PDU */
  PRV_SR_ST_WAIT_CONFIRMATION,          /* (3)  Waiting for Provisioning Confirmation PDU */
  PRV_SR_ST_CALC_CONFIRMATION,          /* (3)  Calculating the provisioning confirmation */
  PRV_SR_ST_SEND_CONFIRMATION,          /* (3)  Sending Provisioning Confirmation PDU */
  PRV_SR_ST_WAIT_RANDOM,                /* (3)  Waiting for Provisioning Random PDU */
  PRV_SR_ST_CHECK_CONFIRMATION,         /* (3)  Checking Confirmation */
  PRV_SR_ST_CALC_SESSION_KEY,           /* (3)  Calculating Session Key */
  PRV_SR_ST_SEND_RANDOM,                /* (3)  Sending Provisioning Random PDU */
  PRV_SR_ST_WAIT_DATA,                  /* (4)  Waiting for Provisioning Data PDU */
  PRV_SR_ST_DECRYPT_DATA,               /* (4)  Decrypting the provisioning data */
  PRV_SR_ST_SEND_COMPLETE,              /* (4)  Sending Provisioning Complete PDU */
  PRV_SR_ST_LINK_FAILED,                /* Error state on which the device waits for the link to close */

  PRV_SR_ST_NO_STATE_CHANGE,            /* Fictious state value, never reached by the state machine.
                                           Used as "next state" in event handling tables to indicate
                                           that the event does not change the current state. */
};

/*! State machine states - type; see ::meshPrvSrSmStateValues */
typedef uint8_t meshPrvSrSmState_t;

/*! State machines events - enumeration values */
enum meshPrvSrSmEvt_tag
{
  PRV_SR_EVT_BEGIN_NO_LINK,         /* Command received to begin provisioning when no link is opened */
  PRV_SR_EVT_BEGIN_LINK_OPEN,       /* Command received to begin provisioning when link is already opened */
  PRV_SR_EVT_LINK_OPENED,           /* Provisioning link has opened */
  PRV_SR_EVT_LINK_CLOSED_FAIL,      /* Provisioning link has closed on provisioning failure */
  PRV_SR_EVT_LINK_CLOSED_SUCCESS,   /* Provisioning link has closed on provisioning complete */
  PRV_SR_EVT_RECV_TIMEOUT,          /* Timeout on receiving Provisioning PDU */
  PRV_SR_EVT_SEND_TIMEOUT,          /* Timeout on sending Provisioning PDU */
  PRV_SR_EVT_SENT_CAPABILITIES,     /* Provisioning Capabilities PDU has been sent */
  PRV_SR_EVT_SENT_PUBLIC_KEY,       /* Provisioning Public Key PDU has been sent */
  PRV_SR_EVT_SENT_CONFIRMATION,     /* Provisioning Confirmation PDU has been sent */
  PRV_SR_EVT_SENT_RANDOM,           /* Provisioning Random PDU has been sent */
  PRV_SR_EVT_SENT_INPUT_COMPLETE,   /* Provisioning Input Complete PDU has been sent */
  PRV_SR_EVT_SENT_COMPLETE,         /* Provisioning Complete PDU has been sent */
  PRV_SR_EVT_GOTO_INPUT,            /* Input OOB is used */
  PRV_SR_EVT_GOTO_CONFIRMATION,     /* Output, Static or No OOB is used */
  PRV_SR_EVT_INPUT_READY,           /* Input OOB data has been received from user */
  PRV_SR_EVT_CONFIRMATION_READY,    /* Provisioning confirmation has been computed */
  PRV_SR_EVT_CONFIRMATION_VERIFIED, /* Provisioning confirmation has been verified */
  PRV_SR_EVT_CONFIRMATION_FAILED,   /* Provisioning confirmation could not be verified */
  PRV_SR_EVT_SESSION_KEY_READY,     /* Session key has been computed */
  PRV_SR_EVT_RECV_INVITE,           /* Provisioning Invite PDU has been received */
  PRV_SR_EVT_RECV_START,            /* Provisioning Start PDU has been received */
  PRV_SR_EVT_RECV_PUBLIC_KEY,       /* Provisioning Public Key PDU has been received */
  PRV_SR_EVT_PUBLIC_KEY_VALID,      /* Received public key has been validated */
  PRV_SR_EVT_PUBLIC_KEY_INVALID,    /* Received public key is invalid */
  PRV_SR_EVT_PUBLIC_KEY_GENERATED,  /* Own public key was generated */
  PRV_SR_EVT_RECV_CONFIRMATION,     /* Provisioning Confirmation PDU has been received */
  PRV_SR_EVT_RECV_RANDOM,           /* Provisioning Random PDU has been received */
  PRV_SR_EVT_RECV_DATA,             /* Provisioning Data PDU has been received */
  PRV_SR_EVT_DATA_DECRYPTED,        /* Provisioning data has been decrypted */
  PRV_SR_EVT_DATA_NOT_DECRYPTED,    /* Provisioning data could not be decrypted */
  PRV_SR_EVT_RECV_BAD_PDU,          /* Received PDU with invalid opcode or format */
  PRV_SR_EVT_SENT_FAILED_PDU,       /* Provisioning Failed sent */
};

/*! State machine events - type; see ::meshPrvSrSmEvt_tag */
typedef uint8_t meshPrvSrSmEvt_t;

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Data type for state machine table entry */
typedef uint8_t meshPrvSrTblEntry_t[MESH_PRV_SR_SM_NUM_COLS];

/*! Mesh Provisioning PDU type data type. See ::meshPrvPduTypes */
typedef uint8_t meshPrvPduTypes_t;

/*! State machine action function type */
typedef void (*meshPrvSrAct_t)(void *pCcb, void *pMsg);

/*! State machine interface type */
typedef struct
{
  meshPrvSrTblEntry_t const * const *pStateTbl;   /* Pointer to state table */
  meshPrvSrAct_t              const *pActionTbl;  /* Pointer to action table */
  meshPrvSrTblEntry_t         const *pCommonTbl;  /* Pointer to common action table */
} meshPrvSrSmIf_t;

/*! Provisioning Server session data */
typedef struct meshPrvSrSessionData_tag
{
  struct startParams_tag
  {
    bool_t      oobPublicKey;
    uint8_t     authMethod;
    uint8_t     authAction;
    uint8_t     authSize;
  } startParams;
  meshPrvEccKeys_t eccKeys;
  struct authenticationParams_tag
  {
    uint8_t     confirmationInputs[MESH_PRV_CONFIRMATION_INPUTS_SIZE];
    uint8_t     tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE +
                                       MESH_PRV_AUTH_VALUE_SIZE];
    uint8_t     confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE +
                                                2 * MESH_PRV_PDU_RANDOM_RANDOM_SIZE];
    uint8_t     provisioningSalt[MESH_PRV_PROVISIONING_SALT_SIZE];
    uint8_t     sessionKey[MESH_SEC_TOOL_AES_BLOCK_SIZE];
    uint8_t     sessionNonce[MESH_PRV_SESSION_NONCE_SIZE];
    uint8_t     confirmationKey[MESH_SEC_TOOL_AES_BLOCK_SIZE];
    uint8_t     peerConfirmation[MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE];
    bool_t      peerConfirmationReceived;
  } authParams;
  uint8_t ecdhSecret[MESH_SEC_TOOL_ECC_KEY_SIZE];
  uint8_t provisioningDataAndMic[MESH_PRV_PDU_DATA_PARAM_SIZE];
  uint8_t deviceKey[MESH_SEC_TOOL_AES_BLOCK_SIZE];
} meshPrvSrSessionData_t;

/*! Provisioning Server main control block */
typedef struct meshPrvSrCb_tag
{
  wsfTimer_t                                  timer;         /* WSF timer */
  meshPrvSrSmIf_t const                       *pSm;          /* State machine interface */
  meshPrvSrEvtNotifyCback_t                   prvSrEvtNotifyCback; /* Upper Layer callback */
  meshPrvSrSmState_t                          state;         /* Current state */
  const meshPrvSrUnprovisionedDeviceInfo_t    *pUpdInfo;     /* Unprovisioned device information */
  meshPrvSrSessionData_t                      *pSessionData; /* Session data */
} meshPrvSrCb_t;

/*! Event data for EnterPbAdvProvisioningMode API */
typedef struct meshPrvSrEnterPbAdv_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     ifId;
  uint32_t    beaconInterval;
} meshPrvSrEnterPbAdv_t;

/*! Event data for EnterPbGattProvisioningMode API */
typedef struct meshPrvSrEnterPbGatt_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     connId;
} meshPrvSrEnterPbGatt_t;

/*! Event data for Received Provisioning Invite */
typedef struct meshPrvSrRecvInvite_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     attentionTimer;
} meshPrvSrRecvInvite_t;

/*! Event data for Received Provisioning Start */
typedef struct meshPrvSrRecvStart_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     packedPduParam[MESH_PRV_PDU_START_PARAM_SIZE];
  uint8_t     algorithm;
  uint8_t     oobPubKeyUsed;
  uint8_t     authMethod;
  uint8_t     authAction;
  uint8_t     authSize;
} meshPrvSrRecvStart_t;

/*! Event data for Received Public Key */
typedef struct meshPrvSrRecvPubKey_tag
{
  wsfMsgHdr_t   hdr;
  uint8_t       pubKeyPdu[MESH_PRV_PDU_PUB_KEY_PDU_SIZE];
} meshPrvSrRecvPubKey_t;

/*! Event data for Input OOB */
typedef struct meshPrvSrInputOob_tag
{
  wsfMsgHdr_t hdr;
  meshPrvInputOobSize_t  inputOobSize;
  meshPrvInOutOobData_t  inputOobData;
} meshPrvSrInputOob_t;

/*! Event data for Received Provisioning Confirmation */
typedef struct meshPrvSrRecvConfirm_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     confirm[MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE];
} meshPrvSrRecvConfirm_t;

/*! Event data for Received Provisioning Random */
typedef struct meshPrvSrRecvRandom_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     random[MESH_PRV_PDU_RANDOM_RANDOM_SIZE];
} meshPrvSrRecvRandom_t;

/*! Event data for Confirmation Ready */
typedef struct meshPrvSrOwnConfirm_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     confirmation[MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE];
} meshPrvSrOwnConfirm_t;

/*! Event data for Received Provisioning Data */
typedef struct meshPrvSrRecvData_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     encryptedDataAndMic[MESH_PRV_PDU_DATA_PARAM_SIZE];
} meshPrvSrRecvData_t;

/*! Union of event handler data types */
typedef union meshPrvSrSmMsg_tag
{
  wsfMsgHdr_t             hdr;            /* Header structure */
  meshPrvSrEnterPbAdv_t   enterPbAdv;
  meshPrvSrEnterPbGatt_t  enterPbGatt;
  meshPrvSrRecvInvite_t   recvInvite;
  meshPrvSrRecvStart_t    recvStart;
  meshPrvSrRecvPubKey_t   recvPubKey;
  meshPrvSrInputOob_t     inputOob;
  meshPrvSrRecvConfirm_t  recvConfirm;
  meshPrvSrRecvRandom_t   recvRandom;
  meshPrvSrRecvData_t     recvData;
  meshPrvSrOwnConfirm_t   ownConfirm;
} meshPrvSrSmMsg_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Control block */
extern meshPrvSrCb_t meshPrvSrCb;
/*! State machine instance */
extern const meshPrvSrSmIf_t meshPrvSrSmIf;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* Send Provisioning Failed PDU at any moment during the protocol */
void meshPrvSrSendFailedPdu(uint8_t errorCode);

/* State machine execute function */
void meshPrvSrSmExecute(meshPrvSrCb_t *pCcb, meshPrvSrSmMsg_t *pMsg);

/*
 * State machine action functions
 */
void meshPrvSrActNone(void *pCcb, void *pMsg);
void meshPrvSrActLinkClosed(void *pCcb, void *pMsg);
void meshPrvSrActRecvTimeout(void *pCcb, void *pMsg);
void meshPrvSrActSendTimeout(void *pCcb, void *pMsg);
void meshPrvSrActSuccess(void *pCcb, void *pMsg);
void meshPrvSrActWaitLink(void *pCcb, void *pMsg);
void meshPrvSrActWaitInvite(void *pCcb, void *pMsg);
void meshPrvSrActSendCapabilities(void *pCcb, void *pMsg);
void meshPrvSrActWaitStart(void *pCcb, void *pMsg);
void meshPrvSrActWaitPublicKey(void *pCcb, void *pMsg);
void meshPrvSrActGeneratePublicKey(void *pCcb, void *pMsg);
void meshPrvSrActValidatePublicKey(void *pCcb, void *pMsg);
void meshPrvSrActSendPublicKey(void *pCcb, void *pMsg);
void meshPrvSrActPrepareOobAction(void *pCcb, void *pMsg);
void meshPrvSrActWaitInput(void *pCcb, void *pMsg);
void meshPrvSrActSendInputComplete(void *pCcb, void *pMsg);
void meshPrvSrActWaitConfirmation(void *pCcb, void *pMsg);
void meshPrvSrActSaveConfirmation(void *pCcb, void *pMsg);
void meshPrvSrActCalcConfirmation(void *pCcb, void *pMsg);
void meshPrvSrActSendConfirmation(void *pCcb, void *pMsg);
void meshPrvSrActWaitRandom(void *pCcb, void *pMsg);
void meshPrvSrActCheckConfirmation(void *pCcb, void *pMsg);
void meshPrvSrActCalcSessionKey(void *pCcb, void *pMsg);
void meshPrvSrActSendRandom(void *pCcb, void *pMsg);
void meshPrvSrActWaitData(void *pCcb, void *pMsg);
void meshPrvSrActDecryptData(void *pCcb, void *pMsg);
void meshPrvSrActSendComplete(void *pCcb, void *pMsg);
void meshPrvSrActSendUnexpectedPdu(void *pCcb, void *pMsg);
void meshPrvSrActHandleProtocolError(void *pCcb, void *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_SR_MAIN_H */
