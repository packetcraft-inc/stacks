/*************************************************************************************************/
/*!
 *  \file   mesh_prv.h
 *
 *  \brief  Provisioning common definitions.
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
 * \addtogroup MESH_PROVISIONING_COMMON_API Mesh Provisioning Common API
 * @{
 *************************************************************************************************/

#ifndef MESH_PRV_H
#define MESH_PRV_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Includes
**************************************************************************************************/

#include "wsf_types.h"
#include "wsf_msg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*!
 *  \addtogroup MESH_STACK_FIXED_PARAM Fixed parameters of the Mesh stack
 *  @{
 */

/*! \brief Size of the device UUID, in octets */
#define MESH_PRV_DEVICE_UUID_SIZE                 16

/*! \brief Size of the static OOB data, in octets */
#define MESH_PRV_STATIC_OOB_SIZE                  16

/*! \brief Maximum size of Input/Output OOB data, in octets */
#define MESH_PRV_INOUT_OOB_MAX_SIZE               8

/*!
 *  @}
 */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*!
 *  Mesh Provisioning Algorithm type enumerations
 *
 *  \brief  These values should be logically ORed to to indicate support for multiple algorithms
 */
enum meshPrvAlgorithms
{
  MESH_PRV_ALGO_FIPS_P256_ELLIPTIC_CURVE = (1 << 0),  /*!< FIPS P-256 Elliptic Curve bit */
  MESH_PRV_ALGO_RFU_BITMASK              = 0xFFFE,    /*!< RFU bitmask */
};

/*! \brief Mesh Provisioning Algorithm data type. See ::meshPrvAlgorithms */
typedef uint16_t meshPrvAlgorithms_t;

/*!
 *  Mesh Provisioning Public Key type enumerations
 *
 *  \brief  These values can be logically ORed to indicate support for multiple options
 */
enum meshPrvPublicKeyType
{
  MESH_PRV_PUB_KEY_OOB     = (1 << 0),  /*!< Public Key OOB information available bit */
  MESH_PRV_PUB_RFU_BITMASK = 0xFE,      /*!< RFU bits  */
};

/*! \brief Mesh Provisioning Public Key type data type. See ::meshPrvPublicKeyType */
typedef uint8_t meshPrvPublicKeyType_t;

/*!
 *  Mesh Provisioning Static OOB type enumerations
 *
 *  \brief  These values can be logically ORed to indicate support for multiple options
 */
enum meshPrvStaticOobType
{
  MESH_PRV_STATIC_OOB_INFO_AVAILABLE = (1 << 0),  /*!< Static OOB information available bit */
  MESH_PRV_STATIC_OOB_RFU_BITMASK    = 0xFE,      /*!< RFU bits */
};

/*! \brief Mesh Provisioning Static OOB type data type. See ::meshPrvStaticOobType */
typedef uint8_t meshPrvStaticOobType_t;

/*! \brief Mesh Provisioning OOB information source */
enum meshPrvOobInfoSourceTypes
{
  MESH_PRV_OOB_INFO_OTHER                     = (1 << 0),   /*!< Other source */
  MESH_PRV_OOB_INFO_ELECTRONIC_URI            = (1 << 1),   /*!< Electronic/URI */
  MESH_PRV_OOB_INFO_2D_MACHINE_READABLE_CODE  = (1 << 2),   /*!< 2D machine-readable code */
  MESH_PRV_OOB_INFO_BAR_CODE                  = (1 << 3),   /*!< Bar code */
  MESH_PRV_OOB_INFO_NFC                       = (1 << 4),   /*!< Near Field Communication (NFC) */
  MESH_PRV_OOB_INFO_NUMBER                    = (1 << 5),   /*!< Number */
  MESH_PRV_OOB_INFO_STRING                    = (1 << 6),   /*!< String */
  MESH_PRV_OOB_INFO_ON_BOX                    = (1 << 11),  /*!< On box */
  MESH_PRV_OOB_INFO_INSIDE_BOX                = (1 << 12),  /*!< Inside box */
  MESH_PRV_OOB_INFO_ON_PIECE_OF_PAPER         = (1 << 13),  /*!< On piece of paper */
  MESH_PRV_OOB_INFO_INSIDE_MANUAL             = (1 << 14),  /*!< Inside manual */
  MESH_PRV_OOB_INFO_ON_DEVICE                 = (1 << 15),  /*!< On device */
};

/*! \brief Mesh Provisioning Server OOB information sources. See ::meshPrvOobInfoSourceTypes */
typedef uint16_t meshPrvOobInfoSource_t;

/*!
 *  Mesh Provisioning Output OOB size enumerations
 *
 *  \brief  The Output OOB Size defines the number of digits that can be output (e.g. displayed
 *          or spoken) when the value Output Numeric in the Output OOB action field
 *          (see ::meshPrvOutputOobAction) is selected
 */
enum meshPrvOutputOobSize
{
  MESH_PRV_OUTPUT_OOB_NOT_SUPPORTED    = 0x00,  /*!< The device does not support output OOB */
  MESH_PRV_OUTPUT_OOB_SIZE_ONE_OCTET   = 0x01,  /*!< Maximum 1 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_TWO_OCTET   = 0x02,  /*!< Maximum 2 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_THREE_OCTET = 0x03,  /*!< Maximum 3 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_FOUR_OCTET  = 0x04,  /*!< Maximum 4 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_FIVE_OCTET  = 0x05,  /*!< Maximum 5 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_SIX_OCTET   = 0x06,  /*!< Maximum 6 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_SEVEN_OCTET = 0x07,  /*!< Maximum 7 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_EIGHT_OCTET = 0x08,  /*!< Maximum 8 octet can be output */
  MESH_PRV_OUTPUT_OOB_SIZE_RFU_START   = 0x09,  /*!< All numbers above this value are RFUs */
};

/*! \brief Mesh Provisioning Output OOB size data type. See ::meshPrvOutputOobSize */
typedef uint8_t meshPrvOutputOobSize_t;

/*!
 *  Mesh Provisioning Output OOB action enumerations
 *
 *  \brief  The Output OOB action bit field defines possible mechanism available for outputting
 *          OOB information. These values can be logically OR'ed to indicate support for multiple
 *          options.
 */
enum meshPrvOutputOobAction
{
  MESH_PRV_OUTPUT_OOB_ACTION_BLINK           = (1 << 0),  /*!< Blink - Numeric */
  MESH_PRV_OUTPUT_OOB_ACTION_BEEP            = (1 << 1),  /*!< Beep - Numeric */
  MESH_PRV_OUTPUT_OOB_ACTION_VIBRATE         = (1 << 2),  /*!< Vibrate - Numeric */
  MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_NUMERIC  = (1 << 3),  /*!< Output Numeric - Numeric */
  MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM = (1 << 4),  /*!< Output Alphanumeric - Octets array
                                                           */
  MESH_PRV_OUTPUT_OOB_ACTION_RFU_BITMASK     = 0xFFE0,    /*!< RFU bits */
};

/*! \brief Mesh Provisioning Output OOB action data type. See ::meshPrvOutputOobAction */
typedef uint16_t meshPrvOutputOobAction_t;

/*!
 *  Mesh Provisioning Input OOB size enumerations
 *
 *  \brief  The Input OOB Size defines the number of digits that can be entered when the value
 *          Input Numeric in the Input OOB Action field (see ::meshPrvOutputOobAction) is selected
 */
enum meshPrvInputOobSize
{
  MESH_PRV_INPUT_OOB_NOT_SUPPORTED    = 0x00,  /*!< The device does not support input OOB */
  MESH_PRV_INPUT_OOB_SIZE_ONE_OCTET   = 0x01,  /*!< Maximum 1 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_TWO_OCTET   = 0x02,  /*!< Maximum 2 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_THREE_OCTET = 0x03,  /*!< Maximum 3 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_FOUR_OCTET  = 0x04,  /*!< Maximum 4 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_FIVE_OCTET  = 0x05,  /*!< Maximum 5 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_SIX_OCTET   = 0x06,  /*!< Maximum 6 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_SEVEN_OCTET = 0x07,  /*!< Maximum 7 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_EIGHT_OCTET = 0x08,  /*!< Maximum 8 octet can be input */
  MESH_PRV_INPUT_OOB_SIZE_RFU_START   = 0x09,  /*!< All numbers above this value are RFUs */
};

/*! \brief Mesh Provisioning Input OOB size data type. See ::meshPrvInputOobSize */
typedef uint8_t meshPrvInputOobSize_t;

/*!
 *  Mesh Provisioning Input OOB action enumerations
 *
 *  \brief  The Input OOB action bit field defines possible mechanism available for inputting OOB
 *          information. These values can be logically OR'ed to indicate support for multiple
 *          options.
 */
enum meshPrvInputOobAction
{
  MESH_PRV_INPUT_OOB_ACTION_PUSH           = (1 << 0),  /*!< Push - Numeric */
  MESH_PRV_INPUT_OOB_ACTION_TWIST          = (1 << 1),  /*!< Twist - Numeric */
  MESH_PRV_INPUT_OOB_ACTION_INPUT_NUMERIC  = (1 << 2),  /*!< Input number - Numeric */
  MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM = (1 << 3),  /*!< Input Alphanumeric - Octets array */
  MESH_PRV_INPUT_OOB_ACTION_RFU_BITMASK    = 0xFFF0,    /*!< RFU bits */
};

/*! \brief Mesh Provisioning Input OOB action data type. See ::meshPrvInputOobAction */
typedef uint16_t meshPrvInputOobAction_t;

/*! \brief Mesh Provisioning Capabilities PDU structure */
typedef struct meshPrvCapabilities_tag
{
  uint8_t                  numOfElements;    /*!< Number of elements supported by the device,
                                              *   value 0x00 is prohibited
                                              */
  meshPrvAlgorithms_t      algorithms;       /*!< Supported algorithms and other capabilities.
                                              *   See ::meshPrvAlgorithms_t
                                              */
  meshPrvPublicKeyType_t   publicKeyType;    /*!< Supported public key types.
                                              *   See ::meshPrvPublicKeyType_t
                                              */
  meshPrvStaticOobType_t   staticOobType;    /*!< Supported static OOB types.
                                              *   See ::meshPrvStaticOobType_t
                                              */
  meshPrvOutputOobSize_t   outputOobSize;    /*!< Maximum size of output OOB data supported.
                                              *   See ::meshPrvOutputOobSize_t
                                              */
  meshPrvOutputOobAction_t outputOobAction;  /*!< Supported output OOB actions.
                                              *   See ::meshPrvOutputOobAction_t
                                              */
  meshPrvInputOobSize_t    inputOobSize;     /*!< Maximum size of input OOB data supported.
                                              *   See ::meshPrvInputOobSize_t
                                              */
  meshPrvInputOobAction_t  inputOobAction;   /*!< Supported input OOB actions.
                                              *   See ::meshPrvInputOobAction_t
                                              */
} meshPrvCapabilities_t;

/*! \brief Mesh Provisioning Input/Output OOB data */
typedef union meshPrvInOutOobData_tag
{
  uint32_t  numericOob;                                    /*!< OOB - numeric format. */
  uint8_t   alphanumericOob[MESH_PRV_INOUT_OOB_MAX_SIZE];  /*!< OOB - alphanumeric format. */
} meshPrvInOutOobData_t;

/*! \brief Mesh ECC Keys */
typedef struct meshPrvEccKeys_tag
{
  uint8_t *pPubKeyX;      /*!< Pointer to the ECC Public Key X component, 32-octet array. */
  uint8_t *pPubKeyY;      /*!< Pointer to the ECC Public Key Y component, 32-octet array. */
  uint8_t *pPrivateKey;   /*!< Pointer to the ECC Private Key, 32-octet array. */
} meshPrvEccKeys_t;

/*! \brief Peer Public Key structure - used when the key is available OOB */
typedef struct meshPrvOobPublicKey_tag
{
  uint8_t *pPubKeyX;      /*!< Pointer to the ECC Public Key X component, 32-octet array. */
  uint8_t *pPubKeyY;      /*!< Pointer to the ECC Public Key Y component, 32-octet array. */
} meshPrvOobPublicKey_t;

/*! \brief Provisioning fail reasons - enumeration values */
enum meshPrvFailReasonValues
{
  MESH_PRV_FAIL_LINK_NOT_ESTABLISHED,       /*!< PB-ADV link could not be established
                                             *   (client-only)
                                             */
  MESH_PRV_FAIL_LINK_CLOSED_BY_PEER,        /*!< Link was closed by the peer device */
  MESH_PRV_FAIL_RECEIVE_TIMEOUT,            /*!< Peer device did not send an expected
                                             *   Provisioning PDU
                                             */
  MESH_PRV_FAIL_SEND_TIMEOUT,               /*!< Peer device did not acknowledge an outbound
                                             *   Provisioning PDU
                                             */
  MESH_PRV_FAIL_INVALID_PUBLIC_KEY,         /*!< Provisioning Server has sent an invalid Public Key
                                             *   (client-only)
                                             */
  MESH_PRV_FAIL_CONFIRMATION,               /*!< Confirmation value received from the Provisioning
                                             *   Server was incorrect (client-only)
                                             */
  MESH_PRV_FAIL_PROTOCOL_ERROR,             /*!< Provisioning has encountered a protocol error
                                             *   (client-only)
                                             */
};

/*! Provisioning fail reasons - type.
 *  See ::meshPrvFailReasonValues
 */
typedef uint8_t meshPrvFailReason_t;

/*! \brief Parameters structure for the provisioning data */
typedef struct meshPrvProvisioningData_tag
{
  wsfMsgHdr_t   hdr;              /*!< Header structure */
  uint8_t       *pDevKey;         /*!< Pointer to Device Key */
  uint8_t       *pNetKey;         /*!< Pointer to Network Key */
  uint16_t      netKeyIndex;      /*!< Network Key Index */
  uint8_t       flags;            /*!< Flags bitmask */
  uint32_t      ivIndex;          /*!< Current value of the IV Index */
  uint16_t      address;          /*!< Address assigned to the primary element */
} meshPrvProvisioningData_t;

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
