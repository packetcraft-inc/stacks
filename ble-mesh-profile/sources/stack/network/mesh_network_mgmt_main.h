/*************************************************************************************************/
/*!
 *  \file   mesh_network_mgmt_main.h
 *
 *  \brief  Network Management internal definitions.
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

#ifndef MESH_NETWORK_MGMT_MAIN_H
#define MESH_NETWORK_MGMT_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Action function prototype for handling Key Refresh transitions.
 */
/*************************************************************************************************/
typedef void (*keyRefreshTransAct_t)(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                     meshKeyRefreshStates_t newState);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

static void meshNwkMgmtTransNone(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                 meshKeyRefreshStates_t newState);

static void meshNwkMgmTransJustSet(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                   meshKeyRefreshStates_t newState);

static void meshNwkMgmtTransRevokeOld(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                      meshKeyRefreshStates_t newState);


#ifdef __cplusplus
}
#endif

#endif /* MESH_NETWORK_MGMT_MAIN_H */
