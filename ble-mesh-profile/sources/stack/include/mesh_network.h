/*************************************************************************************************/
/*!
 *  \file   mesh_network.h
 *
 *  \brief  Network module interface.
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

#ifndef MESH_NETWORK_H
#define MESH_NETWORK_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Transport layer receives PDU's from the Network layer this format */
typedef struct meshNwkPduRxInfo_tag
{
  uint8_t          *pLtrPdu;     /*!< Pointer to the Lower Transport PDU */
  uint8_t          pduLen;       /*!< Length of the Lower Transport PDU */
  uint8_t          ctl;          /*!< Control or Access PDU: 1 for Control PDU, 0 for Access PDU */
  uint8_t          ttl;          /*!< TTL to be used. Shall be a valid value. */
  meshAddress_t    src;          /*!< SRC address */
  meshAddress_t    dst;          /*!< DST address */
  meshAddress_t    friendLpnAddr;/*!< Friend or LPN address to identify credentials used on decrypt
                                  */
  meshSeqNumber_t  seqNo;        /*!< Sequence number */
  uint32_t         ivIndex;      /*!< IV index */
  uint16_t         netKeyIndex;  /*!< NetKey index to be used for encrypting the packet */
} meshNwkPduRxInfo_t;

/*! Transport layer sends PDU's to the Network layer this format */
typedef struct meshNwkPduTxInfo_tag
{
  uint8_t          *pLtrHdr;     /*!< Pointer to the Lower Transport PDU Header */
  uint8_t          *pUtrPdu;     /*!< Pointer to Upper Transport PDU or segment of it
                                  *   in case the transaction was segmented
                                  */
  uint8_t          ltrHdrLen;    /*!< Lower Transport PDU Header length */
  uint8_t          utrPduLen;    /*!< Length of the Upper Transport PDU or the segment
                                  *   in case the transaction was segmented
                                  */
  uint8_t          ctl;          /*!< Control or Access PDU: 1 for Control PDU, 0 for Access PDU */
  uint8_t          ttl;          /*!< TTL to be used. Shall be a valid value. */
  meshAddress_t    src;          /*!< SRC address */
  meshAddress_t    dst;          /*!< DST address */
  meshAddress_t    friendLpnAddr;/*!< Friend or LPN address to identify credentials used on encrypt
                                  */
  meshSeqNumber_t  seqNo;        /*!< Sequence number */
  uint16_t         netKeyIndex;  /*!< NetKey index to be used for encrypting the packet */
  bool_t           prioritySend; /*!< TRUE if PDU must be sent with priority */
  bool_t           ifPassthr;    /*!< Friendship pass-through flag for Network interface */
} meshNwkPduTxInfo_t;

/*! Mesh Network layer return value. See ::meshReturnValues
 *  for codes starting at ::MESH_NWK_RETVAL_BASE
 */
typedef uint16_t meshNwkRetVal_t;

/*! Mesh Network notification event types enumeration */
enum meshNwkEventTypes
{
  MESH_NWK_SEND_SUCCESS        = 0x00,  /*!< Network PDU transmission completed successfully */
  MESH_NWK_SEND_FAILED         = 0x01,  /*!< Network PDU transmission failed due to failure of
                                         *   encryption or due to bearer error
                                         */
  MESH_NWK_SEND_INVALID_PARAM  = 0x02,  /*!< Network PDU transmission/reception failed due to
                                         *   invalid parameters
                                         */
};

/*! Mesh Network notification event type. See ::meshNwkEventTypes */
typedef uint8_t meshNwkEvent_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network layer callback that verifies if an incoming PDU is destined for an LPN.
 *
 *  \param[in] dst          Destination address of the received PDU.
 *  \param[in] netKeyIndex  Global NetKey identifier.
 *
 *  \return    TRUE if at least one LPN needs the PDU, FALSE otherwise.
 */
/*************************************************************************************************/
typedef bool_t (*meshNwkFriendRxPduCheckCback_t)(meshAddress_t dst, uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN PDU received callback function pointer type.
 *
 *  \param[in] pNwkPduRxInfo  Network PDU RX info.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshNwkLpnRxPduNotifyCback_t)(meshNwkPduRxInfo_t *pNwkPduRxInfo);

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN PDU received filter callback function pointer type.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    TRUE if the PDU needs to be filtered, FALSE otherwise.
 */
/*************************************************************************************************/
typedef bool_t (*meshNwkLpnRxPduFilterCback_t)(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network layer PDU received callback function pointer type.
 *
 *  \param[in] pNwkPduRxInfo  Pointer to the structure holding the received transport PDU and other
 *                            fields. See ::meshNwkPduRxInfo_t
 *
 *  \return    None.
 *
 *  \remarks   The callback is used to send PDU's to the transport layer
 */
/*************************************************************************************************/
typedef void (*meshNwkRecvCback_t)(meshNwkPduRxInfo_t *pNwkPduRxInfo);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network layer event notification callback function pointer type.
 *
 *  \param[in] event        Reason the callback is being invoked. See ::meshNwkEvent_t
 *  \param[in] pEventParam  Pointer to the event parameter passed to the function.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshNwkEventNotifyCback_t)(meshNwkEvent_t event, void *pEventParam);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshNwkGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the network layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshNwkInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callbacks.
 *
 *  \param[in] recvCback   Callback to be invoked when a Network PDU is received.
 *  \param[in] eventCback  Event notification callback for the upper layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkRegister(meshNwkRecvCback_t recvCback, meshNwkEventNotifyCback_t eventCback);

/*************************************************************************************************/
/*!
 *  \brief     Sends the given transport PDU to the network layer.
 *
 *  \param[in] pNwkPduTxInfo  Pointer to a buffer containing a Lower Transport PDU and additional
 *                            fields. See ::meshNwkPduTxInfo_t
 *
 *  \return    Success or error code. See ::meshReturnValues
 */
/*************************************************************************************************/
meshNwkRetVal_t MeshNwkSendLtrPdu(const meshNwkPduTxInfo_t *pNwkPduTxInfo);

/*************************************************************************************************/
/*!
 *  \brief     Packs a Network PDU header using the parameters provided in the request
 *
 *  \param[in] pNwkPduTxInfo  Pointer to a structure holding network PDU. See ::meshNwkPduTxInfo_t
 *  \param[in] pHdr           Pointer to a buffer where the packed Network PDU Header is stored
 *  \param[in] ivi            Least significant bit of the IV Index
 *  \param[in] nid            Network Identifier derived from the Network Key
 *
 *  \remarks   The NID is not deduced internally due to insufficient information such as type of
 *             credentials used which differ in case of operations involving friendship.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkPackHeader(const meshNwkPduTxInfo_t *pNwkPduTxInfo, uint8_t *pHdr, uint8_t ivi,
                       uint8_t nid);

/*************************************************************************************************/
/*!
 *  \brief     Registers callback that verifies if an LPN is destination for a PDU.
 *
 *  \param[in] rxPduCheckCback  Rx PDU check callback.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkRegisterFriend(meshNwkFriendRxPduCheckCback_t rxPduCheckCback);

/*************************************************************************************************/
/*!
 *  \brief     Registers LPN callbacks.
 *
 *  \param[in] rxPduNotifyCback  PDU received from Friend notification callback.
 *  \param[in] rxPduFilterCback  Filter received packets on a specific subnet if Friendship is
 *                               established.
 *
 *  \return    None.
 */
/*************************************************************************************************/

void MeshNwkRegisterLpn(meshNwkLpnRxPduNotifyCback_t rxPduNotifyCback,
                        meshNwkLpnRxPduFilterCback_t rxPduFilterCback);

#ifdef __cplusplus
}
#endif

#endif /* MESH_NETWORK_H */
