/*************************************************************************************************/
/*!
 *  \file   mesh_friend_main.h
 *
 *  \brief  Mesh Friend module interface.
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

#ifndef MESH_FRIEND_MAIN_H
#define MESH_FRIEND_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_types.h"
#include "wsf_os.h"
#include "wsf_timer.h"

#include "mesh_defs.h"
#include "mesh_types.h"

#include "mesh_network.h"
#include "mesh_security_toolbox.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Number of columns in state table */
#define MESH_FRIEND_SM_NUM_COLS       3

/*! Maximum number of LTR PDU bytes than can fit into the Network PDU */
#define MESH_FRIEND_QUEUE_MAX_LTR_PDU (MESH_NWK_MAX_PDU_LEN - MESH_NWK_HEADER_LEN\
                                       - MESH_NETMIC_SIZE_ACC_PDU)

/*! Extracts pointer to context from index */
#define LPN_CTX_PTR(idx)              (&(friendCb.pLpnCtxTbl[(idx)]))
/*! Converts to index from pointer to context */
#define LPN_CTX_IDX(pCtx)             (uint8_t)(pCtx - friendCb.pLpnCtxTbl)

/*! Total number queue entries in a Friend Queue */
#define GET_MAX_NUM_QUEUE_ENTRIES()   (pMeshConfig->pMemoryConfig->maxNumFriendQueueEntries)
/*! Total number of subscription list entries */
#define GET_MAX_SUBSCR_LIST_SIZE()    (pMeshConfig->pMemoryConfig->maxFriendSubscrListSize)
/*! Total number of friendships entries */
#define GET_MAX_NUM_CTX()             (pMeshConfig->pMemoryConfig->maxNumFriendships)

/*! Computes difference between old LPN counter and new LPN counter modulo 65535 */
#define FRIEND_LPN_CTR_WRAP_DIFF(cOld, cNew)  ((0x10000 - (cOld) + (cNew)) & 0xFFFF)

/*! Init value for the local Current/Next FSN field */
#define FRIEND_CRT_NEXT_FSN_INIT_VAL       0xFF
/*! Init value for the Subscription Add/Remove transaction ID */
#define FRIEND_SUBSCR_TRANS_NUM_INIT_VAL   0xFF

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! State machine states */
enum meshFriendSmStates
{
  FRIEND_ST_IDLE,                /*!< Idle. Friendship disabled. */
  FRIEND_ST_WAIT_REQ,            /*!< Waiting for Friend Request */
  FRIEND_ST_START_KEY_DERIV,     /*!< Starting Friendship material derivation */
  FRIEND_ST_KEY_DERIV_LATE,      /*!< Late waiting key derivation to complete. Error state */
  FRIEND_ST_WAIT_RECV_TIMEOUT,   /*!< Waiting for Receive Delay Timeout */
  FRIEND_ST_WAIT_POLL,           /*!< Waiting for LPN Poll. Friendship not established  */
  FRIEND_ST_ESTAB,               /*!< Friendship establishment complete */
};

/*! State definition. See ::meshFriendSmStates. */
typedef uint8_t meshFriendSmState_t;

/*! Data structure for ::MESH_FRIEND_MSG_FRIEND_REQ_RECV */
typedef struct meshFriendReq_tag
{
  wsfMsgHdr_t   hdr;              /*!< Header structure */
  meshAddress_t lpnAddr;          /*!< Address of the LPN. */
  uint16_t      netKeyIndex;      /*!< NetKey Index. */
  uint32_t      localDelay;       /*!< Local delay in milliseconds to send Friend Offer. */
  uint8_t       recvDelay;        /*!< Receive delay in ms */
  uint32_t      pollTimeout;      /*!< Initial value of the PollTimeout timer */
  meshAddress_t prevAddr;         /*!< Previous friend primary element address */
  uint8_t       numElements;      /*!< Number of elements in the LPN */
  uint16_t      lpnCounter;       /*!< Number of Friend Request messages sent by LPN */
  int8_t        rssi;             /*!< RSSI measured on the Friend Requet message */
} meshFriendReq_t;

/*! Data structure for ::MESH_FRIEND_MSG_POLL_RECV */
typedef struct meshFriendPoll_tag
{
  wsfMsgHdr_t   hdr;        /*!< Header structure */
  meshAddress_t lpnAddr;    /*!< Address of the LPN. */
  uint8_t       fsn;        /*!< Friend Sequence number. */
} meshFriendPoll_t;

/*! Data structure for ::MESH_FRIEND_MSG_FRIEND_CLEAR_RECV */
typedef struct meshFriendClear_tag
{
  wsfMsgHdr_t   hdr;        /*!< Header structure */
  meshAddress_t friendAddr; /*!< Address of the previous friend. */
  meshAddress_t lpnAddr;    /*!< Address of the LPN. */
  uint16_t      lpnCounter; /*!< New relationship LPN counter */
} meshFriendClear_t;

/*! Data structure for ::MESH_FRIEND_MSG_CLEAR_CNF_RECV */
typedef meshFriendClear_t meshFriendClearCnf_t;

/*! Data structure for ::MESH_FRIEND_MSG_SUBSCR_LIST_ADD and ::MESH_FRIEND_MSG_SUBSCR_LIST_REM */
typedef struct meshFriendSubscrList_tag
{
  wsfMsgHdr_t   hdr;          /*!< Header structure */
  meshAddress_t *pSubscrList; /*!< Pointer to subscription list. */
  uint8_t       listSize;     /*!< Size of the subscription list */
  uint8_t       transNum;     /*!< Transaction number. */
  meshAddress_t lpnAddr;      /*!< Address of the LPN */
} meshFriendSubscrList_t;

/*! Data structure for ::MESH_FRIEND_MSG_NETKEY_DEL */
typedef struct meshFriendNetKeyDel_tag
{
  wsfMsgHdr_t   hdr;         /*!< Header structure */
  uint16_t      netKeyIndex; /*!< Index of the deleted NetKey. */
} meshFriendNetKeyDel_t;

/*! Messages passed to the state machine */
typedef union meshFriendSmMsg_tag
{
  wsfMsgHdr_t             hdr;            /*!< Header structure. Used for some SM messages */
  meshFriendReq_t         friendReq;      /*!< Friend Request message */
  meshFriendPoll_t        friendPoll;     /*!< Friend Request message */
  meshFriendClear_t       friendClear;    /*!< Friend Request message */
  meshFriendClearCnf_t    friendClearCnf; /*!< Friend Clear Confirm message */
  meshFriendSubscrList_t  friendSubscr;   /*!< Friend Subscription List Add/Remove message */
} meshFriendSmMsg_t;

/*! FSN shift values for current and new FSN */
enum meshFriendFsmMasks
{
  FRIEND_FSN_CRT_SHIFT,
  FRIEND_FSN_NEXT_SHIFT
};

/*! Friendship establishment information structure. */
typedef struct meshFriendEstabInfo_tag
{
  uint32_t            pollTimeout;    /*!< Poll Timeout timer */
  meshAddress_t       prevFriendAddr; /*!< Address of the previous friend of the LPN */
  uint16_t            friendCounter;  /*!< Friend counter */
  uint16_t            lpnCounter;     /*!< LPN counter */
  uint8_t             recvDelay;      /*!< Receive Delay */
  uint8_t             numElements;    /*!< Number of elements on the LPN */
  int8_t              reqRssi;        /*!< RSSI obtained on Friend Request */
} meshFriendEstabInfo_t;

/*! Enumeration of the flags field in the Friend Queue. */
enum meshFriendQueueFlags
{
  FRIEND_QUEUE_FLAG_EMPTY      = 0,      /*!< Entry is empty */
  FRIEND_QUEUE_FLAG_DATA_PDU   = 1 << 0, /*!< Entry contains an Access or a Control PDU */
  FRIEND_QUEUE_FLAG_UPDT_PDU   = 1 << 1, /*!< Entry contains a Friend Update PDU */
  FRIEND_QUEUE_FLAG_ACK_PDU    = 1 << 2, /*!< Entry contains an UTR ACK PDU */
  FRIEND_QUEUE_FLAG_ACK_PEND   = 1 << 3, /*!< Entry sent, pending ACK. */
};

/*! Friend queue entry definition. */
typedef struct meshFriendQueueEntry_tag
{
  void               *pNext;
  uint32_t           ivIndex;                               /*!< IV index. */
  meshSeqNumber_t    seqNo;                                 /*!< Sequence number */
  meshAddress_t      src;                                   /*!< SRC address */
  meshAddress_t      dst;                                   /*!< DST address */
  uint8_t            ctl;                                   /*!< Control or Access PDU: 1 for
                                                             *   Control PDU, 0 for Access PDU
                                                             */
  uint8_t            ttl;                                   /*!< TTL */
  uint8_t            ltrPdu[MESH_FRIEND_QUEUE_MAX_LTR_PDU]; /*!< MAX LTR PDU to be included in the
                                                             *   NWK PDU
                                                             */
  uint8_t            ltrPduLen;                             /*!< LTR PDU length */
  uint8_t            flags;
} meshFriendQueueEntry_t;

/*! LPN context entry definition. */
typedef struct meshFriendLpnCtx_tag
{
  meshFriendEstabInfo_t   estabInfo;               /*!< Establishment information. */
  wsfTimer_t              pollTmr;                 /*!< Poll Timer */
  wsfTimer_t              recvDelayTmr;            /*!< Receive Delay Timer */
  wsfTimer_t              subscrCnfRecvDelayTmr;   /*!< Subscription List Confirm Send Delay Timer
                                                    */
  wsfTimer_t              clearPeriodTmr;          /*!< Friend Clear Period timer */
  uint32_t                clearPeriodTimeSec;      /*!< Friend Clear Period time in seconds */
  wsfQueue_t              pduQueue;                /*!< WSF Queue used for organizing the Friend
                                                    *   Queue
                                                    */
  meshFriendQueueEntry_t  *pQueuePool;             /*!< Pool of Friend Queue entries */
  meshAddress_t           *pSubscrAddrList;        /*!< Pointer to Subscription List */
  meshAddress_t           lpnAddr;                 /*!< LPN address */
  uint16_t                netKeyIndex;             /*!< NetKey index for identifying sub-net */
  uint8_t                 pduQueueFreeCount;       /*!< Count of unallocated queue entries */
  uint8_t                 crtNextFsn;              /*!< Encoding of current and next FSN */
  uint8_t                 transNum;                /*!< Transaction number for Friend Subscription
                                                    */
  meshFriendSmState_t     friendSmState;           /*!< Current State machine state for friendship
                                                    */
  bool_t                  inUse;                   /*!< TRUE if the context is in use */
} meshFriendLpnCtx_t;

/*! State machine action function type */
typedef void (*meshFriendAct_t)(meshFriendLpnCtx_t *pCtx, void *pMsg);

/*! Data type for state machine table entry */
typedef uint8_t meshFriendTblEntry_t[MESH_FRIEND_SM_NUM_COLS];

/*! State machine interface type */
typedef struct meshFriendSmIf_tag
{
  meshFriendTblEntry_t const * const *pStateTbl;  /*!< Pointer to state table */
  meshFriendAct_t      const *pActionTbl;         /*!< Pointer to action table */
  meshFriendTblEntry_t const *pCommonTbl;         /*!< Pointer to common action table */
} meshFriendSmIf_t;

/*! Friend control block */
typedef struct meshFriendCb_tag
{
  meshFriendSmIf_t const   *pSm;           /*!< State machine interface */
  meshFriendLpnCtx_t       *pLpnCtxTbl;    /*!< LPN Context table. */
  meshFriendStates_t       state;          /*!< Friendship module state. */
  uint16_t                 friendCounter;  /*!< Friend counter */
  uint8_t                  recvWindow;     /*!< Receive window */
} meshFriendCb_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* State machine execute function */
void meshFriendSmExecute(meshFriendLpnCtx_t *pCtx, meshFriendSmMsg_t *pMsg);

/* State machine action functions */
void meshFriendActNone(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActDealloc(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActPrepKeyMat(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActSendOffer(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActSetupFriendship(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActStartRecvDelay(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActSendNextPdu(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActSendSubscrCnf(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActTerminate(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActNotifyFriend(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActStopNotifyFriend(meshFriendLpnCtx_t *pCtx, void *pMsg);
void meshFriendActUpdateSubscrList(meshFriendLpnCtx_t *pCtx, void *pMsg);

/* Context reset function. */
void meshFriendResetLpnCtx(uint8_t idx);

/* Friend Queue API */
void meshFriendQueueAddUpdate(meshFriendLpnCtx_t *pCtx);
void meshFriendQueueAddPdu(meshFriendLpnCtx_t *pCtx, uint8_t ctl, uint8_t ttl, uint32_t seqNo,
                           meshAddress_t src, meshAddress_t dst, uint32_t ivIndex,
                           uint8_t *pLtrHdr, uint8_t *pLtrUtrPdu, uint8_t pduLen);
void meshFriendQueueSendNextPdu(meshFriendLpnCtx_t *pCtx);
void meshFriendQueueRmAckPendPdu(meshFriendLpnCtx_t *pCtx);
uint8_t meshFriendQueueGetMaxFreeEntries(meshFriendLpnCtx_t *pCtx);

/* Friend Data Path callbacks. */
bool_t meshFriendLpnDstCheckCback(meshAddress_t dst, uint16_t netKeyIndex);
bool_t meshFriendQueuePduAddCback(const void *pPduInfo, meshFriendQueuePduType_t pduType);
void meshFriendQueueSarRxPduAddCback(meshSarRxPduType_t pduType,
                                     const meshSarRxReassembledPduInfo_t *pReasPduInfo,
                                     const meshSarRxSegInfoFriend_t *pSegInfoArray,
                                     uint32_t ivIndex, uint16_t seqZero, uint8_t segN);

/**************************************************************************************************
  Global variables
**************************************************************************************************/

/*! Mesh Friend control block. */
extern meshFriendCb_t friendCb;

/*! State machine interface */
extern const meshFriendSmIf_t meshFriendSrSmIf;

#ifdef __cplusplus
}
#endif

#endif /* MESH_FRIEND_MAIN_H */
