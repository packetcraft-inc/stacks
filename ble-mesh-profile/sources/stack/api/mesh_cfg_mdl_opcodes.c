/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_opcodes.c
 *
 *  \brief  Configuration opcode definitions.
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

#include "wsf_types.h"

#include "mesh_types.h"
#include "mesh_cfg_mdl.h"

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Configuration Client Opcodes in sync with operation id's */
const meshMsgOpcode_t meshCfgMdlClOpcodes[MESH_CFG_MDL_CL_MAX_OP] =
{
  { { 0x80, 0x09, 0x00 } }, /*!< MESH_CFG_MDL_CL_BEACON_GET */
  { { 0x80, 0x0A, 0x00 } }, /*!< MESH_CFG_MDL_CL_BEACON_SET */
  { { 0x80, 0x08, 0x00 } }, /*!< MESH_CFG_MDL_CL_COMP_DATA_GET */
  { { 0x80, 0x0C, 0x00 } }, /*!< MESH_CFG_MDL_CL_DEFAULT_TTL_GET */
  { { 0x80, 0x0D, 0x00 } }, /*!< MESH_CFG_MDL_CL_DEFAULT_TTL_SET */
  { { 0x80, 0x12, 0x00 } }, /*!< MESH_CFG_MDL_CL_GATT_PROXY_GET */
  { { 0x80, 0x13, 0x00 } }, /*!< MESH_CFG_MDL_CL_GATT_PROXY_SET */
  { { 0x80, 0x26, 0x00 } }, /*!< MESH_CFG_MDL_CL_RELAY_GET */
  { { 0x80, 0x27, 0x00 } }, /*!< MESH_CFG_MDL_CL_RELAY_SET */
  { { 0x80, 0x18, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_PUB_GET */
  { { 0x03, 0x00, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_PUB_SET */
  { { 0x80, 0x1A, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_PUB_VIRT_SET */
  { { 0x80, 0x1B, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_ADD */
  { { 0x80, 0x20, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_ADD */
  { { 0x80, 0x1C, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_DEL */
  { { 0x80, 0x21, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_DEL */
  { { 0x80, 0x1E, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_OVR */
  { { 0x80, 0x22, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_OVR */
  { { 0x80, 0x1D, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_DEL_ALL */
  { { 0x80, 0x29, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_SIG_GET */
  { { 0x80, 0x2B, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_SUBSCR_VENDOR_GET */
  { { 0x80, 0x40, 0x00 } }, /*!< MESH_CFG_MDL_CL_NETKEY_ADD */
  { { 0x80, 0x45, 0x00 } }, /*!< MESH_CFG_MDL_CL_NETKEY_UPDT */
  { { 0x80, 0x41, 0x00 } }, /*!< MESH_CFG_MDL_CL_NETKEY_DEL */
  { { 0x80, 0x42, 0x00 } }, /*!< MESH_CFG_MDL_CL_NETKEY_GET */
  { { 0x00, 0x00, 0x00 } }, /*!< MESH_CFG_MDL_CL_APPKEY_ADD */
  { { 0x01, 0x00, 0x00 } }, /*!< MESH_CFG_MDL_CL_APPKEY_UPDT */
  { { 0x80, 0x00, 0x00 } }, /*!< MESH_CFG_MDL_CL_APPKEY_DEL */
  { { 0x80, 0x01, 0x00 } }, /*!< MESH_CFG_MDL_CL_APPKEY_GET */
  { { 0x80, 0x46, 0x00 } }, /*!< MESH_CFG_MDL_CL_NODE_IDENTITY_GET */
  { { 0x80, 0x47, 0x00 } }, /*!< MESH_CFG_MDL_CL_NODE_IDENTITY_SET */
  { { 0x80, 0x3D, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_APP_BIND */
  { { 0x80, 0x3F, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_APP_UNBIND */
  { { 0x80, 0x4B, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_APP_SIG_GET */
  { { 0x80, 0x4D, 0x00 } }, /*!< MESH_CFG_MDL_CL_MODEL_APP_VENDOR_GET */
  { { 0x80, 0x49, 0x00 } }, /*!< MESH_CFG_MDL_CL_NODE_RESET */
  { { 0x80, 0x0F, 0x00 } }, /*!< MESH_CFG_MDL_CL_FRIEND_GET */
  { { 0x80, 0x10, 0x00 } }, /*!< MESH_CFG_MDL_CL_FRIEND_SET */
  { { 0x80, 0x15, 0x00 } }, /*!< MESH_CFG_MDL_CL_KEY_REF_PHASE_GET */
  { { 0x80, 0x16, 0x00 } }, /*!< MESH_CFG_MDL_CL_KEY_REF_PHASE_SET */
  { { 0x80, 0x38, 0x00 } }, /*!< MESH_CFG_MDL_CL_HB_PUB_GET */
  { { 0x80, 0x39, 0x00 } }, /*!< MESH_CFG_MDL_CL_HB_PUB_SET */
  { { 0x80, 0x3A, 0x00 } }, /*!< MESH_CFG_MDL_CL_HB_SUB_GET */
  { { 0x80, 0x3B, 0x00 } }, /*!< MESH_CFG_MDL_CL_HB_SUB_SET */
  { { 0x80, 0x2D, 0x00 } }, /*!< MESH_CFG_MDL_CL_LPN_PT_GET */
  { { 0x80, 0x23, 0x00 } }, /*!< MESH_CFG_MDL_CL_NWK_TRANSMIT_GET */
  { { 0x80, 0x24, 0x00 } }, /*!< MESH_CFG_MDL_CL_NWK_TRANSMIT_SET */
};

/*! Mesh Configuration Server Opcodes in sync with operation id's */
const meshMsgOpcode_t meshCfgMdlSrOpcodes[MESH_CFG_MDL_SR_MAX_OP] =
{
  { { 0x80, 0x0B, 0x00 } }, /*!< MESH_CFG_MDL_SR_BEACON_STATUS */
  { { 0x02, 0x00, 0x00 } }, /*!< MESH_CFG_MDL_SR_COMP_DATA_STATUS */
  { { 0x80, 0x0E, 0x00 } }, /*!< MESH_CFG_MDL_SR_DEFAULT_TTL_STATUS */
  { { 0x80, 0x14, 0x00 } }, /*!< MESH_CFG_MDL_SR_GATT_PROXY_STATUS */
  { { 0x80, 0x28, 0x00 } }, /*!< MESH_CFG_MDL_SR_RELAY_STATUS */
  { { 0x80, 0x19, 0x00 } }, /*!< MESH_CFG_MDL_SR_MODEL_PUB_STATUS */
  { { 0x80, 0x1F, 0x00 } }, /*!< MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS */
  { { 0x80, 0x2A, 0x00 } }, /*!< MESH_CFG_MDL_SR_MODEL_SUBSCR_SIG_LIST */
  { { 0x80, 0x2C, 0x00 } }, /*!< MESH_CFG_MDL_SR_MODEL_SUBSCR_VENDOR_LIST */
  { { 0x80, 0x44, 0x00 } }, /*!< MESH_CFG_MDL_SR_NETKEY_STATUS */
  { { 0x80, 0x43, 0x00 } }, /*!< MESH_CFG_MDL_SR_NETKEY_LIST */
  { { 0x80, 0x03, 0x00 } }, /*!< MESH_CFG_MDL_SR_APPKEY_STATUS */
  { { 0x80, 0x02, 0x00 } }, /*!< MESH_CFG_MDL_SR_APPKEY_LIST */
  { { 0x80, 0x48, 0x00 } }, /*!< MESH_CFG_MDL_SR_NODE_IDENTITY_STATUS */
  { { 0x80, 0x3E, 0x00 } }, /*!< MESH_CFG_MDL_SR_MODEL_APP_STATUS */
  { { 0x80, 0x4C, 0x00 } }, /*!< MESH_CFG_MDL_SR_MODEL_APP_SIG_LIST */
  { { 0x80, 0x4E, 0x00 } }, /*!< MESH_CFG_MDL_SR_MODEL_APP_VENDOR_LIST */
  { { 0x80, 0x4A, 0x00 } }, /*!< MESH_CFG_MDL_SR_NODE_RESET_STATUS */
  { { 0x80, 0x11, 0x00 } }, /*!< MESH_CFG_MDL_SR_FRIEND_STATUS */
  { { 0x80, 0x17, 0x00 } }, /*!< MESH_CFG_MDL_SR_KEY_REF_PHASE_STATUS */
  { { 0x06, 0x00, 0x00 } }, /*!< MESH_CFG_MDL_SR_HB_PUB_STATUS */
  { { 0x80, 0x3C, 0x00 } }, /*!< MESH_CFG_MDL_SR_HB_SUB_STATUS */
  { { 0x80, 0x2E, 0x00 } }, /*!< MESH_CFG_MDL_SR_LPN_PT_STATUS */
  { { 0x80, 0x25, 0x00 } }, /*!< MESH_CFG_MDL_SR_NWK_TRANSMIT_STATUS */
};
