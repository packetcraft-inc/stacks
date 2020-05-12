/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_defs.h
 *
 *  \brief  Configuration model definitions.
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

#ifndef MESH_CFG_MDL_DEFS_H
#define MESH_CFG_MDL_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Number of bytes for a key encoded as defined in Section 4.3.1.1 */
#define CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES                2
/*! Number of bytes for two keys encoded as defined in Section 4.3.1.1 */
#define CFG_MDL_MSG_2KEY_PACKED_NUM_BYTES                3

/*! Mesh Beacon state packed size */
#define CFG_MDL_MSG_BEACON_STATE_NUM_BYTES               1
/*! Beacon Get message parameters number of bytes */
#define CFG_MDL_MSG_BEACON_GET_NUM_BYTES                 0
/*! Beacon Set message parameters number of bytes */
#define CFG_MDL_MSG_BEACON_SET_NUM_BYTES                 CFG_MDL_MSG_BEACON_STATE_NUM_BYTES
/*! Beacon Status message parameters number of bytes */
#define CFG_MDL_MSG_BEACON_STATUS_NUM_BYTES              CFG_MDL_MSG_BEACON_STATE_NUM_BYTES


/*! Mesh Composition Data state packed size */
#define CFG_MDL_MSG_COMP_DATA_STATE_NUM_BYTES            1
/*! Mesh Composition Get message parameters number of bytes */
#define CFG_MDL_MSG_COMP_DATA_GET_NUM_BYTES              1
/*! Mesh Composition Data Status message parameters number of bytes */
#define CFG_MDL_MSG_COMP_DATA_STATUS_NUM_BYTES          CFG_MDL_MSG_COMP_DATA_STATE_NUM_BYTES
/*! Mesh Composition Data Page 0 number of bytes excluding element list. */
#define CFG_MDL_MSG_COMP_DATA_PG0_EMPTY_NUM_BYTES       10
/*! Mesh Composition Data Page 0 number of bytes of each element header (loc + nums + numv). */
#define CFG_MDL_MSG_COMP_DATA_PG0_ELEM_HDR_NUM_BYTES    4
/*! Mesh Composition Data Page 0  maximum number of bytes. */
#define CFG_MDL_MSG_COMP_DATA_PG0_MAX_NUM_BYTES         378


/*! Mesh Default TTL state packed size */
#define CFG_MDL_MSG_DEFAULT_TTL_STATE_NUM_BYTES          1
/*! Default TTL Get message parameters number of bytes */
#define CFG_MDL_MSG_DEFAULT_TTL_GET_NUM_BYTES            0
/*! Default TTL Set message parameters number of bytes */
#define CFG_MDL_MSG_DEFAULT_TTL_SET_NUM_BYTES            CFG_MDL_MSG_DEFAULT_TTL_STATE_NUM_BYTES
/*! Default TTL Status message parameters number of bytes */
#define CFG_MDL_MSG_DEFAULT_TTL_STATUS_NUM_BYTES         CFG_MDL_MSG_DEFAULT_TTL_STATE_NUM_BYTES


/*! Mesh Gatt Proxy state packed size */
#define CFG_MDL_MSG_GATT_PROXY_STATE_NUM_BYTES           1
/*! Gatt Proxy Get message parameters number of bytes */
#define CFG_MDL_MSG_GATT_PROXY_GET_NUM_BYTES             0
/*! Gatt Proxy Set message parameters number of bytes */
#define CFG_MDL_MSG_GATT_PROXY_SET_NUM_BYTES             CFG_MDL_MSG_GATT_PROXY_STATE_NUM_BYTES
/*! Gatt Proxy Status message parameters number of bytes */
#define CFG_MDL_MSG_GATT_PROXY_STATUS_NUM_BYTES          CFG_MDL_MSG_GATT_PROXY_STATE_NUM_BYTES


/*! Mesh Relay Composite state packed size */
#define CFG_MDL_MSG_RELAY_COMP_STATE_NUM_BYTES           2
/*! Mesh Relay Composite state, relay state offset in message */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RELAY_OFFSET        0
/*! Mesh Relay Composite state, relay retransmit state offset in message */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_OFFSET      1
/*! Mesh Relay Composite state, relay retransmit count mask */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_MASK    0x07
/*! Mesh Relay Composite state, relay retransmit count shift */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_SHIFT   0
/*! Mesh Relay Composite state, relay retransmit count size in bits */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_SIZE    3
/*! Mesh Relay Composite state, relay retransmit interval steps mask */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_MASK  0xF8
/*! Mesh Relay Composite state, relay retransmit interval steps shift */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_SHIFT 3
/*! Mesh Relay Composite state, relay retransmit interval steps size in bits */
#define CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_SIZE  5

/*! Relay Get message parameters number of bytes */
#define CFG_MDL_MSG_RELAY_GET_NUM_BYTES                  0
/*! Relay Set message parameters number of bytes */
#define CFG_MDL_MSG_RELAY_SET_NUM_BYTES                  CFG_MDL_MSG_RELAY_COMP_STATE_NUM_BYTES
/*! Relay Status message parameters number of bytes */
#define CFG_MDL_MSG_RELAY_STATUS_NUM_BYTES               CFG_MDL_MSG_RELAY_COMP_STATE_NUM_BYTES

/*! Model Publication publish Credential Flag mask */
#define CFG_MDL_MSG_MODEL_PUB_FRIEND_CRED_MASK           0x08
/*! Model Publication publish Credential Flag shift */
#define CFG_MDL_MSG_MODEL_PUB_FRIEND_CRED_SHIFT          4
/*! Model Publication publish Credential Flag size in bits */
#define CFG_MDL_MSG_MODEL_PUB_FRIEND_CRED_SIZE           1
/*! Model Publication publish RFU mask */
#define CFG_MDL_MSG_MODEL_PUB_RFU_MASK                   0xE0
/*! Model Publication publish RFU shift */
#define CFG_MDL_MSG_MODEL_PUB_RFU_SHIFT                  5
/*! Model Publication publish RFU size in bits */
#define CFG_MDL_MSG_MODEL_PUB_RFU_SIZE                   3
/*! Model Publication publish period number of steps mask */
#define CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_MASK      0x3F
/*! Model Publication publish period number of steps shift */
#define CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_SHIFT     0
/*! Model Publication publish period number of steps size in bits */
#define CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_SIZE      6
/*! Model Publication publish period step resolution mask */
#define CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_MASK       0xC0
/*! Model Publication publish period step resolution shift */
#define CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_SHIFT      6
/*! Model Publication publish period step resolution size in bits */
#define CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_SIZE       2
/*! Model Publication publish retransmit count mask */
#define CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_MASK           0x07
/*! Model Publication publish retransmit count shift */
#define CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_SHIFT          0
/*! Model Publication publish retransmit count size in bits */
#define CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_SIZE           3
/*! Model Publication publish retransmit steps mask */
#define CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_MASK         0xF8
/*! Model Publication publish retransmit steps shift */
#define CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_SHIFT        3
/*! Model Publication publish retransmit steps size in bits */
#define CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_SIZE         5
/*! Model Publication Get message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_PUB_GET_NUM_BYTES(isSig)       ((isSig) ? 4 : 6)
/*! Model Publication Set message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_PUB_SET_NUM_BYTES(isSig)       ((isSig) ? 11 : 13)
/*! Model Publication Virtual Set message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_PUB_VIRT_SET_NUM_BYTES(isSig)  ((isSig) ? 25 : 27)
/*! Model Publication Status message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_PUB_STATUS_NUM_BYTES(isSig)    ((isSig) ? 12 : 14)
/*! Model Publication Status message parameters maximum number of bytes */
#define CFG_MDL_MSG_MODEL_PUB_STATUS_MAX_NUM_BYTES       14

/*! Model Subscription Add message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_ADD_NUM_BYTES(isSig)       ((isSig) ? 6 : 8)
/*! Model Subscription Virtual Address Add message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_VIRT_ADD_NUM_BYTES(isSig)  ((isSig) ? 20 : 22)
/*! Model Subscription Delete message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_DEL_NUM_BYTES(isSig)       ((isSig) ? 6 : 8)
/*! Model Subscription Virtual Address Delete message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_VIRT_DEL_NUM_BYTES(isSig)  ((isSig) ? 20 : 22)
/*! Model Subscription Overwrite message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_OVR_NUM_BYTES(isSig)       ((isSig) ? 6 : 8)
/*! Model Subscription Virtual Address Overwrite message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_VIRT_OVR_NUM_BYTES(isSig)  ((isSig) ? 20 : 22)
/*! Model Subscription Delete All message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_DEL_ALL_NUM_BYTES(isSig)   ((isSig) ? 4 : 6)
/*! Model Subscription Status message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(isSig)    ((isSig) ? 7 : 9)
/*! Model Subscription Status message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_STATUS_MAX_NUM_BYTES       9
/*! SIG Model Subscription Get message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_SIG_GET_NUM_BYTES          4
/*! Vendor Model Subscription Get message parameters number of bytes */
#define CFG_MDL_MSG_MODEL_SUBSCR_VENDOR_GET_NUM_BYTES       6
/*! SIG/Vendor Model Subscription List message parameters number of bytes with empty address list */
#define CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig) ((isSig) ? 5 : 7)
/*! SIG/Vendor Model Subscription List message parameters maximum number of bytes with empty address
 *  list
 */
#define CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_MAX_NUM_BYTES   7
/*! SIG/Vendor Model Subscription List maximum number of addresses that fit a message. */
#define CFG_MDL_MSG_MODEL_SUBSCR_LIST_MAX_NUM_ADDR(isSig)   ((isSig) ? 187 : 186)

/*! Config NetKey Add number of bytes */
#define CFG_MDL_MSG_NETKEY_ADD_NUM_BYTES                 (CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES + \
                                                          MESH_KEY_SIZE_128)
/*! Config NetKey Update number of bytes */
#define CFG_MDL_MSG_NETKEY_UPDT_NUM_BYTES                CFG_MDL_MSG_NETKEY_ADD_NUM_BYTES
/*! Config NetKey Delete number of bytes */
#define CFG_MDL_MSG_NETKEY_DEL_NUM_BYTES                 CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES
/*! Config NetKey Status number of bytes */
#define CFG_MDL_MSG_NETKEY_STATUS_NUM_BYTES              (1 + CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES)
/*! Config NetKey Get number of bytes */
#define CFG_MDL_MSG_NETKEY_GET_NUM_BYTES                 0

/*! Extract number of NetKey Indexes from a Config NetKey List message. */
#define CFG_MDL_MSG_NETKEY_LIST_TO_NUM_NETKEY(size)      ((((size) / 3) << 1) + (((size) % 3) == 2))
/*! Calculates NetKey List number of bytes based on number of NetKeys. */
#define CFG_MDL_MSG_NETKEY_LIST_NUM_BYTES(numNetKey)     ((((numNetKey) >> 1) * 3) +\
                                                          (((numNetKey) & 0x01) << 1))
/*! Validates NetKey List size constraints. */
#define CFG_MDL_MSG_NETKEY_LIST_SIZE_VALID(size)         (((size) != 0) && (((size) % 3) != 1))

/*! Config AppKey Add/Update/Delete key binding number of bytes */
#define CFG_MDL_MSG_APPKEY_BIND_NUM_BYTES                CFG_MDL_MSG_2KEY_PACKED_NUM_BYTES
/*! Config AppKey Add number of bytes */
#define CFG_MDL_MSG_APPKEY_ADD_NUM_BYTES                 (CFG_MDL_MSG_APPKEY_BIND_NUM_BYTES + \
                                                          MESH_KEY_SIZE_128)
/*! Config AppKey Update number of bytes */
#define CFG_MDL_MSG_APPKEY_UPDT_NUM_BYTES                CFG_MDL_MSG_APPKEY_ADD_NUM_BYTES
/*! Config AppKey Delete number of bytes */
#define CFG_MDL_MSG_APPKEY_DEL_NUM_BYTES                 CFG_MDL_MSG_APPKEY_BIND_NUM_BYTES
/*! Config AppKey Status number of bytes */
#define CFG_MDL_MSG_APPKEY_STATUS_NUM_BYTES              (1 + CFG_MDL_MSG_APPKEY_BIND_NUM_BYTES)
/*! Config AppKey Get number of bytes */
#define CFG_MDL_MSG_APPKEY_GET_NUM_BYTES                 CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES
/*! Config AppKey List number of bytes for empty list. */
#define CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES          (1 + CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES)
/*! Extract number of AppKey Indexes from a Config AppKey List message. */
#define CFG_MDL_MSG_APPKEY_LIST_TO_NUM_APPKEY(size)      (((((size) - CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES) / 3) << 1) +\
                                                         ((((size) - CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES) % 3) == 2))
/*! Calculates AppKey List number of bytes based on number of AppKeys. */
#define CFG_MDL_MSG_APPKEY_LIST_NUM_BYTES(numAppKey)     ((((numAppKey) >> 1) * 3) +\
                                                           (((numAppKey) & 0x01) << 1) +\
                                                          CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES)
/*! Validates AppKey List size constraints. */
#define CFG_MDL_MSG_APPKEY_LIST_SIZE_VALID(size)         (((size) >= CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES)  &&\
                                                          ((((size) - CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES)  % 3) \
                                                           != 1))

/*! Config Node Identity Get message number of bytes */
#define CFG_MDL_MSG_NODE_IDENTITY_GET_NUM_BYTES          2
/*! Config Node Identity Set message number of bytes */
#define CFG_MDL_MSG_NODE_IDENTITY_SET_NUM_BYTES          3
/*! Config Node Identity Status message number of bytes */
#define CFG_MDL_MSG_NODE_IDENTITY_STATUS_NUM_BYTES       4

/*! Config Model App Bind/Unbind binding number of bytes. */
#define CFG_MDL_MSG_MODEL_APP_STATE_NUM_BYTES(isSig)     ((isSig) ? 6 : 8)
/*! Config Model App Bind number of bytes */
#define CFG_MDL_MSG_MODEL_APP_BIND_NUM_BYTES(isSig)      CFG_MDL_MSG_MODEL_APP_STATE_NUM_BYTES(isSig)
/*! Config Model App Unbind number of bytes */
#define CFG_MDL_MSG_MODEL_APP_UNBIND_NUM_BYTES(isSig)    CFG_MDL_MSG_MODEL_APP_STATE_NUM_BYTES(isSig)
/*! Config Model App Status number of bytes */
#define CFG_MDL_MSG_MODEL_APP_STATUS_NUM_BYTES(isSig)    (1 + CFG_MDL_MSG_MODEL_APP_STATE_NUM_BYTES(isSig))
/*! Config Model App Status when model type is vendor. */
#define CFG_MDL_MSG_MODEL_APP_STATUS_MAX_NUM_BYTES       (1 + 8)
/*! Config SIG/Vendor Model App Get number of bytes */
#define CFG_MDL_MSG_MODEL_APP_GET_NUM_BYTES(isSig)       ((isSig) ? 4 : 6)
/*! Config SIG/Vendor Model App List number of bytes with empty list */
#define CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig) ((isSig) ? 5 : 7)
/*! Config Vendor Model App List number of bytes with empty list */
#define CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_MAX_NUM_BYTES   7
/*! Validate Config SIG/Vendor Model App List size */
#define CFG_MDL_MSG_MODEL_APP_LIST_SIZE_VALID(isSig,size) (((size) >= CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig))  &&\
                                                          ((((size) - CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig))  % 3) \
                                                           != 1))
/*! Extract number of AppKey Indexes from a Config Model App List message based on model type and
 *  size.
 */
#define CFG_MDL_MSG_MODEL_APP_LIST_TO_NUM_APPKEY(isSig, size)  (((((size) - CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig)) / 3) << 1) +\
                                                               ((((size) - CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig)) % 3) == 2))
/*! Calculates Config Model App List number of bytes based on model type and number of AppKeys. */
#define CFG_MDL_MSG_MODEL_APP_LIST_NUM_BYTES(isSig, numAppKey) ((((numAppKey) >> 1) * 3) +\
                                                               (((numAppKey) & 0x01) << 1) +\
                                                               CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig))

/*! Mesh Node Reset state packed size */
#define CFG_MDL_MSG_NODE_RESET_STATE_NUM_BYTES           0
/*! Node Reset message parameters number of bytes */
#define CFG_MDL_MSG_NODE_RESET_NUM_BYTES                 CFG_MDL_MSG_NODE_RESET_STATE_NUM_BYTES
/*! Node Reset Status message parameters number of bytes */
#define CFG_MDL_MSG_NODE_RESET_STATUS_NUM_BYTES          CFG_MDL_MSG_NODE_RESET_STATE_NUM_BYTES

/*! Mesh Friend state packed size */
#define CFG_MDL_MSG_FRIEND_STATE_NUM_BYTES               1
/*! Friend Get message parameters number of bytes */
#define CFG_MDL_MSG_FRIEND_GET_NUM_BYTES                 0
/*! Friend Set message parameters number of bytes */
#define CFG_MDL_MSG_FRIEND_SET_NUM_BYTES                 CFG_MDL_MSG_FRIEND_STATE_NUM_BYTES
/*! Friend Status message parameters number of bytes */
#define CFG_MDL_MSG_FRIEND_STATUS_NUM_BYTES              CFG_MDL_MSG_FRIEND_STATE_NUM_BYTES

/*! Key Refresh Phase Get message parameters number of bytes */
#define CFG_MDL_MSG_KEY_REF_PHASE_GET_NUM_BYTES          2
/*! Key Refresh Phase Set message parameters number of bytes */
#define CFG_MDL_MSG_KEY_REF_PHASE_SET_NUM_BYTES          3
/*! Key Refresh Phase Status message parameters number of bytes */
#define CFG_MDL_MSG_KEY_REF_PHASE_STATUS_NUM_BYTES       4

/*! Mesh Heartbeat Publication state packed size */
#define CFG_MDL_MSG_HB_PUB_STATE_NUM_BYTES               9
/*! Mesh Heartbeat Publication Get message parameters number of bytes */
#define CFG_MDL_MSG_HB_PUB_GET_NUM_BYTES                 0
/*! Mesh Heartbeat Publication Set message parameters number of bytes */
#define CFG_MDL_MSG_HB_PUB_SET_NUM_BYTES                 CFG_MDL_MSG_HB_PUB_STATE_NUM_BYTES
/*! Mesh Heartbeat Publication Status message parameters number of bytes */
#define CFG_MDL_MSG_HB_PUB_STATUS_NUM_BYTES              (1 + CFG_MDL_MSG_HB_PUB_STATE_NUM_BYTES)
/*! Mesh Heartbeat Pub Count Log Not Allowed Start value */
#define CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_START         0x12
/*! Mesh Heartbeat Pub Count Log Not Allowed End value */
#define CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_END           0xFE
/*! Mesh Heartbeat Pub Period Log Not Allowed Start value */
#define CFG_MDL_HB_PUB_PERIOD_LOG_NOT_ALLOW_START        0x12
/*! Mesh Heartbeat Pub Period Log Not Allowed End value */
#define CFG_MDL_HB_PUB_PERIOD_LOG_NOT_ALLOW_END          0xFF
/*! Mesh Heartbeat Pub TTL Not Allowed Start value */
#define CFG_MDL_HB_PUB_TTL_NOT_ALLOW_START               0x80
/*! Mesh Heartbeat Pub TTL Not Allowed Start value */
#define CFG_MDL_HB_PUB_TTL_NOT_ALLOW_END                 0xFF

/*! Mesh Heartbeat Subscription state packed size */
#define CFG_MDL_MSG_HB_SUB_STATE_NUM_BYTES               8
/*! Mesh Heartbeat Subscription Get message parameters number of bytes */
#define CFG_MDL_MSG_HB_SUB_GET_NUM_BYTES                 0
/*! Mesh Heartbeat Subscription Set message parameters number of bytes */
#define CFG_MDL_MSG_HB_SUB_SET_NUM_BYTES                 5
/*! Mesh Heartbeat Subscription Status message parameters number of bytes */
#define CFG_MDL_MSG_HB_SUB_STATUS_NUM_BYTES              (1 + CFG_MDL_MSG_HB_SUB_STATE_NUM_BYTES)
/*! Mesh Heartbeat Subscription Period Log Not Allowed Start value */
#define CFG_MDL_HB_SUB_PERIOD_LOG_NOT_ALLOW_START        0x12
/*! Mesh Heartbeat Subscription Period Log Not Allowed End value */
#define CFG_MDL_HB_SUB_PERIOD_LOG_NOT_ALLOW_END          0xFF
/*! Mesh Heartbeat Subscription Count Log Not Allowed Start value */
#define CFG_MDL_HB_SUB_COUNT_LOG_NOT_ALLOW_START         0x12
/*! Mesh Heartbeat Subscription Count Log Not Allowed End value */
#define CFG_MDL_HB_SUB_COUNT_LOG_NOT_ALLOW_END           0xFE
/*! Mesh Heartbeat Subscription Min Hops Not Allowed Start value */
#define CFG_MDL_HB_SUB_MIN_HOPS_NOT_ALLOW_START          0x80
/*! Mesh Heartbeat Subscription Min Hops Not Allowed End value */
#define CFG_MDL_HB_SUB_MIN_HOPS_NOT_ALLOW_END            0xFF
/*! Mesh Heartbeat Subscription Max Hops Not Allowed Start value */
#define CFG_MDL_HB_SUB_MAX_HOPS_NOT_ALLOW_START          0x80
/*! Mesh Heartbeat Subscription Max Hops Not Allowed End value */
#define CFG_MDL_HB_SUB_MAX_HOPS_NOT_ALLOW_END            0xFF

/*! Config Low Power Node PollTimeout Get number of bytes. */
#define CFG_MDL_MSG_LPN_POLLTIMEOUT_GET_NUM_BYTES        2
/*! Config Low Power Node PollTimeout Get number of bytes. */
#define CFG_MDL_MSG_LPN_POLLTIMEOUT_STATUS_NUM_BYTES     5
/*! Validates PollTimeout timer value*/
#define CFG_MDL_MSG_LPN_POLLTIMEOUT_VALID(t)             (!(((t) >= 1) && (((t) <= 9))) ||\
                                                          ((t) >= 0x34BC00))

/*! Mesh Network Transmit state packed size */
#define CFG_MDL_MSG_NWK_TRANS_STATE_NUM_BYTES            1
/*! Mesh Relay Composite state, relay retransmit count mask */
#define CFG_MDL_MSG_NWK_TRANS_STATE_CNT_MASK             0x07
/*! Mesh Relay Composite state, relay retransmit count shift */
#define CFG_MDL_MSG_NWK_TRANS_STATE_CNT_SHIFT            0
/*! Mesh Relay Composite state, relay retransmit count size in bits */
#define CFG_MDL_MSG_NWK_TRANS_STATE_CNT_SIZE             3
/*! Mesh Relay Composite state, relay retransmit interval steps mask */
#define CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_MASK           0xF8
/*! Mesh Relay Composite state, relay retransmit interval steps shift */
#define CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_SHIFT          3
/*! Mesh Relay Composite state, relay retransmit interval steps size in bits */
#define CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_SIZE           5
/*! Network Transmit Get message parameters number of bytes */
#define CFG_MDL_MSG_NWK_TRANS_GET_NUM_BYTES              0
/*! Network Transmit Set message parameters number of bytes */
#define CFG_MDL_MSG_NWK_TRANS_SET_NUM_BYTES              CFG_MDL_MSG_NWK_TRANS_STATE_NUM_BYTES
/*! Network Transmit Status message parameters number of bytes */
#define CFG_MDL_MSG_NWK_TRANS_STATUS_NUM_BYTES           CFG_MDL_MSG_NWK_TRANS_STATE_NUM_BYTES

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_DEFS_H */
