/*************************************************************************************************/
/*!
 *  \file   cfg_mesh_stack.h
 *
 *  \brief  Mesh Stack configuration.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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

#ifndef CFG_MESH_STACK_H
#define CFG_MESH_STACK_H


#ifdef __cplusplus
extern "C" {
#endif

#include "cfg_stack.h"

/**************************************************************************************************
  MESH STACK VERSION
**************************************************************************************************/

/*! Mesh Stack Release Types */
#define MESH_STACK_VERSION         ((const char *)"Packetcraft Mesh v1.0")

/**************************************************************************************************
  Bearer
**************************************************************************************************/

/*! Maximum number of connections */
#ifndef MESH_GATT_MAX_CONNECTIONS
#define MESH_GATT_MAX_CONNECTIONS         DM_CONN_MAX
#endif

/*! Maximum number of advertising interfaces supported */
#ifndef MESH_ADV_MAX_INTERFACES
#define MESH_ADV_MAX_INTERFACES           1
#endif

/*! Queue size for each advertising interface */
#ifndef MESH_ADV_QUEUE_SIZE
#define MESH_ADV_QUEUE_SIZE               10
#endif

/*! Queue size for each GATT interface */
#ifndef MESH_GATT_QUEUE_SIZE
#define MESH_GATT_QUEUE_SIZE              5
#endif

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Get Mesh Stack version number.
 *
 *  \param[out] ppOutVersion  Output parameter for version number.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshGetVersionNumber(const char **ppOutVersion);

#ifdef __cplusplus
};
#endif

#endif /* CFG_MESH_STACK_H */
