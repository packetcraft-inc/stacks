/*************************************************************************************************/
/*!
 *  \file   mesh_security_defs.h
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

#ifndef MESH_SECURITY_DEFS_H
#define MESH_SECURITY_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! ASZMIC shift value for Application or Device Nonce */
#define MESH_SEC_ASZMIC_SHIFT                   7

/*! Mesh Network Security Privacy Random Size */
#define MESH_SEC_PRIV_RAND_SIZE                 7

/*! Mesh Security nonce type field position */
#define MESH_SEC_NONCE_TYPE_POS                 0

/*! Mesh Security nonce ASZMIC and PAD or CTL-TTL or PAD field position */
#define MESH_SEC_NONCE_ASZ_CTL_PAD_POS          1

/*! Mesh Security nonce sequence number field start position */
#define MESH_SEC_NONCE_SEQ_POS                  2

/*! Mesh Security nonce type field position */
#define MESH_SEC_NONCE_SRC_POS                  5

/*! Mesh Security nonce destination address first byte position */
#define MESH_SEC_NONCE_DST_PAD_POS              7

/*! Size of the P buffer of K2 derivation when master credentials are used */
#define MESH_SEC_K2_P_MASTER_SIZE               1

/*! Size of the P buffer of K2 derivation when friendship credentials are used */
#define MESH_SEC_K2_P_FRIEND_SIZE               9

/*! 32-bit (NET/TRANS)MIC size in bytes */
#define MESH_SEC_MIC_SIZE_32                   (4)

/*! 64-bit (NET/TRANS)MIC size in bytes */
#define MESH_SEC_MIC_SIZE_64                   (8)

/*! Check if the AppKeyIndex or NetKeyIndex is in valid range */
#define MESH_SEC_KEY_INDEX_IS_VALID(keyIndex)  ((keyIndex) <= MESH_APP_KEY_INDEX_MAX_VAL)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Nonce types used in PDU security */
enum meshSecNonceTypes
{
  MESH_SEC_NONCE_NWK   = 0x00,  /*!< Nonce is used for Network security */
  MESH_SEC_NONCE_APP   = 0x01,  /*!< Nonce is used for Application security */
  MESH_SEC_NONCE_DEV   = 0x02,  /*!< Nonce is used for Application security using Device Key */
  MESH_SEC_NONCE_PROXY = 0x03,  /*!< Nonce is used for Proxy Configuration messages */
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_SECURITY_DEFS_H */
