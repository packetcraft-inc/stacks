/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl.h
 *
 *  \brief  Configuration Model internal module interface.
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

#ifndef MESH_CFG_MDL_H
#define MESH_CFG_MDL_H

#include "wsf_msg.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Operation identifiers for Configuration Client.
 *  \remarks The Operation IDs MUST match the array indexes in the opcode array and action table
 */
enum meshCfgMdlClOpIds
{
  MESH_CFG_MDL_CL_BEACON_GET = 0,          /*!< Beacon Get */
  MESH_CFG_MDL_CL_BEACON_SET,              /*!< Beacon Set */
  MESH_CFG_MDL_CL_COMP_DATA_GET,           /*!< Composition Data Get */
  MESH_CFG_MDL_CL_DEFAULT_TTL_GET,         /*!< Default TTL Get */
  MESH_CFG_MDL_CL_DEFAULT_TTL_SET,         /*!< Default TTL Set */
  MESH_CFG_MDL_CL_GATT_PROXY_GET,          /*!< Gatt Proxy Get */
  MESH_CFG_MDL_CL_GATT_PROXY_SET,          /*!< Gatt Proxy Set */
  MESH_CFG_MDL_CL_RELAY_GET,               /*!< Relay Get */
  MESH_CFG_MDL_CL_RELAY_SET,               /*!< Relay Set */
  MESH_CFG_MDL_CL_MODEL_PUB_GET,           /*!< Model Publication Get */
  MESH_CFG_MDL_CL_MODEL_PUB_SET,           /*!< Model Publication Set */
  MESH_CFG_MDL_CL_MODEL_PUB_VIRT_SET,      /*!< Model Publication Virtual Address Set*/
  MESH_CFG_MDL_CL_MODEL_SUBSCR_ADD,        /*!< Model Subscription Add */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_ADD,   /*!< Model Subscription Virtual Address Add */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_DEL,        /*!< Model Subscription Delete */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_DEL,   /*!< Model Subscription Virtual Address Delete */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_OVR,        /*!< Model Subscription Overwrite */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_OVR,   /*!< Model Subscription Virtual Address Overwrite */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_DEL_ALL,    /*!< Model Subscription Delete All */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_SIG_GET,    /*!< SIG Model Subscription Get */
  MESH_CFG_MDL_CL_MODEL_SUBSCR_VENDOR_GET, /*!< Vendor Model Subscription Get */
  MESH_CFG_MDL_CL_NETKEY_ADD,              /*!< Config NetKey Add */
  MESH_CFG_MDL_CL_NETKEY_UPDT,             /*!< Config NetKey Update */
  MESH_CFG_MDL_CL_NETKEY_DEL,              /*!< Config NetKey Delete */
  MESH_CFG_MDL_CL_NETKEY_GET,              /*!< Config NetKey Get */
  MESH_CFG_MDL_CL_APPKEY_ADD,              /*!< Config AppKey Add */
  MESH_CFG_MDL_CL_APPKEY_UPDT,             /*!< Config AppKey Update */
  MESH_CFG_MDL_CL_APPKEY_DEL,              /*!< Config AppKey Delete */
  MESH_CFG_MDL_CL_APPKEY_GET,              /*!< Config AppKey Get */
  MESH_CFG_MDL_CL_NODE_IDENTITY_GET,       /*!< Config Node Identity Get */
  MESH_CFG_MDL_CL_NODE_IDENTITY_SET,       /*!< Config Node Identity Get */
  MESH_CFG_MDL_CL_MODEL_APP_BIND,          /*!< Config Model App Bind */
  MESH_CFG_MDL_CL_MODEL_APP_UNBIND,        /*!< Config Model App Unbind */
  MESH_CFG_MDL_CL_MODEL_APP_SIG_GET,       /*!< Config SIG Model App Get */
  MESH_CFG_MDL_CL_MODEL_APP_VENDOR_GET,    /*!< Config Vendor Model App Get */
  MESH_CFG_MDL_CL_NODE_RESET,              /*!< Node Reset */
  MESH_CFG_MDL_CL_FRIEND_GET,              /*!< Friend Get */
  MESH_CFG_MDL_CL_FRIEND_SET,              /*!< Friend Set */
  MESH_CFG_MDL_CL_KEY_REF_PHASE_GET,       /*!< Key Refresh Phase Get */
  MESH_CFG_MDL_CL_KEY_REF_PHASE_SET,       /*!< Key Refresh Phase Set */
  MESH_CFG_MDL_CL_HB_PUB_GET,              /*!< Hearbeat Publication Get */
  MESH_CFG_MDL_CL_HB_PUB_SET,              /*!< Hearbeat Publication Set */
  MESH_CFG_MDL_CL_HB_SUB_GET,              /*!< Hearbeat Subscription Get */
  MESH_CFG_MDL_CL_HB_SUB_SET,              /*!< Hearbeat Subscription Set */
  MESH_CFG_MDL_CL_LPN_PT_GET,              /*!< Low Power Node PollTimeout Get */
  MESH_CFG_MDL_CL_NWK_TRANS_GET,           /*!< Network Transmit Get */
  MESH_CFG_MDL_CL_NWK_TRANS_SET,           /*!< Network Transmit Set */
  MESH_CFG_MDL_CL_MAX_OP,                  /*!< Maximum operation code. Used for determining number of
                                        *   elements
                                        */
};

/*! Operation identifiers for Configuration Server.
 *  \remarks The Operation IDs MUST match the array indexes in the opcode array and callback table
 */
enum meshCfgMdlSrOpIds
{
  MESH_CFG_MDL_SR_BEACON_STATUS = 0,        /*!< Beacon Status */
  MESH_CFG_MDL_SR_COMP_DATA_STATUS,         /*!< Composition Data Status */
  MESH_CFG_MDL_SR_DEFAULT_TTL_STATUS,       /*!< Default TTL Status */
  MESH_CFG_MDL_SR_GATT_PROXY_STATUS,        /*!< Gatt Proxy Status */
  MESH_CFG_MDL_SR_RELAY_STATUS,             /*!< Relay Status */
  MESH_CFG_MDL_SR_MODEL_PUB_STATUS,         /*!< Model Publication Status */
  MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS,      /*!< Model Subscription Status */
  MESH_CFG_MDL_SR_MODEL_SUBSCR_SIG_LIST,    /*!< SIG Model Subscription List */
  MESH_CFG_MDL_SR_MODEL_SUBSCR_VENDOR_LIST, /*!< Vendor Model Subscription List */
  MESH_CFG_MDL_SR_NETKEY_STATUS,            /*!< Config NetKey Status */
  MESH_CFG_MDL_SR_NETKEY_LIST,              /*!< Config NetKey List */
  MESH_CFG_MDL_SR_APPKEY_STATUS,            /*!< Config AppKey Status */
  MESH_CFG_MDL_SR_APPKEY_LIST,              /*!< Config AppKey List */
  MESH_CFG_MDL_SR_NODE_IDENTITY_STATUS,     /*!< Config Node identity Status */
  MESH_CFG_MDL_SR_MODEL_APP_STATUS,         /*!< Config Model App Status */
  MESH_CFG_MDL_SR_MODEL_APP_SIG_LIST,       /*!< Config SIG Model App List */
  MESH_CFG_MDL_SR_MODEL_APP_VENDOR_LIST,    /*!< Config Vendor Model App List */
  MESH_CFG_MDL_SR_NODE_RESET_STATUS,        /*!< Node Reset Status */
  MESH_CFG_MDL_SR_FRIEND_STATUS,            /*!< Friend Status */
  MESH_CFG_MDL_SR_KEY_REF_PHASE_STATUS,     /*!< Key Refresh Phase Status */
  MESH_CFG_MDL_SR_HB_PUB_STATUS,            /*!< Heartbeat Publication Status */
  MESH_CFG_MDL_SR_HB_SUB_STATUS,            /*!< Heartbeat Subscription Status */
  MESH_CFG_MDL_SR_LPN_PT_STATUS,            /*!< Low Power Node PollTimeout Status */
  MESH_CFG_MDL_SR_NWK_TRANS_STATUS,         /*!< Network Transmit Status */
  MESH_CFG_MDL_SR_MAX_OP,                   /*!< Maximum operation code. Used for determining number of
                                         *   elements
                                         */
};

/*! Mesh Configuration Client operation ID. See ::meshCfgMdlClOpIds */
typedef uint8_t meshCfgMdlClOpId_t;

/*! Mesh Configuration Server operation ID. See ::meshCfgMdlSrOpIds */
typedef uint8_t meshCfgMdlSrOpId_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/
extern const meshMsgOpcode_t meshCfgMdlSrOpcodes[];
extern const meshMsgOpcode_t meshCfgMdlClOpcodes[];

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_H */
