/*************************************************************************************************/
/*!
 *  \file   mesh_security_crypto.h
 *
 *  \brief  Security common definitions.
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

#ifndef MESH_SECURITY_CRYPTO_H
#define MESH_SECURITY_CRYPTO_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Network Security minimum allowed PDU considering number of bytes needed for obfuscation */
#define MESH_SEC_NWK_PDU_MIN_SIZE            (MESH_DST_ADDR_POS + MESH_SEC_PRIV_RAND_SIZE)

/*! Secure Network Beacon number of bytes used as input for CMAC. (FLAGS + NWKID + IV) */
#define MESH_SEC_BEACON_AUTH_INPUT_NUM_BYTES (1 + MESH_NWK_ID_NUM_BYTES + sizeof(uint32_t))

/*! Number of request sources for a network encrypt procedure. Network layer, Friendship module and
 *  Proxy module should each be capable of independent requests.
 */
#define MESH_SEC_NWK_ENC_NUM_SOURCES          3

/*! Number of request sources for a network decrypt procedure. Network layer, Friendship module
 *  should share a request since there is no way to know before decryption the actual destination
 *  and Proxy should be capable of independent requests.
 *
 */
#define MESH_SEC_NWK_DEC_NUM_SOURCES          2

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Enumeration of network encrypt procedure request sources */
enum meshSecNwkEncReqSources
{
  MESH_SEC_NWK_ENC_SRC_NWK    = 0, /*!< Request source is Network Layer. */
  MESH_SEC_NWK_ENC_SRC_FRIEND = 1, /*!< Request source is Friendship module. */
  MESH_SEC_NWK_ENC_SRC_PROXY  = 2, /*!< Request source is Proxy module. */
};

/*! Enumeration of Network decrypt procedure requeest sources */
enum meshSecNwkDecReqSources
{
  MESH_SEC_NWK_DEC_SRC_NWK_FRIEND = 0, /*!< Request source is either Network Layer or Friendship
                                        *   module
                                        */
  MESH_SEC_NWK_DEC_SRC_PROXY      = 1, /*!< Request source is Proxy module. */
};
/*! Upper Transport Encrypt request parameters */
typedef struct meshSecUtrEncReq_tag
{
  meshSecUtrEncryptCback_t cback;                               /*!< Callback to invoke when
                                                                 *   encryption is complete or NULL
                                                                 *   if no request is in progress
                                                                 */
  void                     *pParam;                             /*!< Generic callback parameter
                                                                 *   provided in the request
                                                                 */
  uint8_t                  *pEncUtrPdu;                         /*!< Pointer to destination buffer
                                                                 */
  uint8_t                  *pTransMic;                          /*!< Pointer to TRANSMIC buffer */
  uint16_t                 encUtrPduSize;                       /*!< Size of the destination buffer
                                                                 */
  uint8_t                  transMicSize;                        /*!< Size of the TRANSMIC buffer */
  uint8_t                  nonce[MESH_SEC_TOOL_CCM_NONCE_SIZE]; /*!< Nonce generated from the
                                                                 *   input parameters
                                                                 */
  uint8_t                  key[MESH_KEY_SIZE_128];              /*!< Application key or Device key
                                                                 *   needed since Application Keys
                                                                 *   are susceptible to updates
                                                                 */
  uint8_t                  aid;                                 /*!< AID of Application Key
                                                                 *   or ::MESH_SEC_DEVICE_KEY_AID
                                                                 */
} meshSecUtrEncReq_t;

/*! Upper transport Decrypt request parameters */
typedef struct meshSecUtrDecReq_tag
{
  meshSecUtrDecryptCback_t  cback;                               /*!< Callback to invoke when
                                                                  *   decryption complete or NULL
                                                                  *   if no request is in progress
                                                                  */
  void                      *pParam;                             /*!< Generic callback parameter
                                                                  *   provided in the request
                                                                  */
  meshSecToolCcmParams_t    ccmParams;                           /*!< CCM parameters that must be
                                                                  *   stored across multiple
                                                                  *   decryption attempts
                                                                  */
  uint8_t                   nonce[MESH_SEC_TOOL_CCM_NONCE_SIZE]; /*!< Nonce generated from the
                                                                  *   input parameters
                                                                  */
  uint8_t                   key[MESH_KEY_SIZE_128];              /*!< Application key or Device key
                                                                  *   needed since Application Keys
                                                                  *   are susceptible to updates
                                                                  */
  uint8_t                   aid;                                 /*!< AID of Application Key
                                                                  *   or ::MESH_SEC_DEVICE_KEY_AID
                                                                  */
  meshAddress_t             vtad;                                /*!< Virtual Address to be
                                                                  *   searched
                                                                  */
  uint16_t                  vtadSearchIdx;                       /*!< Index of the current virtual
                                                                  *   address Label UUID used for
                                                                  *   authentication
                                                                  */
  uint16_t                  netKeyIndex;                         /*!< Global identifier of the
                                                                  *   Network Key to which the
                                                                  *   Application Key should be
                                                                  *   bound
                                                                  */
  uint16_t                  appKeyIndex;                         /*!< Global identifier of the
                                                                  *   Application Key that
                                                                  *   authenticates the PDU
                                                                  */
  uint16_t                  keySearchIdx;                        /*!< Index of the current key
                                                                  *   material entry matching AID
                                                                  */
} meshSecUtrDecReq_t;

/*! Network Encrypt request parameters */
typedef struct meshSecNwkEncObfReq_tag
{
  meshSecNwkEncObfCback_t  cback;                               /*!< Callback to invoke when
                                                                 *   encryption and obfuscation
                                                                 *   are complete or NULL
                                                                 *   if no request is in progress
                                                                 */
  void                     *pParam;                             /*!< Generic callback parameter
                                                                 *   provided in the request
                                                                 */
  uint8_t                  *pEncObfNwkPdu;                      /*!< Pointer to destination buffer
                                                                 */
  uint8_t                  *pNetMic;                            /*!< Pointer to NETMIC buffer */
  uint8_t                  encObfNwkPduSize;                    /*!< Size of the destination buffer
                                                                 */
  uint8_t                  netMicSize;                          /*!< Size of the NETMIC buffer */
  uint8_t                  nonce[MESH_SEC_TOOL_CCM_NONCE_SIZE]; /*!< Nonce generated from the
                                                                 *   input parameters
                                                                 */
  uint8_t                  eK[MESH_KEY_SIZE_128];               /*!< Encrypt key stored to ensure
                                                                 *   atomicity of the operation
                                                                 *   since keys are susceptible to
                                                                 *   change by OTA requests
                                                                 */
  uint8_t                  pK[MESH_KEY_SIZE_128];               /*!< Privacy key stored to ensure
                                                                 *   atomicity of the operation
                                                                 *   since keys are susceptible to
                                                                 *   change by OTA requests
                                                                 */
  uint8_t                  obfIn[MESH_KEY_SIZE_128];            /*!< Input for AES encrypt function
                                                                 *   used in obfuscation
                                                                 */
} meshSecNwkEncObfReq_t;

/*! Network Decrypt request parameters */
typedef struct meshSecNwkDeobfDecReq_tag
{
  meshSecNwkDeobfDecCback_t cback;                               /*!< Callback to invoke when
                                                                  *   encryption and obfuscation
                                                                  *   are complete or NULL
                                                                  *   if no request is in progress
                                                                  */
  void                      *pParam;                             /*!< Generic callback parameter
                                                                  *   provided in the request
                                                                  */
  uint8_t                   *pEncObfNwkPdu;                      /*!< Pointer to input buffer */
  uint8_t                   *pNwkPdu;                            /*!< Pointer to destination buffer
                                                                  */
  uint8_t                   encObfNwkPduSize;                    /*!< Size of the destination buffer
                                                                  */
  uint8_t                   nonce[MESH_SEC_TOOL_CCM_NONCE_SIZE]; /*!< Nonce generated from the
                                                                  *   input parameters
                                                                  */
  uint8_t                   eK[MESH_KEY_SIZE_128];               /*!< Decrypt key stored to ensure
                                                                  *   atomicity of the operation
                                                                  *   since keys are susceptible to
                                                                  *   change by OTA requests
                                                                  */
  uint8_t                   pK[MESH_KEY_SIZE_128];               /*!< Privacy key stored to ensure
                                                                  *   atomicity of the operation
                                                                  *   since keys are susceptible to
                                                                  *   change by OTA requests
                                                                  */
  uint8_t                   obfIn[MESH_KEY_SIZE_128];            /*!< Input for AES encrypt function
                                                                  *   used in deobfuscation
                                                                  */
  uint32_t                  ivIndex;                             /*!< IV index used by the
                                                                  *   decryption. Although it is
                                                                  *   recoverable from obfIn,
                                                                  *   having it saves time
                                                                  */
  meshSeqNumber_t           seqNo;                               /*!< Stored sequence number after
                                                                  *   deobfuscation
                                                                  */
  meshAddress_t             srcAddr;                             /*!< Stored source address after
                                                                  *   deobfuscation
                                                                  */
  uint8_t                   ctlTtl;                              /*!< Stored CTL-TTL byte after
                                                                  *   deobfuscation
                                                                  */
  uint16_t                  netKeyIndex;                         /*!< Global index of the key that
                                                                  *   is used for decryption
                                                                  */
  uint16_t                  keySearchIndex;                      /*!< Index of the current key
                                                                  *   material entry matching NID
                                                                  */
  bool_t                    searchInFriendshipMat;               /*!< TRUE if NID search happens
                                                                  *   in friendship material
                                                                  */
} meshSecNwkDeobfDecReq_t;

/*! Mesh Security Beacon compute authentication value request. */
typedef struct meshSecNwkBeaconComputeAuthReq_tag
{
  meshSecBeaconComputeAuthCback_t cback;                 /*!< User callback to be invoked after
                                                          *   computation
                                                          */
  void                            *pParam;               /*!< Generic parameter provided in the
                                                          *   request
                                                          */
  uint8_t                         *pSecBeacon;           /*!< Pointer to Secure Network Beacon
                                                          */
  uint16_t                        netKeyIndex;           /*!< NetKey Index associated to Network ID
                                                          */
  uint8_t                         bk[MESH_KEY_SIZE_128]; /*!< Beacon Key for the request since
                                                          *   Network Keys are susceptible to
                                                          *   removal
                                                          */
} meshSecNwkBeaconComputeAuthReq_t;

/*! Mesh Security Beacon authentication request. */
typedef struct meshSecNwkBeaconAuthReq_tag
{
  meshSecBeaconAuthCback_t cback;                 /*!< User callback to be invoked after
                                                   *   authentication
                                                   */
  void                     *pParam;               /*!< Generic parameter provided in the request */
  uint8_t                  *pSecBeacon;           /*!< Pointer to Secure Network Beacon */
  uint16_t                 netKeyIndex;           /*!< NetKey Index associated to Network ID */
  uint16_t                 keySearchIndex;        /*!< Index used to iterate through Network Key
                                                   *   material for matching Network Identifiers
                                                   */
  uint8_t                  bk[MESH_KEY_SIZE_128]; /*!< Beacon Key for the request since Network Keys
                                                   *   are susceptible to removal
                                                   */
  bool_t                   newKeyUsed;            /*!< TRUE if the new Key is currently tested. */
} meshSecNwkBeaconAuthReq_t;

/*! Request sources for crypto operations. */
typedef struct meshSecCryptoRequests_tag
{
  /*! Upper Transport encrypt request parameters */
  meshSecUtrEncReq_t               utrEncReq;
  /*! Upper Transport decrypt request parameters */
  meshSecUtrDecReq_t               utrDecReq;
  /*! Network encrypt request parameters for NWK, Friend, Proxy */
  meshSecNwkEncObfReq_t            nwkEncObfReq[MESH_SEC_NWK_ENC_NUM_SOURCES];
  /*! Network decrypt request parameters for NWK and Proxy */
  meshSecNwkDeobfDecReq_t          nwkDeobfDecReq[MESH_SEC_NWK_DEC_NUM_SOURCES];
  /*! Secure Network Beacon compute authentication value request */
  meshSecNwkBeaconComputeAuthReq_t beaconCompAuthReq;
  /*! Secure Network Beacon authenticate request */
  meshSecNwkBeaconAuthReq_t        beaconAuthReq;
} meshSecCryptoRequests_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

extern meshSecCryptoRequests_t   secCryptoReq;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Builds nonce based on input parameters.
 *
 *  \param[in]  nonceType        Identifies type of nonce.
 *  \param[in]  ttlCtlAszMicPad  Either TTL-CTL byte, or ASZMIC-PAD byte or just PAD.
 *  \param[in]  src              Address of the element originating the PDU to be encrypted.
 *  \param[in]  dstPad           Destination address byte or PAD bytes.
 *  \param[in]  seqNumber        Unique number identifying the PDU.
 *  \param[in]  ivIndex          IV index.
 *  \param[out] pNonceBuff       Pointer to memory where the nonce is stored.
 *
 *  \return     Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static inline void meshSecBuildNonce(uint8_t nonceType, uint8_t ttlCtlAszMicPad, uint16_t src,
                                     uint16_t dstPad, meshSeqNumber_t seqNumber, uint32_t ivIndex,
                                     uint8_t *pNonceBuff)
{
  /* Set nonce type. */
  *(pNonceBuff++) = nonceType;

  if (nonceType == MESH_SEC_NONCE_PROXY)
  {
    /* CTL-TTL byte is padded with 0x00. */
    *(pNonceBuff++) = 0x00;
  }
  else
  {
    /* Set CTL-TTL byte or ASZMIC_PAD. */
    *(pNonceBuff++) = ttlCtlAszMicPad;
  }

  /* Set sequence number. */
  *(pNonceBuff++) = (uint8_t)(seqNumber >> 16);
  *(pNonceBuff++) = (uint8_t)(seqNumber >> 8);
  *(pNonceBuff++) = (uint8_t)(seqNumber);

  /* Set source address. */
  *(pNonceBuff++) = (uint8_t)(src >> 8);
  *(pNonceBuff++) = (uint8_t)(src);

  /* Set destination address or PAD. */
  *(pNonceBuff++) = (uint8_t)(dstPad >> 8);
  *(pNonceBuff++) = (uint8_t)(dstPad);

  /* Set IV index. */
  *(pNonceBuff++) = (uint8_t)(ivIndex >> 24);
  *(pNonceBuff++) = (uint8_t)(ivIndex >> 16);
  *(pNonceBuff++) = (uint8_t)(ivIndex >> 8);
  *(pNonceBuff++) = (uint8_t)(ivIndex);
}

#ifdef __cplusplus
}
#endif

#endif /* MESH_SECURITY_CRYPTO_H */
