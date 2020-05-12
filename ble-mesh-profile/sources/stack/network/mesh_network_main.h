/*************************************************************************************************/
/*!
 *  \file   mesh_network_main.h
 *
 *  \brief  Network Layer internal module interface.
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

#ifndef MESH_NETWORK_MAIN_H
#define MESH_NETWORK_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Network Cache level types enumeration */
enum meshNwkCacheTypes
{
  MESH_NWK_CACHE_L1 = 0x00,  /*!< Level 1 Network message cache */
  MESH_NWK_CACHE_L2 = 0x01   /*!< Level 2 Network message cache */
};

/*! Mesh Network Cache level type. See ::meshNwkCacheTypes */
typedef uint8_t meshNwkCacheType_t;

/*! Mesh Network Message Cache layer return value. See ::meshReturnValues
 *  for codes starting at ::MESH_NWK_CACHE_RETVAL_BASE
 */
typedef uint16_t meshNwkCacheRetVal_t;

/*! Mesh Network Interfaces control block type definition */
typedef struct meshNwkIfCb_tag
{
  meshNwkIf_t   interfaces[MESH_BR_MAX_INTERFACES]; /*!< List of interfaces */
  uint8_t       maxFilterSize;                      /*!< Maximum size of an interface filter */
} meshNwkIfCb_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

uint32_t meshNwkCacheGetRequiredMemory(void);
void meshNwkCacheInit(void);
void meshNwkCacheClear(void);
meshNwkCacheRetVal_t meshNwkCacheAdd(meshNwkCacheType_t cacheType, const uint8_t *pNwkPdu,
                                     uint8_t pduLen);

uint32_t meshNwkIfGetRequiredMemory(uint8_t filterSize);
void meshNwkIfInit(void);
void meshNwkIfAddInterface(meshBrInterfaceId_t brIfId, meshBrType_t brIfType);
void meshNwkIfRemoveInterface(meshBrInterfaceId_t brIfId);
meshNwkIf_t *meshNwkIfBrIdToNwkIf(meshBrInterfaceId_t brIfId);
bool_t meshNwkIfFilterOutMsg(meshNwkIfFilter_t *pIfFilter, meshAddress_t dstAddr);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/
extern meshNwkIfCb_t nwkIfCb;

#ifdef __cplusplus
}
#endif

#endif /* MESH_NETWORK_MAIN_H */
