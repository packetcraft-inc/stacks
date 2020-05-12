/*************************************************************************************************/
/*!
 *  \file   mesh_lpn.h
 *
 *  \brief  LPN module interface.
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

#ifndef MESH_LPN_H
#define MESH_LPN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Event handler messages for LPN state machines */
enum meshLpnWsfMsgEvents
{
  MESH_LPN_MSG_ESTABLISH = MESH_FRIENDSHIP_MSG_START,   /*!< Friendship Establish */
  MESH_LPN_MSG_TERMINATE,                               /*!< Friendship Terminate */

  MESH_LPN_MSG_SEND_FRIEND_REQ,                         /*!< Send Friend Request PDU */
  MESH_LPN_MSG_SEND_FRIEND_POLL,                        /*!< Send Friend Poll PDU */
  MESH_LPN_MSG_SEND_FRIEND_CLEAR,                       /*!< Send Friend Clear PDU */
  MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM,               /*!< Send Friend Subscription List
                                                         *   Add/Remove PDU
                                                         */
  MESH_LPN_MSG_RESEND_FRIEND_SUBSCR_ADD_RM,             /*!< Re-send Friend Subscription
                                                         *   Add/Remove PDU
                                                         */
  MESH_LPN_MSG_FRIEND_OFFER,                            /*!< Friend Offer selected */
  MESH_LPN_MSG_FRIEND_UPDATE,                           /*!< Received Friend Update PDU */
  MESH_LPN_MSG_FRIEND_MESSAGE,                          /*!< Received Friend message PDU */
  MESH_LPN_MSG_FRIEND_SUBSCR_CNF,                       /*!< Received Friend Subscription List
                                                         *   Confirm PDU
                                                         */
  MESH_LPN_MSG_RECV_DELAY_TIMEOUT,                      /*!< LPN Receive Delay timer timeout */
  MESH_LPN_MSG_RECV_WIN_TIMEOUT,                        /*!< LPN Receive Window timer timeout */
  MESH_LPN_MSG_POLL_TIMEOUT                             /*!< LPN timer poll timeout */
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_LPN_H */
