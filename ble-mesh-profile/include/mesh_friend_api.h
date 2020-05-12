/*************************************************************************************************/
/*!
 *  \file   mesh_friend_api.h
 *
 *  \brief  Mesh Friend Node API.
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

/*! ***********************************************************************************************
*
* @addtogroup MESH_FRIEND_API Mesh Friend API
* @{
************************************************************************************************ */

#ifndef MESH_FRIEND_API_H
#define MESH_FRIEND_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Memory required value in bytes if success or ::MESH_MEM_REQ_INVALID_CFG in case fail.
 */
/*************************************************************************************************/
uint32_t MeshFriendGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief     Initialize Friend Node memory requirements.
 *
 *  \param[in] pFreeMem     Pointer to free memory.
 *  \param[in] freeMemSize  Size of pFreeMem.
 *
 *  \return    Amount of free memory consumed.
 *
 *  \note      This function must be called once after Mesh Stack initialization.
 */
/*************************************************************************************************/
uint32_t MeshFriendMemInit(uint8_t *pFreeMem, uint32_t freeMemSize);

/*************************************************************************************************/
/*!
 *  \brief     Initialize Friend Node feature.
 *
 *  \param[in] recvWinMs    Receive Window in milliseconds.
 *
 *  \return    None.
 *
 *  \note      This function and MeshLpnInit() are mutually exclusive.
 */
/*************************************************************************************************/
void MeshFriendInit(uint8_t recvWinMs);

#ifdef __cplusplus
}
#endif

#endif /* MESH_FRIEND_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
