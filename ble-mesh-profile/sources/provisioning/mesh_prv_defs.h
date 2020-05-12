/*************************************************************************************************/
/*!
 *  \file   mesh_prv_defs.h
 *
 *  \brief  Mesh Provisioning common definitions.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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

#ifndef MESH_PRV_DEFS_H
#define MESH_PRV_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Maximum size of an unprovisioned device beacon */
#define MESH_PRV_MAX_BEACON_SIZE               23

/*! Maximum size of an unprovisioned device beacon without an associated URI */
#define MESH_PRV_MAX_NO_URI_BEACON_SIZE        19

/*! Offset of the device UUID in a beacon */
#define MESH_PRV_BEACON_DEVICE_UUID_OFFSET     1

/*! Offset of the OOB info in a beacon */
#define MESH_PRV_BEACON_OOB_INFO_OFFSET        17

/*! Offset of the URI hash in a beacon */
#define MESH_PRV_BEACON_URI_HASH_OFFSET        19

/*! Maximum size of the URI hash is 4 Octets */
#define MESH_PRV_BEACON_URI_HASH_SIZE          4

/*! GPCF value for Transaction Start */
#define MESH_PRV_GPCF_START                    0x00

/*! GPCF value for Transaction ACK */
#define MESH_PRV_GPCF_ACK                      0x01

/*! GPCF value for Transaction Continuation */
#define MESH_PRV_GPCF_CONTINUATION             0x02

/*! GPCF value for Provisioning Bearer Control */
#define MESH_PRV_GPCF_CONTROL                  0x03

/*! GPCF mask for a Generic Provisioning PDU */
#define MESH_PRV_GPCF_MASK                     0x03

/*! GPCF size in bits for a Generic Provisioning PDU */
#define MESH_PRV_GPCF_SIZE                     2

/*! Mesh Provisioning Link Open Opcode */
#define MESH_PRV_LINK_OPEN_OPCODE              0x00

/*! Mesh Provisioning Link Open PDU Size */
#define MESH_PRV_LINK_OPEN_PDU_SIZE            17

/*! Mesh Provisioning Link ACK Opcode */
#define MESH_PRV_LINK_ACK_OPCODE               0x01

/*! Mesh Provisioning Link ACK PDU Size */
#define MESH_PRV_LINK_ACK_PDU_SIZE             1

/*! Mesh Provisioning Link Close Opcode */
#define MESH_PRV_LINK_CLOSE_OPCODE             0x02

/*! Mesh Provisioning Link Close PDU Size */
#define MESH_PRV_LINK_CLOSE_PDU_SIZE           2

/*! Mesh Provisioning PB-ADV Transaction Number Offset */
#define MESH_PRV_PB_ADV_TRAN_NUM_OFFSET        4

/*! Mesh Provisioning PB-ADV Generic Provisioning PDU Offset */
#define MESH_PRV_PB_ADV_GEN_PDU_OFFSET         (4 + 1)

/*! Mesh Provisioning PB-ADV Generic Provisioning PDU Data Offset */
#define MESH_PRV_PB_ADV_GEN_DATA_OFFSET        (4 + 1 + 1)

/*! Transaction Number used by a node for the first time over an unprovisioned link */
#define MESH_PRV_SR_TRAN_NUM_START             0x80

/*! Transaction Number wrap value for a node */
#define MESH_PRV_SR_TRAN_NUM_WRAP              0xFF

/*! Transaction Number used by a provisioner for the first time over an unprovisioned link */
#define MESH_PRV_CL_TRAN_NUM_START             0x00

/*! Transaction Number wrap value for a provisioner */
#define MESH_PRV_CL_TRAN_NUM_WRAP              0x7F

/*! Provisioning Bearer Minimum Tx Delay in ms */
#define MESH_PRV_PROVISIONER_MIN_TX_DELAY_MS   20

/*! Provisioning Bearer Maximum Tx Delay in ms */
#define MESH_PRV_PROVISIONER_MAX_TX_DELAY_MS   50

/*! Mesh Provisioning PB-ADV PDU minimum length */
#define MESH_PRV_MIN_PB_ADV_PDU_SIZE           6

/*! Mesh Provisioning PB-ADV PDU maximum length */
#define MESH_PRV_MAX_PB_ADV_PDU_SIZE           29

/*! Mesh Generic Provisioning PDU maximum length */
#define MESH_PRV_MAX_GEN_PB_PDU_SIZE           24

/*! Mesh Provisioning Data PDU Segment 0 maximum length */
#define MESH_PRV_MAX_SEG0_PB_PDU_SIZE          20

/*! Mesh Provisioning Data PDU Segment 0 header length */
#define MESH_PRV_MAX_SEG0_PB_HDR_SIZE          4

/*! Mesh Provisioning Data PDU Segment X maximum length, except when X is 0 */
#define MESH_PRV_MAX_SEGX_PB_PDU_SIZE          23

/*! Mesh Provisioning Data PDU Segment X header length */
#define MESH_PRV_MAX_SEGX_PB_HDR_SIZE          1

/*! Mesh Provisioning Segment Receive Mask Length - Max 64 fragments */
#define MESH_PRV_SEG_MASK_SIZE                 2

/*! Provisioning transaction timeout in ms */
#define MESH_PRV_TRAN_TIMEOUT_MS               60000

/*! Provisioning Link Establishment timeout in ms */
#define MESH_PRV_LINK_TIMEOUT_MS               60000

/* Provisioning PDU format definitions */

/*! Opcode location */
#define MESH_PRV_PDU_OPCODE_INDEX               0
/*! Opcode size */
#define MESH_PRV_PDU_OPCODE_SIZE                1
/*! Parameter location */
#define MESH_PRV_PDU_PARAM_INDEX                (MESH_PRV_PDU_OPCODE_INDEX \
                                                + MESH_PRV_PDU_OPCODE_SIZE)

/*! Invite PDU: Attention parameter index */
#define MESH_PRV_PDU_INVITE_ATTENTION_INDEX     (MESH_PRV_PDU_PARAM_INDEX)
/*! Invite PDU: Attention parameter size */
#define MESH_PRV_PDU_INVITE_ATTENTION_SIZE      1
/*! Invite PDU: Total parameter size */
#define MESH_PRV_PDU_INVITE_PARAM_SIZE          (MESH_PRV_PDU_INVITE_ATTENTION_SIZE)
/*! Invite PDU: Total PDU size */
#define MESH_PRV_PDU_INVITE_PDU_SIZE            (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_INVITE_PARAM_SIZE)

/*! Capabilities PDU: Number of Elements parameter index */
#define MESH_PRV_PDU_CAPAB_NUM_ELEM_INDEX      (MESH_PRV_PDU_PARAM_INDEX)
/*! Capabilitites PDU: Number of Elements parameter size */
#define MESH_PRV_PDU_CAPAB_NUM_ELEM_SIZE        1
/*! Capabilitites PDU: Algorithms parameter index */
#define MESH_PRV_PDU_CAPAB_ALGORITHMS_INDEX     (MESH_PRV_PDU_CAPAB_NUM_ELEM_INDEX \
                                                + MESH_PRV_PDU_CAPAB_NUM_ELEM_SIZE)
/*! Capabilitites PDU: Algorithms parameter size */
#define MESH_PRV_PDU_CAPAB_ALGORITHMS_SIZE      2
/*! Capabilitites PDU: Public Key Type parameter index */
#define MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_INDEX   (MESH_PRV_PDU_CAPAB_ALGORITHMS_INDEX \
                                                + MESH_PRV_PDU_CAPAB_ALGORITHMS_SIZE)
/*! Capabilitites PDU: Public Key Type parameter size */
#define MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_SIZE    1
/*! Capabilitites PDU: Static OOB Type parameter index */
#define MESH_PRV_PDU_CAPAB_STATIC_OOB_INDEX     (MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_INDEX \
                                                + MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_SIZE)
/*! Capabilitites PDU: Static OOB Type parameter size */
#define MESH_PRV_PDU_CAPAB_STATIC_OOB_SIZE      1
/*! Capabilitites PDU: Output OOB Size parameter index */
#define MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_INDEX   (MESH_PRV_PDU_CAPAB_STATIC_OOB_INDEX \
                                                + MESH_PRV_PDU_CAPAB_STATIC_OOB_SIZE)
/*! Capabilitites PDU: Output OOB Size parameter size */
#define MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_SIZE    1
/*! Capabilitites PDU: Output OOB Action parameter index */
#define MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_INDEX    (MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_INDEX \
                                                + MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_SIZE)
/*! Capabilitites PDU: Output OOB Action parameter size */
#define MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_SIZE     2
/*! Capabilitites PDU: Input OOB Size parameter index */
#define MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_INDEX    (MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_INDEX \
                                                + MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_SIZE)
/*! Capabilitites PDU: Input OOB Size parameter size */
#define MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_SIZE     1
/*! Capabilitites PDU: Input OOB Action parameter index */
#define MESH_PRV_PDU_CAPAB_IN_OOB_ACT_INDEX     (MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_INDEX \
                                                + MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_SIZE)
/*! Capabilitites PDU: Input OOB Action parameter size */
#define MESH_PRV_PDU_CAPAB_IN_OOB_ACT_SIZE      2
/*! Capabilitites PDU: Total parameter size */
#define MESH_PRV_PDU_CAPAB_PARAM_SIZE           (MESH_PRV_PDU_CAPAB_NUM_ELEM_SIZE \
                                                + MESH_PRV_PDU_CAPAB_ALGORITHMS_SIZE \
                                                + MESH_PRV_PDU_CAPAB_PUB_KEY_TYPE_SIZE \
                                                + MESH_PRV_PDU_CAPAB_STATIC_OOB_SIZE \
                                                + MESH_PRV_PDU_CAPAB_OUT_OOB_SIZE_SIZE \
                                                + MESH_PRV_PDU_CAPAB_OUT_OOB_ACT_SIZE \
                                                + MESH_PRV_PDU_CAPAB_IN_OOB_SIZE_SIZE \
                                                + MESH_PRV_PDU_CAPAB_IN_OOB_ACT_SIZE)
/*! Capabilitites PDU: Total PDU size */
#define MESH_PRV_PDU_CAPAB_PDU_SIZE             (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_CAPAB_PARAM_SIZE)

/*! Start PDU: Algorithm parameter index */
#define MESH_PRV_PDU_START_ALGORITHM_INDEX      (MESH_PRV_PDU_PARAM_INDEX)
/*! Start PDU: Algorithm parameter size */
#define MESH_PRV_PDU_START_ALGORITHM_SIZE       1
/*! Start PDU: Public Key parameter index */
#define MESH_PRV_PDU_START_PUB_KEY_INDEX        (MESH_PRV_PDU_START_ALGORITHM_INDEX \
                                                + MESH_PRV_PDU_START_ALGORITHM_SIZE)
/*! Start PDU: Public Key parameter size */
#define MESH_PRV_PDU_START_PUB_KEY_SIZE         1
/*! Start PDU: Authentication Method parameter index */
#define MESH_PRV_PDU_START_AUTH_METHOD_INDEX    (MESH_PRV_PDU_START_PUB_KEY_INDEX \
                                                + MESH_PRV_PDU_START_PUB_KEY_SIZE)
/*! Start PDU: Authentication Method parameter size */
#define MESH_PRV_PDU_START_AUTH_METHOD_SIZE     1
/*! Start PDU: Authentication Action parameter index */
#define MESH_PRV_PDU_START_AUTH_ACTION_INDEX    (MESH_PRV_PDU_START_AUTH_METHOD_INDEX \
                                                + MESH_PRV_PDU_START_AUTH_METHOD_SIZE)
/*! Start PDU: Authentication Action parameter size */
#define MESH_PRV_PDU_START_AUTH_ACTION_SIZE     1
/*! Start PDU: Authentication Size parameter index */
#define MESH_PRV_PDU_START_AUTH_SIZE_INDEX      (MESH_PRV_PDU_START_AUTH_ACTION_INDEX \
                                                + MESH_PRV_PDU_START_AUTH_ACTION_SIZE)
/*! Start PDU: Authentication Size parameter size */
#define MESH_PRV_PDU_START_AUTH_SIZE_SIZE       1
/*! Start PDU: Total parameter size */
#define MESH_PRV_PDU_START_PARAM_SIZE           (MESH_PRV_PDU_START_ALGORITHM_SIZE \
                                                + MESH_PRV_PDU_START_PUB_KEY_SIZE \
                                                + MESH_PRV_PDU_START_AUTH_METHOD_SIZE \
                                                + MESH_PRV_PDU_START_AUTH_ACTION_SIZE \
                                                + MESH_PRV_PDU_START_AUTH_SIZE_SIZE)
/*! Start PDU: Total PDU size */
#define MESH_PRV_PDU_START_PDU_SIZE             (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_START_PARAM_SIZE)

/*! Public Key PDU: Public Key X parameter index */
#define MESH_PRV_PDU_PUB_KEY_X_INDEX            (MESH_PRV_PDU_PARAM_INDEX)
/*! Public Key PDU: Public Key X parameter size */
#define MESH_PRV_PDU_PUB_KEY_X_SIZE             32
/*! Public Key PDU: Public Key Y parameter index */
#define MESH_PRV_PDU_PUB_KEY_Y_INDEX            (MESH_PRV_PDU_PUB_KEY_X_INDEX \
                                                + MESH_PRV_PDU_PUB_KEY_X_SIZE)
/*! Public Key PDU: Public Key Y parameter size */
#define MESH_PRV_PDU_PUB_KEY_Y_SIZE             32
/*! Public Key PDU: Total parameter size */
#define MESH_PRV_PDU_PUB_KEY_PARAM_SIZE         (MESH_PRV_PDU_PUB_KEY_X_SIZE \
                                                + MESH_PRV_PDU_PUB_KEY_Y_SIZE)
/*! Public Key PDU: Total PDU size */
#define MESH_PRV_PDU_PUB_KEY_PDU_SIZE           (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_PUB_KEY_PARAM_SIZE)

/*! Input Complete PDU: Total parameter size */
#define MESH_PRV_PDU_INPUT_COMPLETE_PARAM_SIZE  0
/*! Input Complete PDU: Total PDU size */
#define MESH_PRV_PDU_INPUT_COMPLETE_PDU_SIZE    (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_INPUT_COMPLETE_PARAM_SIZE)

/*! Confirmation PDU: Confirmation parameter index */
#define MESH_PRV_PDU_CONFIRM_CONFIRM_INDEX      (MESH_PRV_PDU_PARAM_INDEX)
/*! Confirmation PDU: Confirmation parameter size */
#define MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE       16
/*! Confirmation PDU: Total parameter size */
#define MESH_PRV_PDU_CONFIRM_PARAM_SIZE         (MESH_PRV_PDU_CONFIRM_CONFIRM_SIZE)
/*! Confirmation PDU: Total PDU size */
#define MESH_PRV_PDU_CONFIRM_PDU_SIZE           (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_CONFIRM_PARAM_SIZE)

/*! Random PDU: Random parameter index */
#define MESH_PRV_PDU_RANDOM_RANDOM_INDEX        (MESH_PRV_PDU_PARAM_INDEX)
/*! Random PDU: Random parameter size */
#define MESH_PRV_PDU_RANDOM_RANDOM_SIZE         16
/*! Random PDU: Total parameter size */
#define MESH_PRV_PDU_RANDOM_PARAM_SIZE          (MESH_PRV_PDU_RANDOM_RANDOM_SIZE)
/*! Random PDU: Total PDU size */
#define MESH_PRV_PDU_RANDOM_PDU_SIZE            (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_RANDOM_PARAM_SIZE)

/*! Data PDU: Encrypted Provisioning Data parameter index */
#define MESH_PRV_PDU_DATA_ENC_DATA_INDEX        (MESH_PRV_PDU_PARAM_INDEX)
/*! Data PDU: Encrypted Provisioning Data parameter size */
#define MESH_PRV_PDU_DATA_ENC_DATA_SIZE         25
/*! Data PDU: Provisioning Data MIC parameter index */
#define MESH_PRV_PDU_DATA_MIC_INDEX             (MESH_PRV_PDU_DATA_ENC_DATA_INDEX \
                                                + MESH_PRV_PDU_DATA_ENC_DATA_SIZE)
/*! Data PDU: Provisioning Data MIC parameter size */
#define MESH_PRV_PDU_DATA_MIC_SIZE              8
/*! Data PDU: Total parameter size */
#define MESH_PRV_PDU_DATA_PARAM_SIZE            (MESH_PRV_PDU_DATA_ENC_DATA_SIZE \
                                                + MESH_PRV_PDU_DATA_MIC_SIZE)
/*! Data PDU: Total PDU size */
#define MESH_PRV_PDU_DATA_PDU_SIZE              (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_DATA_PARAM_SIZE)

/*! Decrypted Data: Network Key parameter index */
#define MESH_PRV_DECRYPTED_DATA_NETKEY_INDEX      0
/*! Decrypted Data: Network Key parameter size */
#define MESH_PRV_DECRYPTED_DATA_NETKEY_SIZE       16
/*! Decrypted Data: NetKey Index parameter index */
#define MESH_PRV_DECRYPTED_DATA_NETKEYIDX_INDEX   (MESH_PRV_DECRYPTED_DATA_NETKEY_INDEX \
                                                  + MESH_PRV_DECRYPTED_DATA_NETKEY_SIZE)
/*! Decrypted Data: NetKey Index parameter size */
#define MESH_PRV_DECRYPTED_DATA_NETKEYIDX_SIZE    2
/*! Decrypted Data: Flags parameter index */
#define MESH_PRV_DECRYPTED_DATA_FLAGS_INDEX       (MESH_PRV_DECRYPTED_DATA_NETKEYIDX_INDEX \
                                                  + MESH_PRV_DECRYPTED_DATA_NETKEYIDX_SIZE)
/*! Decrypted Data: Flags parameter size */
#define MESH_PRV_DECRYPTED_DATA_FLAGS_SIZE        1
/*! Decrypted Data: IV Index parameter index */
#define MESH_PRV_DECRYPTED_DATA_IVIDX_INDEX       (MESH_PRV_DECRYPTED_DATA_FLAGS_INDEX \
                                                  + MESH_PRV_DECRYPTED_DATA_FLAGS_SIZE)
/*! Decrypted Data: IV Index parameter size */
#define MESH_PRV_DECRYPTED_DATA_IVIDX_SIZE        4
/*! Decrypted Data: Address parameter index */
#define MESH_PRV_DECRYPTED_DATA_ADDRESS_INDEX     (MESH_PRV_DECRYPTED_DATA_IVIDX_INDEX \
                                                  + MESH_PRV_DECRYPTED_DATA_IVIDX_SIZE)
/*! Decrypted Data: Address parameter size */
#define MESH_PRV_DECRYPTED_DATA_ADDRESS_SIZE      2

/*! Complete PDU: Total parameter size */
#define MESH_PRV_PDU_COMPLETE_PARAM_SIZE        0
/*! Complete PDU: Total PDU size */
#define MESH_PRV_PDU_COMPLETE_PDU_SIZE          (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_COMPLETE_PARAM_SIZE)

/*! Failed PDU: Error Code parameter index */
#define MESH_PRV_PDU_FAILED_ERROR_CODE_INDEX    (MESH_PRV_PDU_PARAM_INDEX)
/*! Failed PDU: Error Code parameter size */
#define MESH_PRV_PDU_FAILED_ERROR_CODE_SIZE     1
/*! Failed PDU: Total parameter size */
#define MESH_PRV_PDU_FAILED_PARAM_SIZE          (MESH_PRV_PDU_FAILED_ERROR_CODE_SIZE)
/*! Failed PDU: Total PDU size */
#define MESH_PRV_PDU_FAILED_PDU_SIZE            (MESH_PRV_PDU_OPCODE_SIZE \
                                                + MESH_PRV_PDU_FAILED_PARAM_SIZE)

/* Provisioning crypto constants */

/*! Size of the ConfirmationInputs array */
#define MESH_PRV_CONFIRMATION_INPUTS_SIZE       (MESH_PRV_PDU_INVITE_PARAM_SIZE \
                                                + MESH_PRV_PDU_CAPAB_PARAM_SIZE \
                                                + MESH_PRV_PDU_START_PARAM_SIZE \
                                                + 2 * MESH_PRV_PDU_PUB_KEY_PARAM_SIZE)
/*! Size of the ConfirmationSalt value */
#define MESH_PRV_CONFIRMATION_SALT_SIZE         16
/*! Size of the AuthValue */
#define MESH_PRV_AUTH_VALUE_SIZE                16
#define MESH_PRV_CONFIRMATION_KEY_TEMP          "prck"
/*! Size of the ProvisioningSalt value */
#define MESH_PRV_PROVISIONING_SALT_SIZE         16
#define MESH_PRV_SESSION_KEY_TEMP               "prsk"
#define MESH_PRV_SESSION_NONCE_TEMP             "prsn"
#define MESH_PRV_DEVICE_KEY_TEMP                "prdk"
#define MESH_PRV_SESSION_NONCE_SIZE             13
#define MESH_PRV_MAX_OOB_SIZE                   8
#define MESH_PRV_NUMERIC_OOB_SIZE_OCTETS        4

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Provisioning PDU type enumerations */
enum meshPrvPduTypes
{
  MESH_PRV_PDU_INVITE         = 0x00,  /*!< Indicates invitation to join a mesh network */
  MESH_PRV_PDU_CAPABILITIES   = 0x01,  /*!< Indicates the capabilities of the device */
  MESH_PRV_PDU_START          = 0x02,  /*!< Indicates provisioning method selected by the
                                        *   provisioner based on the capabilities of the device
                                        */
  MESH_PRV_PDU_PUB_KEY        = 0x03,  /*!< Contains the public key data of the device or
                                        *   provisioner
                                        */
  MESH_PRV_PDU_INPUT_COMPLETE = 0x04,  /*!< Indicates user has completed inputting the values */
  MESH_PRV_PDU_CONFIRMATION   = 0x05,  /*!< Contains the provisioning confirmation value of the
                                        *   device or the Provisioner
                                        */
  MESH_PRV_PDU_RANDOM         = 0x06,  /*!< Contains the provisioning random value of the device or
                                        *   the Provisioner
                                        */
  MESH_PRV_PDU_DATA           = 0x07,  /*!< Includes the assigned unicast address of the primary
                                        *   element, a network key, NetKey Index, Flags and the
                                        *   IV Index
                                        */
  MESH_PRV_PDU_COMPLETE       = 0x08,  /*!< Indicates that provisioning is complete */
  MESH_PRV_PDU_FAILED         = 0x09,  /*!< Indicates that provisioning was unsuccessful */
  MESH_PRV_PDU_RFU_START      = 0x0A,  /*!< First RFU PDU type */
};

/* Provisioning PDU parameters */

/*! Provisioning Start PDU: Algorithm values */
enum meshPrvPduStartAlgorithmValues_tag
{
  MESH_PRV_START_ALGO_FIPS_P_256_EC  = 0x00,  /*!< FIPS-P256 Elliptic Curve */
  MESH_PRV_START_ALGO_RFU_START      = 0x01,  /*!< All numbers above this value are RFUs */
};

/*! Provisioning Start PDU: Algorithm type. See ::meshPrvPduStartAlgorithmValues_tag */
typedef uint8_t meshPrvPduStartAlgorithm_t;

/*! Provisioning Start PDU: Public Key values */
enum meshPrvPduStartPubKeyValues_tag
{
  MESH_PRV_START_PUB_KEY_OOB_NOT_AVAILABLE  = 0x00,  /*!< No OOB Public Key is used */
  MESH_PRV_START_PUB_KEY_OOB_AVAILABLE      = 0x01,  /*!< OOB Public Key is used */
  MESH_PRV_START_PUB_KEY_PROHIBITED_START   = 0x02,  /*!< All numbers above this value are Prohibited */
};

/*! Provisioning Start PDU: Authentication Method values */
enum meshPrvPduStartAuthMethodValues_tag
{
  MESH_PRV_START_AUTH_METHOD_NO_OOB             = 0x00,  /*!< No OOB authentication is used */
  MESH_PRV_START_AUTH_METHOD_STATIC_OOB         = 0x01,  /*!< Static OOB authentication is used */
  MESH_PRV_START_AUTH_METHOD_OUTPUT_OOB         = 0x02,  /*!< Output OOB authentication is used */
  MESH_PRV_START_AUTH_METHOD_INPUT_OOB          = 0x03,  /*!< Input OOB authentication is used */
  MESH_PRV_START_AUTH_METHOD_PROHIBITED_START   = 0x04,  /*!< All numbers above this value are Prohibited */
};

/*! Provisioning Start PDU: Output OOB Action values */
enum meshPrvPduStartOutOobActionValues_tag
{
  MESH_PRV_START_OUT_OOB_ACTION_BLINK         = 0x00,  /*!< Blink */
  MESH_PRV_START_OUT_OOB_ACTION_BEEP          = 0x01,  /*!< Beep */
  MESH_PRV_START_OUT_OOB_ACTION_VIBRATE       = 0x02,  /*!< Vibrate */
  MESH_PRV_START_OUT_OOB_ACTION_NUMERIC       = 0x03,  /*!< Numeric */
  MESH_PRV_START_OUT_OOB_ACTION_ALPHANUMERIC  = 0x04,  /*!< Alphanumeric */
  MESH_PRV_START_OUT_OOB_ACTION_RFU_START     = 0x05,  /*!< All numbers above this value are RFU */
};

/*! Provisioning Start PDU: Input OOB Action values */
enum meshPrvPduStartInOobActionValues_tag
{
  MESH_PRV_START_IN_OOB_ACTION_PUSH           = 0x00,  /*!< Push */
  MESH_PRV_START_IN_OOB_ACTION_TWIST          = 0x01,  /*!< Twist */
  MESH_PRV_START_IN_OOB_ACTION_NUMERIC        = 0x02,  /*!< Numeric */
  MESH_PRV_START_IN_OOB_ACTION_ALPHANUMERIC   = 0x03,  /*!< Alphanumeric */
  MESH_PRV_START_IN_OOB_ACTION_RFU_START      = 0x04,  /*!< All numbers above this value are RFU */
};

/*! Provisioning Start PDU: Input/Output OOB Size values */
enum meshPrvPduStartOobSizeValues_tag
{
  MESH_PRV_START_OOB_SIZE_PROHIBITED  = 0x00,  /*!< Prohibited value for this field */
  MESH_PRV_START_OOB_SIZE_MIN         = 0x01,  /*!< Minimum value for this field */
  MESH_PRV_START_OOB_SIZE_MAX         = 0x08,  /*!< Maximum value for this field */
  MESH_PRV_START_OOB_SIZE_RFU_START   = 0x09,  /*!< All numbers above this value are RFU */
};

/*! Action and size values for Static and No Oob */
#define MESH_PRV_START_OOB_NO_SIZE_NO_ACTION      0x00

/*! Provisioning Failed PDU: Error Code values */
enum meshPrvErrorCodeValues_tag
{
  MESH_PRV_ERR_PROHIBITED          = 0x00,  /*!< Prohibited value */
  MESH_PRV_ERR_INVALID_PDU         = 0x01,  /*!< The provisioning protocol PDU is not recognized by
                                             *   the device
                                             */
  MESH_PRV_ERR_INVALID_FORMAT      = 0x02,  /*!< The arguments of the protocol PDUs are outside
                                             *   expected values or the length of the PDU is
                                             *   different than expected
                                             */
  MESH_PRV_ERR_UNEXPECTED_PDU      = 0x03,  /*!< The PDU received was not expected at this moment
                                             *   of the procedure
                                             */
  MESH_PRV_ERR_CONFIRMATION_FAILED = 0x04,  /*!< The computed confirmation value was not
                                             *   successfully verified
                                             */
  MESH_PRV_ERR_OUT_OF_RESOURCES    = 0x05,  /*!< The provisioning protocol cannot be continued due
                                             *   to insufficient resources in the device
                                             */
  MESH_PRV_ERR_DECRYPTION_FAILED   = 0x06,  /*!< The Data block was not successfully decrypted */
  MESH_PRV_ERR_UNEXPECTED_ERROR    = 0x07,  /*!< An unexpected error occurred that may not be
                                             *   recoverable
                                             */
  MESH_PRV_ERR_CANNOT_ASSIGN_ADD   = 0x08,  /*!< The device cannot assign consecutive unicast
                                             *   addresses to all elements
                                             */
  MESH_PRV_ERR_RFU_START           = 0x09,  /*!< This value and all values above are RFU. */
};

/*! Mesh Provisioning PB-ADV Link Close reason types enumeration */
enum meshPrvBrReasonTypes
{
  MESH_PRV_BR_REASON_SUCCESS = 0x00, /*!< The provisioning was successful */
  MESH_PRV_BR_REASON_TIMEOUT = 0x01, /*!< The provisioning transaction timed out */
  MESH_PRV_BR_REASON_FAIL    = 0x02  /*!< The provisioning failed */
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_DEFS_H */
