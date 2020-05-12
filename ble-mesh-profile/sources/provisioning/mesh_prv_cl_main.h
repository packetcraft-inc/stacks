/*************************************************************************************************/
/*!
 *  \file   mesh_prv_cl_main.h
 *
 *  \brief  Mesh Provisioning Client internal module interface.
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

#ifndef MESH_PRV_CL_MAIN_H
#define MESH_PRV_CL_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_types.h"
#include "wsf_os.h"
#include "wsf_timer.h"
#include "mesh_security_toolbox.h"
#include "mesh_prv_defs.h"
#include "mesh_prv_cl_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Number of columns in state table */
#define MESH_PRV_CL_SM_NUM_COLS     3

/*! State machine states - enumeration values */
enum meshPrvClSmStateValues
{
  PRV_CL_ST_IDLE,                       /*! Idle */
  PRV_CL_ST_WAIT_LINK,                  /*! Waiting for Link Opened event */
  PRV_CL_ST_SEND_INVITE,                /*! (1)  Sending Provisioning Invite PDU */
  PRV_CL_ST_WAIT_CAPABILITIES,          /*! (1)  Waiting for Provisioning Capabilities PDU */
  PRV_CL_ST_WAIT_SELECT_AUTH,           /*! (2)  Waiting for user selection of authentication */
  PRV_CL_ST_SEND_START,                 /*! (2)  Sending Provisioning Start PDU */
  PRV_CL_ST_SEND_PUBLIC_KEY,            /*! (2)  Sending Provisioning Public Key PDU */
  PRV_CL_ST_WAIT_PUBLIC_KEY,            /*! (2a) Waiting for Provisioning Public Key PDU */
  PRV_CL_ST_VALIDATE_PUBLIC_KEY,        /*! (2)  Validate received Provisioning Public Key */
  PRV_CL_ST_GENERATE_PUBLIC_KEY,        /*! (2)  Generate own Provisioning Public Key */
  PRV_CL_ST_PREPARE_OOB_ACTION,         /*! (3)  Preparing OOB action */
  PRV_CL_ST_WAIT_INPUT,                 /*! (3b) Waiting for user input */
  PRV_CL_ST_WAIT_INPUT_COMPLETE,        /*! (3b) Waiting for Provisioning Input Complete PDU */
  PRV_CL_ST_CALC_CONFIRMATION,          /*! (3)  Calculating the provisioning confirmation */
  PRV_CL_ST_SEND_CONFIRMATION,          /*! (3)  Sending Provisioning Confirmation PDU */
  PRV_CL_ST_WAIT_CONFIRMATION,          /*! (3)  Waiting for Provisioning Confirmation PDU */
  PRV_CL_ST_SEND_RANDOM,                /*! (3)  Sending Provisioning Random PDU */
  PRV_CL_ST_WAIT_RANDOM,                /*! (3)  Waiting for Provisioning Random PDU */
  PRV_CL_ST_CHECK_CONFIRMATION,         /*! (3)  Checking Confirmation */
  PRV_CL_ST_CALC_SESSION_KEY,           /*! (3)  Calculating Session Key */
  PRV_CL_ST_ENCRYPT_DATA,               /*! (4)  Encrypting the provisioning data */
  PRV_CL_ST_SEND_DATA,                  /*! (4)  Sending Provisioning Data PDU */
  PRV_CL_ST_WAIT_COMPLETE,              /*! (4)  Waiting for Provisioning Complete PDU */
};
/*! State machine states - type; see ::meshPrvClSmStateValues */
typedef uint8_t meshPrvClSmState_t;

/*! State machine events - enumeration values */
enum meshPrvClSmEvtValues
{
  PRV_CL_EVT_BEGIN_NO_LINK,         /*! Command received to begin provisioning when no link is opened */
  PRV_CL_EVT_BEGIN_LINK_OPEN,       /*! Command received to begin provisioning when link is already opened */
  PRV_CL_EVT_LINK_OPENED,           /*! Provisioning link has opened */
  PRV_CL_EVT_LINK_FAILED,           /*! Provisioning link failed to open */
  PRV_CL_EVT_LINK_CLOSED_FAIL,      /*! Provisioning link has closed on provisioning failure */
  PRV_CL_EVT_BAD_PDU,               /*! A PDU with invalid opcode, format or parameters was received */
  PRV_CL_EVT_LINK_CLOSED_SUCCESS,   /*! Provisioning link has closed on provisioning complete */
  PRV_CL_EVT_RECV_TIMEOUT,          /*! Timeout on receiving Provisioning PDU */
  PRV_CL_EVT_SEND_TIMEOUT,          /*! Timeout on sending Provisioning PDU */
  PRV_CL_EVT_SENT_INVITE,           /*! Provisioning Invite PDU has been sent */
  PRV_CL_EVT_SENT_START,            /*! Provisioning Start PDU has been sent */
  PRV_CL_EVT_SENT_PUBLIC_KEY,       /*! Provisioning Public Key PDU has been sent */
  PRV_CL_EVT_SENT_CONFIRMATION,     /*! Provisioning Confirmation PDU has been sent */
  PRV_CL_EVT_SENT_RANDOM,           /*! Provisioning Random PDU has been sent */
  PRV_CL_EVT_SENT_DATA,             /*! Provisioning Data PDU has been sent */
  PRV_CL_EVT_GOTO_WAIT_INPUT,       /*! Output OOB is used */
  PRV_CL_EVT_GOTO_WAIT_IC,          /*! Input OOB is used */
  PRV_CL_EVT_GOTO_CONFIRMATION,     /*! Static or No OOB is used */
  PRV_CL_EVT_INPUT_READY,           /*! Input OOB data has been received from user */
  PRV_CL_EVT_AUTH_SELECTED,         /*! User has selected authentication method */
  PRV_CL_EVT_CONFIRMATION_READY,    /*! Provisioning confirmation has been computed */
  PRV_CL_EVT_CONFIRMATION_VERIFIED, /*! Provisioning confirmation has been verified */
  PRV_CL_EVT_CONFIRMATION_FAILED,   /*! Provisioning confirmation could not be verified */
  PRV_CL_EVT_SESSION_KEY_READY,     /*! Session key has been computed */
  PRV_CL_EVT_RECV_CAPABILITIES,     /*! Provisioning Capabilities PDU has been received */
  PRV_CL_EVT_RECV_PUBLIC_KEY,       /*! Provisioning Public Key PDU has been received */
  PRV_CL_EVT_PUBLIC_KEY_VALID,      /*! Received public key has been validated */
  PRV_CL_EVT_PUBLIC_KEY_INVALID,    /*! Received public key is invalid */
  PRV_CL_EVT_PUBLIC_KEY_GENERATED,  /*! Own public key was generated */
  PRV_CL_EVT_RECV_INPUT_COMPLETE,   /*! Provisioning Input Complete PDU has been received */
  PRV_CL_EVT_RECV_CONFIRMATION,     /*! Provisioning Confirmation PDU has been received */
  PRV_CL_EVT_RECV_RANDOM,           /*! Provisioning Random PDU has been received */
  PRV_CL_EVT_RECV_COMPLETE,         /*! Provisioning Data Complete PDU has been received */
  PRV_CL_EVT_DATA_ENCRYPTED,        /*! Provisioning data has been encrypted */
  PRV_CL_EVT_CANCEL,                /*! Protocol was canceled by user */
};

/*! State machine events - type; see ::meshPrvClSmEvtValues */
typedef uint8_t meshPrvClSmEvt_t;

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Data type for state machine table entry */
typedef uint8_t meshPrvClTblEntry_t[MESH_PRV_CL_SM_NUM_COLS];

/*! Mesh Provisioning PDU type data type. See ::meshPrvPduTypes */
typedef uint8_t meshPrvPduTypes_t;

/*! State machine action function type */
typedef void (*meshPrvClAct_t)(void *pCcb, void *pMsg);

/*! State machine interface type */
typedef struct
{
  meshPrvClTblEntry_t const * const *pStateTbl;   /*! Pointer to state table */
  meshPrvClAct_t              const *pActionTbl;  /*! Pointer to action table */
  meshPrvClTblEntry_t         const *pCommonTbl;  /*! Pointer to common action table */
} meshPrvClSmIf_t;

/*! Provisioning Client session data */
typedef struct meshPrvClSessionData_tag
{
  meshPrvCapabilities_t deviceCapab;
  meshPrvClSelectAuth_t selectAuth;
  meshPrvEccKeys_t eccKeys;
  uint8_t ecdhSecret[MESH_SEC_TOOL_ECC_KEY_SIZE];
  struct authenticationParams_tag
  {
    uint8_t     confirmationInputs[MESH_PRV_CONFIRMATION_INPUTS_SIZE];
    uint8_t     tempRandomAndAuthValue[MESH_PRV_PDU_RANDOM_RANDOM_SIZE
                                  + MESH_PRV_AUTH_VALUE_SIZE];
    uint8_t     confirmationSaltAndFinalRandoms[MESH_PRV_CONFIRMATION_SALT_SIZE + 2 * MESH_PRV_PDU_RANDOM_RANDOM_SIZE];
    uint8_t     provisioningSalt[MESH_PRV_PROVISIONING_SALT_SIZE];
    uint8_t     sessionKey[MESH_SEC_TOOL_AES_BLOCK_SIZE];
    uint8_t     sessionNonce[MESH_PRV_SESSION_NONCE_SIZE];
    uint8_t     confirmationKey[MESH_SEC_TOOL_AES_BLOCK_SIZE];
    uint8_t     peerConfirmation[MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE];
  } authParams;
  uint8_t provisioningDataAndMic[MESH_PRV_PDU_DATA_PARAM_SIZE];
  uint8_t deviceKey[MESH_SEC_TOOL_AES_BLOCK_SIZE];
} meshPrvClSessionData_t;

/*! Provisioning Client main control block */
typedef struct
{
  wsfTimer_t                             timer;               /*! WSF timer */
  meshPrvClSmIf_t const                  *pSm;                /*! State machine interface */
  meshPrvClEvtNotifyCback_t              prvClEvtNotifyCback; /*! Upper Layer callback */
  meshPrvClSmState_t                     state;               /*! Current state */
  meshPrvClSessionInfo_t const           *pSessionInfo;       /*! Session information */
  meshPrvClSessionData_t                 *pSessionData;       /*! Session data */
} meshPrvClCb_t;

/*! Event data for MeshPrvClStartPbAdvProvisioning API */
typedef struct meshPrvClStartPbAdv_tag
{
  wsfMsgHdr_t                   hdr;
  uint8_t                       ifId;
  meshPrvClSessionInfo_t const  *pSessionInfo;
} meshPrvClStartPbAdv_t;

/*! Event data for MeshPrvClStartPbGattProvisioning API */
typedef struct meshPrvClStartPbGatt_tag
{
  wsfMsgHdr_t                   hdr;
  uint8_t                       connId;
  meshPrvClSessionInfo_t const  *pSessionInfo;
} meshPrvClStartPbGatt_t;

typedef struct meshPrvClRecvCapab_tag
{
  wsfMsgHdr_t   hdr;
  uint8_t       capabPdu[MESH_PRV_PDU_CAPAB_PDU_SIZE];
} meshPrvClRecvCapab_t;

typedef struct meshPrvClSelectAuthInternal_tag
{
  wsfMsgHdr_t           hdr;
  meshPrvClSelectAuth_t selectAuthParams;
} meshPrvClSelAuthParam_t;

typedef struct meshPrvClRecvPubKey_tag
{
  wsfMsgHdr_t   hdr;
  uint8_t       pubKeyPdu[MESH_PRV_PDU_PUB_KEY_PDU_SIZE];
} meshPrvClRecvPubKey_t;

/*! Event data for Enter Output OOB */
typedef struct meshPrvClEnterOob_tag
{
  wsfMsgHdr_t hdr;
  meshPrvOutputOobSize_t outputOobSize;
  meshPrvInOutOobData_t  outputOobData;
} meshPrvClEnterOob_t;

/*! Event data for Received Provisioning Confirmation */
typedef struct meshPrvClRecvConfirm_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     confirm[MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE];
} meshPrvClRecvConfirm_t;

/*! Event data for Received Provisioning Random */
typedef struct meshPrvClRecvRandom_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     random[MESH_PRV_PDU_RANDOM_RANDOM_SIZE];
} meshPrvClRecvRandom_t;

/*! Event data for Confirmation Ready */
typedef struct meshPrvClOwnConfirm_tag
{
  wsfMsgHdr_t hdr;
  uint8_t     confirmation[MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE];
} meshPrvClOwnConfirm_t;

/*! Union of event handler data types */
typedef union meshPrvClSmMsg_tag
{
  wsfMsgHdr_t             hdr;            /*! Header structure */
  meshPrvClStartPbAdv_t   startPbAdv;
  meshPrvClStartPbGatt_t  startPbGatt;
  meshPrvClRecvPubKey_t   recvPubKey;
  meshPrvClRecvCapab_t    recvCapab;
  meshPrvClSelAuthParam_t selectAuth;
  meshPrvClEnterOob_t     enterOob;
  meshPrvClRecvConfirm_t  recvConfirm;
  meshPrvClRecvRandom_t   recvRandom;
  meshPrvClOwnConfirm_t   ownConfirm;
} meshPrvClSmMsg_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Control block */
extern meshPrvClCb_t meshPrvClCb;
/*! State machine instance */
extern const meshPrvClSmIf_t meshPrvClSmIf;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*! State machine execute function */
void meshPrvClSmExecute(meshPrvClCb_t *pCcb, meshPrvClSmMsg_t *pMsg);

/* State machine action functions */
void meshPrvClActNone(void *pCcb, void *pMsg);
void meshPrvClActLinkFailed(void *pCcb, void *pMsg);
void meshPrvClActLinkClosed(void *pCcb, void *pMsg);
void meshPrvClActProtocolError(void *pCcb, void *pMsg);
void meshPrvClActRecvTimeout(void *pCcb, void *pMsg);
void meshPrvClActSendTimeout(void *pCcb, void *pMsg);
void meshPrvClActSuccess(void *pCcb, void *pMsg);
void meshPrvClActOpenLink(void *pCcb, void *pMsg);
void meshPrvClActSendInvite(void *pCcb, void *pMsg);
void meshPrvClActWaitCapabilities(void *pCcb, void *pMsg);
void meshPrvClActWaitSelectAuth(void *pCcb, void *pMsg);
void meshPrvClActSendStart(void *pCcb, void *pMsg);
void meshPrvClActSendPublicKey(void *pCcb, void *pMsg);
void meshPrvClActWaitPublicKey(void *pCcb, void *pMsg);
void meshPrvClActValidatePublicKey(void *pCcb, void *pMsg);
void meshPrvClActGeneratePublicKey(void *pCcb, void *pMsg);
void meshPrvClActPublicKeyInvalid(void *pCcb, void *pMsg);
void meshPrvClActPrepareOobAction(void *pCcb, void *pMsg);
void meshPrvClActWaitInput(void *pCcb, void *pMsg);
void meshPrvClActWaitInputComplete(void *pCcb, void *pMsg);
void meshPrvClActCalcConfirmation(void *pCcb, void *pMsg);
void meshPrvClActSendConfirmation(void *pCcb, void *pMsg);
void meshPrvClActWaitConfirmation(void *pCcb, void *pMsg);
void meshPrvClActSendRandom(void *pCcb, void *pMsg);
void meshPrvClActWaitRandom(void *pCcb, void *pMsg);
void meshPrvClActCheckConfirmation(void *pCcb, void *pMsg);
void meshPrvClActConfirmationFailed(void *pCcb, void *pMsg);
void meshPrvClActCalcSessionKey(void *pCcb, void *pMsg);
void meshPrvClActEncryptData(void *pCcb, void *pMsg);
void meshPrvClActSendData(void *pCcb, void *pMsg);
void meshPrvClActWaitComplete(void *pCcb, void *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_CL_MAIN_H */
