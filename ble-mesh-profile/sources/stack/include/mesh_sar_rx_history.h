/*************************************************************************************************/
/*!
 *  \file   mesh_sar_rx_history.h
 *
 *  \brief  SAR Rx transaction history interface.
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
#ifndef MESH_SAR_RX_HISTORY_H
#define MESH_SAR_RX_HISTORY_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Apply seqZero 13-bit mask on SEQ */
#define SAR_RX_SEQZERO(seqNo)   ((uint16_t)seqNo & MESH_SEQ_ZERO_MASK)

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes.
 */
/*************************************************************************************************/
uint32_t MeshSarRxHistoryGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the SAR Rx Transaction History table and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Adds the SAR Rx transaction parameters in the SAR Rx Transaction History table.
 *
 *  \param[in] srcAddr  Source address.
 *  \param[in] seqNo    Sequence number that is used on SAR TX for seqZero computation.
 *  \param[in] iviLsb   Least significant 2 bits of the IV Index for the received packet.
 *  \param[in] segN     SegN value for the transaction.
 *  \param[in] obo      TRUE if transaction was realized on behalf of a LPN node.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryAdd(meshAddress_t srcAddr, uint32_t seqNo, uint8_t iviLsb, uint8_t segN,
                         bool_t obo);

/*************************************************************************************************/
/*!
 *  \brief      Checks if the SAR Rx Transaction History table contains a transaction that matches
 *              the parameters.
 *
 *  \param[in]  srcAddr      Source address.
 *  \param[in]  seqNo        Sequence number.
 *  \param[in]  seqZero      Seq Zero value.
 *  \param[in]  iviLsb       Least significant 2 bits of the IV Index for the received packet.
 *  \param[in]  segN         SegN value for the transaction.
 *  \param[out] pOutSendAck  Set to TRUE if we need to send ACK. Valid only for FALSE return value.
 *  \param[out] pOutObo      Set to TRUE if OBO flag should be set, FALSE otherwise.
 *
 *  \return     TRUE if transaction was not found, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSarRxHistoryCheck(meshAddress_t srcAddr, uint32_t seqNo, uint16_t seqZero,
                             uint8_t iviLsb, uint8_t segN, bool_t *pOutSendAck, bool_t *pOutObo);

/*************************************************************************************************/
/*!
 *  \brief  Resets the SAR Rx Transaction History table.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryReset(void);

/*************************************************************************************************/
/*!
 *  \brief     Clears History entries for source address with older SeqAuth.
 *
 *  \param[in] srcAddr  Source address.
 *  \param[in] seqZero  Seq Zero value.
 *  \param[in] iviLsb   Least significant 2 bits of the IV Index for the received packet.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryCleanupOld(meshAddress_t srcAddr, uint16_t seqZero, uint8_t iviLsb);

/*************************************************************************************************/
/*!
 *  \brief     Clears entries with lower IV values than new IV index.
 *
 *  \param[in] newIvIndex  New value for the IV index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryIviCleanup(uint32_t newIvIndex);

#ifdef __cplusplus
}
#endif

#endif /* MESH_SAR_RX_HISTORY_H */
