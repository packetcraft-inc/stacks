/*************************************************************************************************/
/*!
*  \file   mesh_sar_rx.h
*
*  \brief  SAR Rx module interface.
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
 * @file
 *
 * @addtogroup SAR_RX
 * @{
 *************************************************************************************************/

#ifndef MESH_SAR_RX_H
#define MESH_SAR_RX_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh SAR Rx return value data type. See ::meshReturnValues
 *  for codes starting at ::MESH_SAR_RX_RETVAL_BASE
 */
typedef uint16_t meshSarRxRetVal_t;

/*! Mesh SAR Rx transaction type */
enum meshSarRxPduTypeValues
{
  MESH_SAR_RX_TYPE_ACCESS = 0x00, /*!< Access PDU type */
  MESH_SAR_RX_TYPE_CTL    = 0x01  /*!< Control PDU type */
};

/*! Mesh SAR Rx reassembled PDU data type. See ::meshSarRxPduTypeValues */
typedef uint8_t meshSarRxPduType_t;

/*! Mesh SAR Rx reassembled PDU information */
typedef union meshSarRxReassembledPduInfo_tag
{
  meshLtrAccPduInfo_t accPduInfo;  /*!< Access PDU information */
  meshLtrCtlPduInfo_t ctlPduInfo;  /*!< Control PDU information */
} meshSarRxReassembledPduInfo_t;

/*! Segment information required to disassemble PDU in Friend Queue. */
typedef struct meshSarRxSegInfoFriend_tag
{
  meshSeqNumber_t segSeqNo; /*!< Segment sequence number. */
  uint16_t        offset;   /*!< Pointer to segment data in reassembled PDU. */
  uint8_t         segO;     /*!< Segment number. */
} meshSarRxSegInfoFriend_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR Rx reassemble complete callback.
 *
 *  \param[in] pduType       Type and format of the reassembled PDU.
 *  \param[in] pReasPduInfo  Pointer to reassembled PDU information.
 *
 *  \return    None.
 *
 *  \see meshSarRxReassembledPduInfo_t
 */
/*************************************************************************************************/
typedef void (*meshSarRxPduReassembledCback_t)(meshSarRxPduType_t pduType,
                                               const meshSarRxReassembledPduInfo_t *pReasPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR RX callback that verifies if an incoming PDU is destined for an LPN.
 *
 *  \param[in] dst          Destination address of the received PDU.
 *  \param[in] netKeyIndex  Global NetKey identifier.
 *
 *  \return    TRUE if at least one LPN needs the PDU, FALSE otherwise.
 */
/*************************************************************************************************/
typedef bool_t (*meshSarRxLpnDstCheckCback_t)(meshAddress_t dst, uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR Rx reassemble complete callback for Friend Queue.
 *
 *  \param[in] pduType        Type and format of the reassembled PDU.
 *  \param[in] pReasPduInfo   Pointer to reassembled PDU information.
 *  \param[in] pSegInfoArray  Additional information required to add segments in the Friend Queue.
 *  \param[in] ivIndex        IV index of the received segments.
 *  \param[in] seqZero        SeqZero field of the segments.
 *  \param[in] segN           Last segment number.
 *
 *  \return    None.
 *
 *  \see meshSarRxReassembledPduInfo_t
 */
/*************************************************************************************************/
typedef void (*meshSarRxFriendPduReassembledCback_t) (meshSarRxPduType_t pduType,
                                                      const meshSarRxReassembledPduInfo_t *pReasPduInfo,
                                                      const meshSarRxSegInfoFriend_t *pSegInfoArray,
                                                      uint32_t ivIndex, uint16_t seqZero, uint8_t segN);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief   Initializes the table that contains all ongoing SAR Rx transactions.
 *
 *  \remarks This function also cleans the associated resources and stops the timers used by the
 *           module.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshSarRxInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callback used by the SAR Rx.
 *
 *  \param[in] pduReassembledCback  Callback invoked by the SAR Rx module when a complete PDU is
 *                                  reassembled. The PDU is formatted depending on the intended
 *                                  recipient.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxRegister(meshSarRxPduReassembledCback_t pduReassembledCback);

/*************************************************************************************************/
/*!
 *  \brief     Registers callbacks for checking and adding reassembled PDU's to Friend Queue.
 *
 *  \param[in] lpnDstCheckCback    Callback that checks if at least one LPN is destination for a
 *                                 PDU.
 *  \param[in] friendPduReasCback  Callback invoked by the SAR Rx module when a complete PDU is
 *                                 reassembled and destination is at least one LPN.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxRegisterFriend(meshSarRxLpnDstCheckCback_t lpnDstCheckCback,
                             meshSarRxFriendPduReassembledCback_t friendPduReasCback);

/*************************************************************************************************/
/*!
 *  \brief     Processes a segment contained in a Network PDU Info structure.
 *
 *  \param[in] pNwkPduInfo  Pointer to the Network PDU Information structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxProcessSegment(const meshNwkPduRxInfo_t *pNwkPduInfo);

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or 0 in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshSarRxGetRequiredMemory(void);

#ifdef __cplusplus
}
#endif

#endif /* MESH_SAR_RX_H */

/*! **********************************************************************************************
 * @}
 *************************************************************************************************/
