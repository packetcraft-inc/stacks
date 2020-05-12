/*************************************************************************************************/
/*!
 *  \file   light_config.h
 *
 *  \brief  Light application configuration.
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

#ifndef LIGHT_CONFIG_H
#define LIGHT_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Advertising Interface ID */
#ifndef LIGHT_ADV_IF_ID
#define LIGHT_ADV_IF_ID                             0
#endif

/*! Light number of elements. */
#define LIGHT_ELEMENT_COUNT                         4

/*! Main element */
#define ELEM_MAIN                                   0

/*! HSL element */
#define ELEM_HSL                                    1

/*! HUE element */
#define ELEM_HUE                                    2

/*! SAT element */
#define ELEM_SAT                                    3

/*! Mesh Models NVM dataset count */
#define MESH_MODELS_NVM_DATASET_COUNT               4

/*! Mesh Generic Power OnOff Model Internal NVM dataset IDs */
#define MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID     0xD000

/*! Mesh Generic OnOff Model Internal NVM dataset IDs */
#define MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID         0xD001

/*! Mesh Lighting Models Internal NVM dataset IDs */
#define MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID    0xD002

/*! Mesh Lighting HSL Models Internal NVM dataset IDs */
#define MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID         0xD003

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh ADV Bearer configure parameters */
extern const advBearerCfg_t lightAdvBearerCfg;

/*! Mesh GATT Bearer Server configure parameters */
extern const gattBearerSrCfg_t lightGattBearerSrCfg;

/*! List of elements supported on this node */
extern const meshElement_t lightElements[LIGHT_ELEMENT_COUNT];

/*! Mesh Unprovisioned Device info */
extern meshPrvSrUnprovisionedDeviceInfo_t lightPrvSrUpdInfo;

/*! Mesh Provisioning Server configuration parameters */
extern meshPrvSrCfg_t lightMeshPrvSrCfg;

/*! Mesh configuration */
extern meshConfig_t lightMeshConfig;

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Applies runtime configuration for the Light App.
 *
 *  \return None.
 */
/*************************************************************************************************/
void LightConfig(void);

/*************************************************************************************************/
/*!
 *  \brief  Erase configuration for the Light App.
 *
 *  \return None.
 */
/*************************************************************************************************/
void LightConfigErase(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_CONFIG_H */
