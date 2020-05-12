/*************************************************************************************************/
/*!
 *  \file   mesh_seq_manager.h
 *
 *  \brief  SEQ manager module interface.
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

#ifndef MESH_SEQ_MANAGER_H
#define MESH_SEQ_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh SEQ Manager return value. See ::meshReturnValues
 *  for codes starting at ::MESH_SEQ_RETVAL_BASE
 */
typedef uint16_t meshSeqRetVal_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh SEQ Manager SEQ number threshold exceeded callback function pointer type.
 *
 *  \param[in] lowThresh   TRUE if lower threshold exceeded
 *  \param[in] highThresh  TRUE if higher threshold exceeded
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSeqThreshCback_t)(bool_t lowThresh, bool_t highThresh);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh SEQ Manager.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSeqInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Mesh SEQ Manager exhaust callback registration.
 *
 *  \param[in] seqThreshCback  Threshold exceeded callback.
 *  \param[in] lowThresh       Lower threshold for which a notification is triggered.
 *  \param[in] highThresh      Higher threshold for which a notification is triggered.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSeqRegister(meshSeqThreshCback_t seqThreshCback, uint32_t lowThresh, uint32_t highThresh);

/*************************************************************************************************/
/*!
 *  \brief      Gets the current SEQ number for a source address.
 *
 *  \param[in]  srcAddr    Source address for which to get the SEQ number.
 *  \param[out] pOutSeqNo  Pointer to store the current SEQ number for the input source address.
 *  \param[in]  autoInc    Auto increment the SEQ number after get.
 *
 *  \return     Success or error reason. \see meshSeqRetVal_t
 */
/*************************************************************************************************/
meshSeqRetVal_t MeshSeqGetNumber(meshAddress_t srcAddr, meshSeqNumber_t *pOutSeqNo, bool_t autoInc);

/*************************************************************************************************/
/*!
 *  \brief     Increments the current SEQ number for a source address.
 *
 *  \param[in] srcAddr  Source address for which to increment the SEQ number.
 *
 *  \return    Success or error reason. \see meshSeqRetVal_t
 */
/*************************************************************************************************/
meshSeqRetVal_t MeshSeqIncNumber(meshAddress_t srcAddr);

/*************************************************************************************************/
/*!
 *  \brief  Resets all SEQ numbers for all addresses.
 *
 *  \return None
 */
/*************************************************************************************************/
void MeshSeqReset(void);

#ifdef __cplusplus
}
#endif

#endif /* MESH_SEQ_MANAGER_H */
