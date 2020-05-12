/*************************************************************************************************/
/*!
 *  \file   mesh_prv_sr_sm.c
 *
 *  \brief  Mesh Provisioning Server state machine.
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

#include "wsf_types.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_main.h"
#include "mesh_prv_sr_api.h"
#include "mesh_prv_br_main.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/
/* State machine table constants */
#define MESH_PRV_SR_SM_POS_EVENT          0       /* Column position for event */
#define MESH_PRV_SR_SM_POS_NEXT_STATE     1       /* Column position for next state */
#define MESH_PRV_SR_SM_POS_ACTION         2       /* Column position for action */
#define MESH_PRV_SR_STATE_TBL_COMMON_MAX  15      /* Number of entries in the common state table */

/* Action function enumeration */
enum
{
  PRV_SR_ACT_NONE,                       /* No action */
  PRV_SR_ACT_LINK_CLOSED,                /* End provisioning when link was closed */
  PRV_SR_ACT_RECV_TIMEOUT,               /* End provisioning when a Provisioning PDU was not received */
  PRV_SR_ACT_SEND_TIMEOUT,               /* End provisioning when unable to send a Provisioning PDU */
  PRV_SR_ACT_SUCCESS,                    /* End provisioning in success */
  PRV_SR_ACT_WAIT_LINK,                  /* Wait for Link Opened event */
  PRV_SR_ACT_WAIT_INVITE,                /* (1)  Wait for Provisioning Invite PDU */
  PRV_SR_ACT_SEND_CAPABILITIES,          /* (1)  Send Provisioning Capabilities PDU */
  PRV_SR_ACT_WAIT_START,                 /* (2)  Wait for Provisioning Start PDU */
  PRV_SR_ACT_WAIT_PUBLIC_KEY,            /* (2)  Wait for Provisioning Public Key PDU */
  PRV_SR_ACT_GENERATE_PUBLIC_KEY,        /* (2)  Generate own Public Key */
  PRV_SR_ACT_VALIDATE_PUBLIC_KEY,        /* (2)  Validate peer's Public Key */
  PRV_SR_ACT_SEND_PUBLIC_KEY,            /* (2a) Send Provisioning Public Key PDU */
  PRV_SR_ACT_PREPARE_OOB_ACTION,         /* (3)  Prepare OOB action */
  PRV_SR_ACT_WAIT_INPUT,                 /* (3b) Wait for user input */
  PRV_SR_ACT_SEND_INPUT_COMPLETE,        /* (3b) Send Provisioning Input Complete PDU */
  PRV_SR_ACT_WAIT_CONFIRMATION,          /* (3)  Wait for Provisioning Confirmation PDU */
  PRV_SR_ACT_SAVE_CONFIRMATION,          /* (3)  Save the provisioning confirmation */
  PRV_SR_ACT_CALC_CONFIRMATION,          /* (3)  Calculate the provisioning confirmation */
  PRV_SR_ACT_SEND_CONFIRMATION,          /* (3)  Send Provisioning Confirmation PDU */
  PRV_SR_ACT_WAIT_RANDOM,                /* (3)  Wait for Provisioning Random PDU */
  PRV_SR_ACT_CHECK_CONFIRMATION,         /* (3)  Check Confirmation */
  PRV_SR_ACT_CALC_SESSION_KEY,           /* (3)  Calculate Session Key */
  PRV_SR_ACT_SEND_RANDOM,                /* (3)  Send Provisioning Random PDU */
  PRV_SR_ACT_WAIT_DATA,                  /* (4)  Wait for Provisioning Data PDU */
  PRV_SR_ACT_DECRYPT_DATA,               /* (4)  Decrypt the provisioning data */
  PRV_SR_ACT_SEND_COMPLETE,              /* (4)  Send Provisioning Complete PDU */
  PRV_SR_ACT_SEND_UNEXPECTED_PDU,        /* Sends Provisioning Failed with Unexpected PDU
                                          * while in an error state
                                          */
  PRV_SR_ACT_HANDLE_PROTOCOL_ERROR,      /* Handles an error in the protocol and sends
                                          * Provisioning Failed PDU
                                          */
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Action function table; order matches action function enumeration */
static const meshPrvSrAct_t prvSrActionTbl[] =
{
  meshPrvSrActNone,                       /* No action */
  meshPrvSrActLinkClosed,                 /* End provisioning when link was closed */
  meshPrvSrActRecvTimeout,                /* End provisioning when a Provisioning PDU was not received */
  meshPrvSrActSendTimeout,                /* End provisioning when unable to send a Provisioning PDU */
  meshPrvSrActSuccess,                    /* End provisioning in success */
  meshPrvSrActWaitLink,                   /* Wait for Link Opened event */
  meshPrvSrActWaitInvite,                 /* (1)  Wait for Provisioning Invite PDU */
  meshPrvSrActSendCapabilities,           /* (1)  Send Provisioning Capabilities PDU */
  meshPrvSrActWaitStart,                  /* (2)  Wait for Provisioning Start PDU */
  meshPrvSrActWaitPublicKey,              /* (2)  Wait for Provisioning Public Key PDU */
  meshPrvSrActGeneratePublicKey,          /* (2)  Generate own Public Key */
  meshPrvSrActValidatePublicKey,          /* (2)  Validate peer's Public Key */
  meshPrvSrActSendPublicKey,              /* (2a) Send Provisioning Public Key PDU */
  meshPrvSrActPrepareOobAction,           /* (3)  Prepare OOB action */
  meshPrvSrActWaitInput,                  /* (3b) Wait for user input */
  meshPrvSrActSendInputComplete,          /* (3b) Send Provisioning Input Complete PDU */
  meshPrvSrActWaitConfirmation,           /* (3)  Wait for Provisioning Confirmation PDU */
  meshPrvSrActSaveConfirmation,           /* (3)  Save the peer provisioning confirmation */
  meshPrvSrActCalcConfirmation,           /* (3)  Calculate the provisioning confirmation */
  meshPrvSrActSendConfirmation,           /* (3)  Send Provisioning Confirmation PDU */
  meshPrvSrActWaitRandom,                 /* (3)  Wait for Provisioning Random PDU */
  meshPrvSrActCheckConfirmation,          /* (3)  Check Confirmation */
  meshPrvSrActCalcSessionKey,             /* (3)  Calculate Session Key */
  meshPrvSrActSendRandom,                 /* (3)  Send Provisioning Random PDU */
  meshPrvSrActWaitData,                   /* (4)  Wait for Provisioning Data PDU */
  meshPrvSrActDecryptData,                /* (4)  Decrypt the provisioning data */
  meshPrvSrActSendComplete,               /* (4)  Send Provisioning Complete PDU */
  meshPrvSrActSendUnexpectedPdu,          /* Send provisioning failed PDU */
  meshPrvSrActHandleProtocolError,        /* Send provisioning failed PDU */
};

/*! State table for common actions */
static const meshPrvSrTblEntry_t prvSrStateTblCommon[MESH_PRV_SR_STATE_TBL_COMMON_MAX] =
{
  /* Event                          Next state                    Action */
  { PRV_SR_EVT_LINK_CLOSED_FAIL,    PRV_SR_ST_IDLE,               PRV_SR_ACT_LINK_CLOSED },
  { PRV_SR_EVT_LINK_CLOSED_SUCCESS, PRV_SR_ST_IDLE,               PRV_SR_ACT_LINK_CLOSED },
  { PRV_SR_EVT_RECV_TIMEOUT,        PRV_SR_ST_IDLE,               PRV_SR_ACT_RECV_TIMEOUT },
  { PRV_SR_EVT_SEND_TIMEOUT,        PRV_SR_ST_IDLE,               PRV_SR_ACT_SEND_TIMEOUT },
  { PRV_SR_EVT_RECV_BAD_PDU,        PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_RECV_INVITE,         PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_RECV_START,          PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_RECV_PUBLIC_KEY,     PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_RECV_CONFIRMATION,   PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_RECV_RANDOM,         PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_RECV_DATA,           PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_PUBLIC_KEY_INVALID,  PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_CONFIRMATION_FAILED, PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { PRV_SR_EVT_DATA_NOT_DECRYPTED,  PRV_SR_ST_LINK_FAILED,        PRV_SR_ACT_HANDLE_PROTOCOL_ERROR },
  { 0,                              0,                            0}
};

/*! State table for maintaining the error state until link is closed */
static const meshPrvSrTblEntry_t prvSrStateLinkFailed[] =
{
  /* Event                          Next state                    Action */
  { PRV_SR_EVT_RECV_BAD_PDU,        PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_RECV_INVITE,         PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_RECV_START,          PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_RECV_PUBLIC_KEY,     PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_RECV_CONFIRMATION,   PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_RECV_RANDOM,         PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_RECV_DATA,           PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_PUBLIC_KEY_INVALID,  PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_CONFIRMATION_FAILED, PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { PRV_SR_EVT_DATA_NOT_DECRYPTED,  PRV_SR_ST_NO_STATE_CHANGE,    PRV_SR_ACT_SEND_UNEXPECTED_PDU },
  { 0,                              0,                            0}
};

/*! State table for IDLE */
static const meshPrvSrTblEntry_t prvSrStateTblIdle[] =
{
  /* Event                            Next state                   Action  */
  { PRV_SR_EVT_BEGIN_NO_LINK,        PRV_SR_ST_WAIT_LINK,         PRV_SR_ACT_WAIT_LINK },
  { PRV_SR_EVT_BEGIN_LINK_OPEN,      PRV_SR_ST_WAIT_INVITE,       PRV_SR_ACT_WAIT_INVITE },
  { PRV_SR_EVT_LINK_CLOSED_FAIL,     PRV_SR_ST_IDLE,              PRV_SR_ACT_NONE },
  { PRV_SR_EVT_LINK_CLOSED_SUCCESS,  PRV_SR_ST_IDLE,              PRV_SR_ACT_NONE },
  { 0,                               0,                           0}
};

/*! State table for WAIT_LINK */
static const meshPrvSrTblEntry_t prvSrStateTblWaitLink[] =
{
  /* Event                        Next state                 Action  */
  { PRV_SR_EVT_BEGIN_LINK_OPEN,  PRV_SR_ST_WAIT_INVITE,     PRV_SR_ACT_WAIT_INVITE },
  { PRV_SR_EVT_LINK_OPENED,      PRV_SR_ST_WAIT_INVITE,     PRV_SR_ACT_WAIT_INVITE },
  { 0,                           0,                         0 }
};

/*! State table for WAIT_INVITE */
static const meshPrvSrTblEntry_t prvSrStateTblWaitInvite[] =
{
  /* Event                        Next state                    Action  */
  { PRV_SR_EVT_RECV_INVITE,      PRV_SR_ST_SEND_CAPABILITIES,  PRV_SR_ACT_SEND_CAPABILITIES },
  { 0,                           0,                            0 }
};

/*! State table for SEND_CAPABILITIES */
static const meshPrvSrTblEntry_t prvSrStateTblSendCapabilities[] =
{
  /* Event                          Next state                 Action  */
  { PRV_SR_EVT_SENT_CAPABILITIES,  PRV_SR_ST_WAIT_START,      PRV_SR_ACT_WAIT_START },
  { PRV_SR_EVT_RECV_START,         PRV_SR_ST_WAIT_PUBLIC_KEY, PRV_SR_ACT_WAIT_PUBLIC_KEY }, /* Only if device missed ACKs */
  { 0,                             0,                         0 }
};

/*! State table for WAIT_START */
static const meshPrvSrTblEntry_t prvSrStateTblWaitStart[] =
{
  /* Event                        Next state                   Action  */
  { PRV_SR_EVT_RECV_START,       PRV_SR_ST_WAIT_PUBLIC_KEY,   PRV_SR_ACT_WAIT_PUBLIC_KEY },
  { 0,                           0,                           0 }
};

/*! State table for WAIT_PUBLIC_KEY */
static const meshPrvSrTblEntry_t prvSrStateTblWaitPublicKey[] =
{
  /* Event                       Next state                      Action  */
  { PRV_SR_EVT_RECV_PUBLIC_KEY,  PRV_SR_ST_GENERATE_PUBLIC_KEY,  PRV_SR_ACT_GENERATE_PUBLIC_KEY },
                                                                 /* This action will simulate the
                                                                    PRV_SR_EVT_PUBLIC_KEY_GENERATED event if the
                                                                    Provisioning Server is using a Public Key
                                                                    provided by the application
                                                                 */
  { 0,                           0,                              0 }
};

/*! State table for GENERATE_PUBLIC_KEY */
static const meshPrvSrTblEntry_t prvSrStateTblGeneratePublicKey[] =
{
  /* Event                              Next state                     Action  */
  { PRV_SR_EVT_PUBLIC_KEY_GENERATED,   PRV_SR_ST_VALIDATE_PUBLIC_KEY,  PRV_SR_ACT_VALIDATE_PUBLIC_KEY },
  { PRV_SR_EVT_RECV_CONFIRMATION,      PRV_SR_ST_NO_STATE_CHANGE,      PRV_SR_ACT_SAVE_CONFIRMATION },
  { 0,                                 0,                              0 }
};

/*! State table for VALIDATE_PUBLIC_KEY */
static const meshPrvSrTblEntry_t prvSrStateTblValidatePublicKey[] =
{
  /* Event                            Next state                    Action  */
  { PRV_SR_EVT_PUBLIC_KEY_VALID,     PRV_SR_ST_SEND_PUBLIC_KEY,     PRV_SR_ACT_SEND_PUBLIC_KEY },
                                                                   /* This action will simulate the
                                                                      PRV_SR_EVT_SENT_PUBLIC_KEY event if the
                                                                      Provisioning Client has the public key
                                                                      of the Provisioning Server from OOB
                                                                    */
  { PRV_SR_EVT_RECV_CONFIRMATION,    PRV_SR_ST_NO_STATE_CHANGE,     PRV_SR_ACT_SAVE_CONFIRMATION },
  { 0,                               0,                             0 }
};

/*! State table for SEND_PUBLIC_KEY */
static const meshPrvSrTblEntry_t prvSrStateTblSendPublicKey[] =
{
  /* Event                           Next state                     Action  */
  { PRV_SR_EVT_SENT_PUBLIC_KEY,     PRV_SR_ST_PREPARE_OOB_ACTION,  PRV_SR_ACT_PREPARE_OOB_ACTION },
                                                                  /* This action will generate the
                                                                     PRV_SR_EVT_GOTO_INPUT event if the
                                                                     OOB type is Input or the
                                                                     PRV_SR_EVT_GOTO_CONFIRMATION event
                                                                     if the OOB type is Output, Static Or None
                                                                  */
  { PRV_SR_EVT_RECV_CONFIRMATION,   PRV_SR_ST_CALC_CONFIRMATION,   PRV_SR_ACT_CALC_CONFIRMATION }, /* Only if device missed ACKs */
  { 0,                              0,                             0 }
};

/*! State table for PREPARE_OOB_ACTION */
static const meshPrvSrTblEntry_t prvSrStateTblPrepareOobAction[] =
{
  /* Event                          Next state                    Action  */
  { PRV_SR_EVT_GOTO_INPUT,         PRV_SR_ST_WAIT_INPUT,         PRV_SR_ACT_WAIT_INPUT },
  { PRV_SR_EVT_GOTO_CONFIRMATION,  PRV_SR_ST_WAIT_CONFIRMATION,  PRV_SR_ACT_WAIT_CONFIRMATION },
  { PRV_SR_EVT_RECV_CONFIRMATION,  PRV_SR_ST_CALC_CONFIRMATION,  PRV_SR_ACT_CALC_CONFIRMATION }, /* Only if device missed ACKs */
  { 0,                             0,                            0 }
};

/*! State table for WAIT_INPUT */
static const meshPrvSrTblEntry_t prvSrStateTblWaitInput[] =
{
  /* Event                        Next state                       Action  */
  { PRV_SR_EVT_INPUT_READY,      PRV_SR_ST_SEND_INPUT_COMPLETE,   PRV_SR_ACT_SEND_INPUT_COMPLETE },
  { 0,                           0,                               0 }
};

/*! State table for SEND_INPUT_COMPLETE */
static const meshPrvSrTblEntry_t prvSrStateTblSendInputComplete[] =
{
  /* Event                            Next state                     Action  */
  { PRV_SR_EVT_SENT_INPUT_COMPLETE,  PRV_SR_ST_WAIT_CONFIRMATION,   PRV_SR_ACT_WAIT_CONFIRMATION },
  { PRV_SR_EVT_RECV_CONFIRMATION,    PRV_SR_ST_CALC_CONFIRMATION,   PRV_SR_ACT_CALC_CONFIRMATION }, /* Only if device missed ACKs */
  { 0,                               0,                             0 }
};

/*! State table for WAIT_CONFIRMATION */
static const meshPrvSrTblEntry_t prvSrStateTblWaitConfirmation[] =
{
  /* Event                           Next state                     Action  */
  { PRV_SR_EVT_RECV_CONFIRMATION,   PRV_SR_ST_CALC_CONFIRMATION,   PRV_SR_ACT_CALC_CONFIRMATION },
  { 0,                              0,                             0 }
};

/*! State table for CALC_CONFIRMATION */
static const meshPrvSrTblEntry_t prvSrStateTblCalcConfirmation[] =
{
  /* Event                            Next state                     Action  */
  { PRV_SR_EVT_CONFIRMATION_READY,   PRV_SR_ST_SEND_CONFIRMATION,   PRV_SR_ACT_SEND_CONFIRMATION },
  { 0,                               0,                             0 }
};

/*! State table for SEND_CONFIRMATION */
static const meshPrvSrTblEntry_t prvSrStateTblSendConfirmation[] =
{
  /* Event                         Next state                     Action  */
  { PRV_SR_EVT_SENT_CONFIRMATION, PRV_SR_ST_WAIT_RANDOM,         PRV_SR_ACT_WAIT_RANDOM },
  { PRV_SR_EVT_RECV_RANDOM,       PRV_SR_ST_CHECK_CONFIRMATION,  PRV_SR_ACT_CHECK_CONFIRMATION }, /* Only if device missed ACKs */
  { 0,                            0,                             0 }
};

/*! State table for WAIT_RANDOM */
static const meshPrvSrTblEntry_t prvSrStateTblWaitRandom[] =
{
  /* Event                        Next state                        Action  */
  { PRV_SR_EVT_RECV_RANDOM,      PRV_SR_ST_CHECK_CONFIRMATION,     PRV_SR_ACT_CHECK_CONFIRMATION },
  { 0,                           0,                                0 }
};

/*! State table for CHECK_CONFIRMATION */
static const meshPrvSrTblEntry_t prvSrStateTblCheckConfirmation[] =
{
  /* Event                               Next state                   Action  */
  { PRV_SR_EVT_CONFIRMATION_VERIFIED,   PRV_SR_ST_CALC_SESSION_KEY,  PRV_SR_ACT_CALC_SESSION_KEY },
  { 0,                                  0,                           0 }
};

/*! State table for CALC_SESSION_KEY */
static const meshPrvSrTblEntry_t prvSrStateTblCalcSessionKey[] =
{
  /* Event                          Next state                  Action  */
  { PRV_SR_EVT_SESSION_KEY_READY,  PRV_SR_ST_SEND_RANDOM,      PRV_SR_ACT_SEND_RANDOM },
  { 0,                             0,                          0 }
};

/*! State table for SEND_RANDOM */
static const meshPrvSrTblEntry_t prvSrStateTblSendRandom[] =
{
  /* Event                        Next state                  Action  */
  { PRV_SR_EVT_SENT_RANDOM,      PRV_SR_ST_WAIT_DATA,        PRV_SR_ACT_WAIT_DATA },
  { PRV_SR_EVT_RECV_DATA,        PRV_SR_ST_DECRYPT_DATA,     PRV_SR_ACT_DECRYPT_DATA }, /* Only if device missed ACKs */
  { 0,                           0,                          0 }
};

/*! State table for WAIT_DATA */
static const meshPrvSrTblEntry_t prvSrStateTblWaitData[] =
{
  /* Event                        Next state                  Action  */
  { PRV_SR_EVT_RECV_DATA,        PRV_SR_ST_DECRYPT_DATA,     PRV_SR_ACT_DECRYPT_DATA },
  { 0,                           0,                          0 }
};

/*! State table for DECRYPT_DATA */
static const meshPrvSrTblEntry_t prvSrStateTblDecryptData[] =
{
  /* Event                           Next state                 Action  */
  { PRV_SR_EVT_DATA_DECRYPTED,      PRV_SR_ST_SEND_COMPLETE,   PRV_SR_ACT_SEND_COMPLETE },
  { 0,                              0,                         0 }
};

/*! State table for SEND_COMPLETE */
static const meshPrvSrTblEntry_t prvSrStateTblSendComplete[] =
{
  /* Event                           Next state                 Action  */
  { PRV_SR_EVT_SENT_COMPLETE,       PRV_SR_ST_IDLE,            PRV_SR_ACT_SUCCESS },
  { PRV_SR_EVT_LINK_CLOSED_SUCCESS, PRV_SR_ST_IDLE,            PRV_SR_ACT_SUCCESS },
  { PRV_SR_EVT_SEND_TIMEOUT,        PRV_SR_ST_IDLE,            PRV_SR_ACT_SUCCESS },
  { 0,                              0,                         0 }
};

/*! Table of individual state tables */
const meshPrvSrTblEntry_t * const prvSrStateTbl[] =
{
  prvSrStateTblIdle,
  prvSrStateTblWaitLink,
  prvSrStateTblWaitInvite,
  prvSrStateTblSendCapabilities,
  prvSrStateTblWaitStart,
  prvSrStateTblWaitPublicKey,
  prvSrStateTblGeneratePublicKey,
  prvSrStateTblValidatePublicKey,
  prvSrStateTblSendPublicKey,
  prvSrStateTblPrepareOobAction,
  prvSrStateTblWaitInput,
  prvSrStateTblSendInputComplete,
  prvSrStateTblWaitConfirmation,
  prvSrStateTblCalcConfirmation,
  prvSrStateTblSendConfirmation,
  prvSrStateTblWaitRandom,
  prvSrStateTblCheckConfirmation,
  prvSrStateTblCalcSessionKey,
  prvSrStateTblSendRandom,
  prvSrStateTblWaitData,
  prvSrStateTblDecryptData,
  prvSrStateTblSendComplete,
  prvSrStateLinkFailed
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! state machine interface */
const meshPrvSrSmIf_t meshPrvSrSmIf =
{
  prvSrStateTbl,
  prvSrActionTbl,
  prvSrStateTblCommon
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Convert state into string for diagnostics.
 *
 *  \param[in] state  State enum value.
 *
 *  \return    State string.
 */
/*************************************************************************************************/
static uint8_t *meshPrvSrStateStr(meshPrvSrSmState_t state)
{
  switch (state)
  {
    case PRV_SR_ST_IDLE: return (uint8_t*) "IDLE";
    case PRV_SR_ST_WAIT_LINK: return (uint8_t*) "WAIT_LINK";
    case PRV_SR_ST_WAIT_INVITE: return (uint8_t*) "WAIT_INVITE";
    case PRV_SR_ST_SEND_CAPABILITIES: return (uint8_t*) "SEND_CAPABILITIES";
    case PRV_SR_ST_WAIT_START: return (uint8_t*) "WAIT_START";
    case PRV_SR_ST_WAIT_PUBLIC_KEY: return (uint8_t*) "WAIT_PUBLIC_KEY";
    case PRV_SR_ST_VALIDATE_PUBLIC_KEY: return (uint8_t*) "VALIDATE_PUBLIC_KEY";
    case PRV_SR_ST_GENERATE_PUBLIC_KEY: return (uint8_t*) "GENERATE_PUBLIC_KEY";
    case PRV_SR_ST_SEND_PUBLIC_KEY: return (uint8_t*) "SEND_PUBLIC_KEY";
    case PRV_SR_ST_PREPARE_OOB_ACTION: return (uint8_t*) "PREPARE_OOB_ACTION";
    case PRV_SR_ST_WAIT_INPUT: return (uint8_t*) "WAIT_INPUT";
    case PRV_SR_ST_SEND_INPUT_COMPLETE: return (uint8_t*) "SEND_INPUT_COMPLETE";
    case PRV_SR_ST_WAIT_CONFIRMATION: return (uint8_t*) "WAIT_CONFIRMATION";
    case PRV_SR_ST_CALC_CONFIRMATION: return (uint8_t*) "CALC_CONFIRMATION";
    case PRV_SR_ST_SEND_CONFIRMATION: return (uint8_t*) "SEND_CONFIRMATION";
    case PRV_SR_ST_WAIT_RANDOM: return (uint8_t*) "WAIT_RANDOM";
    case PRV_SR_ST_CHECK_CONFIRMATION: return (uint8_t*) "CHECK_CONFIRMATION";
    case PRV_SR_ST_CALC_SESSION_KEY: return (uint8_t*) "CALC_SESSION_KEY";
    case PRV_SR_ST_SEND_RANDOM: return (uint8_t*) "SEND_RANDOM";
    case PRV_SR_ST_WAIT_DATA: return (uint8_t*) "WAIT_DATA";
    case PRV_SR_ST_DECRYPT_DATA: return (uint8_t*) "DECRYPT_DATA";
    case PRV_SR_ST_SEND_COMPLETE: return (uint8_t*) "SEND_COMPLETE";

    case PRV_SR_ST_NO_STATE_CHANGE: return (uint8_t*) "NO_STATE_CHANGE";

    default: return (uint8_t*) "Unknown";
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Convert event into string for diagnostics.
 *
 *  \param[in] evt  Event enum value.
 *
 *  \return    Event string.
 */
/*************************************************************************************************/
static uint8_t *meshPrvSrEvtStr(meshPrvSrSmEvt_t evt)
{
  switch (evt)
  {
    case PRV_SR_EVT_BEGIN_NO_LINK: return (uint8_t*) "BEGIN_NO_LINK";
    case PRV_SR_EVT_BEGIN_LINK_OPEN: return (uint8_t*) "BEGIN_LINK_OPEN";
    case PRV_SR_EVT_LINK_OPENED: return (uint8_t*) "LINK_OPENED";
    case PRV_SR_EVT_LINK_CLOSED_FAIL: return (uint8_t*) "LINK_CLOSED_FAIL";
    case PRV_SR_EVT_LINK_CLOSED_SUCCESS: return (uint8_t*) "LINK_CLOSED_SUCCESS";
    case PRV_SR_EVT_RECV_TIMEOUT: return (uint8_t*) "RECV_TIMEOUT";
    case PRV_SR_EVT_SEND_TIMEOUT: return (uint8_t*) "SEND_TIMEOUT";
    case PRV_SR_EVT_SENT_CAPABILITIES: return (uint8_t*) "SENT_CAPABILITIES";
    case PRV_SR_EVT_SENT_PUBLIC_KEY: return (uint8_t*) "SENT_PUBLIC_KEY";
    case PRV_SR_EVT_SENT_INPUT_COMPLETE: return (uint8_t*) "SENT_INPUT_COMPLETE";
    case PRV_SR_EVT_SENT_CONFIRMATION: return (uint8_t*) "SENT_CONFIRMATION";
    case PRV_SR_EVT_SENT_RANDOM: return (uint8_t*) "SENT_RANDOM";
    case PRV_SR_EVT_SENT_COMPLETE: return (uint8_t*) "SENT_COMPLETE";
    case PRV_SR_EVT_GOTO_INPUT: return (uint8_t*) "GOTO_INPUT";
    case PRV_SR_EVT_GOTO_CONFIRMATION: return (uint8_t*) "GOTO_CONFIRMATION";
    case PRV_SR_EVT_INPUT_READY: return (uint8_t*) "INPUT_READY";
    case PRV_SR_EVT_CONFIRMATION_READY: return (uint8_t*) "CONFIRMATION_READY";
    case PRV_SR_EVT_CONFIRMATION_VERIFIED: return (uint8_t*) "CONFIRMATION_VERIFIED";
    case PRV_SR_EVT_CONFIRMATION_FAILED: return (uint8_t*) "CONFIRMATION_FAILED";
    case PRV_SR_EVT_SESSION_KEY_READY: return (uint8_t*) "SESSION_KEY_READY";
    case PRV_SR_EVT_RECV_INVITE: return (uint8_t*) "RECV_INVITE";
    case PRV_SR_EVT_RECV_START: return (uint8_t*) "RECV_START";
    case PRV_SR_EVT_RECV_PUBLIC_KEY: return (uint8_t*) "RECV_PUBLIC_KEY";
    case PRV_SR_EVT_PUBLIC_KEY_VALID: return (uint8_t*) "PUBLIC_KEY_VALID";
    case PRV_SR_EVT_PUBLIC_KEY_INVALID: return (uint8_t*) "PUBLIC_KEY_INVALID";
    case PRV_SR_EVT_PUBLIC_KEY_GENERATED: return (uint8_t*) "PUBLIC_KEY_GENERATED";
    case PRV_SR_EVT_RECV_CONFIRMATION: return (uint8_t*) "RECV_CONFIRMATION";
    case PRV_SR_EVT_RECV_RANDOM: return (uint8_t*) "RECV_RANDOM";
    case PRV_SR_EVT_RECV_DATA: return (uint8_t*) "RECV_DATA";
    case PRV_SR_EVT_DATA_DECRYPTED: return (uint8_t*) "DATA_DECRYPTED";
    case PRV_SR_EVT_DATA_NOT_DECRYPTED: return (uint8_t*) "DATA_NOT_DECRYPTED";
    case PRV_SR_EVT_RECV_BAD_PDU: return (uint8_t*) "RECV_BAD_PDU";

    default: return (uint8_t*) "Unknown";
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Execute the Provisioning Server state machine.
 *
 *  \param[in] pCcb  Connection control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvSrSmExecute(meshPrvSrCb_t *pCcb, meshPrvSrSmMsg_t *pMsg)
{
  meshPrvSrTblEntry_t const *pTblEntry;
  meshPrvSrSmIf_t const *pSmIf = pCcb->pSm;
  meshPrvSrSmState_t oldState;

  /* Silence compiler warnings when trace is disabled */
  (void)meshPrvSrEvtStr((meshPrvSrSmEvt_t)0);
  (void)meshPrvSrStateStr((meshPrvSrSmState_t)0);

  pTblEntry = prvSrStateTbl[pCcb->state];

  MESH_TRACE_INFO2("MESH_PRV_SR_SM Event Handler: state=%s event=%s",
    meshPrvSrStateStr(pCcb->state),
    meshPrvSrEvtStr(pMsg->hdr.event));

  /* run through state machine twice; once with state table for current state
  * and once with the state table for common events
  */
  for (;;)
  {
    /* look for event match and execute action */
    do
    {
      /* if match */
      if ((*pTblEntry)[MESH_PRV_SR_SM_POS_EVENT] == pMsg->hdr.event)
      {
        if ((*pTblEntry)[MESH_PRV_SR_SM_POS_NEXT_STATE] != PRV_SR_ST_NO_STATE_CHANGE)
        {
          /* set next state */
          oldState = pCcb->state;
          pCcb->state = (*pTblEntry)[MESH_PRV_SR_SM_POS_NEXT_STATE];
          MESH_TRACE_INFO2("MESH_PRV_SR_SM State Change: old=%s new=%s",
                           meshPrvSrStateStr(oldState),
                           meshPrvSrStateStr(pCcb->state));
        }
        else
        {
          /* state does not change */
          MESH_TRACE_INFO1("MESH_PRV_SR_SM No State Change while in %s",
                           meshPrvSrStateStr(pCcb->state));
        }

        /* execute action */
        (*pSmIf->pActionTbl[(*pTblEntry)[MESH_PRV_SR_SM_POS_ACTION]])(pCcb, pMsg);

        (void)oldState;

        return;
      }

      /* next entry */
      pTblEntry++;

      /* while not at end */
    } while ((*pTblEntry)[MESH_PRV_SR_SM_POS_EVENT] != 0);

    /* if we've reached end of the common state table */
    if (pTblEntry == (pSmIf->pCommonTbl + MESH_PRV_SR_STATE_TBL_COMMON_MAX - 1))
    {
      /* we're done */
      break;
    }
    /* else we haven't run through common state table yet */
    else
    {
      /* set it up */
      pTblEntry = pSmIf->pCommonTbl;
    }
  }
}
