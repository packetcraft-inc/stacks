/*************************************************************************************************/
/*!
 *  \file   switch_config.h
 *
 *  \brief  Switch application configuration.
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
 */
/*************************************************************************************************/

#ifndef SWITCH_CONFIG_H
#define SWITCH_CONFIG_H

#include "app_mesh_api.h"
#include "adv_bearer.h"
#include "mesh_prv_sr_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Advertising Interface ID */
#ifndef SWITCH_ADV_IF_ID
#define SWITCH_ADV_IF_ID                   0
#endif

/*! Switch number of elements. */
enum
{
  SWITCH_ELEMENT_0,
  SWITCH_ELEMENT_1,
  SWITCH_ELEMENT_COUNT
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh ADV Bearer configure parameters */
extern const advBearerCfg_t switchAdvBearerCfg;

/*! Mesh Provisioning Server configuration parameters */
extern meshPrvSrCfg_t switchMeshPrvSrCfg;

/*! List of elements supported on this node */
extern const meshElement_t switchElements[];

/*! Mesh Unprovisioned Device info */
extern meshPrvSrUnprovisionedDeviceInfo_t switchPrvSrUpdInfo;

/*! Mesh configuration */
extern meshConfig_t switchMeshConfig;

#ifdef __cplusplus
}
#endif

#endif /* SWITCH_CONFIG_H */
