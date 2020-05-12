/*************************************************************************************************/
/*!
 *  \file   mesh_nwk_mgmt.h
 *
 *  \brief  Network Management interface.
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

#ifndef MESH_NWK_MGMT_H
#define MESH_NWK_MGMT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Friendship security parameters changed callback */
typedef void (*meshNwkMgmtFriendshipSecChgCback_t)(bool_t ivChg, bool_t keyChg,
                                                   uint16_t netKeyIndex);

/*! Mesh Network Management WSF events */
enum meshNwkMgmtWsfEvents
{
  MESH_NWK_MGMT_MSG_IV_UPDT_TMR = MESH_NWK_MGMT_MSG_START, /*!< IV Update guard time expired */
  MESH_NWK_MGMT_MSG_IV_RECOVER_TMR,                        /*!< IV recovery guard time expired */
  MESH_NWK_MGMT_MSG_IV_UPDT_DISALLOWED,                    /*!< Node is disallowed to switch to
                                                            *   new IV
                                                            */
  MESH_NWK_MGMT_MSG_IV_UPDT_ALLOWED,                       /*!< Node is allowed to switch to new
                                                            *   IV
                                                            */
  MESH_NWK_MGMT_MSG_PRV_COMPLETE,                          /*!< Provisioning complete event */
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief   Initializes the Network Management module.
 *
 *  \return  None.
 *
 *  \remarks This function must be called after initializing the Sequence Manager module
 */
/*************************************************************************************************/
void MeshNwkMgmtInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers friendship notification callback.
 *
 *  \param[in] secChgCback  Security parameters changed notification callback.
 *
 *  \return    None.
 *
 *  \remarks   This function must be called after initializing the Sequence Manager module
 */
/*************************************************************************************************/
void MeshNwkMgmtRegisterFriendship(meshNwkMgmtFriendshipSecChgCback_t secChgCback);

/*************************************************************************************************/
/*!
 *  \brief     Manages a new Key Refresh State of a NetKey.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] oldState     Old Key Refresh Phase State.
 *  \param[in] newState     New Key Refresh Phase State.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkMgmtHandleKeyRefreshTrans(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                      meshKeyRefreshStates_t newState);

/*************************************************************************************************/
/*!
 *  \brief     Manages key and IV information obtained from a Secure Network Beacon for a subnet.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] newKeyUsed   TRUE if the new key was used to authenticate.
 *  \param[in] ivIndex      Received IV index.
 *  \param[in] keyRefresh   TRUE if Key Refresh flag is set, FALSE otherwise.
 *  \param[in] ivUpdate     TRUE if IV Update flag is set, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkMgmtHandleBeaconData(uint16_t netKeyIndex, bool_t newKeyUsed, uint32_t ivIndex,
                                 bool_t keyRefresh, bool_t ivUpdate);

#ifdef __cplusplus
}
#endif

#endif /* MESH_NWK_MGMT_H */
