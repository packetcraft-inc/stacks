/*************************************************************************************************/
/*!
 *  \file   mesh_friend.h
 *
 *  \brief  Friend module interface.
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

#ifndef MESH_FRIEND_H
#define MESH_FRIEND_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! State machines events */
enum meshFriendWsfMsgEvents
{
  MESH_FRIEND_MSG_STATE_ENABLED = MESH_FRIENDSHIP_MSG_START, /*!< State has changed to enabled */
  MESH_FRIEND_MSG_STATE_DISABLED,                            /*!< State has changed to disabled */
  MESH_FRIEND_MSG_FRIEND_REQ_RECV,                           /*!< Friend Request received */
  MESH_FRIEND_MSG_POLL_RECV,                                 /*!< Friend Poll received */
  MESH_FRIEND_MSG_FRIEND_CLEAR_RECV,                         /*!< Friend Clear received */
  MESH_FRIEND_MSG_FRIEND_CLEAR_CNF_RECV,                     /*!< Friend Clear received */
  MESH_FRIEND_MSG_KEY_DERIV_SUCCESS,                         /*!< Key material derivation
                                                              *   successful.
                                                              */
  MESH_FRIEND_MSG_KEY_DERIV_FAILED,                          /*!< Key material derivation failed */
  MESH_FRIEND_MSG_RECV_DELAY,                                /*!< Receive delay timer expired */
  MESH_FRIEND_MSG_SUBSCR_CNF_DELAY,                          /*!< Subscription List Confirm
                                                              *   Receive delay timer expired
                                                              */
  MESH_FRIEND_MSG_CLEAR_SEND_TIMEOUT,                        /*!< Friend Clear Period Timeout */
  MESH_FRIEND_MSG_TIMEOUT,                                   /*!< Establishment timer or Poll
                                                              *   timeout
                                                              */
  MESH_FRIEND_MSG_SUBSCR_LIST_ADD,                           /*!< Subscription List Add */
  MESH_FRIEND_MSG_SUBSCR_LIST_REM,                           /*!< Subscription List Remove */
  MESH_FRIEND_MSG_NETKEY_DEL                                 /*!< NetKey Deleted */
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_FRIEND_H */
