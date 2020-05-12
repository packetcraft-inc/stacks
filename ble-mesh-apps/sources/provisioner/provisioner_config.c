/*************************************************************************************************/
/*!
 *  \file   provisioner_config.c
 *
 *  \brief  Provisioner application configuration.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_os.h"
#include "wsf_assert.h"

#include "dm_api.h"
#include "att_api.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_prv.h"
#include "mesh_prv_cl_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"

#include "adv_bearer.h"
#include "gatt_bearer_cl.h"

#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"
#include "app_mesh_api.h"

#include "provisioner_config.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Length of URI data for unprovisioned device beacons */
#define MESH_PRV_URI_DATA_LEN                4

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Stack memory configuration structure */
static meshMemoryConfig_t provisionerMeshMemConfig =
{
  .addrListMaxSize          = 10,
  .virtualAddrListMaxSize   = 2,
  .appKeyListSize           = 10,
  .netKeyListSize           = 10,
  .nwkCacheL1Size           = 3,
  .nwkCacheL2Size           = 3,
  .maxNumFriendships        = 0,
  .maxFriendSubscrListSize  = 0,
  .maxNumFriendQueueEntries = 0,
  .sarRxTranHistorySize     = 5,
  .sarRxTranInfoSize        = 3,
  .sarTxMaxTransactions     = 3,
  .rpListSize               = 32,
  .nwkOutputFilterSize      = 10,
  .cfgMdlClMaxSrSupported   = 2,
};

/*! Mesh Provisioning Client Device UUID */
static uint8_t provisionerPrvClDevUuid[MESH_PRV_DEVICE_UUID_SIZE] =
{ 0x70, 0xcf, 0x7c, 0x97, 0x32, 0xa3, 0x45, 0xb6, 0x91, 0x49, 0x48, 0x10, 0xd2, 0xe9, 0xcb, 0xf4 };

/*! Mesh Provisioning Client NetKey */
static uint8_t provisionerPrvClNetKey[MESH_KEY_SIZE_128] =
{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

/*! Mesh Provisioning Client Static OOB data */
static uint8_t provisionerPrvClStaticOobData[MESH_PRV_STATIC_OOB_SIZE] =
{ 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef };

/*! Mesh Provisioning Client OOB Public Key X */
static uint8_t provisionerPrvClPeerOobPublicKeyX[MESH_PRV_PUB_KEY_SIZE] =
{ 0xF4, 0x65, 0xE4, 0x3F, 0xF2, 0x3D, 0x3F, 0x1B, 0x9D, 0xC7, 0xDF, 0xC0, 0x4D, 0xA8, 0x75, 0x81,
  0x84, 0xDB, 0xC9, 0x66, 0x20, 0x47, 0x96, 0xEC, 0xCF, 0x0D, 0x6C, 0xF5, 0xE1, 0x65, 0x00, 0xCC };

/*! Mesh Provisioning Client OOB Public Key Y */
static uint8_t provisionerPrvClPeerOobPublicKeyY[MESH_PRV_PUB_KEY_SIZE] =
{ 0x02, 0x01, 0xD0, 0x48, 0xBC, 0xBB, 0xD8, 0x99, 0xEE, 0xEF, 0xC4, 0x24, 0x16, 0x4E, 0x33, 0xC2,
  0x01, 0xC2, 0xB0, 0x10, 0xCA, 0x6B, 0x4D, 0x43, 0xA8, 0xA1, 0x55, 0xCA, 0xD8, 0xEC, 0xB2, 0x79 };

/*! Mesh Provisioning Client OOB Public Key */
static meshPrvOobPublicKey_t provisionerPrvClPeerOobPublicKey ={ provisionerPrvClPeerOobPublicKeyX,
                                                                 provisionerPrvClPeerOobPublicKeyY };

/*! Mesh Provisioner data */
static meshPrvProvisioningData_t provisionerPrvClProvData =
{
    .pDevKey        = NULL,
    .pNetKey        = provisionerPrvClNetKey,
    .netKeyIndex    = 0x0000,
    .flags          = 0,
    .ivIndex        = 0,
    .address        = 0x0000, /* Needs to be set before it is used */
};

/*! Descriptor for the element 0 instance of the Health Server */
static meshHtSrDescriptor_t provisionerElem0HtSrDesc;

/*! List of SIG models supported on element 0 */
static const meshSigModel_t  provisionerElem0SigModelList[] =
{
  {
    .opcodeCount        = MESH_HT_SR_NUM_RECVD_OPCODES,
    .pRcvdOpcodeArray   = meshHtSrRcvdOpcodes,
    .pHandlerId         = &meshHtSrHandlerId,
    .modelId            = (meshSigModelId_t)MESH_HT_SR_MDL_ID,
    .pModelDescriptor   = (void *)&provisionerElem0HtSrDesc,
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
    .opcodeCount        = MMDL_LIGHT_HSL_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslClRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! List of elements supported on this node */
const meshElement_t provisionerElements[PROVISIONER_ELEMENT_COUNT] =
{
  {
    .locationDescriptor = 0xA5A5,
    .numSigModels = sizeof(provisionerElem0SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels = 0,
    .pSigModelArray = provisionerElem0SigModelList,
    .pVendorModelArray = NULL,
  }
};

/*! Mesh Stack configuration structure */
meshConfig_t provisionerMeshConfig =
{
  .pElementArray = provisionerElements,
  .elementArrayLen = PROVISIONER_ELEMENT_COUNT,
  .pMemoryConfig = &provisionerMeshMemConfig,
};

/*! Mesh ADV Bearer configure parameters */
const advBearerCfg_t provisionerAdvBearerCfg =
{
  24,                      /*!< The scan interval, in 0.625 ms units */
  24,                      /*!< The scan window, in 0.625 ms units */
  DM_DISC_MODE_NONE,       /*!< The GAP discovery mode */
  DM_SCAN_TYPE_PASSIVE,    /*!< The scan type (active or passive) */
  10,                      /*!< The advertising duration in ms */
  32,                      /*!< The minimum advertising interval, in 0.625 ms units */
  32                       /*!< The maximum advertising interval, in 0.625 ms units */
};

/*! Mesh GATT Bearer Client configure parameters */
const gattBearerClCfg_t provisionerProxyClCfg =
{
  96,                           /*!< The scan interval, in 0.625 ms units */
  48,                           /*!< The scan window, in 0.625 ms units */
  DM_DISC_MODE_GENERAL,         /*!< The GAP discovery mode */
  DM_SCAN_TYPE_ACTIVE,          /*!< The scan type (active or passive) */
  ATT_UUID_MESH_PROXY_SERVICE   /*!< The searched service UUID */
};

/*! Mesh GATT Bearer Client configure parameters */
const gattBearerClCfg_t provisionerPrvClCfg =
{
  96,                           /*!< The scan interval, in 0.625 ms units */
  48,                           /*!< The scan window, in 0.625 ms units */
  DM_DISC_MODE_GENERAL,         /*!< The GAP discovery mode */
  DM_SCAN_TYPE_ACTIVE,          /*!< The scan type (active or passive) */
  ATT_UUID_MESH_PRV_SERVICE     /*!< The searched service UUID */
};

/*! Mesh GATT Bearer Client connection parameters */
const hciConnSpec_t provisionerConnCfg =
{
  40,                           /*!< Minimum connection interval in 1.25ms units */
  40,                           /*!< Maximum connection interval in 1.25ms units */
  0,                            /*!< Connection latency */
  600,                          /*!< Supervision timeout in 10ms units */
  0,                            /*!< Unused */
  0                             /*!< Unused */
};

/*! Mesh Provisioning Client session info */
meshPrvClSessionInfo_t provisionerPrvClSessionInfo =
{
    .pDeviceUuid        = provisionerPrvClDevUuid,
    .pDevicePublicKey   = &provisionerPrvClPeerOobPublicKey,
    .pStaticOobData     = provisionerPrvClStaticOobData,
    .pAppEccKeys        = NULL,
    .pData              = &provisionerPrvClProvData,
    .attentionDuration  = 0
};
