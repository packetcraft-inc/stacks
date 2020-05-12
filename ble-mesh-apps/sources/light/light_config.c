/*************************************************************************************************/
/*!
 *  \file   light_config.c
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

#include <stdio.h>
#include <string.h>

#include "wsf_types.h"
#include "wsf_os.h"
#include "wsf_nvm.h"
#include "wsf_assert.h"

#include "dm_api.h"
#include "att_api.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"

#include "adv_bearer.h"
#include "gatt_bearer_sr.h"

#include "mesh_ht_sr_api.h"
#include "mmdl_bindings_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"

#include "app_mesh_api.h"

#include "light_config.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Length of URI data for unprovisioned device beacons */
#define MESH_PRV_URI_DATA_LEN                4

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Stack memory configuration structure */
static meshMemoryConfig_t lightMeshMemConfig =
{
  .addrListMaxSize          = 20,
  .virtualAddrListMaxSize   = 2,
  .appKeyListSize           = 2,
  .netKeyListSize           = 2,
  .nwkCacheL1Size           = 3,
  .nwkCacheL2Size           = 3,
  .maxNumFriendships        = 1,
  .maxFriendSubscrListSize  = 10,
  .maxNumFriendQueueEntries = 20,
  .sarRxTranHistorySize     = 5,
  .sarRxTranInfoSize        = 3,
  .sarTxMaxTransactions     = 3,
  .rpListSize               = 32,
  .nwkOutputFilterSize      = 10,
  .cfgMdlClMaxSrSupported    = 0,
};

/*! Mesh Provisioning Server Capabilities */
static const meshPrvCapabilities_t lightPrvSrCapabilities =
{
  LIGHT_ELEMENT_COUNT,
  MESH_PRV_ALGO_FIPS_P256_ELLIPTIC_CURVE,
  MESH_PRV_PUB_KEY_OOB,
  MESH_PRV_STATIC_OOB_INFO_AVAILABLE,
  MESH_PRV_OUTPUT_OOB_NOT_SUPPORTED,
  MESH_PRV_OUTPUT_OOB_ACTION_BLINK,
  MESH_PRV_INPUT_OOB_NOT_SUPPORTED,
  MESH_PRV_INPUT_OOB_ACTION_PUSH
};

/*! Mesh Provisioning Server Static OOB data */
static uint8_t lightPrvSrStaticOobData[MESH_PRV_STATIC_OOB_SIZE] =
{ 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef };

/*! Mesh Provisioning Server URI data */
static uint8_t lightPrvSrUriData[MESH_PRV_URI_DATA_LEN] = { 0xde, 0xad, 0xbe, 0xef };

/*! Descriptor for the lightElement 0 instance of the Health Server */
static meshHtSrDescriptor_t lightElem0HtSrDesc;

/*! Model instances states */
static mmdlGenOnOffState_t lightElem01GenOnOffStates[2][MMDL_GEN_ONOFF_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlGenOnPowerUpState_t lightElem01GenPowOnOffStates[2][MMDL_GEN_POWER_ONOFF_STATE_CNT +
                                                               MMDL_NUM_OF_SCENES];
static uint16_t lightElem0Scenes[MMDL_SCENE_STATE_CNT + MMDL_NUM_OF_SCENES];

static mmdlGenDefaultTransState_t lightElem1GenDefaultTransStates[MMDL_GEN_DEFAULT_TRANS_STATE_CNT +
                                                                  MMDL_NUM_OF_SCENES];
static mmdlGenLevelState_t lightElem1GenLevelStates[MMDL_GEN_LEVEL_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlLightLightnessState_t lightElem1LightLightnessStates[MMDL_LIGHT_LIGHTNESS_STATE_CNT +
                                                                MMDL_NUM_OF_SCENES] =
{
  0,                                  /* Actual */
  0,                                  /* Linear */
  0,                                  /* Target */
  MMDL_LIGHT_LIGHTNESS_STATE_HIGHEST, /* Last */
  0,                                  /* Default */
  1,                                  /* RangeMin */
  MMDL_LIGHT_LIGHTNESS_STATE_HIGHEST  /* RangeMax */
};

static uint16_t lightElem1Scenes[MMDL_SCENE_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlLightHslSrStoredState_t lightElem1LightHslSrState =
{
  .minHue = 0,
  .defaultHue = 1,
  .maxHue = 0xFFFF,
  .defaultSat = 1,
  .minSat = 0,
  .maxSat = 0xFFFF
};

static mmdlGenLevelState_t lightElem2GenLevelStates[MMDL_GEN_LEVEL_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlLightHslHueStoredState_t lightElem2HueState =
{
  .presentHue = 0
};

static mmdlGenLevelState_t lightElem3GenLevelStates[MMDL_GEN_LEVEL_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlLightHslSatStoredState_t lightElem3SatState =
{
  .presentSat = 0
};

/*! Descriptor for the lightElement 0 instance of the Generic OnOff Server */
static mmdlGenOnOffSrDesc_t lightElem0GenOnOffSr =
{
  .pStoredStates        = lightElem01GenOnOffStates[0]
};

/*! Descriptor for the lightElement 0 instance of the Scene Server */
static mmdlSceneSrDesc_t lightElem0SceneSr =
{
  .pStoredScenes = lightElem0Scenes
};

/*! Descriptor for the lightElement 0 instance of the Generic Power OnOff Server */
static mmdlGenPowOnOffSrDesc_t lightElem0GenPowOnOffSr =
{
  .pStoredStates = lightElem01GenPowOnOffStates[0]
};

/*! Descriptor for the lightElement 1 instance of the Generic Power OnOff Server */
static mmdlGenPowOnOffSrDesc_t lightElem1GenPowOnOffSr =
{
  .pStoredStates = lightElem01GenPowOnOffStates[1]
};

/*! Descriptor for the lightElement 1 instance of the Generic OnOff Server */
static mmdlGenOnOffSrDesc_t lightElem1GenOnOffSr =
{
  .pStoredStates        = lightElem01GenOnOffStates[1]
};

/*! Descriptor for the lighElement 1 instance of the Generic Default Transition Time Server */
static mmdlGenDefaultTransSrDesc_t lightElem1GenDefaultTransSr =
{
  .pStoredStates = lightElem1GenDefaultTransStates
};

/*! Descriptor for the lightElement 1 instance of Generic Level Server */
static mmdlGenLevelSrDesc_t lightElem1GenLevelSr =
{
  .pStoredStates        = lightElem1GenLevelStates
};

/*! Descriptor for the lightElement 1 instance of the Scene Server */
static mmdlSceneSrDesc_t lightElem1SceneSr =
{
  .pStoredScenes = lightElem1Scenes
};

/*! Descriptor for the lightElement 1 instance of Light Lightness Server */
static mmdlLightLightnessSrDesc_t  lightElem1LightLightnessSr =
{
  .pStoredStates        = lightElem1LightLightnessStates
};

/*! Descriptor for the lightElement 1 instance of Light HSL Server */
static mmdlLightHslSrDesc_t lightElem1LightHslSr =
{
  .pStoredState         = &lightElem1LightHslSrState
};

/*! Descriptor for the lightElement 2 instance of Generic Level Server */
static mmdlGenLevelSrDesc_t lightElem2GenLevelSr =
{
  .pStoredStates        = lightElem2GenLevelStates
};

/*! Descriptor for the lightElement 2 instance of Light HSL Hue Server */
static mmdlLightHslHueSrDesc_t lightElem2LightHslHueSr =
{
  .pStoredState        = &lightElem2HueState
};

/*! Descriptor for the lightElement 3 instance of Generic Level Server */
static mmdlGenLevelSrDesc_t lightElem3GenLevelSr =
{
  .pStoredStates        = lightElem3GenLevelStates
};

/*! Descriptor for the lightElement 3 instance of Light HSL Saturation Server */
static mmdlLightHslSatSrDesc_t lightElem3LightHslSatSr =
{
  .pStoredState        = &lightElem3SatState
};

/*! List of SIG models supported on lightElement 0 */
static const meshSigModel_t  lightElem0SigModelList[] =
{
  {
    .opcodeCount        = MESH_HT_SR_NUM_RECVD_OPCODES,
    .pRcvdOpcodeArray   = meshHtSrRcvdOpcodes,
    .pHandlerId         = &meshHtSrHandlerId,
    .modelId            = (meshSigModelId_t)MESH_HT_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem0HtSrDesc,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem0GenOnOffSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem0GenPowOnOffSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_ONOFFSETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowOnOffSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowOnOffSetupSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_ONOFFSETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCENE_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSceneSrRcvdOpcodes,
    .pHandlerId         = &mmdlSceneSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCENE_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem0SceneSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCENE_SETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSceneSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlSceneSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCENE_SETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/*! List of SIG models supported on lightElement 0 */
static const meshSigModel_t  lightElem1SigModelList[] =
{
  {
    .opcodeCount        = MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem1GenOnOffSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_DEFAULT_TRANS_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenDefaultTransSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenDefaultTransSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem1GenDefaultTransSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem1GenPowOnOffSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_ONOFFSETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowOnOffSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowOnOffSetupSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_ONOFFSETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem1GenLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_LIGHTNESS_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightLightnessSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightLightnessSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_LIGHTNESS_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem1LightLightnessSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_LIGHTNESSSETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightLightnessSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightLightnessSetupSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_LIGHTNESSSETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCENE_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSceneSrRcvdOpcodes,
    .pHandlerId         = &mmdlSceneSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCENE_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem1SceneSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCENE_SETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSceneSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlSceneSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCENE_SETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_HSL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem1LightHslSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_HSL_SETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_SETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/*! List of SIG models supported on lightElement 2 */
static const meshSigModel_t  lightElem2SigModelList[] =
{
  {
    .opcodeCount        = MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem2GenLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_HSL_HUE_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslHueSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslHueSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_HUE_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem2LightHslHueSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/*! List of SIG models supported on lightElement 3 */
static const meshSigModel_t  lightElem3SigModelList[] =
{
  {
    .opcodeCount        = MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem3GenLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_HSL_SAT_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslSatSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslSatSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_SAT_SR_MDL_ID,
    .pModelDescriptor   = (void *)&lightElem3LightHslSatSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh ADV Bearer configure parameters */
const advBearerCfg_t lightAdvBearerCfg =
{
  24,                      /*!< The scan interval, in 0.625 ms units */
  24,                      /*!< The scan window, in 0.625 ms units */
  DM_DISC_MODE_NONE,       /*!< The GAP discovery mode */
  DM_SCAN_TYPE_PASSIVE,    /*!< The scan type (active or passive) */
  10,                      /*!< The advertising duration in ms */
  32,                      /*!< The minimum advertising interval, in 0.625 ms units */
  32                       /*!< The maximum advertising interval, in 0.625 ms units */
};

/*! Mesh GATT Bearer Server configure parameters */
const gattBearerSrCfg_t lightGattBearerSrCfg =
{
  300,                      /*!< Minimum advertising interval in 0.625 ms units */
  300,                      /*!< Maximum advertising interval in 0.625 ms units */
  DM_ADV_CONN_UNDIRECT,     /*!< The advertising type */
};

/*! List of lightElements supported on this node */
const meshElement_t lightElements[LIGHT_ELEMENT_COUNT] =
{
  {
    .locationDescriptor   = 0xA5A5,
    .numSigModels         = sizeof(lightElem0SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = lightElem0SigModelList,
    .pVendorModelArray    = NULL,
  },
  {
    .locationDescriptor   = 0xA5A6,
    .numSigModels         = sizeof(lightElem1SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = lightElem1SigModelList,
    .pVendorModelArray    = NULL,
  },
  {
    .locationDescriptor   = 0xA5A7,
    .numSigModels         = sizeof(lightElem2SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = lightElem2SigModelList,
    .pVendorModelArray    = NULL,
  },
  {
    .locationDescriptor   = 0xA5A8,
    .numSigModels         = sizeof(lightElem3SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = lightElem3SigModelList,
    .pVendorModelArray    = NULL,
  }
};

/*! Mesh Provisioning Server configuration parameters*/
meshPrvSrCfg_t lightMeshPrvSrCfg =
{
  {0},                       /*!< Device UUID.  */
  1000,                      /*!< Provisioning Bearer advertising interval */
  0,                         /*!< Provisioning Bearer ADV interface ID */
  FALSE,                     /*!< Auto-restart Provisioning */
};

/*! Mesh Unprovisioned Device info */
meshPrvSrUnprovisionedDeviceInfo_t lightPrvSrUpdInfo =
{
  &lightPrvSrCapabilities,
  lightMeshPrvSrCfg.devUuid,
  MESH_PRV_OOB_INFO_OTHER,
  lightPrvSrStaticOobData,
  MESH_PRV_URI_DATA_LEN,
  lightPrvSrUriData,
  NULL
};

/*! Mesh Stack configuration structure */
meshConfig_t lightMeshConfig =
{
  /* Configure Mesh. */
  .pElementArray = lightElements,
  .elementArrayLen = LIGHT_ELEMENT_COUNT,
  .pMemoryConfig = &lightMeshMemConfig,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Generic OnPowerUp State on lightElement 0.
 *
 *  \param[in] lightElementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void genOnPowerUpNvmSave(meshElementId_t lightElementId)
{
  WSF_ASSERT(lightElementId <= 1);

  WsfNvmWriteData((uint64_t)MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID, (uint8_t *)lightElem01GenPowOnOffStates,
                             sizeof(lightElem01GenPowOnOffStates), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Generic OnOff States on lightElements 0 and 1.
 *
 *  \param[in] lightElementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void genOnOffNvmSave(meshElementId_t lightElementId)
{
  WsfNvmWriteData((uint64_t)MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID, (uint8_t *)lightElem01GenOnOffStates,
                          sizeof(lightElem01GenOnOffStates), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Light Lightness State on lightElement 1.
 *
 *  \param[in] lightElementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightLightnessNvmSave(meshElementId_t lightElementId)
{
  WsfNvmWriteData((uint64_t)MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID, (uint8_t *)lightElem1LightLightnessStates,
                             sizeof(lightElem1LightLightnessStates), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Light HSL State on lightElement 1.
 *
 *  \param[in] lightElementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightHslNvmSave(meshElementId_t lightElementId)
{
  WSF_ASSERT(lightElementId == 1);

  WsfNvmWriteData((uint64_t)MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID, (uint8_t *)&lightElem1LightHslSrState,
                        sizeof(lightElem1LightHslSrState), NULL);

  (void)lightElementId;
}

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
void LightConfig(void)
{
  bool_t retVal;

  memset(lightElem01GenPowOnOffStates, 0, sizeof(lightElem01GenPowOnOffStates));

  retVal = WsfNvmReadData((uint64_t)MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID, (uint8_t *)lightElem01GenPowOnOffStates,
                             sizeof(lightElem01GenPowOnOffStates), NULL);

  memset(lightElem01GenOnOffStates, 0, sizeof(lightElem01GenOnOffStates));

  retVal = WsfNvmReadData((uint64_t)MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID, (uint8_t *)lightElem01GenOnOffStates,
                             sizeof(lightElem01GenOnOffStates), NULL);

  retVal = WsfNvmReadData((uint64_t)MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID, (uint8_t *)lightElem1LightLightnessStates,
                             sizeof(lightElem1LightLightnessStates), NULL);

  retVal = WsfNvmReadData((uint64_t)MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID, (uint8_t *)&lightElem1LightHslSrState,
                             sizeof(lightElem1LightHslSrState), NULL);

  /* Link NVM save functions to wrappers. */
  lightElem0GenPowOnOffSr.fNvmSaveStates    = genOnPowerUpNvmSave;
  lightElem1GenPowOnOffSr.fNvmSaveStates    = genOnPowerUpNvmSave;
  lightElem0GenOnOffSr.fNvmSaveStates       = genOnOffNvmSave;
  lightElem1GenOnOffSr.fNvmSaveStates       = genOnOffNvmSave;
  lightElem1LightLightnessSr.fNvmSaveStates = lightLightnessNvmSave;
  lightElem1LightHslSr.fNvmSaveStates       = lightHslNvmSave;

  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief  Erase configuration for the Light App.
 *
 *  \return None.
 */
/*************************************************************************************************/
void LightConfigErase(void)
{
  WsfNvmEraseData((uint64_t)MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID, NULL);
}
