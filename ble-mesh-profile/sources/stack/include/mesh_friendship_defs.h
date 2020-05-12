/*************************************************************************************************/
/*!
 *  \file   mesh_friendship_defs.h
 *
 *  \brief  Mesh Friendship common definitions.
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

#ifndef MESH_FRIENDSHIP_DEFS_H
#define MESH_FRIENDSHIP_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Friend Poll message parameters number of bytes */
#define MESH_FRIEND_POLL_NUM_BYTES                          0x01
/*! Mesh Friend Poll FSN offset in message */
#define MESH_FRIEND_POLL_FSN_OFFSET                         0
/*! Mesh Friend Poll FSN mask */
#define MESH_FRIEND_POLL_FSN_MASK                           0x01
/*! Mesh Friend Poll FSN shift */
#define MESH_FRIEND_POLL_FSN_SHIFT                          0
/*! Mesh Friend Poll FSN size */
#define MESH_FRIEND_POLL_FSN_SIZE                           1

/*! Mesh Friend Update message parameters number of bytes */
#define MESH_FRIEND_UPDATE_NUM_BYTES                        0x06
/*! Mesh Friend Update Flags offset in message */
#define MESH_FRIEND_UPDATE_FLAGS_OFFSET                     0
/*! Mesh Friend Update IV Index offset in message */
#define MESH_FRIEND_UPDATE_IVINDEX_OFFSET                   1
/*! Mesh Friend Update MD offset in message */
#define MESH_FRIEND_UPDATE_MD_OFFSET                        5
/*! Mesh Friend Update Key Refresh Flag mask */
#define MESH_FRIEND_UPDATE_KEY_REFRESH_FLAG_MASK            0x01
/*! Mesh Friend Update Key Refresh Flag shift */
#define MESH_FRIEND_UPDATE_KEY_REFRESH_FLAG_SHIFT           0
/*! Mesh Friend Update Key Refresh Flag size in bits */
#define MESH_FRIEND_UPDATE_KEY_REFRESH_FLAG_SIZE            1
/*! Mesh Friend Update IV Update Flag mask */
#define MESH_FRIEND_UPDATE_IV_UPDATE_FLAG_MASK              0x01
/*! Mesh Friend Update IV Update Flag shift */
#define MESH_FRIEND_UPDATE_IV_UPDATE_FLAG_SHIFT             1
/*! Mesh Friend Update IV Update Flag size in bits */
#define MESH_FRIEND_UPDATE_IV_UPDATE_FLAG_SIZE              1

/*! Mesh Friend Request message parameters number of bytes */
#define MESH_FRIEND_REQUEST_NUM_BYTES                       0x0A
/*! Mesh Friend Request Criteria offset in message */
#define MESH_FRIEND_REQUEST_CRITERIA_OFFSET                 0
/*! Mesh Friend Request Receive Delay offset in message */
#define MESH_FRIEND_REQUEST_RECV_DELAY_OFFSET               1
/*! Mesh Friend Request Poll Timeout offset in message */
#define MESH_FRIEND_REQUEST_POLL_TIMEOUT_OFFSET             2
/*! Mesh Friend Request Previous Address offset in message */
#define MESH_FRIEND_REQUEST_PREV_ADDR_OFFSET                5
/*! Mesh Friend Request Num Elements offset in message */
#define MESH_FRIEND_REQUEST_NUM_ELEMENTS_OFFSET             7
/*! Mesh Friend Request LPN Counter offset in message */
#define MESH_FRIEND_REQUEST_LPN_COUNTER_OFFSET              8
/*! Mesh Friend Request Min Queue Size Log mask */
#define MESH_FRIEND_REQUEST_MIN_QUEUE_SIZE_MASK             0x07
/*! Mesh Friend Request Min Queue Size Log shift */
#define MESH_FRIEND_REQUEST_MIN_QUEUE_SIZE_SHIFT            0
/*! Mesh Friend Request Min Queue Size Log size in bits */
#define MESH_FRIEND_REQUEST_MIN_QUEUE_SIZE_SIZE             3
/*! Mesh Friend Request Receive Window Factor mask */
#define MESH_FRIEND_REQUEST_RECV_WIN_FACTOR_MASK            0x03
/*! Mesh Friend Request Receive Window Factor shift */
#define MESH_FRIEND_REQUEST_RECV_WIN_FACTOR_SHIFT           3
/*! Mesh Friend Request Receive Window Factor size */
#define MESH_FRIEND_REQUEST_RECV_WIN_FACTOR_SIZE            2
/*! Mesh Friend Request RSSI Factor mask */
#define MESH_FRIEND_REQUEST_RSSI_FACTOR_MASK                0x03
/*! Mesh Friend Request RSSI Factor shift */
#define MESH_FRIEND_REQUEST_RSSI_FACTOR_SHIFT               5
/*! Mesh Friend Request RSSI Factor size */
#define MESH_FRIEND_REQUEST_RSSI_FACTOR_SIZE                2
/*! Mesh Friend Request Criteria RFU mask */
#define MESH_FRIEND_REQUEST_CRITERIA_RFU_MASK               0x01
/*! Mesh Friend Request Criteria RFU shift */
#define MESH_FRIEND_REQUEST_CRITERIA_RFU_SHIFT              7
/*! Mesh Friend Request Criteria RFU size */
#define MESH_FRIEND_REQUEST_CRITERIA_RFU_SIZE               1

/*! Mesh Friend Offer message parameters number of bytes */
#define MESH_FRIEND_OFFER_NUM_BYTES                         0x06
/*! Mesh Friend Offer Receive Window offset in message */
#define MESH_FRIEND_OFFER_RECV_WIN_OFFSET                   0
/*! Mesh Friend Offer Queue Size offset in message */
#define MESH_FRIEND_OFFER_QUEUE_SIZE_OFFSET                 1
/*! Mesh Friend Offer RSSI offset in message */
#define MESH_FRIEND_OFFER_RSSI_OFFSET                       2
/*! Mesh Friend Offer Subscription List Size offset in message */
#define MESH_FRIEND_OFFER_SUBSCR_LIST_SIZE_OFFSET           3
/*! Mesh Friend Offer Friend Counter offset in message */
#define MESH_FRIEND_OFFER_FRIEND_COUNTER_OFFSET             4

/*! Mesh Friend Clear message parameters number of bytes */
#define MESH_FRIEND_CLEAR_NUM_BYTES                         0x04
/*! Mesh Friend Clear LPN Address offset in message */
#define MESH_FRIEND_CLEAR_LPN_ADDR_OFFSET                   0
/*! Mesh Friend Clear LPN Counter offset in message */
#define MESH_FRIEND_CLEAR_LPN_COUNTER_OFFSET                2

/*! Mesh Friend Clear Confirm message parameters number of bytes */
#define MESH_FRIEND_CLEAR_CNF_NUM_BYTES                     0x04
/*! Mesh Friend Clear Confirm LPN Address offset in message */
#define MESH_FRIEND_CLEAR_CNF_LPN_ADDR_OFFSET               0
/*! Mesh Friend Clear Confirm LPN Counter offset in message */
#define MESH_FRIEND_CLEAR_CNF_LPN_COUNTER_OFFSET            2

/*! Mesh Friend Subscription List Add/Remove message parameters number of bytes */
#define MESH_FRIEND_SUBSCR_LIST_ADD_RM_NUM_BYTES(numAddr)      (((numAddr) << 1) + 1)
/*! Mesh Friend Subscription List Add number of addresses from size. */
#define MESH_FRIEND_SUBSCR_LIST_ADD_RM_NUM_ADDR(size)          (((size) - 1) >> 1)
/*! Mesh Friend Subscription List Add Transaction Number offset in message */
#define MESH_FRIEND_SUBSCR_LIST_ADD_RM_TRAN_NUM_OFFSET         0
/*! Mesh Friend Subscription List Add Address List start offset in message */
#define MESH_FRIEND_SUBSCR_LIST_ADD_RM_ADDR_LIST_START_OFFSET  1
/*! Mesh Friend Subscription List Add message parameters maximum number of bytes */
#define MESH_FRIEND_SUBSCR_LIST_ADD_RM_MAX_NUM_BYTES           0x0B

/*! Validates Address List size constraints for Friend Subscription List */
#define MESH_FRIEND_SUBSCR_LIST_VALID(size)                (((size) & 0x01) == 1)

/*! Mesh Friend Subscription List Confirm message parameters number of bytes */
#define MESH_FRIEND_SUBSCR_LIST_CNF_NUM_BYTES              0x01
/*! Mesh Friend Subscription List Remove Transaction Number offset in message */
#define MESH_FRIEND_SUBSCR_LIST_CNF_TRAN_NUM_OFFSET        0

/*! Validates MD constraints */
#define MESH_FRIEND_MD_VALID(md)                           ((md) < MESH_FRIEND_MD_PROHIBITED_START)

/*! Validates RSSI Factor constraints */
#define MESH_FRIEND_RSSI_FACTOR_VALID(factor)              ((factor) <= MESH_FRIEND_RSSI_FACTOR_2_5)
/*! Validates Receive Window Factor constraints */
#define MESH_FRIEND_RECV_WIN_FACTOR_VALID(factor)          ((factor) <= MESH_FRIEND_RECV_WIN_FACTOR_2_5)
/*! Validates Receive Window constraints */
#define MESH_FRIEND_RECV_WIN_VALID(win)                    ((win) != 0x00)
/*! Validates Min Queue Size Log constraints */
#define MESH_FRIEND_MIN_QUEUE_SIZE_VALID(size)             (((size) != MESH_FRIEND_MIN_QUEUE_SIZE_PROHIBITED) &&\
                                                            ((size) <= MESH_FRIEND_MIN_QUEUE_SIZE_128))
/*! Validates Receive Delay constraints */
#define MESH_FRIEND_RECV_DELAY_VALID(delay)                ((delay) >= MESH_FRIEND_RECV_DELAY_MS_MIN)
/*! Mesh Poll Timeout step in milliseconds */
#define MESH_FRIEND_POLL_TIMEOUT_STEP_MS                   100
/*! Validates Poll Timeout constraints */
#define MESH_FRIEND_POLL_TIMEOUT_MS_VALID(timeout)         ((((timeout) / MESH_FRIEND_POLL_TIMEOUT_STEP_MS) >=\
                                                             MESH_FRIEND_POLL_TIMEOUT_MIN) &&\
                                                            (((timeout) / MESH_FRIEND_POLL_TIMEOUT_STEP_MS) <=\
                                                             MESH_FRIEND_POLL_TIMEOUT_MAX))
/*! RSSI value when it is not available */
#define MESH_FRIEND_RSSI_UNAVAILBLE                        127
/*! Minimum delay for sending Friend Offer in milliseconds */
#define MESH_FRIEND_MIN_OFFER_DELAY_MS                     100
/*! Maximum allowed difference between Friend Request LPN counter and Friend Clear LPN Counter */
#define MESH_FRIEND_MAX_LPN_CNT_WRAP_DIFF                  255

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Friendship MD values */
enum meshFriendMdValues
{
  MESH_FRIEND_MD_QUEUE_IS_EMPTY,
  MESH_FRIEND_MD_QUEUE_IS_NOT_EMPTY,
  MESH_FRIEND_MD_PROHIBITED_START
};

/*! Mesh Friendship Poll Timeout in 100ms steps values */
enum meshFriendPollTimeoutValues
{
  MESH_FRIEND_POLL_TIMEOUT_MIN = 0x00000A,
  MESH_FRIEND_POLL_TIMEOUT_MAX = 0x34BBFF
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_FRIENDSHIP_DEFS_H */
