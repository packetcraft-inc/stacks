/*************************************************************************************************/
/*!
 *  \file   mesh_replay_protection.h
 *
 *  \brief  Message replay protection definitions.
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

#ifndef MESH_REPLAY_PROTECTION_H
#define MESH_REPLAY_PROTECTION_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Replay Protection List and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshRpInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshRpGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief     Verifies a PDU for replay attacks.
 *
 *  \param[in] srcAddr  Address of originating element.
 *  \param[in] seqNo    Sequence number used to identify the PDU.
 *  \param[in] ivIndex  The IV used with the PDU.
 *
 *  \return    TRUE if PDU is replay attack, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshRpIsReplayAttack(meshAddress_t srcAddr, meshSeqNumber_t seqNo, uint32_t ivIndex);

/*************************************************************************************************/
/*!
 *  \brief     Updates the Replay Protection List for a given element with a specific sequnce number.
 *
 *  \param[in] srcAddr  Address of originating element.
 *  \param[in] seqNo    Sequence number used to identify the PDU.
 *  \param[in] ivIndex  The IV used with the PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRpUpdateList(meshAddress_t srcAddr, meshSeqNumber_t seqNo, uint32_t ivIndex);

/*************************************************************************************************/
/*!
 *  \brief  Clears the Replay Protection List.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshRpClearList(void);

  /*************************************************************************************************/
  /*!
   *  \brief  Clears the Replay Protection List.
   *
   *  \return None.
   */
  /*************************************************************************************************/
  void MeshRpNvmErase(void);

#ifdef __cplusplus
}
#endif

#endif /* MESH_REPLAY_PROTECTION_H */
