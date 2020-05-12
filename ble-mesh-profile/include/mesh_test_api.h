/*************************************************************************************************/
/*!
 *  \file   mesh_test_api.h
 *
 *  \brief  Mesh Stack Test API.
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
 * @addtogroup MESH_TEST_API Mesh Stack Test API
 * @{
 *************************************************************************************************/

#ifndef MESH_TEST_API_H
#define MESH_TEST_API_H

/* Require compile time directive. */
#if defined(MESH_ENABLE_TEST)
#if (MESH_ENABLE_TEST != 1)
#error "MESH_ENABLE_TEST compilation directive must be set to 1."
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Listen Mode values */
enum meshTestListenMaskValues
{
  MESH_TEST_LISTEN_OFF          = 0x0000,    /*!< Listen mode disabled */
  MESH_TEST_PRVBR_LISTEN        = (1 << 0),  /*!< Dump Prv Bearer messages in terminal */
  MESH_TEST_NWK_LISTEN          = (1 << 1),  /*!< Dump NWK messages in terminal */
  MESH_TEST_SAR_LISTEN          = (1 << 2),  /*!< Dump SAR messages in terminal */
  MESH_TEST_UTR_LISTEN          = (1 << 3),  /*!< Dump UTR messages in terminal */
  MESH_TEST_PROXY_LISTEN        = (1 << 4),  /*!< Dump Proxy Config messages in terminal */
  MESH_TEST_LISTEN_ALL_MASK     = 0x000F     /*!< Dump all messages */
};

/*! \brief Link control callback interface events */
enum meshTestEvtValues
{
  MESH_TEST_PB_LINK_CLOSED_IND,            /*!< Provisioning Bearer Link Closed. */
  MESH_TEST_PB_INVALID_OPCODE_IND,         /*!< Provisioning Bearer Invalid opcode received. */
  MESH_TEST_NWK_PDU_RCVD_IND,              /*!< Network PDU received. */
  MESH_TEST_SAR_RX_TIMEOUT_IND,            /*!< SAR RX timeout. */
  MESH_TEST_UTR_ACC_PDU_RCVD_IND,          /*!< UTR Access PDU received. */
  MESH_TEST_UTR_CTL_PDU_RCVD_IND,          /*!< UTR Control PDU received. */
  MESH_TEST_PROXY_PDU_RCVD_IND,            /*!< Proxy PDU received. */
  MESH_TEST_SEC_NWK_BEACON_RCVD_IND,       /*!< Secure Network Beacon received. */
  MESH_TEST_MPRVS_WRITE_INVALID_RCVD_IND,  /*!< MPRVS write invalid data received. */
};

/*! \brief Provisioning Bearer Link Closed indication event structure for test API */
typedef struct meshTestPbLinkClosedInd_tag
{
  wsfMsgHdr_t  hdr;            /*!< Event header */
} meshTestPbLinkClosedInd_t;

/*! \brief Provisioning Bearer invalid opcode indication event structure for test API */
typedef struct meshTestPbInvalidOpcodeInd_tag
{
  wsfMsgHdr_t  hdr;            /*!< Event header */
  uint8_t      opcode;         /*!< Opcode value */
} meshTestPbInvalidOpcodeInd_t;

/*! \brief Network PDU received indication event structure for test API */
typedef struct meshTestNwkPduRcvdInd_tag
{
  wsfMsgHdr_t      hdr;          /*!< Event header. */
  uint8_t          *pLtrPdu;     /*!< Pointer to the Lower Transport PDU */
  uint8_t          pduLen;       /*!< Length of the Lower Transport PDU */
  uint8_t          nid;          /*!< Sub-net identifier */
  uint8_t          ctl;          /*!< Control or Access PDU: 1 for Control PDU, 0 for Access PDU */
  uint8_t          ttl;          /*!< TTL to be used. Shall be a valid value. */
  meshAddress_t    src;          /*!< SRC address */
  meshAddress_t    dst;          /*!< DST address */
  meshSeqNumber_t  seqNo;        /*!< Sequence number */
  uint32_t         ivIndex;      /*!< IV index */
  uint16_t         netKeyIndex;  /*!< NetKey index to be used for encrypting the packet */
} meshTestNwkPduRcvdInd_t;

/*! \brief UTR Access PDU received indication event structure for test API */
typedef struct meshTestUtrAccPduRcvdInd_tag
{
  wsfMsgHdr_t    hdr;            /*!< Event header. */
  meshAddress_t  src;            /*!< SRC address */
  meshAddress_t  dst;            /*!< DST address */
  const uint8_t  *pDstLabelUuid; /*!< Pointer to Label UUID for destination virtual address */
  uint16_t       appKeyIndex;    /*!< AppKey index to be used for encrypting the Access PDU */
  uint16_t       netKeyIndex;    /*!< NetKey index to be used for encrypting the Transport PDU */
  uint8_t        ttl;            /*!< TTL to be used. If invalid, Default TTL will be used */
  bool_t         devKeyUse;      /*!< Device Key is used instead of Application Key */
  const uint8_t  *pAccPdu;       /*!< Pointer to the Access PDU */
  uint16_t       pduLen;         /*!< Size of the PDU */
} meshTestUtrAccPduRcvdInd_t;

/*! \brief UTR Control PDU received indication event structure for test API */
typedef struct meshTestUtrCtlPduRcvdInd_tag
{
  wsfMsgHdr_t      hdr;          /*!< Event header. */
  meshAddress_t    src;          /*!< SRC address */
  meshAddress_t    dst;          /*!< DST address */
  uint16_t         netKeyIndex;  /*!< NetKey index to be used for encrypting the Transport PDU */
  uint8_t          ttl;          /*!< TTL to be used. If invalid, Default TTL will be used */
  meshSeqNumber_t  seqNo;        /*!< Sequence number */
  uint8_t          opcode;       /*!< Control Message opcode */
  uint8_t          *pUtrCtlPdu;  /*!< Pointer to the Upper Transport Control PDU */
  uint16_t         pduLen;       /*!< Size of the PDU */
} meshTestUtrCtlPduRcvdInd_t;

/*! \brief SAR RX timeout indication event structure for test API */
typedef struct meshTestSarRxTimeoutInd_tag
{
  wsfMsgHdr_t    hdr;            /*!< Event header */
  meshAddress_t  srcAddr;        /*!< Source address */
} meshTestSarRxTimeoutInd_t;

/*! \brief Proxy Config PDU received indication event structure for test API */
typedef struct meshTestProxyCfgPduRcvdInd_tag
{
  wsfMsgHdr_t             hdr;        /*!< Event header */
  const uint8_t           *pPdu;      /*!< Pointer to the Proxy Configuration PDU */
  uint16_t                pduLen;     /*!< Size of the PDU */
  meshGattProxyPduType_t  pduType;    /*!< PDU Type */
} meshTestProxyCfgPduRcvdInd_t;

/*! \brief Secure Network Beacon received indication event structure for test API */
typedef struct meshTestSecNwkBeaconRcvdInd_tag
{
  wsfMsgHdr_t  hdr;                               /*!< Event header */
  uint32_t     ivi;                               /*!< IV index */
  bool_t       ivUpdate;                          /*!< IV update flag */
  bool_t       keyRefresh;                        /*!< Key Refresh flag */
  uint8_t      networkId[MESH_NWK_ID_NUM_BYTES];  /*!< Network ID */
} meshTestSecNwkBeaconRcvdInd_t;

/*! \brief MPRVS write invalid data received indication event structure for test API */
typedef struct meshTestMprvsWriteInvalidRcvdInd_tag
{
  wsfMsgHdr_t  hdr;          /*!< Event header. */
  uint16_t     handle;       /*!< Attribute handle. */
  uint8_t      *pValue;      /*!< Pointer to data to write. */
  uint16_t     len;          /*!< Length of data to write. */
} meshTestMprvsWriteInvalidRcvdInd_t;

/*! \brief Union of all Mesh Test event types */
typedef union meshTestEvt_tag
{
  wsfMsgHdr_t                         hdr;                      /*!< Event header */
  meshTestPbLinkClosedInd_t           pbLinkClosedInd;          /*!< PB Link Closed event */
  meshTestPbInvalidOpcodeInd_t        pbInvalidOpcodeInd;       /*!< PB Invalid opcode received */
  meshTestNwkPduRcvdInd_t             nwkPduRcvdInd;            /*!< Network PDU received */
  meshTestSarRxTimeoutInd_t           sarRxTimeoutInd;          /*!< SAR RX timeout indication */
  meshTestProxyCfgPduRcvdInd_t        proxyCfgPduRcvdInd;       /*!< Proxy Configuration PDU received */
  meshTestSecNwkBeaconRcvdInd_t       secNwkBeaconRcvdInd;      /*!< Secure Network Beacon received */
  meshTestMprvsWriteInvalidRcvdInd_t  mprvsWriteInvalidRcvdInd; /*!< MPRVS write invalid data received */
} meshTestEvt_t;

/*! \brief Mesh Stack Test event notification callback */
typedef void(*meshTestCback_t) (meshTestEvt_t *pEvt);

/*! \brief Mesh Test Control Block. */
typedef struct meshTestCb_tag
{
  meshTestCback_t       testCback;      /*!< Mesh Test event notification callback */
  uint16_t              listenMask;     /*!< Enable Test Listen Mode on various layers */
} meshTestCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Stack Test mode control block */
extern meshTestCb_t  meshTestCb;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Stack Test module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshTestInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the Mesh Stack Test events callback.
 *
 *  \param[in] meshTestCback  Callback function for Mesh Stack Test events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestRegister(meshTestCback_t meshTestCback);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Test Listen mask. Only masked events are reported.
 *
 *  \param[in] mask  Listen events mask to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestSetListenMask(uint16_t mask);

/*************************************************************************************************/
/*!
 *  \brief  Clear Replay Protection list.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshTestRpClearList(void);

/*************************************************************************************************/
/*!
 *  \brief     Alters the NetKey list size to a lower value than the one set at compile time.
 *
 *  \param[in] listSize  NetKey list size.
 *
 *  \return    NetKey list size set.
 *
 *  \remarks   If a value larger than the one set at compile time or 0 is passed, the value set at
 *             compile time is restored.
 */
/*************************************************************************************************/
uint16_t MeshTestAlterNetKeyListSize(uint16_t listSize);

/*************************************************************************************************/
/*!
 *  \brief     Alters the NetKey list size in Local Config for Mesh Test.
 *
 *  \param[in] listSize  NetKey list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
extern void MeshTestLocalCfgAlterNetKeyListSize(uint16_t listSize);

/*************************************************************************************************/
/*!
 *  \brief     Alters the NetKey list size in Security for Mesh Test.
 *
 *  \param[in] listSize  NetKey list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
extern void MeshTestSecAlterNetKeyListSize(uint16_t listSize);

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh Control Message.
 *
 *  \param[in] dstAddr      DST address
 *  \param[in] netKeyIndex  NetKey index to be used for encrypting the Transport PDU
 *  \param[in] ttl          TTL to be used. If invalid, Default TTL will be used
 *  \param[in] opcode       Control Message opcode
 *  \param[in] ackRequired  Acknowledgement is requested for this PDU
 *  \param[in] pCtlPdu      Pointer to the Control PDU
 *  \param[in] pduLen       Size of the PDU
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestSendCtlMsg(meshAddress_t dstAddr, uint16_t netKeyIndex, uint8_t opcode, uint8_t ttl,
                        bool_t ackRequired, const uint8_t *pCtlPdu, uint16_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Sends beacons on all available interfaces for one or all NetKeys as a result of a
 *             trigger.
 *
 *  \param[in] netKeyIndex  Index of the NetKey that triggered the beacon sending or
 *                          0xFFFF for all netkeys.
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestSendNwkBeacon(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief      Configures IV Test Mode.
 *
 *  \param[in]  disableTmr     TRUE to enable test mode and disable guard timers,
 *                             FALSE to disable it.
 *  \param[in]  signalTrans    TRUE if a transition is also intended.
 *  \param[in]  transToUpdate  Valid if signalTrans is TRUE. If TRUE transition is to IV Update,
 *                             otherwise transition is to normal operation.
 *  \param[out] pOutIv         Pointer to memory where to store IV index value.
 *  \param[out] pOutIvUpdate   Pointer to memory where to store current IV Update Flag value.
 *
 *  remarks     Output parameters can be NULL.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshTestIvConfigTestMode(bool_t disableTmr, bool_t signalTrans, bool_t transToUpdate,
                              uint32_t *pOutIv, bool_t *pOutIvUpdate);

/*************************************************************************************************/
/*!
*  \brief  Closes the PB-ADV link with failure.
*
*  \return None.
*/
/*************************************************************************************************/
void MeshTestPrvBrTriggerLinkClose(void);

#endif // defined(MESH_ENABLE_TEST)

#ifdef __cplusplus
}
#endif

#endif /* MESH_TEST_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
