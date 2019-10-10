/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 constant definitions.
 *
 *  Copyright (c) 2016-2018 Arm Ltd.
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
 */
/*************************************************************************************************/

#ifndef MAC_154_DEFS_H
#define MAC_154_DEFS_H

#include "wsf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  PHY definitions
**************************************************************************************************/

/*! Symbol duration in microseconds (2450 MHz O-QPSK). */
#define PHY_154_SYMBOL_DURATION         16

/*! ED duration in symbols (802.15.4-2006 6.9.7). */
#define PHY_154_ED_DURATION             8

/*! The number of symbols per octets for the current PHY (2450 MHz O-QPSK). */
#define PHY_154_SYMBOLS_PER_OCTET       2

/**************************************************************************************************
  Preamble field length
  802.15.4-2006 Table 19
**************************************************************************************************/

/*! Preamble field length in symbols (2450 MHz O-QPSK). */
#define PHY_154_PREAMBLE_FIELD_LENGTH     8

/*! SFD field length in symbols (2450 MHz O-QPSK). */
#define PHY_154_SFD_FIELD_LENGTH          2

/**************************************************************************************************
  PHY constants
  802.15.4-2006 Table 22
**************************************************************************************************/

/*! The maximum PSDU size (in octets) the PHY shall be able to receive. */
#define PHY_154_aMaxPHYPacketSize         127

/*! RX-to-TX or TX-to-RX maximum turnaround time (in symbol periods). */
#define PHY_154_aTurnaroundTime           12

/**************************************************************************************************
  PHY PIB attributes
  802.15.4-2006 Table 23
**************************************************************************************************/

/*! The maximum number of symbols in a frame (2450 MHz O-QPSK). */
#define PHY_154_phyMaxFrameDuration       (PHY_154_phySHRDuration + (PHY_154_aMaxPHYPacketSize + 1) * PHY_154_phySymbolsPerOctet)

/*! The duration of the synchronization header (SHR) in symbols for the current PHY (2450 MHz O-QPSK). */
#define PHY_154_phySHRDuration            (PHY_154_PREAMBLE_FIELD_LENGTH + PHY_154_SFD_FIELD_LENGTH)

/*! The number of symbols per octets for the current PHY (2450 MHz O-QPSK). */
#define PHY_154_phySymbolsPerOctet        2

/*! Channels supported - only 2.4GHz O-QPSK page 0 supported so one entry in array. */
#define PHY_154_phyChannelsSupported      0x07FFF800
#define PHY_154_INVALID_CHANNEL           0           /*!< Invalid channel. */
#define PHY_154_FIRST_CHANNEL             11          /*!< First channel.   */
#define PHY_154_LAST_CHANNEL              26          /*!< Last channel.    */

/**************************************************************************************************
  Minimum LIFS and SIFS period
  802.15.4-2006 Table 3
**************************************************************************************************/

/*! Minimum LIFS period in symbols. */
#define MAC_154_macMinLIFSPeriod          40

/*! Minimum SIFS period in symbols. */
#define MAC_154_macMinSIFSPeriod          12

/**************************************************************************************************
  MAC frame format
  802.15.4-2006 Section 7.2
**************************************************************************************************/

/*! Size of frame control in octets. */
#define MAC_154_FRAME_CONTROL_LEN         2

/*! Size of FCS in octets. */
#define MAC_154_FCS_LEN                   2

/*! FCS initialization value. */
#define MAC_154_FCS_INIT_VALUE            0x0000

/*! Frame control bits and fields. */
#define MAC_154_FC_FRAME_TYPE_SHIFT       0
#define MAC_154_FC_FRAME_TYPE_MASK        0x0007  /*!< Bitmask. */
#define MAC_154_FC_FRAME_TYPE_CD_MASK     0x0001  /*!< Convenient way of checking if MAC command or data. */
#define MAC_154_FC_FRAME_TYPE_CMD_MASK    0x0003  /*!< Convenient way of checking if MAC command. */
#define MAC_154_FC_FRAME_TYPE(x)          (((x) & MAC_154_FC_FRAME_TYPE_MASK)/*>> MAC_154_FC_FRAME_TYPE_SHIFT*/)  /*!< Optimized bit 0. */

#define MAC_154_FC_SECURITY_ENABLED_SHIFT 3
#define MAC_154_FC_SECURITY_ENABLED_MASK  0x0008
#define MAC_154_FC_SECURITY_ENABLED(x)    (((x) & MAC_154_FC_SECURITY_ENABLED_MASK) >> MAC_154_FC_SECURITY_ENABLED_SHIFT)

#define MAC_154_FC_FRAME_PENDING_SHIFT    4
#define MAC_154_FC_FRAME_PENDING_MASK     0x0010
#define MAC_154_FC_FRAME_PENDING(x)       (((x) & MAC_154_FC_FRAME_PENDING_MASK) >> MAC_154_FC_FRAME_PENDING_SHIFT)

#define MAC_154_FC_ACK_REQUEST_SHIFT      5
#define MAC_154_FC_ACK_REQUEST_MASK       0x0020
#define MAC_154_FC_ACK_REQUEST(x)         (((x) & MAC_154_FC_ACK_REQUEST_MASK) >> MAC_154_FC_ACK_REQUEST_SHIFT)

#define MAC_154_FC_PAN_ID_COMP_SHIFT      6
#define MAC_154_FC_PAN_ID_COMP_MASK       0x0040
#define MAC_154_FC_PAN_ID_COMP(x)         (((x) & MAC_154_FC_PAN_ID_COMP_MASK) >> MAC_154_FC_PAN_ID_COMP_SHIFT)

#define MAC_154_FC_DST_ADDR_MODE_SHIFT    10
#define MAC_154_FC_DST_ADDR_MODE_MASK     0x0C00
#define MAC_154_FC_DST_ADDR_MODE(x)       (((x) & MAC_154_FC_DST_ADDR_MODE_MASK) >> MAC_154_FC_DST_ADDR_MODE_SHIFT)

#define MAC_154_FC_FRAME_VERSION_SHIFT    12
#define MAC_154_FC_FRAME_VERSION_MASK     0x3000
#define MAC_154_FC_FRAME_VERSION(x)       (((x) & MAC_154_FC_FRAME_VERSION_MASK) >> MAC_154_FC_FRAME_VERSION_SHIFT)

#define MAC_154_FC_SRC_ADDR_MODE_SHIFT    14
#define MAC_154_FC_SRC_ADDR_MODE_MASK     0xC000
#define MAC_154_FC_SRC_ADDR_MODE(x)       (((x) & MAC_154_FC_SRC_ADDR_MODE_MASK) >> MAC_154_FC_SRC_ADDR_MODE_SHIFT)

/*! \brief Mask for checking legacy security */
#define MAC_154_FC_LEGACY_SEC_TEST(x)     (((x) & (MAC_154_FC_SECURITY_ENABLED_MASK | MAC_154_FC_FRAME_VERSION_MASK)) == MAC_154_FC_SECURITY_ENABLED_MASK)

/*! \brief Mask for checking frame pending processing based on ack. requested and being a MAC command frame */
#define MAC_154_FC_FRAME_TYPE_FP_TEST     (MAC_154_FC_ACK_REQUEST_MASK | MAC_154_FC_FRAME_TYPE_CMD_MASK)

/*! \brief Mask for checking ack. sending based on ack. requested and being a MAC command or data frame */
#define MAC_154_FC_FRAME_TYPE_ACK_TEST    (MAC_154_FC_ACK_REQUEST_MASK | MAC_154_FC_FRAME_TYPE_CD_MASK)

#define MAC_154_SCAN_MAX_PD_ENTRIES       16  /*!< Note - arbitrary number and could make it easy by sending all beacons using beacon notify indication. */
#define MAC_154_SCAN_MAX_ED_ENTRIES       16  /*!< Note - can't have more than 16 for 2.4GHz. */

/*! MAC enumerations, 802.15.4-2006 Table 78. */
typedef enum
{
  MAC_154_ENUM_SUCCESS = 0,             /*!< Success (0x00) */
  /* Gap */
  MAC_154_ENUM_COUNTER_ERROR = 0xdb,    /*!< Frame counter invalid (0xdb) */
  MAC_154_ENUM_IMPROPER_KEY_TYPE,       /*!< Key does not meet key usage policy (0xdc) */
  MAC_154_ENUM_IMPROPER_SECURITY_LEVEL, /*!< Security level does not meet minimum security level (0xdd) */
  MAC_154_ENUM_UNSUPPORTED_LEGACY,      /*!< Frame using 802.15.4-2003 security (0xde) */
  MAC_154_ENUM_UNSUPPORTED_SECURITY,    /*!< Security not supported (0xdf) */
  MAC_154_ENUM_BEACON_LOSS,             /*!< Beacon loss after synchronisation request (0xe0) */
  MAC_154_ENUM_CHANNEL_ACCESS_FAILURE,  /*!< CSMA/CA channel access failure (0xe1) */
  MAC_154_ENUM_DENIED,                  /*!< GTS request denied (0xe2) */
  MAC_154_ENUM_DISABLE_TRX_FAILURE,     /*!< Could not disable transmit or receive (0xe3) */
  /* Gap */
  MAC_154_ENUM_FRAME_TOO_LONG = 0xe5,   /*!< Frame too long after security processing to be sent (0xe5) */
  MAC_154_ENUM_INVALID_GTS,             /*!< GTS transmission failed (0xe6) */
  MAC_154_ENUM_INVALID_HANDLE,          /*!< Purge request failed to find entry in queue (0xe7) */
  MAC_154_ENUM_INVALID_PARAMETER,       /*!< Parameter is out of range in primitive (0xe8) */
  MAC_154_ENUM_NO_ACK,                  /*!< No acknowledgement received when expected (0xe9) */
  MAC_154_ENUM_NO_BEACON,               /*!< Scan failed to find any beacons (0xeA) */
  MAC_154_ENUM_NO_DATA,                 /*!< No response data after a data request (0xeB) */
  MAC_154_ENUM_NO_SHORT_ADDRESS,        /*!< No allocated short address for operation (0xeC) */
  MAC_154_ENUM_OUT_OF_CAP,              /*!< Receiver enable request could not be executed as CAP finished (0xed) */
  MAC_154_ENUM_PAN_ID_CONFLICT,         /*!< PAN ID conflict has been detected (0xee) */
  MAC_154_ENUM_REALIGNMENT,             /*!< Coordinator realignment has been received (0xef) */
  MAC_154_ENUM_TRANSACTION_EXPIRED,     /*!< Pending transaction has expired and data discarded (0xf0) */
  MAC_154_ENUM_TRANSACTION_OVERFLOW,    /*!< No capacity to store transaction (0xf1) */
  MAC_154_ENUM_TX_ACTIVE,               /*!< Receiver enable request could not be executed as in transmit state (0xf2) */
  MAC_154_ENUM_UNAVAILABLE_KEY,         /*!< Appropriate key is not available in ACL (0xf3) */
  MAC_154_ENUM_UNSUPPORTED_ATTRIBUTE,   /*!< PIB Set/Get on unsupported attribute (0xf4) */
  MAC_154_ENUM_INVALID_ADDRESS,         /*!< Frame does not have valid addresses (0xf5) */
  MAC_154_ENUM_ON_TIME_TOO_LONG,        /*!< Rx enable request longer than beacon interval (0xf6) */
  MAC_154_ENUM_PAST_TIME,               /*!< Rx enable request cannot be completed (0xf7) */
  MAC_154_ENUM_TRACKING_OFF,            /*!< Unable to send aligned beacons as not tracking coordinator (0xf8) */
  MAC_154_ENUM_INVALID_INDEX,           /*!< PIB table index out of range (0xf9) */
  MAC_154_ENUM_LIMIT_REACHED,           /*!< Number of PAN descriptors reached implementation maximum (0xfa) */
  MAC_154_ENUM_READ_ONLY,               /*!< Set request issued to read-only attribute (0xfb) */
  MAC_154_ENUM_SCAN_IN_PROGRESS,        /*!< Scan already in progress when scan request issued (0xfc) */
  MAC_154_ENUM_SUPERFRAME_OVERLAP,      /*!< Unable to to send aligned beacons due to overlap with coordinator (0xfd) */
} Mac154Enums_t;

/*! MAC PIB enumerations, 802.15.4-2006 Table 86. */
typedef enum
{
  MAC_154_PIB_ENUM_ACK_WAIT_DURATION = 0x40,              /*!< macAckWaitDuration */
  MAC_154_PIB_ENUM_ASSOCIATION_PERMIT = 0x41,             /*!< macAssociationPermit */
  MAC_154_PIB_ENUM_AUTO_REQUEST = 0x42,                   /*!< macAutoRequest */
  MAC_154_PIB_ENUM_BEACON_PAYLOAD = 0x45,                 /*!< macBeaconPayload */
  MAC_154_PIB_ENUM_BEACON_PAYLOAD_LENGTH = 0x46,          /*!< macBeaconPayloadLength */
  MAC_154_PIB_ENUM_BSN = 0x49,                            /*!< macBSN */
  MAC_154_PIB_ENUM_COORD_EXTENDED_ADDRESS = 0x4a,         /*!< macCoordExtendedAddress */
  MAC_154_PIB_ENUM_COORD_SHORT_ADDRESS = 0x4b,            /*!< macCoordShortAddress */
  MAC_154_PIB_ENUM_DSN = 0x4c,                            /*!< macDSN */
  MAC_154_PIB_ENUM_MAX_CSMA_BACKOFFS = 0x4e,              /*!< macMaxCSMABackoffs */
  MAC_154_PIB_ENUM_MIN_BE = 0x4f,                         /*!< macMinBE */
  MAC_154_PIB_ENUM_PAN_ID = 0x50,                         /*!< macPANId */
  MAC_154_PIB_ENUM_PROMISCUOUS_MODE = 0x51,               /*!< macPromiscuousMode */
  MAC_154_PIB_ENUM_RX_ON_WHEN_IDLE = 0x52,                /*!< macRxOnWhenIdle */
  MAC_154_PIB_ENUM_SHORT_ADDRESS = 0x53,                  /*!< macShortAddress */
  MAC_154_PIB_ENUM_TRANSACTION_PERSISTENCE_TIME = 0x55,   /*!< macTransactionPersistenceTime */
  MAC_154_PIB_ENUM_ASSOCIATED_PAN_COORD = 0x56,           /*!< macAssociatedPANCoord */
  MAC_154_PIB_ENUM_MAX_BE = 0x57,                         /*!< macMaxBE */
  MAC_154_PIB_ENUM_MAX_FRAME_TOTAL_WAIT_TIME = 0x58,      /*!< macMaxFrameTotalWaitTime */
  MAC_154_PIB_ENUM_MAX_FRAME_RETRIES = 0x59,              /*!< macMaxFrameRetries */
  MAC_154_PIB_ENUM_RESPONSE_WAIT_TIME = 0x5a,             /*!< macResponseWaitTime */
  MAC_154_PIB_ENUM_SECURITY_ENABLED = 0x5d                /*!< macSecurityEnabled */
} Mac154PibEnums_t;

#define MAC_154_PIB_ENUM_MIN            MAC_154_PIB_ENUM_ACK_WAIT_DURATION
#define MAC_154_PIB_ENUM_MAX            MAC_154_PIB_ENUM_SECURITY_ENABLED
#define MAC_154_PIB_ENUM_RANGE          (MAC_154_PIB_ENUM_MAX - MAC_154_PIB_ENUM_MIN + 1)

/*! MAC PIB vendor-specific enumerations. */
typedef enum
{
  MAC_154_PIB_VS_ENUM_EXT_ADDR = 0x80,                    /*!< aExtendedAddress - not a constant */
  MAC_154_PIB_VS_ENUM_DEVICE_TYPE = 0x81,                 /*!< macVsDeviceType - what sort of device this is */
  MAC_154_PIB_VS_ENUM_DISABLE_CCA = 0x82,                 /*!< macVsDisableCCA - Disable CCA */
  MAC_154_PIB_VS_ENUM_CRC_OVERRIDE = 0x83,                /*!< macVsCRCOverride - CRC override value */
  MAC_154_PIB_VS_ENUM_FCTL_OVERRIDE = 0x84,               /*!< macVsFctlOverride - Frame control override */
  MAC_154_PIB_VS_ENUM_RAW_RX = 0x85                       /*!< macVsRawRx - Raw frame receive */
} Mac154PibVsEnums_t;

#define MAC_154_PIB_VS_ENUM_MIN         MAC_154_PIB_VS_ENUM_EXT_ADDR
#define MAC_154_PIB_VS_ENUM_MAX         MAC_154_PIB_VS_ENUM_RAW_RX
#define MAC_154_PIB_VS_ENUM_RANGE       (MAC_154_PIB_VS_ENUM_MAX - MAC_154_PIB_VS_ENUM_MIN + 1)

/*! MAC PHY PIB enumerations. */
typedef enum
{
  MAC_154_PHY_PIB_ENUM_CHANNEL = 0x90,                    /*!< phyCurrentChannel */
  MAC_154_PHY_PIB_ENUM_TX_POWER = 0x91                    /*!< phyTransmitPower */
} Mac154PhyPibEnums_t;

#define MAC_154_PHY_PIB_ENUM_MIN         MAC_154_PHY_PIB_ENUM_CHANNEL
#define MAC_154_PHY_PIB_ENUM_MAX         MAC_154_PHY_PIB_ENUM_TX_POWER
#define MAC_154_PHY_PIB_ENUM_RANGE       (MAC_154_PHY_PIB_ENUM_MAX - MAC_154_PHY_PIB_ENUM_MIN + 1)

/*! Maximum PIB enumeration from all PIB attribute enums. */
#define MAC_154_ALL_PIB_ENUM_MAX         MAC_154_PHY_PIB_ENUM_MAX

/**************************************************************************************************
  MAC frame types
  802.15.4-2006 Table 79
**************************************************************************************************/

/*! Values of frame type in FC. */
typedef enum
{
  MAC_154_FRAME_TYPE_BEACON         = 0,
  MAC_154_FRAME_TYPE_DATA           = 1,
  MAC_154_FRAME_TYPE_ACKNOWLEDGMENT = 2,
  MAC_154_FRAME_TYPE_MAC_COMMAND    = 3,
  MAC_154_FRAME_TYPE_ILLEGAL4       = 4,
  MAC_154_FRAME_TYPE_ILLEGAL5       = 5,
  MAC_154_FRAME_TYPE_ILLEGAL6       = 6,
  MAC_154_FRAME_TYPE_ILLEGAL7       = 7
} Mac154FrameType_t;

/*! FCTL(2) + Seq(1) + SPID(2) + SADR(8) + SS(2) + GTS(1) + Pending(1) + beacon payload */
#define MAC_154_BCN_FRAME_LEN_HDR     17

/**************************************************************************************************
  MAC command types
  802.15.4-2006 Table 82
**************************************************************************************************/

/*! Values of MAC command type. */
typedef enum
{
  MAC_154_CMD_FRAME_TYPE_ASSOC_REQ      = 1,
  MAC_154_CMD_FRAME_TYPE_ASSOC_RSP      = 2,
  MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF   = 3,
  MAC_154_CMD_FRAME_TYPE_DATA_REQ       = 4,
  MAC_154_CMD_FRAME_TYPE_PANID_CNFL_NTF = 5,
  MAC_154_CMD_FRAME_TYPE_ORPHAN_NTF     = 6,
  MAC_154_CMD_FRAME_TYPE_BEACON_REQ     = 7,
  MAC_154_CMD_FRAME_TYPE_COORD_REALIGN  = 8,
  MAC_154_CMD_FRAME_TYPE_GTS_REQ        = 9
} Mac154CmdType_t;

/**************************************************************************************************
  MAC command max frame lengths
  802.15.4-2006 Table 82
**************************************************************************************************/

/*! FCTL(2) + SEQ(1) + DPID(dpidl) + DADR(dadrl) + SPID(spidl) + SADR(sadrl) + 1 */
#define MAC_154_CMD_FRAME_LEN_HDR(dpidl, dadrl, spidl, sadrl)  (2 + 1 + (dpidl) + (dadrl) + (spidl) + (sadrl) + 1)

#define MAC_154_CMD_FRAME_LEN_ASSOC_REQ_SHT       (MAC_154_CMD_FRAME_LEN_HDR(2,2,2,8) + 1)
#define MAC_154_CMD_FRAME_LEN_ASSOC_REQ_EXT       (MAC_154_CMD_FRAME_LEN_HDR(2,8,2,8) + 1)
#define MAC_154_CMD_FRAME_MAX_LEN_ASSOC_REQ       MAC_154_CMD_FRAME_LEN_ASSOC_REQ_EXT

#define MAC_154_CMD_FRAME_LEN_ASSOC_RSP           (MAC_154_CMD_FRAME_LEN_HDR(2,8,0,8) + 2 + 1)

#define MAC_154_CMD_FRAME_LEN_DISASSOC_NTF_SHT    (MAC_154_CMD_FRAME_LEN_HDR(2,2,0,8) + 1)
#define MAC_154_CMD_FRAME_LEN_DISASSOC_NTF_EXT    (MAC_154_CMD_FRAME_LEN_HDR(2,8,0,8) + 1)
#define MAC_154_CMD_FRAME_MAX_LEN_DISASSOC_NTF    MAC_154_CMD_FRAME_LEN_ASSOC_RSP_EXT

#define MAC_154_CMD_FRAME_LEN_DATA_REQ_SHT_SHT    (MAC_154_CMD_FRAME_LEN_HDR(2,2,0,2))
#define MAC_154_CMD_FRAME_LEN_DATA_REQ_SHT_EXT    (MAC_154_CMD_FRAME_LEN_HDR(2,2,0,8))
#define MAC_154_CMD_FRAME_LEN_DATA_REQ_EXT_SHT    (MAC_154_CMD_FRAME_LEN_HDR(2,8,0,2))
#define MAC_154_CMD_FRAME_LEN_DATA_REQ_EXT_EXT    (MAC_154_CMD_FRAME_LEN_HDR(2,8,0,8))
#define MAC_154_CMD_FRAME_MAX_LEN_DATA_REQ        MAC_154_CMD_FRAME_LEN_DATA_REQ_EXT_EXT

#define MAC_154_CMD_FRAME_LEN_ORPHAN_NTF          (MAC_154_CMD_FRAME_LEN_HDR(2,2,0,8))

#define MAC_154_CMD_FRAME_LEN_BEACON_REQ          (MAC_154_CMD_FRAME_LEN_HDR(2,2,0,0))

#define MAC_154_CMD_FRAME_LEN_COORD_REALIGN_SHT   (MAC_154_CMD_FRAME_LEN_HDR(2,2,2,8) + 2 + 2 + 1 + 2)
#define MAC_154_CMD_FRAME_LEN_COORD_REALIGN_EXT   (MAC_154_CMD_FRAME_LEN_HDR(2,8,2,8) + 2 + 2 + 1 + 2)

/**************************************************************************************************
  MAC addressing modes
  802.15.4-2006 Table 80
**************************************************************************************************/

/*! Addressing mode. */
typedef enum
{
  MAC_154_ADDR_MODE_NONE     = 0,
  MAC_154_ADDR_MODE_SHORT    = 2,
  MAC_154_ADDR_MODE_EXTENDED = 3
} Mac154AddrMode_t;

#define MAC_154_BROADCAST_PANID 0xffff
#define MAC_154_BROADCAST_ADDR 0xffff
#define MAC_154_UNASSIGNED_ADDR 0xffff
#define MAC_154_NO_SHT_ADDR 0xfffe
#define MAC_154_UNASSIGNED_PAN_ID 0xffff

#define MAC_154_SHORT_ADDR_LEN      2
#define MAC_154_EXTENDED_ADDR_LEN   8

/**************************************************************************************************
  MAC scan type
  802.15.4-2006 Table 67
**************************************************************************************************/
/*! \brief      Scan type. */
typedef enum
{
  MAC_154_MLME_SCAN_TYPE_ENERGY_DETECT  = 0,
  MAC_154_MLME_SCAN_TYPE_ACTIVE         = 1,
  MAC_154_MLME_SCAN_TYPE_PASSIVE        = 2,
  MAC_154_MLME_SCAN_TYPE_ORPHAN         = 3,
  MAC_154_MLME_NUM_SCAN_TYPE
} Mac154ScanType_t;

/**************************************************************************************************
  MAC association status
  802.15.4-2006 Table 83
**************************************************************************************************/
/*! \brief      Association status. */
typedef enum
{
  MAC_154_ASSOC_STATUS_SUCCESSFUL         = 0,
  MAC_154_ASSOC_STATUS_PAN_AT_CAPACITY    = 1,
  MAC_154_ASSOC_STATUS_PAN_ACCESS_DENIED  = 2
} Mac154AssocStatus_t;

/**************************************************************************************************
  MAC MCPS-DATA.req Tx options
  802.15.4-2006 Table 41
**************************************************************************************************/
/*! \brief      MCPS-DATA.req Tx options. */
typedef enum
{
  MAC_154_MCPS_TX_OPT_ACK                 = 0x1,
  MAC_154_MCPS_TX_OPT_GTS                 = 0x2,
  MAC_154_MCPS_TX_OPT_INDIRECT            = 0x4,
  MAC_154_MCPS_TX_OPT_VS_DISABLE_CCA      = 0x10
} Mac154McpsTxOpt_t;

/**************************************************************************************************
  MAC device type
  Vendor-specific
**************************************************************************************************/
/*! \brief      MAC device type. */
typedef enum
{
  MAC_154_DEV_TYPE_DEVICE,      /*!< Device is not a coordinator */
  MAC_154_DEV_TYPE_COORD,       /*!< Device is a coordinator but not a PAN coordinator */
  MAC_154_DEV_TYPE_PAN_COORD,   /*!< Device is a PAN coordinator */
} Mac154DevType_t;

/**************************************************************************************************
  MAC sublayer constants
  802.15.4-2006 Table 85
**************************************************************************************************/

/*! SHR duration for 802.15.4-2006. (2.4GHz PHY) */
#define MAC_154_phySHRDuration            10

/*! Symbols per octet for 802.15.4-2006. (2.4GHz PHY) */
#define MAC_154_phySymbolsPerOctet        2

/*! The number of symbols forming a superframe slot when the superframe order is equal to 0. */
#define MAC_154_aBaseSlotDuration         60

/*! The number of symbols forming a superframe when the superframe order is equal to 0. */
#define MAC_154_aBaseSuperframeDuration   (MAC_154_aBaseSlotDuration * MAC_154_aNumSuperframeSlots)

/* MAC_154_aExtendedAddress               Now a vendor-specific PIB attribute */

/*! The number of superframes in which a GTS descriptor exists in the beacon frame of the PAN
 *  coordinator. */
#define MAC_154_aGTSDescPersistenceTime   4

/*! The maximum number of octets added by the MAC sublayer to the MAC payload of a beacon frame. */
#define MAC_154_aMaxBeaconOverhead        75

/*! The maximum beacon payload length. */
#define MAC_154_aMaxBeaconPayloadLength   (PHY_154_aMaxPHYPacketSize - MAC_154_aMaxBeaconOverhead)

/*! The number of consecutive lost beacons that will cause the MAC sublayer of a receiving device
 *  to declare a loss of synchronization. */
#define MAC_154_aMaxLostBeacons           4

/*! The maximum number of octets added by the MAC sublayer to the PSDU without security. */
#define MAC_154_aMaxMPDUUnsecuredOverhead 25

/*! The maximum size of an MPDU, in octets, that can be followed by a SIFS period. */
#define MAC_154_aMaxSIFSFrameSize         18

/*! The minimum number of symbols forming the CAP. This ensures that MAC commands can still be
 *  transferred to devices when GTSs are being used. An exception to this minimum shall be allowed
 *  for the accommodation of the temporary increase in the beacon frame length needed to perform GTS
 *  maintenance. */
#define MAC_154_aMinCAPLength             440

/*! The minimum number of octets added by the MAC sublayer to the PSDU. */
#define MAC_154_aMinMPDUOverhead          9

/*! The number of slots contained in any superframe. */
#define MAC_154_aNumSuperframeSlots       16

/*! The number of symbols forming the basic time period used by the CSMA-CA algorithm. */
#define MAC_154_aUnitBackoffPeriod        20

#define MAC_154_RX_ACK_TIMEOUT_SYMB       MAC_154_aUnitBackoffPeriod + \
                                          PHY_154_aTurnaroundTime + \
                                          PHY_154_phySHRDuration + \
                                          (6 * PHY_154_phySymbolsPerOctet)
/*! PIB defaults */

/*! Ack. wait duration */
#define MAC_154_PIB_ACK_WAIT_DURATION_DEF               ((uint8_t)(MAC_154_aUnitBackoffPeriod + \
                                                                   PHY_154_aTurnaroundTime + \
                                                                   MAC_154_phySHRDuration + \
                                                                   (6 * MAC_154_phySymbolsPerOctet)))

/*! PAN coordinator default */
#define MAC_154_PIB_DEVICE_TYPE_DEF                     ((uint8_t)MAC_154_DEV_TYPE_DEVICE)

/*! Disable CCA default */
#define MAC_154_PIB_DISABLE_CCA_DEF                     ((bool_t)FALSE)

/*! Association permit default */
#define MAC_154_PIB_ASSOCIATION_PERMIT_DEF              ((bool_t)FALSE)

/*! Associated PAN coordinator default */
#define MAC_154_PIB_ASSOCIATED_PAN_COORD_DEF            ((bool_t)FALSE)

/*! Auto request default */
#define MAC_154_PIB_AUTO_REQUEST_DEF                    ((bool_t)TRUE)

/*! Coordinator short address minimum */
#define MAC_154_PIB_COORD_SHORT_ADDRESS_MIN             ((uint16_t)0x0000)
/*! Coordinator short address default */
#define MAC_154_PIB_COORD_SHORT_ADDRESS_DEF             ((uint16_t)0xFFFF)
/*! Coordinator short address maximum */
#define MAC_154_PIB_COORD_SHORT_ADDRESS_MAX             ((uint16_t)0xFFFF)

/*! PAN ID minimum */
#define MAC_154_PIB_PAN_ID_MIN                          ((uint16_t)0x0000)
/*! PAN ID default */
#define MAC_154_PIB_PAN_ID_DEF                          ((uint16_t)0xFFFF)
/*! PAN ID maximum */
#define MAC_154_PIB_PAN_ID_MAX                          ((uint16_t)0xFFFF)

/*! Short address minimum */
#define MAC_154_PIB_SHORT_ADDRESS_MIN                   ((uint16_t)0x0000)
/*! Short address default */
#define MAC_154_PIB_SHORT_ADDRESS_DEF                   ((uint16_t)0xFFFF)
/*! Short address maximum */
#define MAC_154_PIB_SHORT_ADDRESS_MAX                   ((uint16_t)0xFFFF)

/*! Transaction persistence time minimum */
#define MAC_154_PIB_TRANSACTION_PERSISTENCE_TIME_MIN    ((uint16_t)0x0000)
/*! Transaction persistence time default */
#define MAC_154_PIB_TRANSACTION_PERSISTENCE_TIME_DEF    ((uint16_t)0x01F4)
/*! Transaction persistence time maximum */
#define MAC_154_PIB_TRANSACTION_PERSISTENCE_TIME_MAX    ((uint16_t)0xFFFF)

/*! Minimum backoff exponent minimum */
#define MAC_154_PIB_MIN_BE_MIN                          ((uint8_t)0)
/*! Minimum backoff exponent default */
#define MAC_154_PIB_MIN_BE_DEF                          ((uint8_t)3)

/*! Maximum backoff exponent minimum */
#define MAC_154_PIB_MAX_BE_MIN                          ((uint8_t)3)
/*! Maximum backoff exponent default */
#define MAC_154_PIB_MAX_BE_DEF                          ((uint8_t)5)
/*! Maximum backoff exponent maximum */
#define MAC_154_PIB_MAX_BE_MAX                          ((uint8_t)8)

/*! Maximum CSMA backoffs minimum */
#define MAC_154_PIB_MAX_CSMA_BACKOFFS_MIN               ((uint8_t)0)
/*! Maximum CSMA backoffs default */
#define MAC_154_PIB_MAX_CSMA_BACKOFFS_DEF               ((uint8_t)4)
/*! Maximum CSMA backoffs maximum */
#define MAC_154_PIB_MAX_CSMA_BACKOFFS_MAX               ((uint8_t)5)

/*! Maximum frame retries minimum */
#define MAC_154_PIB_MAX_FRAME_RETRIES_MIN               ((uint8_t)0)
/*! Maximum frame retries default */
#define MAC_154_PIB_MAX_FRAME_RETRIES_DEF               ((uint8_t)3)
/*! Maximum frame retries maximum */
#define MAC_154_PIB_MAX_FRAME_RETRIES_MAX               ((uint8_t)7)

/*! Promiscuous mode default */
#define MAC_154_PIB_PROMISCUOUS_MODE_DEF                ((bool_t)FALSE)

/*! Receive on when idle default */
#define MAC_154_PIB_RX_ON_WHEN_IDLE_DEF                 ((bool_t)FALSE)

/*! Maximum frame total wait time minimum */
#define MAC_154_PIB_MAX_FRAME_TOTAL_WAIT_TIME_MIN       ((uint16_t)143)
/*! Maximum frame total wait time default */
#define MAC_154_PIB_MAX_FRAME_TOTAL_WAIT_TIME_DEF       ((uint16_t)1220)
/*! Maximum frame total wait time maximum */
#define MAC_154_PIB_MAX_FRAME_TOTAL_WAIT_TIME_MAX       ((uint16_t)25776)

/*! Response wait time minimum */
#define MAC_154_PIB_RESPONSE_WAIT_TIME_MIN              ((uint8_t)2)
/*! Response wait time default */
//#define MAC_154_PIB_RESPONSE_WAIT_TIME_DEF              ((uint8_t)32)
#define MAC_154_PIB_RESPONSE_WAIT_TIME_DEF              ((uint32_t)1000000/16) /* Test - make much larger due to test harness response */
/*! Response wait time maximum */
#define MAC_154_PIB_RESPONSE_WAIT_TIME_MAX              ((uint8_t)64)

/*! Security enabled default */
#define MAC_154_PIB_SECURITY_ENABLED_DEF                ((bool_t)FALSE)

/*! Security material: Key length in words */
#define MAC_154_KEY_LEN_WORDS           4
/*! Security material: Key length in octets */
#define MAC_154_KEY_LEN_OCTETS          16
/*! Security material: Frame counter length in words */
#define MAC_154_FRAME_COUNTER_WORDS     1
/*! Security material: Frame counter length in octets */
#define MAC_154_FRAME_COUNTER_OCTETS    4

/**************************************************************************************************
  MAC security control field
  802.15.4-2006 Figure 75
**************************************************************************************************/
/*! Security control bits and fields. */
#define MAC_154_SECURITY_CONTROL_SHIFT_SECURITY_LEVEL       0
#define MAC_154_SECURITY_CONTROL_SHIFT_KEY_IDENTIFIER_MODE  3

/**************************************************************************************************
  MAC security levels
  802.15.4-2006 Table 95
**************************************************************************************************/

/*! Security levels. */
typedef enum
{
  MAC_154_SECURITY_LEVEL_NONE_OFF   = 0,
  MAC_154_SECURITY_LEVEL_MIC32_OFF  = 1,
  MAC_154_SECURITY_LEVEL_MIC64_OFF  = 2,
  MAC_154_SECURITY_LEVEL_MIC128_OFF = 3,
  MAC_154_SECURITY_LEVEL_NONE_ON    = 4,
  MAC_154_SECURITY_LEVEL_MIC32_ON   = 5,
  MAC_154_SECURITY_LEVEL_MIC64_ON   = 6,
  MAC_154_SECURITY_LEVEL_MIC128_ON  = 7
} Mac154SecLvl_t;

/**************************************************************************************************
  Key identifier modes
  802.15.4-2006 Table 96
**************************************************************************************************/

/*! Key identifier modes. */
typedef enum
{
  MAC_154_KEY_IDENTIFIER_MODE_IMPLICIT = 0,
  MAC_154_KEY_IDENTIFIER_MODE_INDEX    = 1,
  MAC_154_KEY_IDENTIFIER_MODE_SOURCE32 = 2,
  MAC_154_KEY_IDENTIFIER_MODE_SOURCE64 = 3
} Mac154KeyIdMode_t;

/* Typedefs for unsigned int values larger than one byte represented as unsigned byte arrays */
typedef uint8_t uint16a_t[2];
typedef uint8_t uint24a_t[3];
typedef uint8_t uint64a_t[8];

/* Define structures as packed if necessary. */
#if defined (__GNUC__)
# define MAC_154_PACKED                __attribute__ ((packed))
#elif defined (__CC_ARM)
# define MAC_154_PACKED                __attribute__ ((packed))
#elif defined (WIN32)
# define MAC_154_PACKED
#else
# error "Not supported compiler type."
#endif

/* These structures are held as contiguous bytes so they can be simply copied in the CHCI */
typedef struct MAC_154_PACKED     /* TODO don't introduce run-time overhead for CHCI simplicity */
{
  uint8_t       addrMode;         /*!< Address mode (short or extended) */
  uint16a_t     panId;            /*!< PAN ID */
  uint64a_t     addr;             /*!< Short or extended address (short address occupies first two bytes) */
} Mac154Addr_t;

/* Explicit size due to possible trailing packing */
#define MAC_154_SIZEOF_ADDR_T     (1 + 2 + 8)

typedef struct
{
  Mac154Addr_t  coord;            /*|< Coordinator address from where beacon originates */
  uint8_t       logicalChan;      /*|< Logical channel */
  uint16a_t     superframeSpec;   /*!< Superframe specification */
  uint8_t       gtsPermit;        /*!< GTS permit */
  uint8_t       linkQuality;      /*<! Link quality of beacon */
  uint24a_t     timestamp;        /*<! Timestamp of incoming beacon */
  uint8_t       securityFailure;  /*<! Whether incoming beacon failed security check */
  /* TODO security */
} Mac154PanDescr_t;

/*! \brief      Coordinator realignment parameters. */
typedef struct {
  uint16_t panId;
  uint16_t coordShtAddr;
  uint16_t shtAddr;
  uint8_t logChan;
} Mac154CoordRealign_t;

typedef union
{
  Mac154PanDescr_t     panDescr[MAC_154_SCAN_MAX_PD_ENTRIES];   /*!< PAN descriptors. */
  int8_t               edList[MAC_154_SCAN_MAX_ED_ENTRIES];     /*! ED scan results */
  Mac154CoordRealign_t coordRealign;                            /*! Coordinator realignment results */
} Mac154ScanResults_t;

typedef struct MAC_154_PACKED
{
  uint8_t   msduHandle; /*!< Transmit frame handle */
  uint8_t   status;     /*!< Transmit status */
  uint24a_t timestamp;  /*!< Transmit timestamp */
} Mac154DataTxCfm_t;

typedef struct MAC_154_PACKED
{
  uint8_t   msduHandle; /*!< Transmit frame handle */
  uint8_t   status;     /*!< Transmit status */
} Mac154DataPurgeCfm_t;

typedef struct MAC_154_PACKED
{
  uint8_t   status;     /*!< Transmit status */
} Mac154DataPollCfm_t;

typedef struct MAC_154_PACKED
{
  Mac154Addr_t  srcAddr;          /*!< Source address */
  uint8_t       dataFrameSent;    /*!< Data frame sent flag */
} Mac154DataPollInd_t;

/* Explicit size due to possible trailing packing */
#define MAC_154_SIZEOF_DATA_RX_IND_T     (2 * MAC_154_SIZEOF_ADDR_T + 1 + 1 + 1)

#ifdef __cplusplus
};
#endif

#endif /* MAC_154_DEFS_H */
