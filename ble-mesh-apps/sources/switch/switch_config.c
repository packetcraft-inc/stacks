/*************************************************************************************************/
/*!
 *  \file   switch_config.c
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

#include <stdio.h>
#include "wsf_types.h"
#include "wsf_os.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"

#include "adv_bearer.h"

#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "app_mesh_api.h"

#include "switch_config.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Length of URI data for unprovisioned device beacons */
#define MESH_PRV_URI_DATA_LEN                4

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Stack memory configuration structure */
static meshMemoryConfig_t switchMeshMemConfig =
{
  .addrListMaxSize          = 20,
  .virtualAddrListMaxSize   = 2,
  .appKeyListSize           = 2,
  .netKeyListSize           = 2,
  .nwkCacheL1Size           = 3,
  .nwkCacheL2Size           = 3,
  .maxNumFriendships        = 1,
  .maxFriendSubscrListSize  = 0,
  .maxNumFriendQueueEntries = 0,
  .sarRxTranHistorySize     = 5,
  .sarRxTranInfoSize        = 3,
  .sarTxMaxTransactions     = 3,
  .rpListSize               = 32,
  .nwkOutputFilterSize      = 0,
  .cfgMdlClMaxSrSupported   = 0,
};

/*! Mesh Provisioning Server Capabilities */
static const meshPrvCapabilities_t switchPrvSrCapabilities =
{
  SWITCH_ELEMENT_COUNT,
  MESH_PRV_ALGO_FIPS_P256_ELLIPTIC_CURVE,
  MESH_PRV_PUB_KEY_OOB,
  MESH_PRV_STATIC_OOB_INFO_AVAILABLE,
  MESH_PRV_OUTPUT_OOB_NOT_SUPPORTED,
  MESH_PRV_OUTPUT_OOB_ACTION_BLINK,
  MESH_PRV_INPUT_OOB_NOT_SUPPORTED,
  MESH_PRV_INPUT_OOB_ACTION_PUSH
};

/*! Mesh Provisioning Server Static OOB data */
static uint8_t switchPrvSrStaticOobData[MESH_PRV_STATIC_OOB_SIZE] =
{ 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef };

/*! Mesh Provisioning Server URI data */
static uint8_t switchPrvSrUriData[MESH_PRV_URI_DATA_LEN] = { 0xde, 0xad, 0xbe, 0xef };

/*! Descriptor for the element 0 instance of the Health Server */
static meshHtSrDescriptor_t switchElem0HtSrDesc;

/*! List of SIG models supported on element 0 */
static const meshSigModel_t  switchElem0SigModelList[] =
{
  {
    .opcodeCount        = MESH_HT_SR_NUM_RECVD_OPCODES,
    .pRcvdOpcodeArray   = meshHtSrRcvdOpcodes,
    .pHandlerId         = &meshHtSrHandlerId,
    .modelId            = (meshSigModelId_t)MESH_HT_SR_MDL_ID,
    .pModelDescriptor   = (void *)&switchElem0HtSrDesc,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = 1,
    .pRcvdOpcodeArray   = mmdlGenOnOffClRcvdOpcodes,
    .pHandlerId         = &mmdlGenOnOffClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_ONOFF_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_ONOFF_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowOnOffClRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowOnOffClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_ONOFF_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_LEVEL_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelClRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_LIGHTNESS_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightLightnessClRcvdOpcodes,
    .pHandlerId         = &mmdlLightLightnessClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_HSL_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslClRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/*! List of SIG models supported on element 1 */
static const meshSigModel_t  switchElem1SigModelList[] =
{
  {
    .opcodeCount        = 1,
    .pRcvdOpcodeArray   = mmdlGenOnOffClRcvdOpcodes,
    .pHandlerId         = &mmdlGenOnOffClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_ONOFF_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh ADV Bearer configure parameters */
const advBearerCfg_t switchAdvBearerCfg =
{
  10,                      /*!< The scan interval, in 0.625 ms units */
  10,                      /*!< The scan window, in 0.625 ms units */
  DM_DISC_MODE_NONE,       /*!< The GAP discovery mode */
  DM_SCAN_TYPE_PASSIVE,    /*!< The scan type (active or passive) */
  10,                      /*!< The advertising duration in ms */
  32,                      /*!< The minimum advertising interval, in 0.625 ms units */
  32                       /*!< The maximum advertising interval, in 0.625 ms units */
};

/*! List of elements supported on this node */
const meshElement_t switchElements[SWITCH_ELEMENT_COUNT] =
{
  {
    .locationDescriptor   = 0xA5A5,
    .numSigModels         = sizeof(switchElem0SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = switchElem0SigModelList,
    .pVendorModelArray    = NULL,
  },
  {
    .locationDescriptor   = 0xA5A6,
    .numSigModels         = sizeof(switchElem1SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = switchElem1SigModelList,
    .pVendorModelArray    = NULL,
  }
};

/*! Mesh Stack configuration structure */
meshConfig_t switchMeshConfig =
{
  /* Configure Mesh. */
  .pElementArray = switchElements,
  .elementArrayLen = SWITCH_ELEMENT_COUNT,
  .pMemoryConfig = &switchMeshMemConfig,
};

/*! Mesh Provisioning Server configuration parameters*/
meshPrvSrCfg_t switchMeshPrvSrCfg =
{
  {0},                       /*!< Device UUID.  */
  1000,                      /*!< Provisioning Bearer advertising interval */
  0,                         /*!< Provisioning Bearer ADV interface ID */
  FALSE,                     /*!< Auto-restart Provisioning */
};

/*! Mesh Unprovisioned Device info */
meshPrvSrUnprovisionedDeviceInfo_t switchPrvSrUpdInfo =
{
  &switchPrvSrCapabilities,
  switchMeshPrvSrCfg.devUuid,
  MESH_PRV_OOB_INFO_OTHER,
  switchPrvSrStaticOobData,
  MESH_PRV_URI_DATA_LEN,
  switchPrvSrUriData,
  NULL
};
