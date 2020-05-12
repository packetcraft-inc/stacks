/*************************************************************************************************/
/*!
 *  \file   mesh_proxy_main.h
 *
 *  \brief  Mesh Proxy module interface.
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

#ifndef MESH_PROXY_MAIN_H
#define MESH_PROXY_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Filter Type offset inside a Set Filter Type Proxy Configuration message */
#define MESH_PROXY_FILTER_TYPE_OFFSET       1

/*! Addresses offset inside an Add/Remove Address Proxy Configuration message */
#define MESH_PROXY_ADDRESS_OFFSET           1

/*! List Size offset inside a Set Filter Type Proxy Configuration message */
#define MESH_PROXY_LIST_SIZE_OFFSET         (1 + 1)

/*! Length of a Set Filter Type Proxy Configuration message */
#define MESH_PROXY_SET_FILTER_TYPE_LEN      (1 + 1)

/*! Length of a Filter Status Proxy Configuration message */
#define MESH_PROXY_FILTER_STATUS_TYPE_LEN   (MESH_PROXY_LIST_SIZE_OFFSET + 2)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Proxy Configuration message opcodes */
enum meshProxyOpcodes
{
  MESH_PROXY_OPCODE_SET_FILTER_TYPE,
  MESH_PROXY_OPCODE_ADD_ADDRESS,
  MESH_PROXY_OPCODE_REMOVE_ADDRESS,
  MESH_PROXY_OPCODE_FILTER_STATUS
};

/*! Proxy Config PDU and meta information */
typedef struct meshProxyPduMeta_tag
{
  struct meshProxyPduMeta_t *pNext;          /*!< Pointer to next element for queueing. */
  uint32_t                  ivIndex;         /*!< IV index. */
  uint16_t                  netKeyIndex;     /*!< Network Key (sub-net) Index used for security */
  meshBrInterfaceId_t       rcvdBrIfId;      /*!< Interface on which the PDU is received */
  uint8_t                   pduLen;          /*!< Length of the network PDU */
  uint8_t                   pdu[1];          /*!< First byte of the network PDU. Used to wrap on
                                              *   on large buffer. Must always be the last member
                                              *   of the structure
                                              */
} meshProxyPduMeta_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void meshProxyRegister(meshBrEventNotifyCback_t eventCback, meshBrNwkPduRecvCback_t pduRecvCback);

void meshProxySendConfigMessage(meshBrInterfaceId_t brIfId, uint8_t opcode, const uint8_t *pPdu,
                                uint8_t pduLen);

void meshProxyProcessMsgEmpty(wsfMsgHdr_t *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PROXY_MAIN_H */
