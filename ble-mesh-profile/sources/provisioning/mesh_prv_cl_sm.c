/*************************************************************************************************/
/*!
*  \file   mesh_prv_cl_sm.c
*
*  \brief  Mesh Provisioning Client state machine.
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

#include "wsf_types.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_cl_main.h"
#include "mesh_prv_cl_api.h"
#include "mesh_prv_br_main.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! State machine table constants */
#define MESH_PRV_CL_SM_POS_EVENT          0       /*! Column position for event */
#define MESH_PRV_CL_SM_POS_NEXT_STATE     1       /*! Column position for next state */
#define MESH_PRV_CL_SM_POS_ACTION         2       /*! Column position for action */
#define MESH_PRV_CL_STATE_TBL_COMMON_MAX  12      /*! Number of entries in the common state table */

/*! Action function enumeration */
enum
{
  PRV_CL_ACT_NONE,                  /*! No action */
  PRV_CL_ACT_LINK_FAILED,           /*! End provisioning when link opening failed */
  PRV_CL_ACT_LINK_CLOSED,           /*! End provisioning when link was closed */
  PRV_CL_ACT_PROTOCOL_ERROR,        /*! End provisioning on protocol error */
  PRV_CL_ACT_RECV_TIMEOUT,          /*! End provisioning when a Provisioning PDU was not received */
  PRV_CL_ACT_SEND_TIMEOUT,          /*! End provisioning when unable to send a Provisioning PDU */
  PRV_CL_ACT_SUCCESS,               /*! End provisioning in success */
  PRV_CL_ACT_OPEN_LINK,             /*! Open provisioning link */
  PRV_CL_ACT_SEND_INVITE,           /*! (1)  Send Provisioning Invite PDU */
  PRV_CL_ACT_WAIT_CAPABILITIES,     /*! (1)  Wait for Provisioning Capabilities PDU */
  PRV_CL_ACT_WAIT_SELECT_AUTH,      /*! (2)  Wait for user selection of authentication method */
  PRV_CL_ACT_SEND_START,            /*! (2)  Send Provisioning Start PDU */
  PRV_CL_ACT_GENERATE_PUBLIC_KEY,   /*! (2)  Generate own Public Key */
  PRV_CL_ACT_SEND_PUBLIC_KEY,       /*! (2)  Send Provisioning Public Key PDU */
  PRV_CL_ACT_WAIT_PUBLIC_KEY,       /*! (2a) Wait for Provisioning Public Key PDU */
  PRV_CL_ACT_VALIDATE_PUBLIC_KEY,   /*! (2)  Validate peer's Public Key */
  PRV_CL_ACT_PUBLIC_KEY_INVALID,    /*! (2)  End provisioning when peer's Public Key is invalid */
  PRV_CL_ACT_PREPARE_OOB_ACTION,    /*! (3)  Prepare OOB action */
  PRV_CL_ACT_WAIT_INPUT,            /*! (3b) Wait for user input */
  PRV_CL_ACT_WAIT_INPUT_COMPLETE,   /*! (3b) Wait for Provisioning Input Complete PDU */
  PRV_CL_ACT_CALC_CONFIRMATION,     /*! (3)  Calculate the provisioning confirmation */
  PRV_CL_ACT_SEND_CONFIRMATION,     /*! (3)  Send Provisioning Confirmation PDU */
  PRV_CL_ACT_WAIT_CONFIRMATION,     /*! (3)  Wait for Provisioning Confirmation PDU */
  PRV_CL_ACT_SEND_RANDOM,           /*! (3)  Send Provisioning Random PDU */
  PRV_CL_ACT_WAIT_RANDOM,           /*! (3)  Wait for Provisioning Random PDU */
  PRV_CL_ACT_CHECK_CONFIRMATION,    /*! (3)  Check Confirmation */
  PRV_CL_ACT_CONFIRMATION_FAILED,   /*! (3)  End provisioning on confirmation failure */
  PRV_CL_ACT_CALC_SESSION_KEY,      /*! (3)  Calculate Session Key */
  PRV_CL_ACT_ENCRYPT_DATA,          /*! (4)  Encrypt the provisioning data */
  PRV_CL_ACT_SEND_DATA,             /*! (4)  Send Provisioning Data PDU */
  PRV_CL_ACT_WAIT_COMPLETE,         /*! (4)  Wait for Provisioning Complete PDU */
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Action function table; order matches action function enumeration */
static const meshPrvClAct_t prvClActionTbl[] =
{
  meshPrvClActNone,                 /*! No action */
  meshPrvClActLinkFailed,           /*! End provisioning when link opening failed */
  meshPrvClActLinkClosed,           /*! End provisioning when link was closed */
  meshPrvClActProtocolError,        /*! End provisioning on protocol error */
  meshPrvClActRecvTimeout,          /*! End provisioning when a Provisioning PDU was not received */
  meshPrvClActSendTimeout,          /*! End provisioning when unable to send a Provisioning PDU */
  meshPrvClActSuccess,              /*! End provisioning in success */
  meshPrvClActOpenLink,             /*! Open provisioning link */
  meshPrvClActSendInvite,           /*! (1)  Send Provisioning Invite PDU */
  meshPrvClActWaitCapabilities,     /*! (1)  Wait for Provisioning Capabilities PDU */
  meshPrvClActWaitSelectAuth,       /*! (2)  Wait for user selection of authentication method */
  meshPrvClActSendStart,            /*! (2)  Send Provisioning Start PDU */
  meshPrvClActGeneratePublicKey,    /*! (2)  Generate own Public Key */
  meshPrvClActSendPublicKey,        /*! (2)  Send Provisioning Public Key PDU */
  meshPrvClActWaitPublicKey,        /*! (2a) Wait for Provisioning Public Key PDU */
  meshPrvClActValidatePublicKey,    /*! (2)  Validate peer's Public Key */
  meshPrvClActPublicKeyInvalid,     /*! (2)  End provisioning when peer's Public Key is invalid */
  meshPrvClActPrepareOobAction,     /*! (3)  Prepare OOB action */
  meshPrvClActWaitInput,            /*! (3b) Wait for user input */
  meshPrvClActWaitInputComplete,    /*! (3b) Wait for Provisioning Input Complete PDU */
  meshPrvClActCalcConfirmation,     /*! (3)  Calculate the provisioning confirmation */
  meshPrvClActSendConfirmation,     /*! (3)  Send Provisioning Confirmation PDU */
  meshPrvClActWaitConfirmation,     /*! (3)  Wait for Provisioning Confirmation PDU */
  meshPrvClActSendRandom,           /*! (3)  Send Provisioning Random PDU */
  meshPrvClActWaitRandom,           /*! (3)  Wait for Provisioning Random PDU */
  meshPrvClActCheckConfirmation,    /*! (3)  Check Confirmation */
  meshPrvClActConfirmationFailed,   /*! (3)  End provisioning on confirmation failure */
  meshPrvClActCalcSessionKey,       /*! (3)  Calculate Session Key */
  meshPrvClActEncryptData,          /*! (4)  Encrypt the provisioning data */
  meshPrvClActSendData,             /*! (4)  Send Provisioning Data PDU */
  meshPrvClActWaitComplete,         /*! (4)  Wait for Provisioning Complete PDU */
};

/*! State table for common actions */
static const meshPrvClTblEntry_t prvClStateTblCommon[MESH_PRV_CL_STATE_TBL_COMMON_MAX] =
{
  /* Event                            Next state                Action */
  { PRV_CL_EVT_LINK_CLOSED_FAIL,     PRV_CL_ST_IDLE,           PRV_CL_ACT_LINK_CLOSED },
  { PRV_CL_EVT_RECV_TIMEOUT,         PRV_CL_ST_IDLE,           PRV_CL_ACT_RECV_TIMEOUT },
  { PRV_CL_EVT_SEND_TIMEOUT,         PRV_CL_ST_IDLE,           PRV_CL_ACT_SEND_TIMEOUT },
  { PRV_CL_EVT_BAD_PDU,              PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  { PRV_CL_EVT_RECV_CAPABILITIES,    PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  { PRV_CL_EVT_RECV_PUBLIC_KEY,      PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  { PRV_CL_EVT_RECV_INPUT_COMPLETE,  PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  { PRV_CL_EVT_RECV_CONFIRMATION,    PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  { PRV_CL_EVT_RECV_RANDOM,          PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  { PRV_CL_EVT_RECV_COMPLETE,        PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  { PRV_CL_EVT_CANCEL,               PRV_CL_ST_IDLE,           PRV_CL_ACT_PROTOCOL_ERROR },
  {0,                                0,                        0}
};

/*! State table for IDLE */
static const meshPrvClTblEntry_t prvClStateTblIdle[] =
{
  /* Event                          Next state                   Action  */
  { PRV_CL_EVT_BEGIN_NO_LINK,      PRV_CL_ST_WAIT_LINK,         PRV_CL_ACT_OPEN_LINK },
  { PRV_CL_EVT_BEGIN_LINK_OPEN,    PRV_CL_ST_SEND_INVITE,       PRV_CL_ACT_SEND_INVITE },
  {0,                              0,                           0}
};

/*! State table for WAIT_LINK */
static const meshPrvClTblEntry_t prvClStateTblWaitLink[] =
{
  /* Event                        Next state                 Action  */
  { PRV_CL_EVT_LINK_OPENED,      PRV_CL_ST_SEND_INVITE,     PRV_CL_ACT_SEND_INVITE },
  { PRV_CL_EVT_LINK_FAILED,      PRV_CL_ST_IDLE,            PRV_CL_ACT_LINK_FAILED },
  { 0,                           0,                         0 }
};

/*! State table for SEND_INVITE */
static const meshPrvClTblEntry_t prvClStateTblSendInvite[] =
{
  /* Event                          Next state                    Action  */
  { PRV_CL_EVT_SENT_INVITE,        PRV_CL_ST_WAIT_CAPABILITIES,  PRV_CL_ACT_WAIT_CAPABILITIES },
  { PRV_CL_EVT_RECV_CAPABILITIES,  PRV_CL_ST_WAIT_SELECT_AUTH,   PRV_CL_ACT_WAIT_SELECT_AUTH }, /* Only if device missed ACKs */
  { 0,                             0,                            0 }
};

/*! State table for WAIT_CAPABILITIES */
static const meshPrvClTblEntry_t prvClStateTblWaitCapabilities[] =
{
  /* Event                          Next state                   Action  */
  { PRV_CL_EVT_RECV_CAPABILITIES,  PRV_CL_ST_WAIT_SELECT_AUTH,  PRV_CL_ACT_WAIT_SELECT_AUTH },
  { 0,                             0,                           0 }
};

/*! State table for WAIT_SELECT_AUTH */
static const meshPrvClTblEntry_t prvClStateTblWaitSelectAuth[] =
{
  /* Event                          Next state                 Action  */
  { PRV_CL_EVT_AUTH_SELECTED,      PRV_CL_ST_SEND_START,      PRV_CL_ACT_SEND_START },
  { 0,                             0,                         0 }
};

/*! State table for SEND_START */
static const meshPrvClTblEntry_t prvClStateTblSendStart[] =
{
  /* Event                        Next state                     Action  */
  { PRV_CL_EVT_SENT_START,       PRV_CL_ST_GENERATE_PUBLIC_KEY, PRV_CL_ACT_GENERATE_PUBLIC_KEY },
  { 0,                           0,                             0 }
};

/*! State table for GENERATE_PUBLIC_KEY */
static const meshPrvClTblEntry_t prvClStateTblGeneratePublicKey[] =
{
  /* Event                              Next state                  Action  */
  { PRV_CL_EVT_PUBLIC_KEY_GENERATED,   PRV_CL_ST_SEND_PUBLIC_KEY,  PRV_CL_ACT_SEND_PUBLIC_KEY },
  { 0,                                 0,                          0 }
};

/*! State table for SEND_PUBLIC_KEY */
static const meshPrvClTblEntry_t prvClStateTblSendPublicKey[] =
{
  /* Event                        Next state                      Action  */
  { PRV_CL_EVT_SENT_PUBLIC_KEY,  PRV_CL_ST_WAIT_PUBLIC_KEY,      PRV_CL_ACT_WAIT_PUBLIC_KEY },
                                                                 /* This action will simulate the
                                                                  PRV_CL_EVT_RECV_PUBLIC_KEY event if the
                                                                  Provisioning Client has the public key
                                                                  of the Provisioning Server from OOB
                                                                  and it uses that key
                                                                 */
  { PRV_CL_EVT_RECV_PUBLIC_KEY,  PRV_CL_ST_VALIDATE_PUBLIC_KEY,  PRV_CL_ACT_VALIDATE_PUBLIC_KEY }, /* Only if device missed ACKs */
  { 0,                           0,                              0 }
};

/*! State table for WAIT_PUBLIC_KEY */
static const meshPrvClTblEntry_t prvClStateTblWaitPublicKey[] =
{
  /* Event                           Next state                      Action  */
  { PRV_CL_EVT_RECV_PUBLIC_KEY,     PRV_CL_ST_VALIDATE_PUBLIC_KEY,  PRV_CL_ACT_VALIDATE_PUBLIC_KEY },
  { 0,                              0,                              0 }
};

/*! State table for VALIDATE_PUBLIC_KEY */
static const meshPrvClTblEntry_t prvClStateTblValidatePublicKey[] =
{
  /* Event                           Next state                     Action  */
  { PRV_CL_EVT_PUBLIC_KEY_VALID,    PRV_CL_ST_PREPARE_OOB_ACTION,  PRV_CL_ACT_PREPARE_OOB_ACTION },
                                                                    /* This action will change the state
                                                                     immediately based on the OOB method
                                                                    */
  { PRV_CL_EVT_PUBLIC_KEY_INVALID,  PRV_CL_ST_IDLE,                PRV_CL_ACT_PUBLIC_KEY_INVALID },
  { 0,                              0,                             0 }
};

/*! State table for PREPARE_OOB_ACTION */
static const meshPrvClTblEntry_t prvClStateTblPrepareOobAction[] =
{
  /* Event                           Next state                     Action  */
  { PRV_CL_EVT_GOTO_CONFIRMATION,   PRV_CL_ST_CALC_CONFIRMATION,   PRV_CL_ACT_CALC_CONFIRMATION },
  { PRV_CL_EVT_GOTO_WAIT_INPUT,     PRV_CL_ST_WAIT_INPUT,          PRV_CL_ACT_WAIT_INPUT },
  { PRV_CL_EVT_GOTO_WAIT_IC,        PRV_CL_ST_WAIT_INPUT_COMPLETE, PRV_CL_ACT_WAIT_INPUT_COMPLETE },
  { 0,                              0,                             0 }
};

/*! State table for WAIT_INPUT */
static const meshPrvClTblEntry_t prvClStateTblWaitInput[] =
{
  /* Event                        Next state                       Action  */
  { PRV_CL_EVT_INPUT_READY,      PRV_CL_ST_CALC_CONFIRMATION,     PRV_CL_ACT_CALC_CONFIRMATION },
  { 0,                           0,                               0 }
};

/*! State table for WAIT_INPUT_COMPLETE */
static const meshPrvClTblEntry_t prvClStateTblWaitInputComplete[] =
{
  /* Event                            Next state                     Action  */
  { PRV_CL_EVT_RECV_INPUT_COMPLETE,  PRV_CL_ST_CALC_CONFIRMATION,   PRV_CL_ACT_CALC_CONFIRMATION },
  { 0,                               0,                             0 }
};

/*! State table for CALC_CONFIRMATION */
static const meshPrvClTblEntry_t prvClStateTblCalcConfirmation[] =
{
  /* Event                            Next state                     Action  */
  { PRV_CL_EVT_CONFIRMATION_READY,   PRV_CL_ST_SEND_CONFIRMATION,   PRV_CL_ACT_SEND_CONFIRMATION },
  { 0,                               0,                             0 }
};

/*! State table for SEND_CONFIRMATION */
static const meshPrvClTblEntry_t prvClStateTblSendConfirmation[] =
{
  /* Event                           Next state                     Action  */
  { PRV_CL_EVT_SENT_CONFIRMATION,   PRV_CL_ST_WAIT_CONFIRMATION,   PRV_CL_ACT_WAIT_CONFIRMATION },
  { PRV_CL_EVT_RECV_CONFIRMATION,   PRV_CL_ST_SEND_RANDOM,         PRV_CL_ACT_SEND_RANDOM }, /* Only if device missed ACKs */
  { 0,                              0,                             0 }
};

/*! State table for WAIT_CONFIRMATION */
static const meshPrvClTblEntry_t prvClStateTblWaitConfirmation[] =
{
  /* Event                           Next state                     Action  */
  { PRV_CL_EVT_RECV_CONFIRMATION,   PRV_CL_ST_SEND_RANDOM,         PRV_CL_ACT_SEND_RANDOM },
  { 0,                              0,                             0 }
};

/*! State table for SEND_RANDOM */
static const meshPrvClTblEntry_t prvClStateTblSendRandom[] =
{
  /* Event                        Next state                      Action  */
  { PRV_CL_EVT_SENT_RANDOM,      PRV_CL_ST_WAIT_RANDOM,          PRV_CL_ACT_WAIT_RANDOM },
  { PRV_CL_EVT_RECV_RANDOM,      PRV_CL_ST_CHECK_CONFIRMATION,   PRV_CL_ACT_CHECK_CONFIRMATION }, /* Only if device missed ACKs */
  { 0,                           0,                              0 }
};

/*! State table for WAIT_RANDOM */
static const meshPrvClTblEntry_t prvClStateTblWaitRandom[] =
{
  /* Event                        Next state                        Action  */
  { PRV_CL_EVT_RECV_RANDOM,      PRV_CL_ST_CHECK_CONFIRMATION,     PRV_CL_ACT_CHECK_CONFIRMATION },
  { 0,                           0,                                0 }
};

/*! State table for CHECK_CONFIRMATION */
static const meshPrvClTblEntry_t prvClStateTblCheckConfirmation[] =
{
  /* Event                               Next state                   Action  */
  { PRV_CL_EVT_CONFIRMATION_VERIFIED,   PRV_CL_ST_CALC_SESSION_KEY,  PRV_CL_ACT_CALC_SESSION_KEY },
  { PRV_CL_EVT_CONFIRMATION_FAILED,     PRV_CL_ST_IDLE,              PRV_CL_ACT_CONFIRMATION_FAILED },
  { 0,                                  0,                           0 }
};

/*! State table for CALC_SESSION_KEY */
static const meshPrvClTblEntry_t prvClStateTblCalcSessionKey[] =
{
  /* Event                          Next state                  Action  */
  { PRV_CL_EVT_SESSION_KEY_READY,  PRV_CL_ST_ENCRYPT_DATA,     PRV_CL_ACT_ENCRYPT_DATA },
  { 0,                             0,                          0 }
};

/*! State table for ENCRYPT_DATA */
static const meshPrvClTblEntry_t prvClStateTblEncryptData[] =
{
  /* Event                           Next state                 Action  */
  { PRV_CL_EVT_DATA_ENCRYPTED,      PRV_CL_ST_SEND_DATA,       PRV_CL_ACT_SEND_DATA },
  { 0,                              0,                         0 }
};

/*! State table for SEND_DATA */
static const meshPrvClTblEntry_t prvClStateTblSendData[] =
{
  /* Event                        Next state                  Action  */
  { PRV_CL_EVT_SENT_DATA,        PRV_CL_ST_WAIT_COMPLETE,    PRV_CL_ACT_WAIT_COMPLETE },
  { PRV_CL_EVT_RECV_COMPLETE,    PRV_CL_ST_IDLE,             PRV_CL_ACT_SUCCESS }, /* Only if device missed ACKs */
  { 0,                           0,                          0 }
};

/*! State table for WAIT_COMPLETE */
static const meshPrvClTblEntry_t prvClStateTblWaitComplete[] =
{
  /* Event                           Next state                 Action  */
  { PRV_CL_EVT_RECV_COMPLETE,       PRV_CL_ST_IDLE,            PRV_CL_ACT_SUCCESS },
  { PRV_CL_EVT_LINK_CLOSED_SUCCESS, PRV_CL_ST_IDLE,            PRV_CL_ACT_SUCCESS },
  { PRV_CL_EVT_RECV_TIMEOUT,        PRV_CL_ST_IDLE,            PRV_CL_ACT_SUCCESS },
  { 0,                              0,                         0 }
};

/*! Table of individual state tables */
const meshPrvClTblEntry_t * const prvClStateTbl[] =
{
  prvClStateTblIdle,
  prvClStateTblWaitLink,
  prvClStateTblSendInvite,
  prvClStateTblWaitCapabilities,
  prvClStateTblWaitSelectAuth,
  prvClStateTblSendStart,
  prvClStateTblSendPublicKey,
  prvClStateTblWaitPublicKey,
  prvClStateTblValidatePublicKey,
  prvClStateTblGeneratePublicKey,
  prvClStateTblPrepareOobAction,
  prvClStateTblWaitInput,
  prvClStateTblWaitInputComplete,
  prvClStateTblCalcConfirmation,
  prvClStateTblSendConfirmation,
  prvClStateTblWaitConfirmation,
  prvClStateTblSendRandom,
  prvClStateTblWaitRandom,
  prvClStateTblCheckConfirmation,
  prvClStateTblCalcSessionKey,
  prvClStateTblEncryptData,
  prvClStateTblSendData,
  prvClStateTblWaitComplete,
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! State machine interface */
const meshPrvClSmIf_t meshPrvClSmIf =
{
  prvClStateTbl,
  prvClActionTbl,
  prvClStateTblCommon
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
static uint8_t *meshPrvClStateStr(meshPrvClSmState_t state)
{
  switch (state)
  {
  case PRV_CL_ST_IDLE: return (uint8_t*) "IDLE";
  case PRV_CL_ST_WAIT_LINK: return (uint8_t*) "WAIT_LINK";
  case PRV_CL_ST_SEND_INVITE: return (uint8_t*) "SEND_INVITE";
  case PRV_CL_ST_WAIT_CAPABILITIES: return (uint8_t*) "WAIT_CAPABILITIES";
  case PRV_CL_ST_WAIT_SELECT_AUTH: return (uint8_t*) "WAIT_SELECT_AUTH";
  case PRV_CL_ST_SEND_START: return (uint8_t*) "SEND_START";
  case PRV_CL_ST_SEND_PUBLIC_KEY: return (uint8_t*) "SEND_PUBLIC_KEY";
  case PRV_CL_ST_WAIT_PUBLIC_KEY: return (uint8_t*) "WAIT_PUBLIC_KEY";
  case PRV_CL_ST_VALIDATE_PUBLIC_KEY: return (uint8_t*) "VALIDATE_PUBLIC_KEY";
  case PRV_CL_ST_GENERATE_PUBLIC_KEY: return (uint8_t*) "GENERATE_PUBLIC_KEY";
  case PRV_CL_ST_PREPARE_OOB_ACTION: return (uint8_t*) "PREPARE_OOB_ACTION";
  case PRV_CL_ST_WAIT_INPUT: return (uint8_t*) "WAIT_INPUT";
  case PRV_CL_ST_WAIT_INPUT_COMPLETE: return (uint8_t*) "WAIT_INPUT_COMPLETE";
  case PRV_CL_ST_CALC_CONFIRMATION: return (uint8_t*) "CALC_CONFIRMATION";
  case PRV_CL_ST_SEND_CONFIRMATION: return (uint8_t*) "SEND_CONFIRMATION";
  case PRV_CL_ST_WAIT_CONFIRMATION: return (uint8_t*) "WAIT_CONFIRMATION";
  case PRV_CL_ST_SEND_RANDOM: return (uint8_t*) "SEND_RANDOM";
  case PRV_CL_ST_WAIT_RANDOM: return (uint8_t*) "WAIT_RANDOM";
  case PRV_CL_ST_CHECK_CONFIRMATION: return (uint8_t*) "CHECK_CONFIRMATION";
  case PRV_CL_ST_CALC_SESSION_KEY: return (uint8_t*) "CALC_SESSION_KEY";
  case PRV_CL_ST_ENCRYPT_DATA: return (uint8_t*) "ENCRYPT_DATA";
  case PRV_CL_ST_SEND_DATA: return (uint8_t*) "SEND_DATA";
  case PRV_CL_ST_WAIT_COMPLETE: return (uint8_t*) "WAIT_COMPLETE";

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
static uint8_t *meshPrvClEvtStr(meshPrvClSmEvt_t evt)
{
  switch (evt)
  {
  case PRV_CL_EVT_BEGIN_NO_LINK: return (uint8_t*) "BEGIN_NO_LINK";
  case PRV_CL_EVT_BEGIN_LINK_OPEN: return (uint8_t*) "BEGIN_LINK_OPEN";
  case PRV_CL_EVT_LINK_OPENED: return (uint8_t*) "LINK_OPENED";
  case PRV_CL_EVT_LINK_FAILED: return (uint8_t*) "LINK_FAILED";
  case PRV_CL_EVT_LINK_CLOSED_FAIL: return (uint8_t*) "LINK_CLOSED_FAIL";
  case PRV_CL_EVT_BAD_PDU: return (uint8_t*) "BAD_PDU";
  case PRV_CL_EVT_LINK_CLOSED_SUCCESS: return (uint8_t*) "LINK_CLOSED_SUCCESS";
  case PRV_CL_EVT_RECV_TIMEOUT: return (uint8_t*) "RECV_TIMEOUT";
  case PRV_CL_EVT_SEND_TIMEOUT: return (uint8_t*) "SEND_TIMEOUT";
  case PRV_CL_EVT_SENT_INVITE: return (uint8_t*) "SENT_INVITE";
  case PRV_CL_EVT_SENT_START: return (uint8_t*) "SENT_START";
  case PRV_CL_EVT_SENT_PUBLIC_KEY: return (uint8_t*) "SENT_PUBLIC_KEY";
  case PRV_CL_EVT_SENT_CONFIRMATION: return (uint8_t*) "SENT_CONFIRMATION";
  case PRV_CL_EVT_SENT_RANDOM: return (uint8_t*) "SENT_RANDOM";
  case PRV_CL_EVT_SENT_DATA: return (uint8_t*) "SENT_DATA";
  case PRV_CL_EVT_GOTO_WAIT_INPUT: return (uint8_t*) "GOTO_WAIT_INPUT";
  case PRV_CL_EVT_GOTO_WAIT_IC: return (uint8_t*) "GOTO_WAIT_INPUT_COMPLETE";
  case PRV_CL_EVT_GOTO_CONFIRMATION: return (uint8_t*) "GOTO_CONFIRMATION";
  case PRV_CL_EVT_INPUT_READY: return (uint8_t*) "INPUT_READY";
  case PRV_CL_EVT_AUTH_SELECTED: return (uint8_t*) "AUTH_SELECTED";
  case PRV_CL_EVT_CONFIRMATION_READY: return (uint8_t*) "CONFIRMATION_READY";
  case PRV_CL_EVT_CONFIRMATION_VERIFIED: return (uint8_t*) "CONFIRMATION_VERIFIED";
  case PRV_CL_EVT_CONFIRMATION_FAILED: return (uint8_t*) "CONFIRMATION_FAILED";
  case PRV_CL_EVT_SESSION_KEY_READY: return (uint8_t*) "SESSION_KEY_READY";
  case PRV_CL_EVT_RECV_CAPABILITIES: return (uint8_t*) "RECV_CAPABILITIES";
  case PRV_CL_EVT_RECV_PUBLIC_KEY: return (uint8_t*) "RECV_PUBLIC_KEY";
  case PRV_CL_EVT_PUBLIC_KEY_VALID: return (uint8_t*) "PUBLIC_KEY_VALID";
  case PRV_CL_EVT_PUBLIC_KEY_INVALID: return (uint8_t*) "PUBLIC_KEY_INVALID";
  case PRV_CL_EVT_PUBLIC_KEY_GENERATED: return (uint8_t*) "PUBLIC_KEY_GENERATED";
  case PRV_CL_EVT_RECV_CONFIRMATION: return (uint8_t*) "RECV_CONFIRMATION";
  case PRV_CL_EVT_RECV_RANDOM: return (uint8_t*) "RECV_RANDOM";
  case PRV_CL_EVT_RECV_COMPLETE: return (uint8_t*) "RECV_COMPLETE";
  case PRV_CL_EVT_DATA_ENCRYPTED: return (uint8_t*) "DATA_ENCRYPTED";
  case PRV_CL_EVT_CANCEL: return (uint8_t*) "CANCEL";

  default: return (uint8_t*) "Unknown";
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Execute the Provisioning Client state machine.
 *
 *  \param[in] pCcb  Connection control block.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvClSmExecute(meshPrvClCb_t *pCcb, meshPrvClSmMsg_t *pMsg)
{
  meshPrvClTblEntry_t const *pTblEntry;
  meshPrvClSmIf_t const *pSmIf = pCcb->pSm;
  meshPrvClSmState_t oldState;

  /* Silence compiler warnings when trace is disabled */
  (void)meshPrvClEvtStr((meshPrvClSmEvt_t)0);
  (void)meshPrvClStateStr((meshPrvClSmState_t)0);

  pTblEntry = prvClStateTbl[pCcb->state];

  MESH_TRACE_INFO2("MESH_PRV_CL_SM Event Handler: state=%s event=%s",
    meshPrvClStateStr(pCcb->state),
    meshPrvClEvtStr(pMsg->hdr.event));

  /* run through state machine twice; once with state table for current state
  * and once with the state table for common events
  */
  for (;;)
  {
    /* look for event match and execute action */
    do
    {
      /* if match */
      if ((*pTblEntry)[MESH_PRV_CL_SM_POS_EVENT] == pMsg->hdr.event)
      {
        /* set next state */
        oldState = pCcb->state;
        pCcb->state = (*pTblEntry)[MESH_PRV_CL_SM_POS_NEXT_STATE];
        MESH_TRACE_INFO2("MESH_PRV_CL_SM State Change: old=%s new=%s",
          meshPrvClStateStr(oldState),
          meshPrvClStateStr(pCcb->state));

        /* execute action */
        (*pSmIf->pActionTbl[(*pTblEntry)[MESH_PRV_CL_SM_POS_ACTION]])(pCcb, pMsg);

        (void)oldState;

        return;
      }

      /* next entry */
      pTblEntry++;

      /* while not at end */
    } while ((*pTblEntry)[MESH_PRV_CL_SM_POS_EVENT] != 0);

    /* if we've reached end of the common state table */
    if (pTblEntry == (pSmIf->pCommonTbl + MESH_PRV_CL_STATE_TBL_COMMON_MAX - 1))
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
  return;
}
