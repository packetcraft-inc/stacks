/*************************************************************************************************/
/*!
 *  \file   mesh_lpn_sm.c
 *
 *  \brief  Mesh LPN state machine.
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
#include "wsf_timer.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_lpn_api.h"
#include "mesh_lpn_main.h"
#include "mesh_lpn.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! State machine table constants */
#define MESH_LPN_SM_POS_EVENT          0       /*!< Column position for event */
#define MESH_LPN_SM_POS_NEXT_STATE     1       /*!< Column position for next state */
#define MESH_LPN_SM_POS_ACTION         2       /*!< Column position for action */
#define MESH_LPN_STATE_TBL_COMMON_MAX  3       /*!< Number of entries in the common state table */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! State machine states */
enum meshLpnSmStateValues
{
  LPN_SM_ST_IDLE,                       /*! Idle */
  LPN_SM_ST_WAIT_FRIEND_OFFER,          /*! Wait for Friend Offer PDU */
  LPN_SM_ST_WAIT_FRIEND_UPDATE,         /*! Wait for Friend Update PDU */
  LPN_SM_ST_FRIENDSHIP_ESTABLISHED,     /*! Friendship established */
  LPN_SM_ST_WAIT_FRIEND_MESSAGE,        /*! Wait for Friend Message or Friend Update PDU */
  LPN_SM_ST_WAIT_FRIEND_SUBSCR_CNF,     /*! Wait for Friend Subscription List confirm PDU */
};

/*! Action function enumeration */
enum meshLpnSmActValues
{
  LPN_ACT_NONE,                         /*!< No action */
  LPN_ACT_TERMINATE_FRIENDSHIP,         /*!< Friendship Terminated */
  LPN_ACT_SEND_FRIEND_REQ,              /*!< Send Friend Request PDU */
  LPN_ACT_WAIT_FRIEND_OFFER,            /*!< Wait for Friend Offer PDU */
  LPN_ACT_RESEND_FRIEND_REQ,            /*!< Re-send Friend Request PDU */
  LPN_ACT_PROCESS_FRIEND_OFFER,         /*!< Process Friend Offer PDU */
  LPN_ACT_SEND_FRIEND_POLL,             /*!< Send Friend Poll PDU */
  LPN_ACT_WAIT_FRIEND_UPDATE,           /*!< Wait for Friend Update PDU */
  LPN_ACT_RESEND_FRIEND_POLL,           /*!< Re-send Friend Poll PDU */
  LPN_ACT_FRIENDSHIP_ESTABLISHED,       /*!< Friendship Established */
  LPN_ACT_WAIT_FRIEND_MESSAGE,          /*!< Wait Friend Message or Update PDU */
  LPN_ACT_PROCESS_FRIEND_UPDATE,        /*!< Process Friend Update PDU */
  LPN_ACT_PROCESS_FRIEND_MESSAGE,       /*!< Process Friend message PDU */
  LPN_ACT_SEND_FRIEND_SUBSCR_ADD_RM,    /*!< Send Friend Subscription List Add/Remove PDU */
  LPN_ACT_RESEND_FRIEND_SUBSCR_ADD_RM,  /*!< Re-send previous Friend Subscription request */
  LPN_ACT_WAIT_FRIEND_SUBSCR_CNF,       /*!< Wait for Friend Subscription List Confirm PDU */
  LPN_ACT_PROCESS_FRIEND_SUBSCR_CNF     /*!< Process Friend Subscription List Confirm PDU */
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Action function table; order matches action function enumeration */
static const meshLpnAct_t lpnActionTbl[] =
{
  meshLpnActNone,                       /*!< No action */
  meshLpnActTerminateFriendship,        /*!< Terminate Friendship */
  meshLpnActSendFriendReq,              /*!< Send Friend Request PDU */
  meshLpnActWaitFriendOffer,            /*!< Wait for Friend Offer PDU */
  meshLpnActResendFriendReq,            /*!< Re-send Friend Request PDU */
  meshLpnActProcessFriendOffer,         /*!< Process Friend Offer PDU */
  meshLpnActSendFriendPoll,             /*!< Send Friend Poll PDU */
  meshLpnActWaitFriendUpdate,           /*!< Wait for Friend Update PDU */
  meshLpnActResendFriendPoll,           /*!< Re-send Friend Poll PDU */
  meshLpnActFriendshipEstablished,      /*!< Friendship Established */
  meshLpnActWaitFriendMessage,          /*!< Wait Friend Message or Update PDU */
  meshLpnActProcessFriendUpdate,        /*!< Friend Update PDU */
  meshLpnActProcessFriendMessage,       /*!< Friend message PDU */
  meshLpnActSendFriendSubscrAddRm,        /*!< Send Friend Subscription List Add/Remove PDU */
  meshLpnActResendFriendSubscrAddRm,    /*!< Re-send previous Friend Subscription Add/Remove PDU */
  meshLpnActWaitFriendSubscrCnf,        /*!< Wait for Friend Subscription List Confirm PDU */
  meshLpnActProcessFriendSubscrCnf      /*!< Process Friend Subscription List Confirm PDU */
};

/*! State table for common actions */
static const meshLpnTblEntry_t lpnStateTblCommon[MESH_LPN_STATE_TBL_COMMON_MAX] =
{
  /* Event                                Next state                             Action */
  { MESH_LPN_MSG_TERMINATE,               LPN_SM_ST_IDLE,                        LPN_ACT_TERMINATE_FRIENDSHIP },
  { MESH_LPN_MSG_SEND_FRIEND_CLEAR,       LPN_SM_ST_IDLE,                        LPN_ACT_TERMINATE_FRIENDSHIP },
  { 0,                                    0,                                     0 }
};

/*! State table for IDLE */
static const meshLpnTblEntry_t lpnStateTblIdle[] =
{
  /* Event                                Next state                             Action  */
  { MESH_LPN_MSG_ESTABLISH,               LPN_SM_ST_WAIT_FRIEND_OFFER,           LPN_ACT_SEND_FRIEND_REQ },
  { 0,                                    0,                                     0 }
};

/*! State table for WAIT_FRIEND_OFFER */
static const meshLpnTblEntry_t lpnStateTblWaitFriendOffer[] =
{
  /* Event                                Next state                             Action  */
  { MESH_LPN_MSG_RECV_DELAY_TIMEOUT,      LPN_SM_ST_WAIT_FRIEND_OFFER,           LPN_ACT_WAIT_FRIEND_OFFER },
  { MESH_LPN_MSG_RECV_WIN_TIMEOUT,        LPN_SM_ST_WAIT_FRIEND_OFFER,           LPN_ACT_RESEND_FRIEND_REQ },
  { MESH_LPN_MSG_SEND_FRIEND_REQ,         LPN_SM_ST_WAIT_FRIEND_OFFER,           LPN_ACT_SEND_FRIEND_REQ },
  { MESH_LPN_MSG_FRIEND_OFFER,            LPN_SM_ST_WAIT_FRIEND_OFFER,           LPN_ACT_PROCESS_FRIEND_OFFER },
  { MESH_LPN_MSG_SEND_FRIEND_POLL,        LPN_SM_ST_WAIT_FRIEND_UPDATE,          LPN_ACT_SEND_FRIEND_POLL },
  { 0,                                    0,                                     0 }
};

/*! State table for WAIT_FRIEND_UPDATE */
static const meshLpnTblEntry_t lpnStateTblWaitFriendUpdate[] =
{
  /* Event                                Next state                             Action  */
  { MESH_LPN_MSG_RECV_DELAY_TIMEOUT,      LPN_SM_ST_WAIT_FRIEND_UPDATE,          LPN_ACT_WAIT_FRIEND_UPDATE },
  { MESH_LPN_MSG_RECV_WIN_TIMEOUT,        LPN_SM_ST_WAIT_FRIEND_UPDATE,          LPN_ACT_RESEND_FRIEND_POLL },
  { MESH_LPN_MSG_SEND_FRIEND_POLL,        LPN_SM_ST_WAIT_FRIEND_UPDATE,          LPN_ACT_SEND_FRIEND_POLL },
  { MESH_LPN_MSG_SEND_FRIEND_REQ,         LPN_SM_ST_WAIT_FRIEND_OFFER,           LPN_ACT_SEND_FRIEND_REQ },
  { MESH_LPN_MSG_FRIEND_UPDATE,           LPN_SM_ST_FRIENDSHIP_ESTABLISHED,      LPN_ACT_FRIENDSHIP_ESTABLISHED },
  { 0,                                    0,                                     0 }
};

/*! State table for FRIENDSHIP_ESTABLISHED */
static const meshLpnTblEntry_t lpnStateTblFriendshipEstablished[] =
{
  /* Event                                    Next state                            Action  */
  { MESH_LPN_MSG_SEND_FRIEND_POLL,            LPN_SM_ST_WAIT_FRIEND_MESSAGE,        LPN_ACT_SEND_FRIEND_POLL },
  { MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM,   LPN_SM_ST_WAIT_FRIEND_SUBSCR_CNF,     LPN_ACT_SEND_FRIEND_SUBSCR_ADD_RM },
  { MESH_LPN_MSG_POLL_TIMEOUT,                LPN_SM_ST_WAIT_FRIEND_MESSAGE,        LPN_ACT_SEND_FRIEND_POLL },
  { MESH_LPN_MSG_FRIEND_UPDATE,               LPN_SM_ST_FRIENDSHIP_ESTABLISHED,     LPN_ACT_PROCESS_FRIEND_UPDATE},
  { 0,                                        0,                                    0 }
};

/*! State table for WAIT_FRIEND_MESSAGE */
static const meshLpnTblEntry_t lpnStateTblWaitFriendMessage[] =
{
  /* Event                                 Next state                            Action  */
  { MESH_LPN_MSG_RECV_DELAY_TIMEOUT,       LPN_SM_ST_WAIT_FRIEND_MESSAGE,        LPN_ACT_WAIT_FRIEND_MESSAGE },
  { MESH_LPN_MSG_RECV_WIN_TIMEOUT,         LPN_SM_ST_FRIENDSHIP_ESTABLISHED,     LPN_ACT_RESEND_FRIEND_POLL },
  { MESH_LPN_MSG_POLL_TIMEOUT,             LPN_SM_ST_WAIT_FRIEND_MESSAGE,        LPN_ACT_SEND_FRIEND_POLL },
  { MESH_LPN_MSG_FRIEND_UPDATE,            LPN_SM_ST_FRIENDSHIP_ESTABLISHED,     LPN_ACT_PROCESS_FRIEND_UPDATE},
  { MESH_LPN_MSG_FRIEND_MESSAGE,           LPN_SM_ST_FRIENDSHIP_ESTABLISHED,     LPN_ACT_PROCESS_FRIEND_MESSAGE},
  { 0,                                     0,                                    0 }
};

/*! State table for WAIT_FRIEND_SUBSCR_CNF */
static const meshLpnTblEntry_t lpnStateTblWaitFriendSubscrCnf[] =
{
  /* Event                                    Next state                            Action  */
  { MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM,   LPN_SM_ST_WAIT_FRIEND_SUBSCR_CNF,     LPN_ACT_SEND_FRIEND_SUBSCR_ADD_RM },
  { MESH_LPN_MSG_RECV_DELAY_TIMEOUT,          LPN_SM_ST_WAIT_FRIEND_SUBSCR_CNF,     LPN_ACT_WAIT_FRIEND_SUBSCR_CNF },
  { MESH_LPN_MSG_RECV_WIN_TIMEOUT,            LPN_SM_ST_FRIENDSHIP_ESTABLISHED,     LPN_ACT_RESEND_FRIEND_SUBSCR_ADD_RM },
  { MESH_LPN_MSG_RESEND_FRIEND_SUBSCR_ADD_RM, LPN_SM_ST_FRIENDSHIP_ESTABLISHED,     LPN_ACT_RESEND_FRIEND_SUBSCR_ADD_RM },
  { MESH_LPN_MSG_FRIEND_SUBSCR_CNF,           LPN_SM_ST_WAIT_FRIEND_SUBSCR_CNF,     LPN_ACT_PROCESS_FRIEND_SUBSCR_CNF },
  { MESH_LPN_MSG_POLL_TIMEOUT,                LPN_SM_ST_WAIT_FRIEND_MESSAGE,        LPN_ACT_SEND_FRIEND_POLL },
  { MESH_LPN_MSG_SEND_FRIEND_POLL,            LPN_SM_ST_WAIT_FRIEND_MESSAGE,        LPN_ACT_SEND_FRIEND_POLL },
  { 0,                                        0,                                    0 }
};

/*! Table of individual state tables */
const meshLpnTblEntry_t * const lpnStateTbl[] =
{
  lpnStateTblIdle,
  lpnStateTblWaitFriendOffer,
  lpnStateTblWaitFriendUpdate,
  lpnStateTblFriendshipEstablished,
  lpnStateTblWaitFriendMessage,
  lpnStateTblWaitFriendSubscrCnf
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! State machine interface */
const meshLpnSmIf_t meshLpnSmIf =
{
  lpnStateTbl,
  lpnActionTbl,
  lpnStateTblCommon
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
static uint8_t *meshLpnStateStr(uint8_t state)
{
  switch (state)
  {
    case LPN_SM_ST_IDLE: return (uint8_t*) "IDLE";
    case LPN_SM_ST_WAIT_FRIEND_OFFER: return (uint8_t*) "WAIT_FRIEND_OFFER";
    case LPN_SM_ST_WAIT_FRIEND_UPDATE: return (uint8_t*) "WAIT_FRIEND_UPDATE";
    case LPN_SM_ST_FRIENDSHIP_ESTABLISHED: return (uint8_t*) "FRIENDSHIP_ESTABLISHED";
    case LPN_SM_ST_WAIT_FRIEND_MESSAGE: return (uint8_t*) "WAIT_FRIEND_MESSAGE";
    case LPN_SM_ST_WAIT_FRIEND_SUBSCR_CNF: return (uint8_t*) "WAIT_FRIEND_SUBSCR_CNF";

    default: return (uint8_t*) "UNKNOWN_EVENT";
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Convert event into string for diagnostics.
 *
 *  \param[in] eventId  Event enum value.
 *
 *  \return    Event string.
 */
/*************************************************************************************************/
static uint8_t *meshLpnEvtStr(uint8_t eventId)
{
  switch (eventId)
  {
    case MESH_LPN_MSG_ESTABLISH: return (uint8_t*) "FRIENDSHIP_ESTABLISH";
    case MESH_LPN_MSG_TERMINATE: return (uint8_t*) "FRIENDSHIP_TERMINATE";
    case MESH_LPN_MSG_SEND_FRIEND_REQ: return (uint8_t*) "SEND_FRIEND_REQ";
    case MESH_LPN_MSG_SEND_FRIEND_POLL: return (uint8_t*) "SEND_FRIEND_POLL";
    case MESH_LPN_MSG_SEND_FRIEND_CLEAR: return (uint8_t*) "SEND_FRIEND_CLEAR";
    case MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM: return (uint8_t*) "SEND_FRIEND_SUBSCR_ADD_RM";
    case MESH_LPN_MSG_RESEND_FRIEND_SUBSCR_ADD_RM: return (uint8_t*) "RESEND_FRIEND_SUBSCR_ADD_RM";
    case MESH_LPN_MSG_FRIEND_OFFER: return (uint8_t*) "FRIEND_OFFER";
    case MESH_LPN_MSG_FRIEND_UPDATE: return (uint8_t*) "FRIEND_UPDATE";
    case MESH_LPN_MSG_FRIEND_MESSAGE: return (uint8_t*) "FRIEND_MESSAGE";
    case MESH_LPN_MSG_FRIEND_SUBSCR_CNF: return (uint8_t*) "FRIEND_SUBSCR_CNF";
    case MESH_LPN_MSG_RECV_DELAY_TIMEOUT: return (uint8_t*) "RECV_DELAY_TIMEOUT";
    case MESH_LPN_MSG_RECV_WIN_TIMEOUT: return (uint8_t*) "RECV_WIN_TIMEOUT";
    case MESH_LPN_MSG_POLL_TIMEOUT: return (uint8_t*) "POLL_TIMEOUT";

    default: return (uint8_t*) "UNKNOWN_EVENT";
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Execute the LPN state machine.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnSmExecute(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  meshLpnTblEntry_t const *pTblEntry;
  meshLpnSmIf_t const *pSmIf = lpnCb.pSm;

  /* Silence compiler warnings when trace is disabled */
  (void)meshLpnEvtStr(0);
  (void)meshLpnStateStr(0);

  MESH_TRACE_INFO2("MESH_LPN_SM Event Handler: state=%s event=%s",
                   meshLpnStateStr(pLpnCtx->state),
                   meshLpnEvtStr(pMsg->hdr.event));

  pTblEntry = pSmIf->pStateTbl[pLpnCtx->state];

  /* Run through state machine twice; once with state table for current state
   * and once with the state table for common events
   */
  for (;;)
  {
    /* Look for event match and execute action */
    do
    {
      /* If match */
      if ((*pTblEntry)[MESH_LPN_SM_POS_EVENT] == pMsg->hdr.event)
      {
        /* Set next state */
        pLpnCtx->state = (*pTblEntry)[MESH_LPN_SM_POS_NEXT_STATE];

        /* Execute action */
        (*pSmIf->pActionTbl[(*pTblEntry)[MESH_LPN_SM_POS_ACTION]])(pLpnCtx, pMsg);

        return;
      }

      /* Next entry */
      pTblEntry++;

      /* While not at end */
    } while ((*pTblEntry)[MESH_LPN_SM_POS_EVENT] != 0);

    /* If we've reached end of the common state table */
    if (pTblEntry == (pSmIf->pCommonTbl + MESH_LPN_STATE_TBL_COMMON_MAX - 1))
    {
      /* We're done */
      break;
    }
    /* Else we haven't run through common state table yet */
    else
    {
      /* Set it up */
      pTblEntry = pSmIf->pCommonTbl;
    }
  }
}
