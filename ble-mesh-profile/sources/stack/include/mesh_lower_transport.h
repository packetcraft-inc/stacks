/*************************************************************************************************/
/*!
 *  \file   mesh_lower_transport.h
 *
 *  \brief  Lower Transport module interface.
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

#ifndef MESH_LOWER_TRANSPORT_H
#define MESH_LOWER_TRANSPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Maximum length of the Upper Transport Access PDU with TransMIC is 384 Bytes */
#define MESH_LTR_MAX_ACC_PDU_LEN                      384
/*! Minimum length of the Upper Transport Access PDU with TransMIC is 5 Bytes */
#define MESH_LTR_MIN_ACC_PDU_LEN                      5
/*! Maximum length of the Upper Transport Control PDU is 256 Bytes */
#define MESH_LTR_MAX_CTL_PDU_LEN                      256

/*! Maximum length of an Upper Transport PDU */
#define MESH_LTR_MAX_UTR_PDU_LEN                      384
/*! Maximum length of the Unsegmented Upper Transport Access PDU is 15 Bytes */
#define MESH_LTR_MAX_UNSEG_UTR_ACC_PDU_LEN            15
/*! Maximum length of the Segmented Upper Transport Access PDU is 12 Bytes */
#define MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN              12
/*! Maximum length of the Unsegmented Upper Transport Control PDU is 11 Bytes */
#define MESH_LTR_MAX_UNSEG_UTR_CTL_PDU_LEN            11
/*! Maximum length of the Segmented Upper Transport Control PDU is 8 Bytes */
#define MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN              8
/*! Lower Transport unsegmented message header length is 1 byte */
#define MESH_LTR_UNSEG_HDR_LEN                        1
/*! Defines the size of the segmentation header */
#define MESH_LTR_SEG_HDR_LEN                          4

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Lower Transport notification event types enumeration */
enum meshLtrEventTypes
{
  MESH_LTR_SEND_SUCCESS          = 0x00,  /*!< PDU transmission completed successfully */
  MESH_LTR_SEND_FAILED           = 0x01,  /*!< PDU transmission failed */
  MESH_LTR_SEND_SAR_TX_TIMEOUT   = 0x02,  /*!< PDU transmission failed with timeout in SAR-TX */
  MESH_LTR_SEND_SAR_TX_REJECTED  = 0x03   /*!< PDU transmission rejected in SAR-TX */
};

/*! Mesh Lower Transport return value. See ::meshReturnValues
 *  for codes starting at ::MESH_LTR_RETVAL_BASE
 */
typedef uint16_t meshLtrRetVal_t;

/*! Mesh Lower Transport notification event type. See ::meshLtrEventTypes */
typedef uint8_t meshLtrEvent_t;

/*! Upper Transport layer and Lower Transport layer exchange Access packets in this format */
typedef struct meshLtrAccPduInfo_tag
{
  void             *pNext;       /*!< Pointer to next Lower Transport Access PDU */
  meshAddress_t    src;          /*!< SRC address */
  meshAddress_t    dst;          /*!< DST address */
  meshAddress_t    friendLpnAddr;/*!< Friend or LPN address to identify credentials used by security
                                  */
  uint16_t         netKeyIndex;  /*!< NetKey index to be used for encrypting the Transport PDU */
  uint8_t          ttl;          /*!< TTL to be used. If invalid, Default TTL will be used */
  uint8_t          akf;          /*!< Application Key Flag */
  uint8_t          aid;          /*!< Application Key Identifier */
  uint8_t          szMic;        /*!< Size of the TransMIC in the Upper Transport Access PDU.
                                  *   1: TransMIC = 64-bit, 0: TransMIC = 32-bit
                                  */
  meshSeqNumber_t  seqNo;        /*!< Sequence number */
  meshSeqNumber_t  gtSeqNo;      /*!< Greatest sequence number in a segmented reception */
  uint32_t         ivIndex;      /*!< IV Index */
  bool_t           ackRequired;  /*!< Acknowledgement is waited for this PDU */
  uint8_t          *pUtrAccPdu;  /*!< Pointer to the Upper Transport Access PDU */
  uint16_t         pduLen;       /*!< Size of the PDU */
} meshLtrAccPduInfo_t;

/*! Upper Transport layer and Lower Transport layer exchange Control packets in this format */
typedef struct meshLtrCtlPduInfo_tag
{
  void             *pNext;       /*!< Pointer to next Lower Transport Control PDU */
  meshAddress_t    src;          /*!< SRC address */
  meshAddress_t    dst;          /*!< DST address */
  meshAddress_t    friendLpnAddr;/*!< Friend or LPN address to identify credentials used by security
                                  */
  uint16_t         netKeyIndex;  /*!< NetKey index to be used for encrypting the Transport PDU */
  uint8_t          ttl;          /*!< TTL to be used. If invalid, Default TTL will be used */
  meshSeqNumber_t  seqNo;        /*!< Sequence number */
  meshSeqNumber_t  gtSeqNo;      /*!< Greatest sequence number in a segmented reception */
  uint8_t          opcode;       /*!< Control Message opcode */
  bool_t           ackRequired;  /*!< Acknowledgement is waited for this PDU */
  bool_t           ifPassthr;    /*!< Friendship pass-through flag for Network interface */
  uint8_t          *pUtrCtlPdu;  /*!< Pointer to the Upper Transport Control PDU */
  uint16_t         pduLen;       /*!< Size of the PDU */
  bool_t           prioritySend; /*!< TRUE if PDU must be sent with priority */
} meshLtrCtlPduInfo_t;

/*! Enumeration of the PDU types accepted by the Friend Queue */
enum meshFriendQueuePduTypes
{
  MESH_FRIEND_QUEUE_NWK_PDU,     /*!< Network PDU Rx format */
  MESH_FRIEND_QUEUE_LTR_CTL_PDU, /*!< Lower Transport PDU Control format for Tx */
  MESH_FRIEND_QUEUE_LTR_ACC_PDU, /*!< Lower Transport PDU Access format for Tx */
};

/*!< Friend Queue Add API type of the generic PDU. See ::meshFriendQueuePduTypes */
typedef uint8_t meshFriendQueuePduType_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Lower Transport Access PDU received callback function pointer type.
 *
 *  \param[in] pLtrAccPduInfo  Pointer to the structure holding the received Upper Transport Access
 *                             PDU and other fields. See ::meshLtrAccPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshLtrAccRecvCback_t)(meshLtrAccPduInfo_t *pLtrAccPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Lower Transport Control PDU received callback function pointer type.
 *
 *  \param[in] pLtrCtlPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Control PDU and other fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshLtrCtlRecvCback_t)(meshLtrCtlPduInfo_t *pLtrCtlPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Friend Queue Network PDU add callback function pointer type.
 *
 *  \param[in] pPduInfo  Pointer to PDU and information.
 *  \param[in] pduType   Type of the generic pPduInfo parameter. See ::meshFriendQueuePduTypes
 *
 *  \return    TRUE if the PDU is accepted in a Friend Queue, FALSE otherwise.
 */
/*************************************************************************************************/
typedef bool_t (*meshLtrFriendQueueAddCback_t)(const void *pPduInfo,
                                               meshFriendQueuePduType_t pduType);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Lower Transport event notification callback function pointer type.
 *
 *  \param[in] event  Reason the callback is being invoked. See ::meshLtrEvent_t
 *  \param[in] seqNo  Sequence number used to identify the Tx transaction.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshLtrEventNotifyCback_t)(meshLtrEvent_t event, meshSeqNumber_t seqNo);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Lower Transport layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshLtrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callbacks used by the Lower Transport.
 *
 *  \param[in] accRecvCback  Callback to be invoked when an Upper Transport Access PDU is received.
 *  \param[in] ctlRecvCback  Callback to be invoked when an Upper Transport Control PDU is received.
 *  \param[in] eventCback    Event notification callback for the Upper Transport layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLtrRegister(meshLtrAccRecvCback_t accRecvCback, meshLtrCtlRecvCback_t ctlRecvCback,
                     meshLtrEventNotifyCback_t eventCback);

/*************************************************************************************************/
/*!
 *  \brief     Registers the Friend Queue add callback used by the Lower Transport.
 *
 *  \param[in] friendQueueAddCback  Callback to be invoked when sending or receiving a Lower
 *                                  Transport PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLtrRegisterFriend(meshLtrFriendQueueAddCback_t friendQueueAddCback);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Upper Transport Access PDU to Lower Transport Layer.
 *
 *  \param[in] pLtrAccPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Access PDU and other fields. See ::meshLtrAccPduInfo_t
 *
 *  \return    Success or error reason. \see meshLtrRetVal_t
 */
/*************************************************************************************************/
meshLtrRetVal_t MeshLtrSendUtrAccPdu(meshLtrAccPduInfo_t *pLtrAccPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Upper Transport Control PDU to Lower Transport Layer.
 *
 *  \param[in] pLtrCtlPduInfo  Pointer to the structure holding the received Upper Transport
 *                             Control PDU and other fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    Success or error reason. \see meshLtrRetVal_t
 */
/*************************************************************************************************/
meshLtrRetVal_t MeshLtrSendUtrCtlPdu(meshLtrCtlPduInfo_t *pLtrCtlPduInfo);

#ifdef __cplusplus
}
#endif

#endif /* MESH_LOWER_TRANSPORT_H */
