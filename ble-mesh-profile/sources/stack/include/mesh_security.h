/*************************************************************************************************/
/*!
*  \file   mesh_security.h
*
*  \brief  Security main module interface.
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
 * @addtogroup MESH_SECURITY
 * @{
 *************************************************************************************************/

#ifndef MESH_SECURITY_H
#define MESH_SECURITY_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh AppKey Index used by the Security module when key type is local Device Key */
#define MESH_APPKEY_INDEX_LOCAL_DEV_KEY            0xFFFF

/*! Mesh AppKey Index used by the Security module when key type is remote Device Key */
#define MESH_APPKEY_INDEX_REMOTE_DEV_KEY           0xFFFE

/*! Mesh security invalid AID provided in the UTR encryption complete callback if Device Key is
 *  used
 */
#define MESH_SEC_DEVICE_KEY_AID                    0xFF

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Security API return type. See ::meshReturnValues
 *  for codes starting at ::MESH_SEC_RETVAL_BASE
 */
typedef uint16_t meshSecRetVal_t;

/*! Enumeration of key types used by the key material derivation API */
enum meshSecKeyTypeValues
{
  MESH_SEC_KEY_TYPE_NWK = 0x00, /*!< Key type is Network Key */
  MESH_SEC_KEY_TYPE_APP = 0x01, /*!< Key type is Application Key */
};

/*! Mesh Key type used by the key material derivation API. See ::meshSecKeyTypeValues */
typedef uint8_t meshSecKeyType_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security key material derivation complete callback.
 *
 *  \param[in] keyType     The type of the key.
 *  \param[in] keyIndex    Global key index for application and network key types.
 *  \param[in] keyUpdated  TRUE if key material was added for the new key of an existing key index.
 *  \param[in] isSuccess   TRUE if operation is successful.
 *  \param[in] pParam      Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 *
 *  \see meshSecKeyType_t
 */
/*************************************************************************************************/
typedef void (*meshSecKeyMaterialDerivCback_t)(meshSecKeyType_t keyType, uint16_t keyIndex,
                                               bool_t isSuccess, bool_t keyUpdated, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security all keys material derivation complete callback.
 *
 *  \param[in] isSuccess  TRUE if operation is successful.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecAllKeyMaterialRestoreCback_t)(bool_t isSuccess);

/*! Mesh Security friendship credentials information */
typedef struct meshSecFriendshipCred_tag
{
  meshAddress_t friendAddres;   /*!< Address of the friend node */
  meshAddress_t lpnAddress;     /*!< Address of the low power node */
  uint16_t      friendCounter;  /*!< The value from the FriendCounter field of
                                 *   the Friend Offer message
                                 */
  uint16_t      lpnCounter;     /*!< The value from the LPNCounter field of the
                                 *   Friend Request message
                                 */
  uint16_t      netKeyIndex;    /*!<  Global network key index */
} meshSecFriendshipCred_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security friendship credentials derivation complete callback.
 *
 *  \param[in] friendAddress  The address of the friend node.
 *  \param[in] lpnAddress     The address of the low power node.
 *  \param[in] netKeyIndex    Global network key index.
 *  \param[in] isSuccess      TRUE if operation is successful.
 *  \param[in] pParam         Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecFriendCredDerivCback_t)(meshAddress_t friendAddress, meshAddress_t lpnAddress,
                                              uint16_t netKeyIndex, bool_t isSuccess, void *pParam);

/*! Mesh Security Upper transport encryption parameters
 *  Description:
 *  - Pointers to application payload, encrypted application payload and
 *    label UUID must reference buffers available until the encryption callback is invoked
 *  - Pointer to 16 byte Label UUID must be not NULL if destination address type is virtual
 *  - transMicSize must be either 4 or 8 and it is the transport MIC size in bytes
 */
typedef struct meshSecUtrEncryptParams_tag
{
  uint8_t       *pAppPayload;    /*!< Pointer to application payload buffer */
  uint8_t       *pEncAppPayload; /*!< Pointer to encrypted application payload buffer */
  uint8_t       *pTransMic;      /*!< Pointer to transport mic buffer */
  uint8_t       *pLabelUuid;     /*!< Pointer to label UUID for virtual destination addresses */
  uint16_t      appPayloadSize;  /*!< Size of the application payload */
  uint16_t      appKeyIndex;     /*!< Global index of the Application Key to be used */
  uint16_t      netKeyIndex;     /*!< Global index of the Network Key bound to the Application Key*/
  uint32_t      seqNo;           /*!< 24 bit sequence number allocated for the PDU */
  meshAddress_t srcAddr;         /*!< Source address of the element originating the message */
  meshAddress_t dstAddr;         /*!< Destination address */
  uint8_t       transMicSize;    /*!< Size of the transport MIC */
} meshSecUtrEncryptParams_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Upper transport encryption complete callback.
 *
 *  \param[in] isEncryptSuccess  TRUE if encryption finished successfully.
 *  \param[in] pEncAppPayload    Pointer to encrypted application payload, provided in the request.
 *  \param[in] appPayloadSize    Size of the encrypted application payload.
 *  \param[in] pTransMic         Pointer to buffer, provided in the request, used to store
 *                               calculated transport MIC over the application payload.
 *  \param[in] transMicSize      Size of the transport MIC (4 or 8 bytes).
 *  \param[in] aid               AID derived by the key used or invalid if Device Key is used.
 *  \param[in] pParam            Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecUtrEncryptCback_t)(bool_t isEncryptSuccess, uint8_t *pEncAppPayload,
                                         uint16_t appPayloadSize, uint8_t *pTransMic,
                                         uint8_t transMicSize, uint8_t aid, void *pParam);

/*! Mesh Security Upper transport decryption parameters
 *  Description:
 *  - Pointers to application payload and encrypted application payload must reference buffers
 *    available until the decryption callback is invoked
 *  - transMicSize must be either 4 or 8 and it is the provided transport MIC size in bytes
 */
typedef struct meshSecUtrDecryptParams_tag
{
  uint8_t       *pEncAppPayload; /*!< Pointer to encrypted application payload buffer */
  uint8_t       *pAppPayload;    /*!< Pointer to decrypted application payload buffer */
  uint8_t       *pTransMic;      /*!< Pointer to transport mic buffer that needs to be verified */
  uint32_t      seqNo;           /*!< 24 bit sequence number allocated for the PDU */
  uint32_t      recvIvIndex;     /*!< IV index used when Network authenticated the PDU */
  meshAddress_t srcAddr;         /*!< Source address of the element originating the message */
  meshAddress_t dstAddr;         /*!< Destination address */
  uint16_t      netKeyIndex;     /*!< Global network key index of the key used to decrypt
                                  *   the Network PDU
                                  */
  uint16_t      appPayloadSize;  /*!< Size of the application payload */
  uint8_t       transMicSize;    /*!< Size of the transport MIC*/
  uint8_t       aid;             /*!< 5 bit Application Key identifier */
} meshSecUtrDecryptParams_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Upper transport decryption complete callback.
 *
 *  \param[in] isDecryptSuccess  TRUE if decryption and authentication finished successfully.
 *  \param[in] pAppPayload       Pointer to decrypted application payload, provided in the request.
 *  \param[in] pLabelUuid        Pointer to label UUID for virtual destination addresses.
 *  \param[in] appPayloadSize    Size of the decrypted application payload.
 *  \param[in] appKeyIndex       Global application index that matched the AID in the request.
 *  \param[in] netKeyIndex       Global network key index associated to the application key index.
 *  \param[in] pParam            Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 *
 *  \note      Pointer to 16 byte Label UUID must not be NULL if destination address type is
 *             virtual.
 */
/*************************************************************************************************/
typedef void(*meshSecUtrDecryptCback_t)(bool_t isDecryptSuccess, uint8_t *pAppPayload,
                                        uint8_t *pLabelUuid, uint16_t appPayloadSize,
                                        uint16_t appKeyIndex, uint16_t netKeyIndex, void *pParam);

/*! Mesh Security Network PDU encryption and obfuscation parameters
 *  Description:
 *  - The friendOrLpnAddress parameter must be set to the unassigned address if master security
 *    credentials should be used. Otherwise it must be set to the target friend or one of the
 *    low power nodes that has friendship established
 *  - pObfEncNwkPduNoMic must reference memory that does not change until the procedure is complete
 *  - pNwkPduNetMic must reference memory that does not change until the procedure is complete
 */
typedef struct meshSecNwkEncObfParams_tag
{
  uint32_t        ivIndex;             /*!< IV Index */
  uint16_t        netKeyIndex;         /*!< Global network key identifier */
  meshAddress_t   friendOrLpnAddress;  /*!< Unassigned address if master credentials
                                        *   are used, or the friend or low power
                                        *   node address
                                        */
  uint8_t         *pNwkPduNoMic;       /*!< Pointer to buffer containing a
                                        *   network PDU excluding NetMIC
                                        */
  uint8_t         *pObfEncNwkPduNoMic; /*!< Pointer to buffer representing
                                        *   an obfuscated and encrypted network
                                        *   PDU excluding NetMIC
                                        */
  uint8_t         *pNwkPduNetMic;      /*!< Pointer to buffer where the calculated
                                        *   NetMic is stored
                                        */
  uint8_t         nwkPduNoMicSize;     /*!< Size of the network PDU excluding
                                        *   Net MIC in bytes
                                        */
  uint8_t         netMicSize;          /*!< Size of the Net MIC in bytes
                                        *   (can be only 4 or 8 bytes)
                                        */
} meshSecNwkEncObfParams_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Network PDU encryption and obfuscation complete callback
 *
 *  \param[in] isSuccess           TRUE if operation completed successfully.
 *  \param[in] isProxyConfig       TRUE if Network PDU is a Proxy Configuration Message.
 *  \param[in] pObfEncNwkPduNoMic  Pointer to buffer where the encrypted and obfuscated
 *                                 network PDU is stored.
 *  \param[in] nwkPduNoMicSize     Size of the network PDU excluding NetMIC.
 *  \param[in] pNwkPduNetMic       Pointer to buffer where the calculated NetMIC is stored.
 *  \param[in] netMicSize          Size of the calculated NetMIC.
 *  \param[in] pParam              Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecNwkEncObfCback_t)(bool_t isSuccess, bool_t isProxyConfig,
                                        uint8_t *pObfEncNwkPduNoMic, uint8_t nwkPduNoMicSize,
                                        uint8_t *pNwkPduNetMic, uint8_t  netMicSize, void *pParam);

/*! Mesh Security Network PDU deobfuscation and decryption parameters
 *  Description:
 */
typedef struct meshSecNwkDeobfDecParams_tag
{
  uint8_t   *pObfEncAuthNwkPdu; /*!< Pointer to buffer representing a received
                                 *   obfuscated, encrypted and authenticated
                                 *   network PDU
                                 */
  uint8_t   *pNwkPduNoMic;      /*!< Pointer to buffer where the decrypted and
                                 *   deobfuscated network PDU excluding NetMic
                                 *   is stored
                                 */
  uint8_t   nwkPduSize;         /*!< Size of the received encrypted,
                                 *   obfuscated and authenticated network pdu
                                 */
} meshSecNwkDeobfDecParams_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Network deobfuscation and decryption complete callback.
 *
 *  \param[in] isSuccess        TRUE if operation completed succesfully.
 *  \param[in] isProxyConfig    TRUE if Network PDU is a Proxy Configuration Message.
 *  \param[in] pNwkPduNoMic     Pointer to buffer where the deobfuscated and decrypted network PDU
 *                              is stored.
 *  \param[in] nwkPduSizeNoMic  Size of the deobfuscated and decrypted network PDU excluding NetMIC.
 *  \param[in] netKeyIndex      Global network key index associated to the key that successfully
 *                              processed the received network PDU.
 *                              using friendship credentials. Unassigned address otherwise.
 *  \param[in] ivIndex          IV index that successfully authenticated the PDU.
 *  \param[in] friendOrLpnAddr  Friend or LPN address.
 *  \param[in] pParam           Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecNwkDeobfDecCback_t)(bool_t isSuccess, bool_t isProxyConfig,
                                          uint8_t *pNwkPduNoMic, uint8_t nwkPduSizeNoMic,
                                          uint16_t netKeyIndex, uint32_t ivIndex,
                                          meshAddress_t friendOrLpnAddr, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Secure Network Beacon authentication calculated  callback.
 *
 *  \param[in] isSuccess      TRUE if operation completed successfully.
 *  \param[in] pSecNwkBeacon  Pointer to buffer where the Secure Network Beacon is stored.
 *  \param[in] netKeyIndex    Global network key index used to compute the authentication value.
 *  \param[in] pParam         Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecBeaconComputeAuthCback_t)(bool_t isSuccess, uint8_t *pSecNwkBeacon,
                                                uint16_t netKeyIndex, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Secure Network Beacon authentication complete callback.
 *
 *  \param[in] isSuccess      TRUE if operation completed successfully.
 *  \param[in] newKeyUsed     TRUE if the new key was used to authenticate.
 *  \param[in] pSecNwkBeacon  Pointer to buffer where the Secure Network Beacon is stored.
 *  \param[in] netKeyIndex    Global network key index associated to the key that successfully
 *                            processed the received Secure Network Beacon.
 *  \param[in] pParam         Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecBeaconAuthCback_t)(bool_t isSuccess, bool_t newKeyUsed, uint8_t *pSecNwkBeacon,
                                         uint16_t netKeyIndex, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief      Reads the Device Key of a remote node.
 *
 *  \param[in]  addr        Address of the node for which the Device Key is read.
 *  \param[out] pOutDevKey  Pointer to buffer where the Device Key is copied if an address matches.
 */
/*************************************************************************************************/
typedef bool_t (*meshSecRemoteDevKeyReadCback_t)(meshAddress_t addr, uint8_t *pOutDevKey);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the global configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of error.
 */
/*************************************************************************************************/
uint32_t MeshSecGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Security module and allocates configuration memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSecInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the reader function for remote Device Keys.
 *
 *  \param[in] devKeyReader  Function (callback) called when Security needs to read a remote node's
 *                           Device Key.
 *
 *  \return    None.
 *
 *  \note      This function should be called only when an instance of Configuration Client is
 *             present on the local node.
 */
/*************************************************************************************************/
void MeshSecRegisterRemoteDevKeyReader(meshSecRemoteDevKeyReadCback_t devKeyReader);

/*************************************************************************************************/
/*!
 *  \brief     Derives and stores Application or Network Key material.
 *
 *  \param[in] keyType                Type of the key for which derivation is requested.
 *  \param[in] keyIndex               Global key index for application or network keys.
 *  \param[in] isUpdate               Indicates if derivation material is needed for a new key
 *                                    of the same global key index. Used during Key Refresh
 *                                    Procedure transitions (add new key).
 *  \param[in] keyMaterialAddedCback  Callback invoked after key material is stored in the module.
 *  \param[in] pParam                 Pointer to generic parameter for the callback.
 *
 *  \retval    MESH_SUCCESS                     Key derivation started.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters.
 *  \retval    MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *  \retval    MESH_SEC_KEY_NOT_FOUND           An update is requested for a key index that is
 *                                              not stored.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  An update is requested for a key index that has no
 *                                              key material derived from the old key.
 *  \retval    MESH_SEC_KEY_MATERIAL_EXISTS     The key derivation material already exists.
 *
 *  \remarks   For key update procedures, this function also updates the friendship security
 *             credentials that already exist for a specific network key.
 *
 *  \see meshSecKeyType_t
 *  \see meshSecKeyMaterialDerivCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecAddKeyMaterial(meshSecKeyType_t keyType, uint16_t keyIndex, bool_t isUpdate,
                                      meshSecKeyMaterialDerivCback_t keyMaterialAddedCback,
                                      void *pParam);

/*************************************************************************************************/
/*!
 *  \brief  Derives and stores key material for all keys stored in Local Config.
 *
 *  \return None.
 *
 *  \see meshSecAllKeyMaterialRestoreCback_t
 */
/*************************************************************************************************/
void MeshSecRestoreAllKeyMaterial(meshSecAllKeyMaterialRestoreCback_t restoreCback);

/*************************************************************************************************/
/*!
 *  \brief     Removes key derivation material based on keyType and key index.
 *
 *  \param[in] keyType        Type of the key for which key material must be removed.
 *  \param[in] keyIndex       Global key index for application or network keys.
 *  \param[in] deleteOldOnly  Indicates if only old key derivation material is removed. Used
 *                            during Key Refresh Procedure transitions (back to normal operation).
 *
 *  \retval    MESH_SUCCESS                     Key material removed as configured by the
 *                                              parameters.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters. See the Remarks section.
 *  \retval    MESH_SEC_KEY_NOT_FOUND           The module has no material for the specified global
 *                                              key index.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  Caller requests to delete the old key material only,
 *                                              but there is only one set of key derivation
 *                                              material.
 *
 *  \remarks This function cleans up all security materials for a specific key.
 *           For network keys, to remove only the friendship security material use
 *           MeshSecRemoveFriendCredentials. If deleteOldOnly is TRUE, this function also updates
 *           the friendship security credentials that already exist for a specific network key.
 *
 *  \see meshSecKeyType_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecRemoveKeyMaterial(meshSecKeyType_t keyType, uint16_t keyIndex,
                                         bool_t deleteOldOnly);

/*************************************************************************************************/
/*!
 *  \brief     Generates and adds friendship credentials to network key derivation material.
 *
 *  \param[in] pFriendshipCred  Pointer to friendship credentials material information.
 *  \param[in] friendCredAdded  Callback used for procedure complete notification.
 *  \param[in] pParam           Pointer to generic parameter for the callback.
 *
 *  \retval    MESH_SUCCESS                     Friendship material derivation started.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters.
 *  \retval    MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no existing entry for the provided network
 *                                              key index.
 *  \retval    MESH_SEC_KEY_MATERIAL_EXISTS  The friendship credentials already exist.
 *
 *  \remarks This function must be called when a friendship is established.
 *
 *  \see meshSecKeyType_t
 *  \see meshSecFriendCredDerivCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecAddFriendCred(const meshSecFriendshipCred_t *pFriendshipCred,
                                     meshSecFriendCredDerivCback_t friendCredAdded,
                                     void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Removes friendship credentials from existing network key derivation material.
 *
 *  \param[in] friendAddr   Friend address.
 *  \param[in] lpnAddr      LPN address.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *
 *  \retval    MESH_SUCCESS                     Key material removed as configured by the
 *                                              parameters.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters. See the Remarks section.
 *  \retval    MESH_SEC_KEY_NOT_FOUND           The module has no material for the specified global
 *                                              key index.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no friendship material identified by the
 *                                              same parameters.
 *
 *  \remarks This function must be called when a friendship is terminated.
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecRemoveFriendCred(meshAddress_t friendAddr, meshAddress_t lpnAddr,
                                        uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Determines if the NID value exists for the existing network keys.
 *
 *  \param[in] nid  Network identifier to be searched.
 *
 *  \return    TRUE if at least one NID matched or FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSecNidExists(uint8_t nid);

/*************************************************************************************************/
/*!
 *  \brief     Encrypts upper transport PDU.
 *
 *  \param[in] pReqParams               Pointer to encryption setup and storage parameters.
 *  \param[in] utrEncryptCompleteCback  Callback used to signal encryption complete.
 *  \param[in] pParam                   Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Encryption starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: Mic size invalid, or
 *                                           Label UUID NULL for virtual destination address).
 *  \retval MESH_SEC_KEY_NOT_FOUND           Application key index not found.
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no AID computed for the application key index.
 *                                           See the remarks section for more information.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \remarks The AID can be present for the old key in a Key Refresh Procedure phase,
 *           but not for the new key and the phase dictates the new key AID should be returned.
 *
 *  \see meshSecUtrEncryptParams_t
 *  \see meshSecUtrEncryptCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecUtrEncrypt(meshSecUtrEncryptParams_t *pReqParams,
                                  meshSecUtrEncryptCback_t utrEncryptCompleteCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Decrypts upper transport PDU.
 *
 *  \param[in] pReqParams               Pointer to decryption setup and storage parameters.
 *  \param[in] utrDecryptCompleteCback  Callback used to signal decryption complete.
 *  \param[in] pParam                   Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS             All validation is performed. Decryption starts.
 *  \retval MESH_SEC_INVALID_PARAMS  Invalid parameters (E.g.: Mic size invalid, or
 *                                   Label UUID NULL for virtual destination address).
 *  \retval MESH_SEC_KEY_NOT_FOUND   Application key index not found to match AID.
 *  \retval MESH_SEC_OUT_OF_MEMORY   There are no resources to process the request.
 *
 *  \see meshSecUtrDecryptParams_t
 *  \see meshSecUtrDecryptCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecUtrDecrypt(meshSecUtrDecryptParams_t *pReqParams,
                                  meshSecUtrDecryptCback_t utrDecryptCompleteCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Encrypts and obfuscates the network PDU.
 *
 *  \param[in] isProxyConfig        TRUE if the PDU is a Proxy Configuration Message.
 *  \param[in] pReqParams           Pointer to encryption and obfuscation setup and storage
 *                                  parameters.
 *  \param[in] encObfCompleteCback  Callback used to signal encryption and obfuscation complete.
 *  \param[in] pParam               Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Encryption starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: Invalid length of the
 *                                           network PDU).
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no key material associated to the Network Key
 *                                           Index. See the remarks section for more information.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \remarks The NID, Encryption, Privacy keys can be present for the old key during
 *           a Key Refresh Procedure phase, but not for the new key and the phase dictates
 *           the new key material should be returned.
 *
 *  \note    The successful operation of Network encrypt also sets IVI-NID byte in the first byte
 *           of the encrypted and obfuscated Network PDU.
 *
 *  \see meshSecNwkEncObfParams_t
 *  \see meshSecNwkEncObfCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecNwkEncObf(bool_t isProxyConfig, meshSecNwkEncObfParams_t *pReqParams,
                                 meshSecNwkEncObfCback_t encObfCompleteCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Deobfuscates and decrypts a received network PDU.
 *
 *  \param[in] isProxyConfig     TRUE if PDU type is a Proxy Configuration Message.
 *  \param[in] pReqParams        Pointer to deobfuscation and decryption setup and storage
 *                               parameters.
 *  \param[in] nwkDeobfDecCback  Callback used to signal operation complete.
 *  \param[in] pParam            Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Deobfuscation starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: Invalid length of the
 *                                           received PDU).
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  No NetKey index matching NID.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \see meshSecNwkDeobfDecParams_t
 *  \see meshSecNwkDeobfDecCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecNwkDeobfDec(bool_t isProxyConfig, meshSecNwkDeobfDecParams_t *pReqParams,
                                   meshSecNwkDeobfDecCback_t nwkDeobfDecCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Computes Secure Network Beacon authentication value.
 *
 *  \param[in] pSecNwkBeacon         Pointer to 22 byte buffer storing the Secure Network Beacon.
 *  \param[in] netKeyIndex           Global Network Key identifier.
 *  \param[in] useNewKey             TRUE if new key should be used for computation.
 *  \param[in] secNwkBeaconGenCback  Callback used to signal operation complete.
 *  \param[in] pParam                Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Calculation starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: NULL input ).
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  Either there is no material available for the NetKey
 *                                           Index or the Network ID field of the beacon has no
 *                                           match with the Network ID(s) associated to the index.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \see meshSecBeaconComputeAuthCback_t
 *
 *  \remarks This function computes the authentication value and stores it in the last 8 bytes
 *           of the buffer referenced by pSecNwkBeacon. Also copies the Network ID associated to
 *           the Network Key used for computing the authentication value.
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecBeaconComputeAuth(uint8_t *pSecNwkBeacon, uint16_t netKeyIndex,
                                         bool_t useNewKey,
                                         meshSecBeaconComputeAuthCback_t secNwkBeaconGenCback,
                                         void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Authenticates a Secure Network Beacon.
 *
 *  \param[in] pSecNwkBeacon          Pointer to received Secure Network Beacon.
 *  \param[in] secNwkBeaconAuthCback  Callback used to signal operation complete.
 *  \param[in] pParam                 Pointer to generic callback parameter.
 *
 *  \retval    MESH_SUCCESS             All validation is performed. Authentication starts.
 *  \retval    MESH_SEC_INVALID_PARAMS  Invalid parameters (E.g.: NULL input).
 *  \retval    MESH_SEC_KEY_NOT_FOUND   No matching NetKey index is found.
 *  \retval    MESH_SEC_OUT_OF_MEMORY   There are no resources to process the request.
 *
 *  \remarks This function computes the authentication value and compares it with the last 8 bytes
 *           of the buffer referenced by pSecNwkBeacon.
 *
 *  \see meshSecBeaconAuthCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecBeaconAuthenticate(uint8_t *pSecNwkBeacon,
                                          meshSecBeaconAuthCback_t secNwkBeaconAuthCback,
                                          void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Gets the Network ID.
 *
 *  \param[in] netKeyIndex  Netkey index.
 *
 *  \return    Pointer to Network ID, or NULL if not found.
 */
/*************************************************************************************************/
uint8_t *MeshSecNetKeyIndexToNwkId(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Gets the Identity Key.
 *
 *  \param[in] netKeyIndex  Netkey index.
 *
 *  \return    Pointer to Identity Key, or NULL if not found.
 */
/*************************************************************************************************/
uint8_t *MeshSecNetKeyIndexToIdentityKey(uint16_t netKeyIndex);

#ifdef __cplusplus
}
#endif

#endif /* MESH_SECURITY_H */

/*! **********************************************************************************************
* @}
************************************************************************************************ */
