/*************************************************************************************************/
/*!
 *  \file   mesh_sar_tx.h
 *
 *  \brief  SAR Tx module interface.
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

#ifndef MESH_SAR_TX_H
#define MESH_SAR_TX_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh SAR Tx transaction event status enumeration */
enum meshSarTxEventStatus
{
  MESH_SAR_TX_EVENT_SUCCESS   = 0x00,  /*!< SAR Tx transaction completed without errors */
  MESH_SAR_TX_EVENT_TIMEOUT   = 0x01,  /*!< SAR Tx transaction timed out */
  MESH_SAR_TX_EVENT_REJECTED  = 0x02,  /*!< SAR Tx transaction rejected */
};

/*! Mesh SAR Tx transaction event status. See ::meshSarTxEventStatus */
typedef uint8_t meshSarTxEventStatus_t;

/*! Definition of the acknowledged blocks mask. Bit i represents block i
 * Value 0b1 means block is acknowledged.
 * Value 0b0 means block is unacknowledged.
 */
typedef uint32_t meshSarTxBlockAck_t;

/*************************************************************************************************/
/*!
 *  \brief Notifies the upper layer the outcome of a SAR Tx transaction.
 *
 *  \param[in] eventStatus  Event status value. See ::meshSarTxEventStatus.
 *  \param[in] dst          Destination used to identify the SAR Tx transaction.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSarTxNotifyCback_t)(meshSarTxEventStatus_t eventStatus, meshAddress_t dst);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialises the SAR Tx module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the notification callback for the upper layer.
 *
 *  \param[in] notifyCback  Callback invoked when a SAR Tx transaction completes successfully or
 *                          with an error.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarTxRegister(meshSarTxNotifyCback_t notifyCback);

/*************************************************************************************************/
/*!
 *  \brief  Resets all ongoing SAR Tx transactions.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxReset(void);

/*************************************************************************************************/
/*!
 *  \brief  Instructs SAR Tx to reject new transactions.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxRejectIncoming(void);

/*************************************************************************************************/
/*!
 *  \brief  Instructs SAR Tx to accept new transactions.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxAcceptIncoming(void);

/*************************************************************************************************/
/*!
 *  \brief     Creates a SAR Tx transaction for a Control Message that is received by the Lower
 *             Transport from the Upper Transport and requires segmentation.
 *
 *  \param[in] pLtrPduInfo  Pointer to the Lower Transport PDU Information structure.
 *
 *  \return    TRUE if transaction started, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSarTxStartSegCtlTransaction(meshLtrCtlPduInfo_t *pLtrPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Creates a SAR Tx transaction for an Access Message that is received by the Lower
 *             Transport from the Upper Transport and requires segmentation.
 *
 *  \param[in] pLtrPduInfo  Pointer to the Lower Transport PDU Information structure.
 *
 *  \return    TRUE if transaction started, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSarTxStartSegAccTransaction(meshLtrAccPduInfo_t *pLtrPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Find an ongoing SAR Tx transaction and marks segments as acknowledged. If the
 *             segmented transaction is completed, all allocated memory is freed.
 *
 *  \param[in] remoteAddress  Mesh address of the remote device.
 *  \param[in] seqZero        Seq Zero value for the transaction.
 *  \param[in] oboFlag        On-behalf-of flag for this transaction.
 *  \param[in] blockAck       BlockAck field as received in the Segment Acknowledgement.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarTxProcessBlockAck(meshAddress_t remoteAddress,
                              uint16_t seqZero,
                              bool_t oboFlag,
                              meshSarTxBlockAck_t blockAck);

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CONFIG.
 */
/*************************************************************************************************/
uint32_t MeshSarTxGetRequiredMemory(void);

#ifdef __cplusplus
}
#endif

#endif /* MESH_SAR_TX_H */
