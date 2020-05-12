/*************************************************************************************************/
/*!
 *  \file   provisioner_config.h
 *
 *  \brief  Provisioner application configuration.
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
 */
/*************************************************************************************************/

#ifndef PROVISIONER_CONFIG_H
#define PROVISIONER_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Configuration Client timeout in seconds. */
#ifndef PROVISIONER_CFG_CL_TIMEOUT
#define PROVISIONER_CFG_CL_TIMEOUT        10
#endif

/*! Light number of elements. */
#define PROVISIONER_ELEMENT_COUNT         1

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh ADV Bearer configure parameters */
extern const advBearerCfg_t provisionerAdvBearerCfg;

/*! Mesh GATT Bearer Client configure parameters */
extern const gattBearerClCfg_t provisionerProxyClCfg;

/*! Mesh GATT Bearer Client configure parameters */
extern const gattBearerClCfg_t provisionerPrvClCfg;

/*! Mesh GATT Bearer Client connection parameters */
extern const hciConnSpec_t provisionerConnCfg;

/*! List of elements supported on this node */
extern const meshElement_t provisionerElements[PROVISIONER_ELEMENT_COUNT];

/*! Mesh Provisioning Client session info */
extern meshPrvClSessionInfo_t provisionerPrvClSessionInfo;

/*! Mesh configuration */
extern meshConfig_t provisionerMeshConfig;

#ifdef __cplusplus
}
#endif

#endif /* PROVISIONER_CONFIG_H */
