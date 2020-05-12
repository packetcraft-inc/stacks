/*************************************************************************************************/
/*!
 *  \file   mesh_security_main.h
 *
 *  \brief  Security internal definitions.
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

#ifndef MESH_SECURITY_MAIN_H
#define MESH_SECURITY_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Invalid value for network or application key index */
#define MESH_SEC_INVALID_KEY_INDEX            0xFFFF

/*! Invalid value for index in the Key information list */
#define MESH_SEC_INVALID_ENTRY_INDEX          0xFFFF

/*! Maximum value allowed for Application or Network key index */
#define MESH_SEC_MAX_KEY_INDEX                0x0FFF

/*! Number of key material entries per key index */
#define MESH_SEC_KEY_MAT_PER_INDEX            2

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Flags associated to security material */
enum meshSecKeyMaterialFlags
{
  MESH_SEC_KEY_UNUSED              = 0,        /*!< No flags means entry is not used */
  MESH_SEC_KEY_CRT_IN_PROGESS      = (1 << 0), /*!< Entry is used by a derivation procedure */
  MESH_SEC_KEY_UPDT_IN_PROGRESS    = (1 << 1), /*!< Entry is used by an update derivation
                                                *   procedure
                                                */
  MESH_SEC_KEY_CRT_MAT_AVAILABLE   = (1 << 2), /*!< Entry contains key derivation material */
  MESH_SEC_KEY_UPDT_MAT_AVAILABLE  = (1 << 3), /*!< Entry contains key derivation material for the
                                                *   updated key
                                                */
  MESH_SEC_KEY_ALL_DELETE          = (1 << 4), /*!< Entry is used by an ongoing procedure but user
                                                *   requests to be removed
                                                */
  MESH_SEC_KEY_CRT_MAT_DELETE      = (1 << 5), /*!< Entry is used by an ongoing procedure but user
                                                *   requests to switch to updated material
                                                */
};

/*! Bitfield flags defining state of the key material */
typedef uint8_t meshSecMatFlags_t;

/*! Security material derived from the Application Key */
typedef struct meshSecAppKeyMat_tag
{
  uint8_t aid; /*!< Application identifier (6 bits) */
} meshSecAppKeyMat_t;

/*! Security material used in cryptographic operations at the network layer */
typedef struct meshSecNetKeyPduSecMat_tag
{
  uint8_t nid;                            /*!< Identifier of the Network Key used (7 bits) */
  uint8_t encryptKey[MESH_KEY_SIZE_128];  /*!< Encryption/Decryption Key */
  uint8_t privacyKey[MESH_KEY_SIZE_128];  /*!< Obfuscation/Deobfuscation Key */
} meshSecNetKeyPduSecMat_t;

/*! Security material derived from the Network Key */
typedef struct meshSecNetKeyMaterial_tag
{
  meshSecNetKeyPduSecMat_t masterPduSecMat;                   /*!< Master credentials */
  uint8_t                  networkID[MESH_NWK_ID_NUM_BYTES];  /*!< Public subnet
                                                               *   identifier
                                                               */
  uint8_t                  beaconKey[MESH_KEY_SIZE_128];      /*!< Key used for Secure Network
                                                               *   Beacons
                                                               */
  uint8_t                  identityKey[MESH_KEY_SIZE_128];    /*!< Node identity Key */
} meshSecNetKeyMaterial_t;

/*! Security Application or Network Key identification data for derived material */
typedef struct meshSecKeyInfoHdr_tag
{
  uint16_t            keyIndex;  /*!< Global index of the key */
  uint8_t             crtKeyId;  /*!< Index of the material derived from the current key */
  meshSecMatFlags_t   flags;     /*!< Flags identifying state of the derived material */
} meshSecKeyInfoHdr_t;

/*! Security Network Key information. Contains identification data and derivation material */
typedef struct meshSecNetKeyInfo_tag
{
  meshSecKeyInfoHdr_t     hdr;                                      /*!< Identification header */
  meshSecNetKeyMaterial_t keyMaterial[MESH_SEC_KEY_MAT_PER_INDEX];  /*!< Key material entries for
                                                                     *   current and updated key
                                                                     */
} meshSecNetKeyInfo_t;

/*! Security Application Key information. Contains identification data and derivation material */
typedef struct meshSecAppKeyInfo_tag
{
  meshSecKeyInfoHdr_t hdr;                                      /*!< Identification header */
  meshSecAppKeyMat_t  keyMaterial[MESH_SEC_KEY_MAT_PER_INDEX];  /*!< Key material entries for current
                                                                 *   and updated key
                                                                 */
} meshSecAppKeyInfo_t;

/*! Friendship material and identification data */
typedef struct meshSecFriendMat_tag
{
  uint16_t                 netKeyInfoIndex;                         /*!< Index in list of Network
                                                                     *   Key information
                                                                     */
  meshAddress_t            friendAddres;                            /*!< Address of the friend
                                                                     *   node
                                                                     */
  meshAddress_t            lpnAddress;                              /*!< Address of the low power
                                                                     *   node
                                                                     */
  uint16_t                 friendCounter;                           /*!< The value from the
                                                                     *   FriendCounter field of
                                                                     *   the Friend Offer message
                                                                     */
  uint16_t                 lpnCounter;                              /*!< The value from the
                                                                     *   LPNCounter field of the
                                                                     *   Friend Request message
                                                                     */
  bool_t                   hasUpdtMaterial;                         /*!< TRUE if second material
                                                                     *   entry has valid data
                                                                     */
  meshSecNetKeyPduSecMat_t keyMaterial[MESH_SEC_KEY_MAT_PER_INDEX]; /*!< Security material using
                                                                     *   friendship credentials
                                                                     */
} meshSecFriendMat_t;

/*! Security material */
typedef struct meshSecMaterial_tag
{
  meshSecAppKeyInfo_t  *pAppKeyInfoArray;   /*!< Pointer to storage for security information
                                             *   derived from Application Keys
                                             */
  meshSecNetKeyInfo_t  *pNetKeyInfoArray;   /*!< Storage for security information derived from
                                             *   Network Keys
                                             */
  meshSecFriendMat_t   *pFriendMatArray;    /*!< Storage for security information derived from
                                             *   Network Keys using friendship credentials
                                             */
  uint16_t             appKeyInfoListSize;  /*!< Size (number of elements) of the AppKey material
                                             *   list
                                             */
  uint16_t             netKeyInfoListSize;  /*!< Size (number of elements) of the NetKey material
                                             *   list
                                             */
  uint16_t             friendMatListSize;   /*!< Size (number of elements) of the NetKey material
                                             *   list obtained with friendship credentials
                                             */
} meshSecMaterial_t;

/*! Security control block */
typedef struct meshSecCb_tag
{
  meshSecRemoteDevKeyReadCback_t      secRemoteDevKeyReader;  /*!< Remote Node's Device Key reader */
  meshSecAllKeyMaterialRestoreCback_t restoreCback;           /*!< Security material restore callback */
} meshSecCb_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

extern meshSecMaterial_t   secMatLocals;
extern meshSecCb_t         meshSecCb;

#ifdef __cplusplus
}
#endif

#endif /* MESH_SECURITY_MAIN_H */
