/*************************************************************************************************/
/*!
 *  \file   mesh_defs.h
 *
 *  \brief  Common specification definitions.
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

/*! ***********************************************************************************************
 * @addtogroup MESH_CORE_API
 * @{
 *************************************************************************************************/

#ifndef MESH_DEFS_H
#define MESH_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Mesh Advertising Type for Advertising data containing PB-ADV PDU data */
#define MESH_AD_TYPE_PB                             0x29
/*! \brief Mesh Advertising Type for Advertising data containing NWK PDU data */
#define MESH_AD_TYPE_PACKET                         0x2A
/*! \brief Mesh Advertising Type for Advertising data containing beacon data */
#define MESH_AD_TYPE_BEACON                         0x2B

/*! \brief Unprovisioned Device beacon type */
#define MESH_BEACON_TYPE_UNPROV                     0x00
/*! \brief Secure Nwk Device beacon type */
#define MESH_BEACON_TYPE_SEC_NWK                    0x01

/*! \brief Mesh Application key identifier mask */
#define MESH_AID_MASK                               0x3F
/*! \brief Mesh Application key identifier shift value */
#define MESH_AID_SHIFT                              0
/*! \brief Mesh Application key identifier field size bits */
#define MESH_AID_SIZE                               6

/*! \brief Mesh Application key flag mask */
#define MESH_AKF_MASK                               0x40
/*! \brief Mesh Application key flag shift value */
#define MESH_AKF_SHIFT                              6
/*! \brief Mesh Application key flag field size bits */
#define MESH_AKF_SIZE                               1

/*! \brief Mesh SEG field mask */
#define MESH_SEG_MASK                               0x80
/*! \brief Mesh SEG field shift value */
#define MESH_SEG_SHIFT                              7
/*! \brief Mesh SEG field size bits */
#define MESH_SEG_SIZE                               1

/*! \brief Mesh Key index field size bits */
#define MESH_KEY_INDEX_SIZE                         12
/*! \brief Mesh AppKeyIndex maximum value */
#define MESH_APP_KEY_INDEX_MAX_VAL                  0x0FFF
/*! \brief Mesh NetKeyIndex maximum value */
#define MESH_NET_KEY_INDEX_MAX_VAL                  0x0FFF

/*! \brief Mesh Size of TransMIC field mask */
#define MESH_SZMIC_MASK                             0x80
/*! \brief Mesh Size of TransMIC field shift value */
#define MESH_SZMIC_SHIFT                            7
/*! \brief Mesh Size of TransMIC field size bits */
#define MESH_SZMIC_SIZE                             1

/*! \brief Mesh IVI field mask */
#define MESH_IVI_MASK                               0x80
/*! \brief Mesh IVI field shift value */
#define MESH_IVI_SHIFT                              7
/*! \brief Mesh IVI field size */
#define MESH_IVI_SIZE                               1
/*! \brief Mesh Time to Live field mask */
#define MESH_TTL_MASK                               0x7F
/*! \brief Mesh Time to Live field shift value */
#define MESH_TTL_SHIFT                              0
/*! \brief Mesh Time to Live field size bits */
#define MESH_TTL_SIZE                               7

/*! \brief Mesh Control flag mask */
#define MESH_CTL_MASK                               0x80
/*! \brief Mesh Control flag shift value */
#define MESH_CTL_SHIFT                              7
/*! \brief Mesh Time to Live flag field size bits */
#define MESH_CTL_SIZE                               1

/*! \brief Mesh Network identifier field mask */
#define MESH_NID_MASK                               0x7F
/*! \brief Mesh Network identifier field shift value */
#define MESH_NID_SHIFT                              0
/*! \brief Mesh Network identifier field size bits */
#define MESH_NID_SIZE                               7

/*! \brief Mesh IV Index bit mask */
#define MESH_IVI_MASK                               0x80
/*! \brief Mesh IV Index bit shift value */
#define MESH_IVI_SHIFT                              7
/*! \brief Mesh IV Index field size bits */
#define MESH_IVI_SIZE                               1

/*! \brief Mesh Control Opcode bit mask */
#define MESH_CTL_OPCODE_MASK                        0x7F
/*! \brief Mesh Contol Opcode shift value */
#define MESH_CTL_OPCODE_SHIFT                       0
/*! \brief Mesh Control Opcode field size bits */
#define MESH_CTL_OPCODE_SIZE                        7

/*! \brief Mesh OBO field bit mask */
#define MESH_OBO_MASK                               0x80
/*! \brief Mesh OBO field shift value */
#define MESH_OBO_SHIFT                              7
/*! \brief Mesh OBO field size bits */
#define MESH_OBO_SIZE                               1

/*! \brief Mesh SeqZero Upper field bit mask */
#define MESH_SEQ_ZERO_H_MASK                        0x7F
/*! \brief Mesh SeqZero Upper field shift value */
#define MESH_SEQ_ZERO_H_SHIFT                       0
/*! \brief Mesh SeqZero Upper field size bits */
#define MESH_SEQ_ZERO_H_SIZE                        7
/*! \brief Mesh SeqZero Upper field PDU offset */
#define MESH_SEQ_ZERO_H_PDU_OFFSET                  1

/*! \brief Mesh SeqZero Lower field bit mask */
#define MESH_SEQ_ZERO_L_MASK                        0xFC
/*! \brief Mesh SeqZero Lower field shift value */
#define MESH_SEQ_ZERO_L_SHIFT                       2
/*! \brief Mesh SeqZero Lower field size bits */
#define MESH_SEQ_ZERO_L_SIZE                        6
/*! \brief Mesh SeqZero Upper field PDU offset */
#define MESH_SEQ_ZERO_L_PDU_OFFSET                  2

/*! \brief Mesh SegO Upper field bit mask */
#define MESH_SEG_ZERO_H_MASK                        0x03
/*! \brief Mesh SegO Upper field shift value */
#define MESH_SEG_ZERO_H_SHIFT                       3
/*! \brief Mesh SegO Upper field PDU offset */
#define MESH_SEG_ZERO_H_PDU_OFFSET                  2
/*! \brief Mesh SegO Lower field bit mask */
#define MESH_SEG_ZERO_L_MASK                        0xE0
/*! \brief Mesh SegO Lower field shift value */
#define MESH_SEG_ZERO_L_SHIFT                       5
/*! \brief Mesh SegO Lower field PDU offset */
#define MESH_SEG_ZERO_L_PDU_OFFSET                  3

/*! \brief Mesh Sequence Number bit mask */
#define MESH_SEQ_ZERO_MASK                          0x001FFF

/*! \brief Mesh SegN field bit mask */
#define MESH_SEG_N_MASK                             0x1F
/*! \brief Mesh SegN field PDU offset */
#define MESH_SEG_N_PDU_OFFSET                       3

/*! \brief Mesh Segmented Acknowledgement Opcode */
#define MESH_SEG_ACK_OPCODE                         0x00
/*! \brief Mesh Opcode field PDU offset */
#define MESH_SEG_OPCODE_PDU_OFFSET                  0

/*! \brief Segment ACK length is 7 */
#define MESH_SEG_ACK_LENGTH                         7
/*! \brief Segment ACK PDU length is 6 (without Opcode) */
#define MESH_SEG_ACK_PDU_LENGTH                     6

/*! \brief Mesh Access payload maximum size */
#define MESH_ACC_MAX_PDU_SIZE                       380

/*! \brief Maximum number of octets in an ACC segment */
#define MESH_ACC_SEG_MAX_LENGTH                     12

/*! \brief Maximum number of octets in a CTL segment */
#define MESH_CTL_SEG_MAX_LENGTH                     8

/*! \brief Number of octets in the segmentation header */
#define MESH_SEG_HEADER_LENGTH                      4
/*! \brief Offset of Segment data in a Segnmented PDU */
#define MESH_SEG_DATA_PDU_OFFSET                    4

/*! \brief Mesh Sequence Number bit mask */
#define MESH_SEQ_MASK                               0x00FFFFFF
/*! \brief Mesh Sequence Number shift value */
#define MESH_SEQ_SHIFT                              0x00
/*! \brief Mesh Sequence Number maximum value */
#define MESH_SEQ_MAX_VAL                            0x00FFFFFF

/*! \brief Mesh IV index number of bytes */
#define MESH_IV_NUM_BYTES                           4
/*! \brief Mesh Sequence Number number of bytes */
#define MESH_SEQ_NUM_BYTES                          3
/*! \brief Mesh Address Number of bytes */
#define MESH_ADDR_NUM_BYTES                         2
/*! \brief Mesh Public Network Identifier number of bytes*/
#define MESH_NWK_ID_NUM_BYTES                       8


/*! \brief Mesh NetMic size in bytes for Control PDUs */
#define MESH_NETMIC_SIZE_CTL_PDU                    8
/*! \brief Mesh NetMic size in bytes for Proxy Config PDUs */
#define MESH_NETMIC_SIZE_PROXY_PDU                  8
/*! \brief Mesh NetMic size in bytes for Access PDUs */
#define MESH_NETMIC_SIZE_ACC_PDU                    4
/*! \brief Mesh Label UUID size */
#define MESH_LABEL_UUID_SIZE                        16
/*! \brief Mesh 128-bit Key size in bytes for Application, Network and Device Keys. */
#define MESH_KEY_SIZE_128                           16
/*! Mesh Profile Spec 3.4.6.5: The Network Message Cache shall be able to store at least two
 *  Network PDUs
 */
#define MESH_NWK_CACHE_MIN_SIZE                     2

/*! \brief The minimum number of replay protection list entries in a device */
#define MESH_RP_MIN_LIST_SIZE                       1

/*! \brief! PDU offset in advertising data structure */
#define MESH_ADV_PDU_POS                            2
/*! \brief! AD type offset in advertising data structure */
#define MESH_ADV_TYPE_POS                           1
/*! \brief Mesh IVI and NID byte position */
#define MESH_IVI_NID_POS                            0
/*! \brief Mesh CTL and TTL byte position */
#define MESH_CTL_TTL_POS                            1
/*! \brief Mesh SEQ number first(MSB) byte position */
#define MESH_SEQ_POS                                2
/*! \brief Mesh Source Address first(MSB) byte position */
#define MESH_SRC_ADDR_POS                           5
/*! \brief Mesh Destination Address first(MSB) byte position */
#define MESH_DST_ADDR_POS                           7

/*! \brief Unassigned address value */
#define MESH_ADDR_TYPE_UNASSIGNED                   0x0000
/*! \brief Unicast address format in binary is (MSB-LSB) 0b0xxxxxxxxxxxxxxxx except unassigned address */
#define MESH_ADDR_TYPE_UNICAST_MASK                 0x8000
/*! \brief Group or virtual address identification mask */
#define MESH_ADDR_TYPE_GROUP_VIRTUAL_MASK           0xC000
/*! \brief Mesh MSBits combination for a group address (0b11xxxxxxxxxxxxxx) */
#define MESH_ADDR_TYPE_GROUP_MSBITS_VALUE           0x03
/*! \brief Mesh MSBits combination for a virtual address (0b10xxxxxxxxxxxxxx) */
#define MESH_ADDR_TYPE_VIRTUAL_MSBITS_VALUE         0x02
/*! \brief Mesh Address Type fields shift. */
#define MESH_ADDR_TYPE_SHIFT                        14

/*! \brief All-proxies fixed group address  */
#define MESH_ADDR_GROUP_PROXY                       0xFFFC
/*! \brief All-friends fixed group address  */
#define MESH_ADDR_GROUP_FRIEND                      0xFFFD
/*! \brief All-relays fixed group address  */
#define MESH_ADDR_GROUP_RELAY                       0xFFFE
/*! \brief All-nodes fixed group address  */
#define MESH_ADDR_GROUP_ALL                         0xFFFF

/*! \brief Minimum GATT Proxy PDU size */
#define MESH_GATT_PROXY_PDU_MIN_VALUE               20
/*! \brief Minimum value for Mesh ADV PDU size */
#define MESH_ADV_IF_PDU_MIN_VALUE                   0x02
/*! \brief Maxium value for Mesh ADV PDU size */
#define MESH_ADV_IF_PDU_MAX_VALUE                   0x1F

/*! \brief Maximum number of steps for Model Publication Publish Period state */
#define MESH_PUBLISH_PERIOD_NUM_STEPS_MAX           64
/*! \brief Number of steps to disable Publish Period */
#define MESH_PUBLISH_PERIOD_DISABLED_NUM_STEPS      0
/*! \brief Maximum number of published message retransmissions */
#define MESH_PUBLISH_RETRANS_COUNT_MAX              7
/*! \brief Maximum number of 50 ms steps between retransmission of published messages */
#define MESH_PUBLISH_RETRANS_INTVL_STEPS_MAX        31
/*! \brief Maximum number of composition data pages */
#define MESH_COMP_DATA_PAGE_MAX                     255

/*! \brief Mesh Configuration Server Model ID. */
#define MESH_CFG_MDL_SR_MODEL_ID                    0x0000
/*! \brief Mesh Configuration Client Model ID. */
#define MESH_CFG_MDL_CL_MODEL_ID                    0x0001

/*! \brief Length of service data for advertising with Network ID */
#define MESH_PROXY_NWKID_SERVICE_DATA_SIZE          9
/*! \brief Length of service data for advertising with Node Identity */
#define MESH_PROXY_NODE_ID_SERVICE_DATA_SIZE        17
/*! \brief Maximum length of a Provisioning PDU */
#define MESH_PRV_MAX_PDU_LEN                        66

/*! \brief Maximum length of the network PDU is 29 Bytes */
#define MESH_NWK_MAX_PDU_LEN                        29
/*! \brief Minimum length of the network PDU is 14 bytes (9 Hdr + 1 Pld + 4 NetMic */
#define MESH_NWK_MIN_PDU_LEN                        14
/*! \brief Network header length is 9 bytes */
#define MESH_NWK_HEADER_LEN                         9
/*! \brief Mesh Secure Network Beacon number of bytes. */
#define MESH_NWK_BEACON_NUM_BYTES                   22
/*! \brief TTL value that triggers output filter to abort sending */
#define MESH_TX_TTL_FILTER_VALUE                    1

/*! \brief Size of the Public Key used for provisioning */
#define MESH_PRV_PUB_KEY_SIZE                       32

/*! Recommended minimum value for the random delay used in sending an Access message response */
#define MESH_ACC_RSP_MIN_SEND_DELAY_MS              20

/*! Recommended maximum value for the random delay used in sending an Access message response */
#define MESH_ACC_RSP_MAX_SEND_DELAY_MS(unicast)     ((unicast) ? 50 : 500)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Stack supported features bit masks. If bit is set, the feature is supported. */
enum meshFeaturesBitMaskValues
{
  MESH_FEAT_RELAY      = (1 << 0),  /*!< Relay feature support bit */
  MESH_FEAT_PROXY      = (1 << 1),  /*!< Proxy feature support bit */
  MESH_FEAT_FRIEND     = (1 << 2),  /*!< Friend feature support bit */
  MESH_FEAT_LOW_POWER  = (1 << 3),  /*!< Low power feature support bit */
  MESH_FEAT_RFU_START  = (1 << 4),  /*!< Start of the RFU bitmask. Other bits RFU */
};

/*! \brief Mesh Publish Period Step Resolution values enumeration */
enum meshPublishPeriodStepResValues
{
  MESH_PUBLISH_PERIOD_STEP_RES_100MS  = 0x00,  /*!< Publish Period step resolution is
                                                *   100 milliseconds
                                                */
  MESH_PUBLISH_PERIOD_STEP_RES_1S     = 0x01,  /*!< Publish Period step resolution is
                                                *   1 second
                                                */
  MESH_PUBLISH_PERIOD_STEP_RES_10S    = 0x02,  /*!< Publish Period step resolution is
                                                *   10 seconds
                                                */
  MESH_PUBLISH_PERIOD_STEP_RES_10MIN  = 0x03,  /*!< Publish Period step resolution is
                                                *   10 minutes
                                                */
};

/*! \brief Credentials used to publish messages from a model */
enum meshPublishCredValues
{
  MESH_PUBLISH_MASTER_SECURITY = 0x00,  /*!< Master security material is used for publishing */
  MESH_PUBLISH_FRIEND_SECURITY = 0x01,  /*!< Friendship security material is used for publishing */
};

/*! \brief Mesh Relay States values enumeration */
enum meshRelayStatesValues
{
  MESH_RELAY_FEATURE_DISABLED          = 0x00,  /*!< Relay feature is supported but disabled */
  MESH_RELAY_FEATURE_ENABLED           = 0x01,  /*!< Relay feature is supported and enabled */
  MESH_RELAY_FEATURE_NOT_SUPPORTED     = 0x02,  /*!< Relay feature is not supported */
  MESH_RELAY_FEATURE_PROHIBITED_START  = 0x03   /*!< Start of the Prohibited values */
};

/*! \brief Mesh Secure Network Beacon States values enumeration */
enum meshBeaconStatesValues
{
  MESH_BEACON_NOT_BROADCASTING          = 0x00,  /*!< The node is not broadcasting a Beacon */
  MESH_BEACON_BROADCASTING              = 0x01,  /*!< The node is broadcasting a Beacon */
  MESH_BEACON_PROHIBITED_START          = 0x02   /*!< Start of the Prohibited values. */
};

/*! \brief Mesh GATT Proxy states values enumeration */
enum meshGattProxyStatesValues
{
  MESH_GATT_PROXY_FEATURE_DISABLED         = 0x00,  /*!< GATT Proxy feature is supported but
                                                     *   disabled
                                                     */
  MESH_GATT_PROXY_FEATURE_ENABLED          = 0x01,  /*!< GATT Proxy feature is supported and
                                                     *   enabled
                                                     */
  MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED    = 0x02,  /*!< GATT Proxy feature is not supported */
  MESH_GATT_PROXY_FEATURE_PROHIBITED_START = 0x03   /*!< Start of the Prohibited values */
};

/*! \brief Mesh Node Identity states values enumeration */
enum meshNodeIdentityStatesValues
{
  MESH_NODE_IDENTITY_STOPPED           = 0x00,  /*!< Node Identity for a subnet is stopped */
  MESH_NODE_IDENTITY_RUNNING           = 0x01,  /*!< Node Identity for a subnet is running */
  MESH_NODE_IDENTITY_NOT_SUPPORTED     = 0x02,  /*!< Node Identity is not supported */
  MESH_NODE_IDENTITY_PROHIBITED_START  = 0x03   /*!< Start of the Prohibited values */
};

/*! \brief Mesh Friend States values enumeration */
enum meshFriendStatesValues
{
  MESH_FRIEND_FEATURE_DISABLED          = 0x00,  /*!< Friend feature is supported but disabled */
  MESH_FRIEND_FEATURE_ENABLED           = 0x01,  /*!< Friend feature is supported and enabled */
  MESH_FRIEND_FEATURE_NOT_SUPPORTED     = 0x02,  /*!< Friend feature is not supported */
  MESH_FRIEND_FEATURE_PROHIBITED_START  = 0x03   /*!< Start of the Prohibited values */
};

/*! \brief Mesh Low Power Node states values enumeration */
enum meshLowPowerStatesValues
{
  MESH_LOW_POWER_FEATURE_DISABLED         = 0x00,  /*!< Low Power feature is supported */
  MESH_LOW_POWER_FEATURE_ENABLED          = 0x01,  /*!< Low Power feature is not supported */
  MESH_LOW_POWER_FEATURE_PROHIBITED_START = 0x02,  /*!< Start of the Prohibited values */
};

/*! \brief Mesh Key Refresh Phase States values enumeration */
enum meshKeyRefreshStatesValues
{
  MESH_KEY_REFRESH_NOT_ACTIVE           = 0x00,  /*!< Normal operation. Key Refresh procedure is
                                                  *   not active
                                                  */
  MESH_KEY_REFRESH_FIRST_PHASE          = 0x01,  /*!< First phase of Key Refresh procedure */
  MESH_KEY_REFRESH_SECOND_PHASE         = 0x02,  /*!< Second phase of Key Refresh procedure */
  MESH_KEY_REFRESH_THIRD_PHASE          = 0x03,  /*!< Third phase of Key Refresh procedure */
  MESH_KEY_REFRESH_PROHIBITED_START     = 0x04   /*!< Start of the Prohibited values */
};

/*! Mesh Key Refresh transition values enumeration (Table 4.18)
 *  \note: Allowed values (old, transition, new): (0,3,0); (1,2,2); (1,3,0); (2,2,2); (2,3,0)
 */
enum meshKeyRefreshTransValues
{
  MESH_KEY_REFRESH_TRANS02  = 0x02, /*!< Transition 2 */
  MESH_KEY_REFRESH_TRANS03  = 0x03, /*!< Transition 3 */
};

/*! \brief Enumeration of the Health model standard fault values */
enum meshHtModelFaultValues
{
  MESH_HT_MODEL_FAULT_NO_FAULT                      = 0x00, /*!< No Fault */
  MESH_HT_MODEL_FAULT_LOW_BATTERY_WARN              = 0x01, /*!< Battery Low Warning */
  MESH_HT_MODEL_FAULT_LOW_BATTERY_ERR               = 0x02, /*!< Battery Low Error */
  MESH_HT_MODEL_FAULT_SUPPLY_VOLTAGE_LOW_WARN       = 0x03, /*!< Supply Voltage Too Low Warning */
  MESH_HT_MODEL_FAULT_SUPPLY_VOLTAGE_LOW_ERR        = 0x04, /*!< Supply Voltage Too Low Error */
  MESH_HT_MODEL_FAULT_SUPPLY_VOLTAGE_HIGH_WARN      = 0x05, /*!< Supply Voltage Too High Warning */
  MESH_HT_MODEL_FAULT_SUPPLY_VOLTAGE_HIGH_ERR       = 0x06, /*!< Supply Voltage Too High Error */
  MESH_HT_MODEL_FAULT_POWER_SUPPLY_INTERRUPTED_WARN = 0x07, /*!< Power Supply Interrupt Warning */
  MESH_HT_MODEL_FAULT_POWER_SUPPLY_INTERRUPTED_ERR  = 0x08, /*!< Power Supply Interrupt Error */
  MESH_HT_MODEL_FAULT_NO_LOAD_WARN                  = 0x09, /*!< No Load Warning */
  MESH_HT_MODEL_FAULT_NO_LOAD_ERR                   = 0x0A, /*!< No Load Error */
  MESH_HT_MODEL_FAULT_OVERLOAD_WARN                 = 0x0B, /*!< Overload Warning */
  MESH_HT_MODEL_FAULT_OVERLOAD_ERR                  = 0x0C, /*!< Overload Error */
  MESH_HT_MODEL_FAULT_OVERHEAT_WARN                 = 0x0D, /*!< Overheat Warning */
  MESH_HT_MODEL_FAULT_OVERHEAT_ERR                  = 0x0E, /*!< Overheat Error */
  MESH_HT_MODEL_FAULT_CONDENSATION_WARN             = 0x0F, /*!< Condensation Warning */
  MESH_HT_MODEL_FAULT_CONDENSATION_ERR              = 0x10, /*!< Condensation Error */
  MESH_HT_MODEL_FAULT_VIBRATION_WARN                = 0x11, /*!< Vibration Warning */
  MESH_HT_MODEL_FAULT_VIBRATION_ERR                 = 0x12, /*!< Vibration Error */
  MESH_HT_MODEL_FAULT_CONFIGURATION_WARN            = 0x13, /*!< Configuration Warning */
  MESH_HT_MODEL_FAULT_CONFIGURATION_ERR             = 0x14, /*!< Configuration Error */
  MESH_HT_MODEL_FAULT_NO_CALIBRATION_WARN           = 0x15, /*!< Element Not Calibrated Warning */
  MESH_HT_MODEL_FAULT_NO_CALIBRATION_ERR            = 0x16, /*!< Element Not Calibrated Error */
  MESH_HT_MODEL_FAULT_MEMORY_WARN                   = 0x17, /*!< Memory Warning */
  MESH_HT_MODEL_FAULT_MEMORY_ERR                    = 0x18, /*!< Memory Error */
  MESH_HT_MODEL_FAULT_SELFTEST_WARN                 = 0x19, /*!< Self-Test Warning */
  MESH_HT_MODEL_FAULT_SELFTEST_ERR                  = 0x1A, /*!< Self-Test Error */
  MESH_HT_MODEL_FAULT_INPUT_LOW_WARN                = 0x1B, /*!< Input Too Low Warning */
  MESH_HT_MODEL_FAULT_INPUT_LOW_ERR                 = 0x1C, /*!< Input Too Low Error */
  MESH_HT_MODEL_FAULT_INPUT_HIGH_WARN               = 0x1D, /*!< Input Too High Warning */
  MESH_HT_MODEL_FAULT_INPUT_HIGH_ERR                = 0x1E, /*!< Input Too High Error */
  MESH_HT_MODEL_FAULT_INPUT_NO_CHANGE_WARN          = 0x1F, /*!< Input No Change Warning */
  MESH_HT_MODEL_FAULT_INPUT_NO_CHANGE_ERR           = 0x20, /*!< Input No Change Error */
  MESH_HT_MODEL_FAULT_ACTUATOR_BLOCKED_WARN         = 0x21, /*!< Actuator Blocked Warning */
  MESH_HT_MODEL_FAULT_ACTUATOR_BLOCKED_ERR          = 0x22, /*!< Actuator Blocked Error */
  MESH_HT_MODEL_FAULT_HOUSING_OPENED_WARN           = 0x23, /*!< Housing Opened Warning */
  MESH_HT_MODEL_FAULT_HOUSING_OPENED_ERR            = 0x24, /*!< Housing Opened Error */
  MESH_HT_MODEL_FAULT_TAMPER_WARN                   = 0x25, /*!< Tamper Warning */
  MESH_HT_MODEL_FAULT_TAMPER_ERR                    = 0x26, /*!< Tamper Error */
  MESH_HT_MODEL_FAULT_DEVICE_MOVED_WARN             = 0x27, /*!< Device Moved Warning */
  MESH_HT_MODEL_FAULT_DEVICE_MOVED_ERR              = 0x28, /*!< Device Moved Error */
  MESH_HT_MODEL_FAULT_DEVICE_DROPPED_WARN           = 0x29, /*!< Device Dropped Warning */
  MESH_HT_MODEL_FAULT_DEVICE_DROPPED_ERR            = 0x2A, /*!< Device Dropped Error */
  MESH_HT_MODEL_FAULT_OVERFLOW_WARN                 = 0x2B, /*!< Overflow Warning */
  MESH_HT_MODEL_FAULT_OVERFLOW_ERR                  = 0x2C, /*!< Overflow Error */
  MESH_HT_MODEL_FAULT_EMPTY_WARN                    = 0x2D, /*!< Empty Warning */
  MESH_HT_MODEL_FAULT_EMPTY_ERR                     = 0x2E, /*!< Empty Error */
  MESH_HT_MODEL_FAULT_INTERNAL_BUS_WARN             = 0x2F, /*!< Internal Bus Warning */
  MESH_HT_MODEL_FAULT_INTERNAL_BUS_ERR              = 0x30, /*!< Internal Bus Error */
  MESH_HT_MODEL_FAULT_MECHANISM_JAMMED_WARN         = 0x31, /*!< Mechanism Jammed Warning */
  MESH_HT_MODEL_FAULT_MECHANISM_JAMMED_ERR          = 0x32, /*!< Mechanism Jammed Error */
  MESH_HT_MODEL_FAULT_VENDOR_BASE                   = 0x80, /*!< First vendor specific
                                                             *   warning or error
                                                             */
};

/*! \brief Enumeration of the Health model test identifiers */
enum meshHtModelTestIdValues
{
  MESH_HT_MODEL_TEST_ID_STANDARD     = 0x00, /*!< Standard test */
  MESH_HT_MODEL_TEST_ID_VENDOR_FIRST = 0x01, /*!< First vendor-specific test identifier */
  MESH_HT_MODEL_TEST_ID_VENDOR_LAST  = 0xFF  /*!< Last vendor-specific test identifier */
};

/*! \brief Proxy interface filter list types enumeration.*/
enum meshProxyFilterType
{
  MESH_PROXY_WHITE_LIST = 0,   /*!< Filter type is white list */
  MESH_PROXY_BLACK_LIST = 1,   /*!< Filter type is black list */
};

/*! \brief Proxy identification types enumeration.*/
enum meshProxyIdType
{
  MESH_PROXY_NWK_ID_TYPE  = 0,         /*!< Network ID type */
  MESH_PROXY_NODE_IDENTITY_TYPE = 1,   /*!< Node Identity type */
};

/*! \brief GATT Proxy PDU type values enumeration */
enum meshGattProxyPduTypes
{
  MESH_GATT_PROXY_PDU_TYPE_NETWORK_PDU   = 0x00,  /*!< Proxy PDU contains a Network PDU */
  MESH_GATT_PROXY_PDU_TYPE_BEACON        = 0x01,  /*!< Proxy PDU contains a Mesh Beacon */
  MESH_GATT_PROXY_PDU_TYPE_CONFIGURATION = 0x02,  /*!< Proxy PDU contains a Proxy Configuration */
  MESH_GATT_PROXY_PDU_TYPE_PROVISIONING  = 0x03   /*!< Proxy PDU contains a Provisioning PDU */
};

/*! \brief Mesh Friendship RSSI factor values */
enum meshFriendshipRssiFactorValues
{
  MESH_FRIEND_RSSI_FACTOR_1,
  MESH_FRIEND_RSSI_FACTOR_1_5,
  MESH_FRIEND_RSSI_FACTOR_2,
  MESH_FRIEND_RSSI_FACTOR_2_5
};

/*! \brief Mesh Friendship Receive Window factor values */
enum meshFriendshipRecvWinFactorValues
{
  MESH_FRIEND_RECV_WIN_FACTOR_1,
  MESH_FRIEND_RECV_WIN_FACTOR_1_5,
  MESH_FRIEND_RECV_WIN_FACTOR_2,
  MESH_FRIEND_RECV_WIN_FACTOR_2_5
};

/*! \brief Mesh Friendship Min Queue size log values */
enum meshFriendshipMinQueueSizeLogValues
{
  MESH_FRIEND_MIN_QUEUE_SIZE_PROHIBITED,
  MESH_FRIEND_MIN_QUEUE_SIZE_2,
  MESH_FRIEND_MIN_QUEUE_SIZE_4,
  MESH_FRIEND_MIN_QUEUE_SIZE_8,
  MESH_FRIEND_MIN_QUEUE_SIZE_16,
  MESH_FRIEND_MIN_QUEUE_SIZE_32,
  MESH_FRIEND_MIN_QUEUE_SIZE_64,
  MESH_FRIEND_MIN_QUEUE_SIZE_128
};

/*! \brief Mesh Friendship Receive Delay values */
enum meshFriendshipRecvDelayValues
{
  MESH_FRIEND_RECV_DELAY_MS_MIN = 0x0A,
  MESH_FRIEND_RECV_DELAY_MS_MAX = 0xFF
};

/*! \brief Mesh Friendship Receive Window values */
enum meshFriendshipRecvWinValues
{
  MESH_FRIEND_RECV_WIN_MS_MIN = 0x01,
  MESH_FRIEND_RECV_WIN_MS_MAX = 0xFF
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_DEFS_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
