/*************************************************************************************************/
/*!
 *  \file   testapp_config.h
 *
 *  \brief  TestApp application configuration.
 *
 *  Copyright (c) 2010-2019 Arm Ltd.
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

#ifndef TESTAPP_CONFIG_H
#define TESTAPP_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Advertising Interface ID */
#ifndef TESTAPP_ADV_IF_ID
#define TESTAPP_ADV_IF_ID                           0
#endif

/*! Mesh Configuration Client timeout in seconds */
#ifndef TESTAPP_CFG_CL_TIMEOUT
#define TESTAPP_CFG_CL_TIMEOUT                      10
#endif

/*! Light number of elements */
#define TESTAPP_ELEMENT_COUNT                       4

/*! Main generics element */
#define ELEM_GEN                                    0

/*! Main lighting element */
#define ELEM_LIGHT                                  1

/*! Hue element */
#define ELEM_HUE                                    2

/*! Saturation element */
#define ELEM_SAT                                    3

/*! Mesh Models NVM dataset count */
#define MESH_MODELS_NVM_DATASET_COUNT               5

/*! Mesh Generic Power OnOff Model Internal NVM dataset IDs */
#define MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID     0xD000

/*! Mesh Generic OnOff Model Internal NVM dataset IDs */
#define MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID         0xD001

/*! Mesh Generic Power Level Model Internal NVM dataset IDs */
#define MMDL_NVM_GEN_POWER_LEVEL_STATE_DATASET_ID   0xD002

/*! Mesh Lighting Models Internal NVM dataset IDs */
#define MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID    0xD003

/*! Mesh Lighting HSL Models Internal NVM dataset IDs */
#define MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID         0xD004

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh ADV Bearer configure parameters */
extern const advBearerCfg_t testAppAdvBearerCfg;

/*! Mesh GATT Bearer Server configure parameters */
extern const gattBearerSrCfg_t testAppGattBearerSrCfg;

/*! Mesh GATT Bearer Client configure parameters */
extern const gattBearerClCfg_t testAppProxyClCfg;

/*! Mesh GATT Bearer Client configure parameters */
extern const gattBearerClCfg_t testAppPrvClCfg;

/*! Mesh GATT Bearer Client connection parameters */
extern const hciConnSpec_t testAppConnCfg;

/*! Mesh Provisioning Server configuration parameters */
extern meshPrvSrCfg_t testAppMeshPrvSrCfg;

/*! List of elements supported on this node */
extern const meshElement_t testAppElements[TESTAPP_ELEMENT_COUNT];

/*! Mesh Provisioning Server Device UUID */
extern uint8_t testAppPrvSrDevUuid[MESH_PRV_DEVICE_UUID_SIZE];

/*! Mesh Unprovisioned Device info */
extern meshPrvSrUnprovisionedDeviceInfo_t testAppPrvSrUpdInfo;

/*! Mesh Provisioning Client session info */
extern meshPrvClSessionInfo_t testAppPrvClSessionInfo;

/*! Mesh configuration */
extern meshConfig_t testAppMeshConfig;

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Applies runtime configurations for Test App.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppConfig(void);

  /*************************************************************************************************/
  /*!
   *  \brief  Erase runtime configurations for Test App.
   *
   *  \return None.
   */
  /*************************************************************************************************/
  void TestAppConfigErase(void);

#ifdef __cplusplus
}
#endif

#endif /* TESTAPP_CONFIG_H */
