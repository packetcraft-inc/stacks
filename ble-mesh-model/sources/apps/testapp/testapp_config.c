/*************************************************************************************************/
/*!
 *  \file   testapp_config.c
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
#include "mesh_prv_cl_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"

#include "adv_bearer.h"
#include "gatt_bearer_sr.h"
#include "gatt_bearer_cl.h"

#include "mmdl_vendor_test_cl_api.h"
#include "mesh_ht_cl_api.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_gen_powerlevel_cl_api.h"
#include "mmdl_gen_powerlevel_sr_api.h"
#include "mmdl_gen_powerlevelsetup_sr_api.h"
#include "mmdl_gen_default_trans_cl_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_gen_battery_cl_api.h"
#include "mmdl_gen_battery_sr_api.h"
#include "mmdl_time_cl_api.h"
#include "mmdl_time_sr_api.h"
#include "mmdl_timesetup_sr_api.h"
#include "mmdl_scene_cl_api.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_light_hsl_cl_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_scheduler_cl_api.h"
#include "mmdl_scheduler_sr_api.h"

#include "app_mesh_api.h"

#include "testapp_config.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Length of URI data for unprovisioned device beacons */
#define MESH_PRV_URI_DATA_LEN                4

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Stack memory configuration structure */
static meshMemoryConfig_t testAppMeshMemConfig =
{
  .addrListMaxSize          = 5,
  .virtualAddrListMaxSize   = 2,
  .appKeyListSize           = 10,
  .netKeyListSize           = 10,
  .nwkCacheL1Size           = 3,
  .nwkCacheL2Size           = 3,
  .maxNumFriendships        = 1,
  .maxFriendSubscrListSize  = 1,
  .maxNumFriendQueueEntries = 20,
  .sarRxTranHistorySize     = 10,
  .sarRxTranInfoSize        = 3,
  .sarTxMaxTransactions     = 3,
  .rpListSize               = 5,
  .nwkOutputFilterSize      = 10,
  .cfgMdlClMaxSrSupported   = 2,
};

/*! Mesh Provisioning Server Capabilities */
static meshPrvCapabilities_t testAppPrvSrCapabilities =
{
  TESTAPP_ELEMENT_COUNT,
  MESH_PRV_ALGO_FIPS_P256_ELLIPTIC_CURVE,
  MESH_PRV_PUB_KEY_OOB,
  MESH_PRV_STATIC_OOB_INFO_AVAILABLE,
  MESH_PRV_OUTPUT_OOB_SIZE_EIGHT_OCTET,
  MESH_PRV_OUTPUT_OOB_ACTION_BLINK,
  MESH_PRV_INPUT_OOB_SIZE_EIGHT_OCTET,
  MESH_PRV_INPUT_OOB_ACTION_PUSH
};

/*! Mesh Provisioning Server Static OOB data */
static uint8_t testAppPrvSrStaticOobData[MESH_PRV_STATIC_OOB_SIZE] =
{ 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef };

/*! Mesh Provisioning Server URI data */
static uint8_t testAppPrvSrUriData[MESH_PRV_URI_DATA_LEN] = { 0xde, 0xad, 0xbe, 0xef };

/*! Mesh Provisioning Client Device UUID */
static uint8_t testAppPrvClDevUuid[MESH_PRV_DEVICE_UUID_SIZE] =
{ 0x70, 0xcf, 0x7c, 0x97, 0x32, 0xa3, 0x45, 0xb6, 0x91, 0x49, 0x48, 0x10, 0xd2, 0xe9, 0xcb, 0xf4 };

/*! Mesh Provisioning Client NetKey */
static uint8_t testAppPrvClNetKey[MESH_KEY_SIZE_128] =
{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

/*! Mesh Provisioning Client Static OOB data */
static uint8_t testAppPrvClStaticOobData[MESH_PRV_STATIC_OOB_SIZE] =
{ 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef };

/*! Mesh Provisioning Client OOB Public Key X */
static uint8_t testAppPrvClPeerOobPublicKeyX[MESH_PRV_PUB_KEY_SIZE] =
{ 0xF4, 0x65, 0xE4, 0x3F, 0xF2, 0x3D, 0x3F, 0x1B, 0x9D, 0xC7, 0xDF, 0xC0, 0x4D, 0xA8, 0x75, 0x81,
  0x84, 0xDB, 0xC9, 0x66, 0x20, 0x47, 0x96, 0xEC, 0xCF, 0x0D, 0x6C, 0xF5, 0xE1, 0x65, 0x00, 0xCC };

/*! Mesh Provisioning Client OOB Public Key Y */
static uint8_t testAppPrvClPeerOobPublicKeyY[MESH_PRV_PUB_KEY_SIZE] =
{ 0x02, 0x01, 0xD0, 0x48, 0xBC, 0xBB, 0xD8, 0x99, 0xEE, 0xEF, 0xC4, 0x24, 0x16, 0x4E, 0x33, 0xC2,
  0x01, 0xC2, 0xB0, 0x10, 0xCA, 0x6B, 0x4D, 0x43, 0xA8, 0xA1, 0x55, 0xCA, 0xD8, 0xEC, 0xB2, 0x79 };

/*! Mesh Provisioning Client OOB Public Key */
static meshPrvOobPublicKey_t testAppPrvClPeerOobPublicKey ={ testAppPrvClPeerOobPublicKeyX,
                                                      testAppPrvClPeerOobPublicKeyY };

/*! Mesh Provisioner data */
static meshPrvProvisioningData_t testAppPrvClProvData =
{
    .pDevKey        = NULL,
    .pNetKey        = testAppPrvClNetKey,
    .netKeyIndex    = 0x0000,
    .flags          = 0,
    .ivIndex        = 0,
    .address        = 0x0000, /* Needs to be set before it is used */
};

/*! Descriptor for element 0 instance of the Health Server */
static meshHtSrDescriptor_t testAppElem0HtSrDesc;

/*! Structures that store data */
static mmdlGenOnPowerUpState_t testAppElem01GenPowOnOffStates[2][MMDL_GEN_POWER_ONOFF_STATE_CNT +
                                                                 MMDL_NUM_OF_SCENES];
static mmdlGenOnOffState_t testAppElem01GenOnOffStates[2][MMDL_GEN_ONOFF_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlGenLevelState_t testAppElem0GenLevelStates[MMDL_GEN_LEVEL_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlGenDefaultTransState_t testAppElem01GenDefaultTransStates[2][MMDL_GEN_DEFAULT_TRANS_STATE_CNT +
                                                                        MMDL_NUM_OF_SCENES];
static mmdlGenBatteryState_t testAppElem0GenBatteryStates[MMDL_GEN_BATTERY_STATE_CNT +
                                                   MMDL_NUM_OF_SCENES];
static mmdlGenPowerLevelState_t testAppElem0GenPowLevelStates[MMDL_GEN_POWER_LEVEL_STATE_CNT +
                                                       MMDL_NUM_OF_SCENES] =
{
  0,                                 /* Present */
  0,                                 /* Target */
  MMDL_GEN_POWERRANGE_MIN,           /* Last */
  0,                                 /* Default */
  MMDL_GEN_POWERRANGE_MIN,           /* RangeMin */
  MMDL_GEN_POWERRANGE_MAX            /* RangeMax */
};

static uint16_t testAppElem0Scenes[MMDL_SCENE_STATE_CNT + MMDL_NUM_OF_SCENES];

static mmdlGenLevelState_t testAppElem1GenLevelStates[MMDL_GEN_LEVEL_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlLightLightnessState_t testAppElem1LightLightnessStates[MMDL_LIGHT_LIGHTNESS_STATE_CNT +
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


static mmdlLightHslSrStoredState_t testAppElem1LightHslSrState =
{
  .minHue = 0,
  .defaultHue = 1,
  .maxHue = 0xFFFF,
  .defaultSat = 1,
  .minSat = 0,
  .maxSat = 0xFFFF
};

static mmdlGenLevelState_t testAppElem2GenLevelStates[MMDL_GEN_LEVEL_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlLightHslHueStoredState_t testAppElem2HueState =
{
  .presentHue = 0
};

static mmdlGenLevelState_t testAppElem3GenLevelStates[MMDL_GEN_LEVEL_STATE_CNT + MMDL_NUM_OF_SCENES];
static mmdlLightHslSatStoredState_t testAppElem3SatState =
{
  .presentSat = 0
};

/*! Descriptor for the element 0 instance of the Generic OnOff Server */
static mmdlGenOnOffSrDesc_t testAppElem0GenOnOffSr =
{
  .pStoredStates        = testAppElem01GenOnOffStates[0]
};

/*! Descriptor for the element 0 instance of the Generic OnOff Level Server */
static mmdlGenLevelSrDesc_t testAppElem0GenLevelSr =
{
  .pStoredStates        = testAppElem0GenLevelStates
};

/*! Descriptor for the element 0 instance of the Generic Default Transition Time Server */
static mmdlGenDefaultTransSrDesc_t testAppElem0GenDefaultTransSr =
{
  .pStoredStates = testAppElem01GenDefaultTransStates[0]
};

/*! Descriptor for the element 0 instance of the Generic Battery Server */
static mmdlGenBatterySrDesc_t testAppElem0GenBatterySr =
{
  .pStoredStates = testAppElem0GenBatteryStates
};

/*! Descriptor for the element 0 instance of the Scene Server */
static mmdlSceneSrDesc_t testAppElem0SceneSr =
{
  .pStoredScenes = testAppElem0Scenes
};

/*! Descriptor for the element 0 instance of the Generic Power OnOff Server */
static mmdlGenPowOnOffSrDesc_t testAppElem0GenPowOnOffSr =
{
  .pStoredStates = testAppElem01GenPowOnOffStates[0]
};

/*! Descriptor for the element 0 instance of the Generic Power Level Server */
static mmdlGenPowerLevelSrDesc_t testAppElem0GenPowLevelSr =
{
  .pStoredStates        = testAppElem0GenPowLevelStates
};

/*! Descriptor for the element 0 instance of Scheduler Server */
static mmdlSchedulerSrDesc_t testAppElem0SchedSr;

/*! Descriptor for the element 0 instance of Time Server */
static mmdlTimeSrDesc_t testAppElem0TimeSr =
{
  .storedTimeState = {0},
  .storedTimeZoneState = {0},
  .storedTimeDeltaState = {0},
  .storedTimeRoleState = {0}
};

/*! Descriptor for the element 1 instance of the Generic Default Transition Time Server */
static mmdlGenDefaultTransSrDesc_t testAppElem1GenDefaultTransSr =
{
  .pStoredStates = testAppElem01GenDefaultTransStates[1]
};

/*! Descriptor for the element 1 instance of the Generic Power OnOff Server */
static mmdlGenPowOnOffSrDesc_t testAppElem1GenPowOnOffSr =
{
  .pStoredStates = testAppElem01GenPowOnOffStates[1]
};

/*! Descriptor for the element 1 instance of the Generic OnOff Server */
static mmdlGenOnOffSrDesc_t testAppElem1GenOnOffSr =
{
  .pStoredStates        = testAppElem01GenOnOffStates[1]
};

/*! Descriptor for the element 1 instance of Generic Level Server */
static mmdlGenLevelSrDesc_t testAppElem1GenLevelSr =
{
  .pStoredStates        = testAppElem1GenLevelStates
};

/*! Descriptor for the element 1 instance of Light Lightness Server */
static mmdlLightLightnessSrDesc_t  testAppElem1LightLightnessSr =
{
  .pStoredStates = testAppElem1LightLightnessStates
};

/*! Descriptor for the element 1 instance of Light HSL Server */
static mmdlLightHslSrDesc_t testAppElem1LightHslSr =
{
  .pStoredState        = &testAppElem1LightHslSrState
};

/*! Descriptor for the element 2 instance of Generic Level Server */
static mmdlGenLevelSrDesc_t testAppElem2GenLevelSr =
{
  .pStoredStates        = testAppElem2GenLevelStates
};


/*! Descriptor for the element 2 instance of Light HSL Hue Server */
static mmdlLightHslHueSrDesc_t testAppElem2LightHslHueSr =
{
  .pStoredState        = &testAppElem2HueState
};

/*! Descriptor for the element 3 instance of Generic Level Server */
static mmdlGenLevelSrDesc_t testAppElem3GenLevelSr =
{
  .pStoredStates        = testAppElem3GenLevelStates
};

/*! Descriptor for the element 3 instance of Light HSL Saturation Server */
static mmdlLightHslSatSrDesc_t testAppElem3LightHslSatSr =
{
  .pStoredState        = &testAppElem3SatState
};

/*! List of Vendor models supported on element 0 */
static const meshVendorModel_t  testAppElem0VendorTestModelList[] =
{
  {
    .opcodeCount        = 1,
    .pRcvdOpcodeArray   = mmdlVendorTestClRcvdOpcodes,
    .pHandlerId         = &mmdlVendorTestClHandlerId,
    .modelId            = (meshVendorModelId_t)MMDL_VENDOR_TEST_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/*! List of SIG models supported on element 0 */
static const meshSigModel_t  testAppElem0SigModelList[] =
{
  {
    .opcodeCount        = MESH_HT_SR_NUM_RECVD_OPCODES,
    .pRcvdOpcodeArray   = meshHtSrRcvdOpcodes,
    .pHandlerId         = &meshHtSrHandlerId,
    .modelId            = (meshSigModelId_t)MESH_HT_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0HtSrDesc,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MESH_HT_CL_NUM_RECVD_OPCODES,
    .pRcvdOpcodeArray   = meshHtClRcvdOpcodes,
    .pHandlerId         = &meshHtClHandlerId,
    .modelId            = (meshSigModelId_t)MESH_HT_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_ONOFF_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenOnOffClRcvdOpcodes,
    .pHandlerId         = &mmdlGenOnOffClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_ONOFF_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0GenOnOffSr,
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
    .opcodeCount        = MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0GenPowOnOffSr,
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
    .opcodeCount        = MMDL_GEN_LEVEL_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelClRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0GenLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_LEVEL_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowerLevelClRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowerLevelClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_LEVEL_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowerLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowerLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0GenPowLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_LEVELSETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowerLevelSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowerLevelSetupSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_LEVELSETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_DEFAULT_TRANS_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenDefaultTransClRcvdOpcodes,
    .pHandlerId         = &mmdlGenDefaultTransClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_DEFAULT_TRANS_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_DEFAULT_TRANS_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenDefaultTransSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenDefaultTransSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0GenDefaultTransSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_BATTERY_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenBatteryClRcvdOpcodes,
    .pHandlerId         = &mmdlGenBatteryClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_BATTERY_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_BATTERY_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenBatterySrRcvdOpcodes,
    .pHandlerId         = &mmdlGenBatterySrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_BATTERY_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0GenBatterySr,
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
    .opcodeCount        = MMDL_TIME_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlTimeClRcvdOpcodes,
    .pHandlerId         = &mmdlTimeClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_TIME_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_TIME_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlTimeSrRcvdOpcodes,
    .pHandlerId         = &mmdlTimeSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_TIME_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0TimeSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_TIME_SETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlTimeSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlTimeSetupSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_TIMESETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCENE_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSceneClRcvdOpcodes,
    .pHandlerId         = &mmdlSceneClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCENE_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCENE_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSceneSrRcvdOpcodes,
    .pHandlerId         = &mmdlSceneSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCENE_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0SceneSr,
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
    .opcodeCount        = MMDL_LIGHT_HSL_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslClRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCHEDULER_CL_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSchedulerClRcvdOpcodes,
    .pHandlerId         = &mmdlSchedulerClHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCHEDULER_CL_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCHEDULER_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSchedulerSrRcvdOpcodes,
    .pHandlerId         = &mmdlSchedulerSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCHEDULER_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem0SchedSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_SCHEDULER_SETUP_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlSchedulerSetupSrRcvdOpcodes,
    .pHandlerId         = &mmdlSchedulerSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_SCHEDULER_SETUP_SR_MDL_ID,
    .pModelDescriptor   = NULL,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/*! List of SIG models supported on element 1 */
static const meshSigModel_t  testAppElem1SigModelList[] =
{
  {
    .opcodeCount        = MMDL_GEN_DEFAULT_TRANS_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenDefaultTransSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenDefaultTransSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem1GenDefaultTransSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenPowOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenPowOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_POWER_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem1GenPowOnOffSr,
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
    .opcodeCount        = MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenOnOffSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenOnOffSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_ONOFF_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem1GenOnOffSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem1GenLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_LIGHTNESS_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightLightnessSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightLightnessSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_LIGHTNESS_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem1LightLightnessSr,
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
    .opcodeCount        = MMDL_LIGHT_HSL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem1LightHslSr,
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

/*! List of SIG models supported on element 2 */
const meshSigModel_t  testAppElem2SigModelList[] =
{
  {
    .opcodeCount        = MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem2GenLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_HSL_HUE_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslHueSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslHueSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_HUE_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem2LightHslHueSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  }
};

/*! List of SIG models supported on element 3 */
const meshSigModel_t  testAppElem3SigModelList[] =
{
  {
    .opcodeCount        = MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlGenLevelSrRcvdOpcodes,
    .pHandlerId         = &mmdlGenLevelSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_GEN_LEVEL_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem3GenLevelSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
  {
    .opcodeCount        = MMDL_LIGHT_HSL_SAT_SR_NUM_RCVD_OPCODES,
    .pRcvdOpcodeArray   = mmdlLightHslSatSrRcvdOpcodes,
    .pHandlerId         = &mmdlLightHslSatSrHandlerId,
    .modelId            = (meshSigModelId_t)MMDL_LIGHT_HSL_SAT_SR_MDL_ID,
    .pModelDescriptor   = (void *)&testAppElem3LightHslSatSr,
    .subscrListSize     = 2,
    .appKeyBindListSize = 2
  },
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh ADV Bearer configure parameters */
const advBearerCfg_t testAppAdvBearerCfg =
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
const gattBearerSrCfg_t testAppGattBearerSrCfg =
{
  300,                      /*!< Minimum advertising interval in 0.625 ms units */
  300,                      /*!< Maximum advertising interval in 0.625 ms units */
  DM_ADV_CONN_UNDIRECT,    /*!< The advertising type */
};

/*! Mesh GATT Bearer Client configure parameters */
const gattBearerClCfg_t testAppProxyClCfg =
{
  96,                           /*!< The scan interval, in 0.625 ms units */
  48,                           /*!< The scan window, in 0.625 ms units */
  DM_DISC_MODE_GENERAL,         /*!< The GAP discovery mode */
  DM_SCAN_TYPE_ACTIVE,          /*!< The scan type (active or passive) */
  ATT_UUID_MESH_PROXY_SERVICE   /*!< The searched service UUID */
};

/*! Mesh GATT Bearer Client configure parameters */
const gattBearerClCfg_t testAppPrvClCfg =
{
  96,                           /*!< The scan interval, in 0.625 ms units */
  48,                           /*!< The scan window, in 0.625 ms units */
  DM_DISC_MODE_GENERAL,         /*!< The GAP discovery mode */
  DM_SCAN_TYPE_ACTIVE,          /*!< The scan type (active or passive) */
  ATT_UUID_MESH_PRV_SERVICE     /*!< The searched service UUID */
};

/*! Mesh GATT Bearer Client connection parameters */
const hciConnSpec_t testAppConnCfg =
{
  40,                           /*!< Minimum connection interval in 1.25ms units */
  40,                           /*!< Maximum connection interval in 1.25ms units */
  0,                            /*!< Connection latency */
  600,                          /*!< Supervision timeout in 10ms units */
  0,                            /*!< Unused */
  0                             /*!< Unused */
};

/*! Mesh Provisioning Server configuration parameters*/
meshPrvSrCfg_t testAppMeshPrvSrCfg =
{
  {0},                       /*!< Device UUID.  */
  1000,                      /*!< Provisioning Bearer advertising interval */
  0,                         /*!< Provisioning Bearer ADV interface ID */
  FALSE,                     /*!< Auto-restart Provisioning */
};

/*! List of elements supported on this node */
const meshElement_t testAppElements[TESTAPP_ELEMENT_COUNT] =
{
  {
    .locationDescriptor   = 0xA5A5,
    .numSigModels         = sizeof(testAppElem0SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 1,
    .pSigModelArray       = testAppElem0SigModelList,
    .pVendorModelArray    = testAppElem0VendorTestModelList,
  },
  {
    .locationDescriptor   = 0xA5A6,
    .numSigModels         = sizeof(testAppElem1SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = testAppElem1SigModelList,
    .pVendorModelArray    = NULL,
  },
  {
    .locationDescriptor   = 0xA5A7,
    .numSigModels         = sizeof(testAppElem2SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = testAppElem2SigModelList,
    .pVendorModelArray    = NULL,
  },
  {
    .locationDescriptor   = 0xA5A8,
    .numSigModels         = sizeof(testAppElem3SigModelList) / sizeof(meshSigModel_t),
    .numVendorModels      = 0,
    .pSigModelArray       = testAppElem3SigModelList,
    .pVendorModelArray    = NULL,
  }
};

/*! Mesh Provisioning Server Device UUID */
uint8_t testAppPrvSrDevUuid[MESH_PRV_DEVICE_UUID_SIZE] =
{ 0x70, 0xcf, 0x7c, 0x97, 0x32, 0xa3, 0x45, 0xb6, 0x91, 0x49, 0x48, 0x10, 0xd2, 0xe9, 0xcb, 0xf4 };

/*! Mesh Unprovisioned Device info */
meshPrvSrUnprovisionedDeviceInfo_t testAppPrvSrUpdInfo =
{
  .pCapabilities            = &testAppPrvSrCapabilities,
  .pDeviceUuid              = testAppPrvSrDevUuid,
  .oobInfoSrc               = MESH_PRV_OOB_INFO_OTHER,
  .pStaticOobData           = testAppPrvSrStaticOobData,
  .uriLen                   = MESH_PRV_URI_DATA_LEN,
  .pUriData                 = testAppPrvSrUriData,
  .pAppEccKeys              = NULL
};

/*! Mesh Provisioning Client session info */
meshPrvClSessionInfo_t testAppPrvClSessionInfo =
{
  .pDeviceUuid        = testAppPrvClDevUuid,
  .pDevicePublicKey   = &testAppPrvClPeerOobPublicKey,
  .pStaticOobData     = testAppPrvClStaticOobData,
  .pAppEccKeys        = NULL,
  .pData              = &testAppPrvClProvData,
  .attentionDuration  = 0
};

/*! Mesh Stack configuration structure */
meshConfig_t testAppMeshConfig =
{
  .pElementArray = testAppElements,
  .elementArrayLen = TESTAPP_ELEMENT_COUNT,
  .pMemoryConfig = &testAppMeshMemConfig,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Generic OnPowerUp State on elements 0 and 1.
 *
 *  \param[in] elementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void genOnPowerUpNvmSave(meshElementId_t elementId)
{
  WsfNvmWriteData(MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID, (uint8_t *)testAppElem01GenPowOnOffStates,
                             sizeof(testAppElem01GenPowOnOffStates), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Generic OnOff State on elements 0 and 1.
 *
 *  \param[in] elementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void genOnOffNvmSave(meshElementId_t elementId)
{
  WsfNvmWriteData(MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID, (uint8_t *)testAppElem01GenOnOffStates,
                             sizeof(testAppElem01GenOnOffStates), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Generic Power Level State on element 0.
 *
 *  \param[in] elementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void genPowLevelNvmSave(meshElementId_t elementId)
{
  WSF_ASSERT(elementId == 0);

  WsfNvmWriteData(MMDL_NVM_GEN_POWER_LEVEL_STATE_DATASET_ID, (uint8_t *)testAppElem0GenPowLevelStates,
                               sizeof(testAppElem0GenPowLevelStates), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Light Lightness State on element 1.
 *
 *  \param[in] elementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightLightnessNvmSave(meshElementId_t elementId)
{
  WsfNvmWriteData(MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID, (uint8_t *)testAppElem1LightLightnessStates,
                             sizeof(testAppElem1LightLightnessStates), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     NVM Save wrapper for Light HSL State on element 0.
 *
 *  \param[in] elementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightHslNvmSave(meshElementId_t elementId)
{
  WSF_ASSERT(elementId == ELEM_LIGHT);

  WsfNvmWriteData(MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID, (uint8_t *)&testAppElem1LightHslSrState,
                        sizeof(testAppElem1LightHslSrState), NULL);

  /* Remove compiler warning. */
  (void)elementId;
}

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
void TestAppConfig(void)
{
  bool_t retVal;

  memset(testAppElem01GenPowOnOffStates, 0, sizeof(testAppElem01GenPowOnOffStates));

  retVal = WsfNvmReadData(MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID, (uint8_t *)testAppElem01GenPowOnOffStates,
                             sizeof(testAppElem01GenPowOnOffStates), NULL);

  memset(testAppElem01GenOnOffStates, 0, sizeof(testAppElem01GenOnOffStates));

  retVal = WsfNvmReadData(MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID, (uint8_t *)testAppElem01GenOnOffStates,
                             sizeof(testAppElem01GenOnOffStates), NULL);

  retVal = WsfNvmReadData(MMDL_NVM_GEN_POWER_LEVEL_STATE_DATASET_ID, (uint8_t *)testAppElem0GenPowLevelStates,
                             sizeof(testAppElem0GenPowLevelStates), NULL);

  retVal = WsfNvmReadData(MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID, (uint8_t *)testAppElem1LightLightnessStates,
                             sizeof(testAppElem1LightLightnessStates), NULL);

  retVal = WsfNvmReadData(MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID, (uint8_t *)&testAppElem1LightHslSrState,
                             sizeof(testAppElem1LightHslSrState), NULL);

  /* Link NVM save functions to wrappers. */
  testAppElem0GenPowOnOffSr.fNvmSaveStates    = genOnPowerUpNvmSave;
  testAppElem0GenOnOffSr.fNvmSaveStates       = genOnOffNvmSave;
  testAppElem0GenPowLevelSr.fNvmSaveStates    = genPowLevelNvmSave;
  testAppElem1GenOnOffSr.fNvmSaveStates       = genOnOffNvmSave;
  testAppElem1GenPowOnOffSr.fNvmSaveStates    = genOnPowerUpNvmSave;
  testAppElem1LightLightnessSr.fNvmSaveStates = lightLightnessNvmSave;
  testAppElem1LightHslSr.fNvmSaveStates       = lightHslNvmSave;

  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief  Erase runtime configurations for Test App.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppConfigErase(void)
{
  WsfNvmEraseData(MMDL_NVM_GEN_ONPOWERUP_STATE_DATASET_ID, NULL);
  WsfNvmEraseData(MMDL_NVM_GEN_ONOFF_STATE_DATASET_ID, NULL);
  WsfNvmEraseData(MMDL_NVM_GEN_POWER_LEVEL_STATE_DATASET_ID, NULL);
  WsfNvmEraseData(MMDL_NVM_LIGHT_LIGHTNESS_STATE_DATASET_ID, NULL);
  WsfNvmEraseData(MMDL_NVM_LIGHT_HSL_STATE_DATASET_ID, NULL);
}
