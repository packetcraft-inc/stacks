/*************************************************************************************************/
/*!
 *  \file   mesh_upper_transport.h
 *
 *  \brief  Upper Transport module interface.
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

#ifndef MESH_UPPER_TRANSPORT_H
#define MESH_UPPER_TRANSPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Maximum length of the Upper Transport Access PDU */
#define MESH_UTR_MAX_ACC_PDU_LEN                 380
/*! Maximum length of the Upper Transport Control PDU */
#define MESH_UTR_MAX_CTL_PDU_LEN                 256

/*! 32 bit TransMIC size for Upper Transport Access PDU */
#define MESH_UTR_TRANSMIC_32BIT_SIZE             4

/*! 64 bit TransMIC size for Upper Transport Access PDU */
#define MESH_UTR_TRANSMIC_64BIT_SIZE             8

/*! Check if the control opcode is in valid range */
#define MESH_UTR_CTL_OPCODE_IN_RANGE(opcode)     (((opcode) >= MESH_UTR_CTL_START_VALID_OPCODE) &&\
                                                  ((opcode) <= MESH_UTR_CTL_END_VALID_OPCODE))

/*! Extract transMIC size from szMIC */
#define MESH_SZMIC_TO_TRANSMIC(szmic)            (((szmic) == 0) ? 4 : 8)
/*! Extract szMIC size from transMIC */
#define MESH_TRANSMIC_TO_SZMIC(transmic)         (((transmic) == 4) ? 0 : 1)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Upper Transport notification event types enumeration */
enum meshUtrEventTypes
{
  MESH_UTR_SEND_SUCCESS    = 0x00,  /*!< PDU transmission completed successfully */
  MESH_UTR_SEND_FAILED     = 0x01,  /*!< PDU transmission failed */
  MESH_UTR_ENC_FAILED      = 0x02,  /*!< PDU encryption failed */
};

/*! Mesh Transport Control message opcodes types enumeration */
enum meshUtrCtlMessageOpCodeTypes
{
  MESH_UTR_CTL_RESERVED_OPCODE               = 0x00, /*!< Reserved for LTR layer */
  MESH_UTR_CTL_START_VALID_OPCODE            = 0x01, /*!< UTR CTL Opcode valid range start value */
  MESH_UTR_CTL_FRIEND_POLL_OPCODE            = 0x01, /*!< Sent by a Low Power node to its Friend
                                                      *   node to request any messages that it has
                                                      *   stored for the Low Power node
                                                      */
  MESH_UTR_CTL_FRIEND_UPDATE_OPCODE          = 0x02, /*!< Sent by a Friend node to a Low Power node
                                                      *   to inform it about security updates
                                                      */
  MESH_UTR_CTL_FRIEND_REQUEST_OPCODE         = 0x03, /*!< Sent by a Low Power node the all-friends
                                                      *   fixed group address to start to find a
                                                      *   friend
                                                      */
  MESH_UTR_CTL_FRIEND_OFFER_OPCODE           = 0x04, /*!< Sent by a Friend node to a Low Power node
                                                      *   to offer to become its friend
                                                      */
  MESH_UTR_CTL_FRIEND_CLEAR_OPCODE           = 0x05, /*!< Sent to a Friend node to inform a previous
                                                      *   friend of a Low Power node about the
                                                      *   removal of a friendship
                                                      */
  MESH_UTR_CTL_FRIEND_CLEAR_CNF_OPCODE       = 0x06, /*!< Sent from a previous friend to Friend node
                                                      *   to confirm that a prior friend
                                                      *   relationship has been removed
                                                      */
  MESH_UTR_CTL_FRIEND_SUBSCR_LIST_ADD_OPCODE = 0x07, /*!< Sent to a Friend node to add one or more
                                                      *   addresses to the Friend Subscription List
                                                      */
  MESH_UTR_CTL_FRIEND_SUBSCR_LIST_RM_OPCODE  = 0x08, /*!< Sent to a Friend node to remove one or
                                                      *   more addresses from the Friend
                                                      *   Subscription List
                                                      */
  MESH_UTR_CTL_FRIEND_SUBSCR_LIST_CNF_OPCODE = 0x09, /*!< Sent by a Friend node to confirm Friend
                                                      * Subscription List updates
                                                      */
  MESH_UTR_CTL_HB_OPCODE                     = 0x0A, /*!< Sent by a node to let other nodes
                                                      *   determine topology of a subnet
                                                      */
  MESH_UTR_CTL_END_VALID_OPCODE              = 0x0A  /*!< UTR CTL Opcode valid range end value */
};

/*! Mesh Upper Transport return value. See ::meshReturnValues
 *  for codes starting at ::MESH_UTR_RETVAL_BASE
 */
typedef uint16_t meshUtrRetVal_t;

/*! Mesh Upper Transport notification event type. See ::meshUtrEventTypes */
typedef uint8_t meshUtrEvent_t;

/*! Access layer and Upper Transport layer exchange packets in this format on TX data path */
typedef struct meshUtrAccPduTxInfo_tag
{
  meshAddress_t     src;             /*!< SRC address */
  meshAddress_t     dst;             /*!< DST address */
  meshAddress_t     friendLpnAddr;   /*!< Friend or LPN address to identify credentials used by
                                      *   security
                                      */
  const uint8_t     *pDstLabelUuid;  /*!< Pointer to Label UUID for destination virtual address */
  uint16_t          appKeyIndex;     /*!< AppKey index to be used for encrypting the Access PDU */
  uint16_t          netKeyIndex;     /*!< NetKey index to be used for encrypting the Transport PDU
                                      */
  uint8_t           ttl;             /*!< TTL to be used. If invalid, Default TTL will be used */
  bool_t            ackRequired;     /*!< Acknowledgement is waited for this PDU */
  bool_t            devKeyUse;       /*!< Device Key is used instead of Application Key */
  const uint8_t     *pAccPduOpcode;  /*!< Pointer to Access Layer message header */
  const uint8_t     *pAccPduParam;   /*!< Pointer to the Access Layer message params */
  uint16_t          accPduParamLen;  /*!< Size of the Access PDU message parameters in bytes */
  uint8_t           accPduOpcodeLen; /*!< Size of the Access PDU opcode in bytes */
} meshUtrAccPduTxInfo_t;

/*! Access layer and Upper Transport layer exchange packets in this format on Rx data path */
typedef struct meshUtrAccPduRxInfo_tag
{
  meshAddress_t     src;            /*!< SRC address */
  meshAddress_t     dst;            /*!< DST address */
  const uint8_t     *pDstLabelUuid; /*!< Pointer to Label UUID for destination virtual address */
  uint16_t          appKeyIndex;    /*!< AppKey index to be used for encrypting the Access PDU */
  uint16_t          netKeyIndex;    /*!< NetKey index to be used for encrypting the Transport PDU */
  uint8_t           ttl;            /*!< TTL to be used. If invalid, Default TTL will be used */
  bool_t            devKeyUse;      /*!< Device Key is used instead of Application Key */
  const uint8_t     *pAccPdu;       /*!< Pointer to the Access PDU */
  uint16_t          pduLen;         /*!< Size of the PDU */
} meshUtrAccPduRxInfo_t;

/*! Upper Transport layer exchanges Control packets in this format */
typedef struct meshUtrCtlPduInfo_tag
{
  meshAddress_t     src;          /*!< SRC address */
  meshAddress_t     dst;          /*!< DST address */
  meshAddress_t     friendLpnAddr;/*!< Friend or LPN address to identify credentials used by
                                   *   security
                                   */
  uint16_t          netKeyIndex;  /*!< NetKey index to be used for encrypting the Transport PDU */
  uint8_t           ttl;          /*!< TTL to be used. If invalid, Default TTL will be used */
  uint8_t           opcode;       /*!< Control Message opcode */
  bool_t            ackRequired;  /*!< Acknowledgement is requested for this PDU */
  bool_t            ifPassthr;    /*!< Friendship pass-through flag for Network interface */
  const uint8_t    *pCtlPdu;      /*!< Pointer to the Control PDU */
  uint16_t          pduLen;       /*!< Size of the PDU */
  bool_t            prioritySend; /*!< TRUE if PDU must be sent with priority */
} meshUtrCtlPduInfo_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Upper Transport Access PDU received callback function pointer type.
 *
 *  \param[in] pAccPduInfo  Pointer to the structure holding the received Access PDU and other
 *                          fields. See ::meshUtrAccPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshUtrAccRecvCback_t)(const meshUtrAccPduRxInfo_t *pAccPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Friendship Access PDU received callback function pointer type.
 *
 *  \param[in] pAccPduInfo  Pointer to the structure holding the received Access PDU and other
 *                          fields. See ::meshUtrAccPduRxInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshUtrFriendshipAccRecvCback_t)(const meshUtrAccPduRxInfo_t *pAccPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Friendship Control PDU received callback function pointer type.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshUtrFriendshipCtlRecvCback_t)(const meshLtrCtlPduInfo_t *pCtlPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Upper Transport event notification callback function pointer type.
 *
 *  \param[in] event        Reason the callback is being invoked. See ::meshUtrEvent_t
 *  \param[in] pEventParam  Pointer to the event parameter passed to the function.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshUtrEventNotifyCback_t)(meshUtrEvent_t event, void *pEventParam);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Upper Transport layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshUtrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callbacks used by the Upper Transport Layer.
 *
 *  \param[in] accRecvCback  Callback to be invoked when an Access PDU is received.
 *  \param[in] eventCback    Event notification callback for the upper layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshUtrRegister(meshUtrAccRecvCback_t accRecvCback, meshUtrEventNotifyCback_t eventCback);

/*************************************************************************************************/
/*!
 *  \brief     Registers the Friendship required callback used by the Upper Transport Layer.
 *
 *  \param[in] ctlRecvCback  Callback to be invoked when a Friendship Control PDU is received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshUtrRegisterFriendship(meshUtrFriendshipCtlRecvCback_t ctlRecvCback);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Access PDU to Upper Transport Layer.
 *
 *  \param[in] pAccPduInfo  Pointer to the structure holding the received Access PDU and other
 *                          fields. See ::meshUtrAccPduInfo_t
 *
 *  \return    Success or error reason. \see meshUtrRetVal_t
 */
/*************************************************************************************************/
meshUtrRetVal_t MeshUtrSendAccPdu(const meshUtrAccPduTxInfo_t *pAccPduInfo);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Control PDU to Upper Transport Layer.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshUtrCtlPduInfo_t
 *
 *  \return    Success or error reason. \see meshUtrRetVal_t
 */
/*************************************************************************************************/
meshUtrRetVal_t MeshUtrSendCtlPdu(const meshUtrCtlPduInfo_t *pCtlPduInfo);

#ifdef __cplusplus
}
#endif

#endif /* MESH_UPPER_TRANSPORT_H */
