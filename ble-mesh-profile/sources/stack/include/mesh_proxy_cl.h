/*************************************************************************************************/
/*!
 *  \file   mesh_proxy_cl.h
 *
 *  \brief  Proxy Client internal module interface.
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

#ifndef MESH_PROXY_CLIENT_H
#define MESH_PROXY_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Configuration Client WSF Message event handler */
typedef void (*meshProxyClMsgHandler_t)(wsfMsgHdr_t *pMsg);

/*! Mesh Proxy Client Control Block */
typedef struct meshProxyClCb_tag
{
  meshProxyClMsgHandler_t   msgHandlerCback;   /*!< WSF Event handler for API */
} meshProxyClCb_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Checks if the node supports Proxy Client.
 *
 *  \return TRUE if Proxy Client is supported, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshProxyClIsSupported(void);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! Mesh Proxy Client Control Block */
extern meshProxyClCb_t meshProxyClCb;

#ifdef __cplusplus
}
#endif

#endif /* MESH_PROXY_CLIENT_H */
