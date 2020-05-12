/*************************************************************************************************/
/*!
 *  \file   mesh_bearer_defs.h
 *
 *  \brief  Bearer definitions.
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

#ifndef MESH_BEARER_DEFS_H
#define MESH_BEARER_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Timeout value in seconds for a GATT Proxy Rx transaction. */
#define MESH_GATT_PROXY_TIMEOUT_SEC         20

/**************************************************************************************************
  Enums
**************************************************************************************************/

/*! GATT Proxy PDU SAR enumeration */
enum meshGattProxyPduSarValues
{
  MESH_GATT_PROXY_PDU_SAR_COMPLETE_MSG = 0x00,  /*!< Complete Proxy PDU */
  MESH_GATT_PROXY_PDU_SAR_FIRST_SEG    = 0x01,  /*!< First segment of a Proxy PDU */
  MESH_GATT_PROXY_PDU_SAR_CONT_SEG     = 0x02,  /*!< Continuation segment of a Proxy PDU */
  MESH_GATT_PROXY_PDU_SAR_LAST_SEG     = 0x03   /*!< Last segment of a Proxy PDU */
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_BEARER_DEFS_H */
