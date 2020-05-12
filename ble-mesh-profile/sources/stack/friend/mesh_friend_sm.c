/*************************************************************************************************/
/*!
 *  \file   mesh_friend_sm.c
 *
 *  \brief  Mesh Friend state machine.
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
#include "wsf_msg.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_main.h"
#include "mesh_network.h"
#include "mesh_lower_transport.h"
#include "mesh_sar_rx.h"

#include "mesh_friend.h"
#include "mesh_friend_main.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! State machine table constants */
#define MESH_FRIEND_SM_POS_EVENT          0  /*! Column position for event */
#define MESH_FRIEND_SM_POS_NEXT_STATE     1  /*! Column position for next state */
#define MESH_FRIEND_SM_POS_ACTION         2  /*! Column position for action */
#define MESH_FRIEND_STATE_TBL_COMMON_MAX  2  /*! Number of entries in the common state table */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! State machine actions enumeration */
enum meshFriendActValues
{
  FRIEND_ACT_NONE,               /*!< No action */
  FRIEND_ACT_DEALLOC,            /*!< Free Context */
  FRIEND_ACT_PREP_KEY_MAT,       /*!< Prepare key material */
  FRIEND_ACT_SEND_OFFER,         /*!< Send offer */
  FRIEND_ACT_SETUP_FRIENDSHIP,   /*!< Setup friendship */
  FRIEND_ACT_START_RECV_DELAY,   /*!< Start receive delay */
  FRIEND_ACT_SEND_NEXT_PDU,      /*!< Send next PDU from queue */
  FRIEND_ACT_SEND_SUBSCR_CNF,    /*!< Send Friend Subscription List Confirm */
  FRIEND_ACT_TERMINATE,          /*!< Terminate friendship */
  FRIEND_ACT_NOTIFY_FRIEND,      /*!< Notify other friend that friendship is over */
  FRIEND_ACT_STOP_NOTIFY_FRIEND, /*!< Stop notifying other friend that friendship is over */
  FRIEND_ACT_UPDATE_SUBSCR_LIST, /*!< Update subscription list */
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Action function table; order matches action function enumeration */
static const meshFriendAct_t friendActionTbl[] =
{
  meshFriendActNone,                      /*!< No action */
  meshFriendActDealloc,                   /*!< Free Context */
  meshFriendActPrepKeyMat,                /*!< Prepare key material */
  meshFriendActSendOffer,                 /*!< Send Friend Offer as part of friendship
                                           *   establishment
                                           */
  meshFriendActSetupFriendship,           /*!< Setup established friendship */
  meshFriendActStartRecvDelay,            /*!< Start Receive delay timer */
  meshFriendActSendNextPdu,               /*!< Sends Next Friend PDU from the queue */
  meshFriendActSendSubscrCnf,             /*!< Sends Friend Subscription List Confirm */
  meshFriendActTerminate,                 /*!< Terminates friendship */
  meshFriendActNotifyFriend,              /*!< Notify other friend that his friendship is over */
  meshFriendActStopNotifyFriend,          /*!< Stop notifying other friend */
  meshFriendActUpdateSubscrList,          /*!< Update Subscription List */
};

/*! State table for common actions */
static const meshFriendTblEntry_t friendStateTblCommon[MESH_FRIEND_STATE_TBL_COMMON_MAX] =
{
  /* Event                              Next state                   Action */
  { MESH_FRIEND_MSG_STATE_DISABLED,     FRIEND_ST_IDLE,              FRIEND_ACT_TERMINATE },
  { 0,                                  0,                           0}
};

/*! State table for FRIEND_ST_IDLE */
static const meshFriendTblEntry_t friendStateTblIdle[] =
{
  /* Event                              Next state                   Action  */
  { MESH_FRIEND_MSG_FRIEND_REQ_RECV,    FRIEND_ST_IDLE,              FRIEND_ACT_DEALLOC },
  { MESH_FRIEND_MSG_STATE_ENABLED,      FRIEND_ST_WAIT_REQ,          FRIEND_ACT_NONE },
  { 0,                                  0,                           0}
};

/*! State table for FRIEND_ST_WAIT_REQ */
static const meshFriendTblEntry_t friendStateTblWaitReq[] =
{
  /* Event                              Next state                   Action  */
  { MESH_FRIEND_MSG_FRIEND_REQ_RECV,    FRIEND_ST_START_KEY_DERIV,   FRIEND_ACT_PREP_KEY_MAT },
  { 0,                                  0,                           0 }
};

/*! State table for FRIEND_ST_START_KEY_DERIV */
static const meshFriendTblEntry_t friendStateTblStartKeyDeriv[] =
{
  /* Event                              Next state                   Action  */
  { MESH_FRIEND_MSG_KEY_DERIV_FAILED,   FRIEND_ST_WAIT_REQ,          FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_NETKEY_DEL,         FRIEND_ST_WAIT_REQ,          FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_KEY_DERIV_SUCCESS,  FRIEND_ST_WAIT_RECV_TIMEOUT, FRIEND_ACT_NONE },
  { MESH_FRIEND_MSG_RECV_DELAY,         FRIEND_ST_KEY_DERIV_LATE,    FRIEND_ACT_NONE },
  { 0,                                  0,                           0 }
};

/*! State table for FRIEND_ST_KEY_DERIV_LATE */
static const meshFriendTblEntry_t friendStateTblKeyDerivLate[] =
{
  /* Event                              Next state                  Action  */
  { MESH_FRIEND_MSG_KEY_DERIV_FAILED,   FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_KEY_DERIV_SUCCESS,  FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_NETKEY_DEL,         FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { 0,                                  0,                          0 }
};

/*! State table for FRIEND_ST_WAIT_RECV_TIMEOUT */
static const meshFriendTblEntry_t friendStateTblRecvTimeout[] =
{
  /* Event                              Next state                  Action  */
  { MESH_FRIEND_MSG_RECV_DELAY,         FRIEND_ST_WAIT_POLL,        FRIEND_ACT_SEND_OFFER },
  { MESH_FRIEND_MSG_NETKEY_DEL,         FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { 0,                                  0,                          0 }
};

/*! State table for FRIEND_ST_WAIT_POLL */
static const meshFriendTblEntry_t friendStateTblWaitPoll[] =
{
  /* Event                              Next state                  Action  */
  { MESH_FRIEND_MSG_TIMEOUT,            FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_NETKEY_DEL,         FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_POLL_RECV,          FRIEND_ST_ESTAB,            FRIEND_ACT_SETUP_FRIENDSHIP },
  { 0,                                  0,                          0 }
};

/*! State table for FRIEND_ST_ESTAB */
static const meshFriendTblEntry_t friendStateTblEstab[] =
{
  /* Event                                  Next state                  Action  */
  { MESH_FRIEND_MSG_RECV_DELAY,             FRIEND_ST_ESTAB,            FRIEND_ACT_SEND_NEXT_PDU },
  { MESH_FRIEND_MSG_POLL_RECV,              FRIEND_ST_ESTAB,            FRIEND_ACT_START_RECV_DELAY },
  { MESH_FRIEND_MSG_TIMEOUT,                FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_FRIEND_REQ_RECV,        FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_FRIEND_CLEAR_RECV,      FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_NETKEY_DEL,             FRIEND_ST_WAIT_REQ,         FRIEND_ACT_TERMINATE },
  { MESH_FRIEND_MSG_SUBSCR_LIST_ADD,        FRIEND_ST_ESTAB,            FRIEND_ACT_UPDATE_SUBSCR_LIST },
  { MESH_FRIEND_MSG_SUBSCR_LIST_REM,        FRIEND_ST_ESTAB,            FRIEND_ACT_UPDATE_SUBSCR_LIST },
  { MESH_FRIEND_MSG_SUBSCR_CNF_DELAY,       FRIEND_ST_ESTAB,            FRIEND_ACT_SEND_SUBSCR_CNF },
  { MESH_FRIEND_MSG_CLEAR_SEND_TIMEOUT,     FRIEND_ST_ESTAB,            FRIEND_ACT_NOTIFY_FRIEND },
  { MESH_FRIEND_MSG_FRIEND_CLEAR_CNF_RECV,  FRIEND_ST_ESTAB,            FRIEND_ACT_STOP_NOTIFY_FRIEND },
  { 0,                                      0,                          0 }
};

/*! Table of individual state tables */
static const meshFriendTblEntry_t * const friendStateTbl[] =
{
  friendStateTblIdle,
  friendStateTblWaitReq,
  friendStateTblStartKeyDeriv,
  friendStateTblKeyDerivLate,
  friendStateTblRecvTimeout,
  friendStateTblWaitPoll,
  friendStateTblEstab,
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! State machine interface */
const meshFriendSmIf_t meshFriendSrSmIf =
{
  friendStateTbl,
  friendActionTbl,
  friendStateTblCommon
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
static uint8_t *meshFriendStateStr(meshFriendSmState_t state)
{
  switch (state)
  {
    case FRIEND_ST_IDLE: return (uint8_t*) "IDLE";
    case FRIEND_ST_WAIT_REQ: return (uint8_t*) "WAIT_FRIEND_REQ";
    case FRIEND_ST_START_KEY_DERIV: return (uint8_t*) "START_KEY_DERIV";
    case FRIEND_ST_KEY_DERIV_LATE:  return (uint8_t*) "KEY_DERIV_LATE";
    case FRIEND_ST_WAIT_RECV_TIMEOUT:  return (uint8_t*) "RECV_TIMEOUT";
    case FRIEND_ST_WAIT_POLL: return (uint8_t*) "WAIT_FRIEND_POLL";
    case FRIEND_ST_ESTAB: return (uint8_t*) "ESTAB_COMPLETE";

    default: return (uint8_t*) "UNKNOWN_STATE";
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
static uint8_t *meshFriendEvtStr(uint8_t evt)
{
  switch (evt)
  {
    case MESH_FRIEND_MSG_STATE_ENABLED : return (uint8_t*) "FRIEND_STATE_ENABLED";
    case MESH_FRIEND_MSG_STATE_DISABLED: return (uint8_t*) "FRIEND_STATE_DISABLED";
    case MESH_FRIEND_MSG_FRIEND_REQ_RECV: return (uint8_t*) "FRIEND_REQ_RECEIVED";
    case MESH_FRIEND_MSG_POLL_RECV: return (uint8_t*) "FRIEND_POLL_RECEIVED";
    case MESH_FRIEND_MSG_FRIEND_CLEAR_RECV: return (uint8_t*) "FRIEND_CLEAR_RECEIVED";
    case MESH_FRIEND_MSG_FRIEND_CLEAR_CNF_RECV: return (uint8_t*) "FRIEND_CLEAR_CNF_RECEIVED";
    case MESH_FRIEND_MSG_KEY_DERIV_SUCCESS: return (uint8_t*) "FRIEND_KEY_DERIVATION_SUCCESS";
    case MESH_FRIEND_MSG_KEY_DERIV_FAILED: return (uint8_t*) "FRIEND_KEY_DERIVATION_FAILED";
    case MESH_FRIEND_MSG_RECV_DELAY: return (uint8_t*) "FRIEND_RECEIVE_DELAY_TMR";
    case MESH_FRIEND_MSG_SUBSCR_CNF_DELAY: return (uint8_t*) "FRIEND_SUBSCR_CNF_TMR";
    case MESH_FRIEND_MSG_CLEAR_SEND_TIMEOUT: return (uint8_t*)"FRIEND_CLEAR_SEND_TMR";
    case MESH_FRIEND_MSG_TIMEOUT: return (uint8_t*) "FRIEND_TIMEOUT";
    case MESH_FRIEND_MSG_SUBSCR_LIST_ADD: return (uint8_t*) "FRIEND_SUBSCRIPTION_ADD";
    case MESH_FRIEND_MSG_SUBSCR_LIST_REM: return (uint8_t*) "FRIEND_SUBSCRIPTION_REMOVE";
    case MESH_FRIEND_MSG_NETKEY_DEL: return (uint8_t*)"FRIEND_NETKEY_DEL";

    default: return (uint8_t*) "UNKNOWN_EVENT";
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Execute the Provisioning Server state machine.
 *
 *  \param[in] pCtx  Pointer to LPN context.
 *  \param[in] pMsg  Pointer to state machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendSmExecute(meshFriendLpnCtx_t *pCtx, meshFriendSmMsg_t *pMsg)
{
  meshFriendTblEntry_t const *pTblEntry;
  meshFriendSmIf_t const *pSmIf = friendCb.pSm;
  meshFriendSmState_t oldState;

  /* Silence compiler warnings when trace is disabled */
  (void)meshFriendEvtStr(0);
  (void)meshFriendStateStr((meshFriendSmState_t)0);

  pTblEntry = friendStateTbl[pCtx->friendSmState];

  MESH_TRACE_INFO2("MESH_FRIEND_SM Event Handler: state=%s event=%s\r\n",
    meshFriendStateStr(pCtx->friendSmState),
    meshFriendEvtStr(pMsg->hdr.event));

  /* run through state machine twice; once with state table for current state
   * and once with the state table for common events
   */
  for (;;)
  {
    /* look for event match and execute action */
    do
    {
      /* if match */
      if ((*pTblEntry)[MESH_FRIEND_SM_POS_EVENT] == pMsg->hdr.event)
      {
        /* set next state */
        oldState = pCtx->friendSmState;
        pCtx->friendSmState = (*pTblEntry)[MESH_FRIEND_SM_POS_NEXT_STATE];
        MESH_TRACE_INFO2("MESH_FRIEND_SM State Change: old=%s new=%s\r\n",
                         meshFriendStateStr(oldState),
                         meshFriendStateStr(pCtx->friendSmState));

        /* execute action */
        (*pSmIf->pActionTbl[(*pTblEntry)[MESH_FRIEND_SM_POS_ACTION]])(pCtx, pMsg);

        (void)oldState;

        return;
      }

      /* next entry */
      pTblEntry++;

      /* while not at end */
    } while ((*pTblEntry)[MESH_FRIEND_SM_POS_EVENT] != 0);

    /* if we've reached end of the common state table */
    if (pTblEntry == (pSmIf->pCommonTbl + MESH_FRIEND_STATE_TBL_COMMON_MAX - 1))
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
