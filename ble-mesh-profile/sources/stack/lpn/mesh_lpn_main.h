/*************************************************************************************************/
/*!
 *  \file   mesh_lpn_main.h
 *
 *  \brief  LPN feature internal definitions.
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

#ifndef MESH_LPN_MAIN_H
#define MESH_LPN_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Number of columns in state table */
#define MESH_LPN_SM_NUM_COLS                    3

/*! Invalid LPN context index */
#define MESH_LPN_INVALID_CTX_IDX                0xFF

/*! Invalid NetKey index */
#define MESH_LPN_INVALID_NET_KEY_INDEX          0xFFFF

/*! Number of retries for LPN messages */
#define MESH_LPN_TX_NUM_RETRIES                 3

/*! Maximum number of Subscription List requests */
#define MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES    5

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Data type for state machine table entry */
typedef uint8_t meshLpnTblEntry_t[MESH_LPN_SM_NUM_COLS];

/*! LPN friendship history */
typedef struct meshLpnFriendshipHistory_tag
{
  uint16_t       netKeyIndex;
  meshAddress_t  prevAddr;
} meshLpnFriendshipHistory_t;

/*! Event data for Friend Offer message */
typedef struct meshLpnFriendOffer_tag
{
  wsfMsgHdr_t   hdr;
  meshAddress_t friendAddr;
  uint16_t      friendCounter;
  uint8_t       recvWinMs;
  uint8_t       queueSize;
  uint8_t       subscrListSize;
  int8_t        rssi;
} meshLpnFriendOffer_t;

/*! Event data for Friend Update message */
typedef struct meshLpnFriendUpdate_tag
{
  wsfMsgHdr_t   hdr;
  uint8_t       flags;
  uint32_t      ivIndex;
  uint8_t       md;
} meshLpnFriendUpdate_t;

/*! Event data for Friend Subscription Confirm message */
typedef struct meshLpnFriendSubscrCnf_tag
{
  wsfMsgHdr_t   hdr;
  uint8_t       tranNumber;
} meshLpnFriendSubscrCnf_t;

/*! Event data for Friend PDU received message */
typedef struct meshLpnFriendRxPdu_tag
{
  wsfMsgHdr_t   hdr;
  bool_t        toggleFsn;
  bool_t        md;
} meshLpnFriendRxPdu_t;

/*! Union of all Mesh LPN state machine messages */
typedef union
{
  wsfMsgHdr_t                 hdr;
  meshLpnFriendOffer_t        friendOffer;
  meshLpnFriendUpdate_t       friendUpdate;
  meshLpnFriendSubscrCnf_t    friendSubscrCnf;
  meshLpnFriendRxPdu_t        friendRxPdu;
} meshLpnSmMsg_t;

typedef struct meshLpnFriendSubscrEvent_tag
{
  void            *pNext;
  meshAddress_t   address;
  uint8_t         entryIdx;
  bool_t          add;
} meshLpnFriendSubscrEvent_t;

/*! Friend Subscription Add/Remove Request */
typedef struct meshLpnFriendSubscrReq_tag
{
  meshAddress_t           addrList[MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES];
  uint16_t                nextAddressIdx;
  uint16_t                nextVirtualAddrIdx;
  uint8_t                 addrListCount;
  bool_t                  add;
} meshLpnFriendSubscrReq_t;

/*! LPN friendship context */
typedef struct meshLpnCtx_tag
{
  wsfTimer_t                   lpnTimer;               /*!< General LPN timer */
  wsfTimer_t                   pollTimer;              /*!< Poll Timeout timer */
  wsfQueue_t                   subscrListQueue;        /*!< Subscription List requests queue */
  uint32_t                     sleepDurationMs;
  meshLpnFriendSubscrReq_t     subscrReq;
  meshAddress_t                friendAddr;
  uint16_t                     netKeyIndex;
  uint16_t                     lpnCounter;
  meshFriendshipCriteria_t     criteria;
  uint8_t                      tranNumber;
  uint8_t                      recvDelayMs;
  uint8_t                      recvWinMs;
  uint8_t                      msgTimeout;
  uint8_t                      establishRetryCount;
  uint8_t                      txRetryCount;
  uint8_t                      fsn;
  uint8_t                      state;
  bool_t                       established;
  bool_t                       inUse;
} meshLpnCtx_t;

/*! State machine action function type */
typedef void (*meshLpnAct_t)(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);

/*! State machine interface type */
typedef struct
{
  meshLpnTblEntry_t const * const *pStateTbl;   /* Pointer to state table */
  meshLpnAct_t              const *pActionTbl;  /* Pointer to action table */
  meshLpnTblEntry_t         const *pCommonTbl;  /* Pointer to common action table */
} meshLpnSmIf_t;

/*! LPN control block */
typedef struct meshLpnCb_tag
{
  meshLpnCtx_t                  *pLpnTbl;
  meshLpnSmIf_t const           *pSm;              /* State machine interface */
  meshLpnFriendshipHistory_t    *pLpnHistory;
  meshLpnEvtNotifyCback_t       lpnEvtNotifyCback; /* Upper Layer callback */
  uint16_t                      lpnCounter;
  uint8_t                       maxNumFriendships;
} meshLpnCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Control block */
extern meshLpnCb_t lpnCb;

/*! State machine instance */
extern const meshLpnSmIf_t meshLpnSmIf;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void meshLpnSmExecute(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);

uint8_t meshLpnCtxAlloc(uint16_t netKeyIndex);
void meshLpnCtxDealloc(meshLpnCtx_t *pLpnCtx);
meshLpnCtx_t *meshLpnCtxByNetKeyIndex(uint16_t netKeyIndex);
meshLpnCtx_t * meshLpnCtxByIdx(uint8_t ctxIdx);
uint8_t meshLpnCtxIdxByNetKeyIndex(uint16_t netKeyIndex);
void meshLpnHistoryAdd(uint16_t netKeyIndex, meshAddress_t addr);
meshAddress_t meshLpnHistorySearch(uint16_t netKeyIndex);

void meshLpnActNone(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActTerminateFriendship(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActSendFriendReq(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActWaitFriendOffer(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActResendFriendReq(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActProcessFriendOffer(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActSendFriendPoll(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActWaitFriendUpdate(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActResendFriendPoll(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActFriendshipEstablished(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActWaitFriendMessage(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActProcessFriendUpdate(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActProcessFriendMessage(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActSendFriendSubscrAddRm(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActResendFriendSubscrAddRm(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActWaitFriendSubscrCnf(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);
void meshLpnActProcessFriendSubscrCnf(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MESH_LPN_MAIN_H */
