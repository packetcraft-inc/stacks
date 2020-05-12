/*************************************************************************************************/
/*!
 *  \file   mmdl_common_main.c
 *
 *  \brief  Implementation of the Model common utilities.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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
 *
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "wsf_os.h"

#include "mmdl_types.h"
#include "mesh_ht_cl_api.h"
#include "mesh_ht_mdl_api.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_bindings_api.h"
#include "mmdl_defs.h"
#include "mmdl_gen_battery_cl_api.h"
#include "mmdl_gen_battery_sr_api.h"
#include "mmdl_gen_default_trans_cl_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_powerlevelsetup_sr_api.h"
#include "mmdl_gen_powerlevel_cl_api.h"
#include "mmdl_gen_powerlevel_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_light_hsl_cl_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_ctl_cl_api.h"
#include "mmdl_light_ctl_sr_api.h"
#include "mmdl_light_ctl_temp_sr_api.h"
#include "mmdl_opcodes.h"
#include "mmdl_scene_cl_api.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_scheduler_cl_api.h"
#include "mmdl_scheduler_sr_api.h"
#include "mmdl_timesetup_sr_api.h"
#include "mmdl_time_cl_api.h"
#include "mmdl_time_sr_api.h"
#include "mmdl_types.h"
#include "mmdl_vendor_test_cl_api.h"

#include "mmdl_common.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define MESH_HT_CL_OFFSET                 0
#define MESH_HT_SR_OFFSET                 (MESH_HT_CL_OFFSET                + MESH_HT_CL_MAX_EVENT)
#define MMDL_GEN_BATTERY_CL_OFFSET        (MESH_HT_SR_OFFSET                + MESH_HT_SR_MAX_EVENT)
#define MMDL_GEN_BATTERY_SR_OFFSET        (MMDL_GEN_BATTERY_CL_OFFSET       + MMDL_GEN_BATTERY_CL_MAX_EVENT)
#define MMDL_GEN_DEFAULT_TRANS_CL_OFFSET  (MMDL_GEN_BATTERY_SR_OFFSET       + MMDL_GEN_BATTERY_SR_MAX_EVENT)
#define MMDL_GEN_DEFAULT_TRANS_SR_OFFSET  (MMDL_GEN_DEFAULT_TRANS_CL_OFFSET + MMDL_GEN_DEFAULT_TRANS_CL_MAX_EVENT)
#define MMDL_GEN_LEVEL_CL_OFFSET          (MMDL_GEN_DEFAULT_TRANS_SR_OFFSET + MMDL_GEN_DEFAULT_TRANS_SR_MAX_EVENT)
#define MMDL_GEN_LEVEL_SR_OFFSET          (MMDL_GEN_LEVEL_CL_OFFSET         + MMDL_GEN_LEVEL_CL_MAX_EVENT)
#define MMDL_GEN_ONOFF_CL_OFFSET          (MMDL_GEN_LEVEL_SR_OFFSET         + MMDL_GEN_LEVEL_SR_MAX_EVENT)
#define MMDL_GEN_ONOFF_SR_OFFSET          (MMDL_GEN_ONOFF_CL_OFFSET         + MMDL_GEN_ONOFF_CL_MAX_EVENT)
#define MMDL_GEN_POWER_ONOFF_CL_OFFSET    (MMDL_GEN_ONOFF_SR_OFFSET         + MMDL_GEN_ONOFF_SR_MAX_EVENT)
#define MMDL_GEN_POWER_ONOFF_SR_OFFSET    (MMDL_GEN_POWER_ONOFF_CL_OFFSET   + MMDL_GEN_POWER_ONOFF_CL_MAX_EVENT)
#define MMDL_GEN_POWER_LEVEL_CL_OFFSET    (MMDL_GEN_POWER_ONOFF_SR_OFFSET   + MMDL_GEN_POWER_ONOFF_SR_MAX_EVENT)
#define MMDL_GEN_POWER_LEVEL_SR_OFFSET    (MMDL_GEN_POWER_LEVEL_CL_OFFSET   + MMDL_GEN_POWER_CL_MAX_EVENT)
#define MMDL_LIGHT_LIGHTNESS_CL_OFFSET    (MMDL_GEN_POWER_LEVEL_SR_OFFSET   + MMDL_GEN_POWER_SR_MAX_EVENT)
#define MMDL_LIGHT_LIGHTNESS_SR_OFFSET    (MMDL_LIGHT_LIGHTNESS_CL_OFFSET   + MMDL_LIGHT_LIGHTNESS_CL_MAX_EVENT)
#define MMDL_LIGHT_HSL_CL_OFFSET          (MMDL_LIGHT_LIGHTNESS_SR_OFFSET   + MMDL_LIGHT_LIGHTNESS_SR_MAX_EVENT)
#define MMDL_LIGHT_HSL_SR_OFFSET          (MMDL_LIGHT_HSL_CL_OFFSET         + MMDL_LIGHT_HSL_CL_MAX_EVENT)
#define MMDL_SCENE_CL_OFFSET              (MMDL_LIGHT_HSL_SR_OFFSET         + MMDL_LIGHT_HSL_SR_MAX_EVENT)
#define MMDL_SCHEDULER_CL_OFFSET          (MMDL_SCENE_CL_OFFSET             + MMDL_SCENE_CL_MAX_EVENT)
#define MMDL_SCHEDULER_SR_OFFSET          (MMDL_SCHEDULER_CL_OFFSET         + MMDL_SCHEDULER_CL_MAX_EVENT)
#define MMDL_TIME_CL_OFFSET               (MMDL_SCHEDULER_SR_OFFSET         + MMDL_SCHEDULER_SR_MAX_EVENT)
#define MMDL_TIME_SR_OFFSET               (MMDL_TIME_CL_OFFSET              + MMDL_TIME_CL_MAX_EVENT)
#define MMDL_LIGHT_CTL_CL_OFFSET          (MMDL_TIME_SR_OFFSET              + MMDL_TIME_SR_MAX_EVENT)
#define MMDL_LIGHT_CTL_SR_OFFSET          (MMDL_LIGHT_CTL_CL_OFFSET         + MMDL_LIGHT_CTL_CL_MAX_EVENT)


/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Model event lengths. */
static const uint16_t mmdlEventCbackLen[] =
{
  sizeof(meshHtClAttentionStatusEvt_t),            /*!< MESH_HT_CL_ATTENTION_STATUS_EVENT */
  sizeof(meshHtClFaultStatusEvt_t),                /*!< MESH_HT_CL_CURRENT_STATUS_EVENT */
  sizeof(meshHtClFaultStatusEvt_t),                /*!< MESH_HT_CL_FAULT_STATUS_EVENT */
  sizeof(meshHtClPeriodStatusEvt_t),               /*!< MESH_HT_CL_PERIOD_STATUS_EVENT */
  sizeof(meshHtSrTestStartEvt_t),                  /*!< MESH_HT_SR_TEST_START_EVENT */
  sizeof(mmdlGenBatteryClStatusEvent_t),           /*!< MMDL_GEN_BATTERY_CL_STATUS_EVENT */
  sizeof(mmdlGenBatterySrCurrentState_t),          /*!< MMDL_GEN_BATTERY_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenBatterySrStateUpdate_t),           /*!< MMDL_GEN_BATTERY_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlGenDefaultTransClStatusEvent_t),      /*!< MMDL_GEN_DEFAULT_TRANS_CL_STATUS_EVENT */
  sizeof(mmdlGenDefaultTransSrCurrentState_t),     /*!< MMDL_GEN_DEFAULT_TRANS_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenDefaultTransSrStateUpdate_t),      /*!< MMDL_GEN_DEFAULT_TRANS_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlGenLevelClStatusEvent_t),             /*!< MMDL_GEN_LEVEL_CL_STATUS_EVENT */
  sizeof(mmdlGenLevelSrCurrentState_t),            /*!< MMDL_GEN_LEVEL_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenLevelSrStateUpdate_t),             /*!< MMDL_GEN_LEVEL_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlGenOnOffClStatusEvent_t),             /*!< MMDL_GEN_ONOFF_CL_STATUS_EVENT */
  sizeof(mmdlGenOnOffSrCurrentState_t),            /*!< MMDL_GEN_ONOFF_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenOnOffSrStateUpdate_t),             /*!< MMDL_GEN_ONOFF_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlGenPowOnOffClStatusEvent_t),          /*!< MMDL_GEN_POWER_ONOFF_CL_STATUS_EVENT */
  sizeof(mmdlGenPowOnOffSrCurrentState_t),         /*!< MMDL_GEN_POWER_ONOFF_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenPowOnOffSrStateUpdate_t),          /*!< MMDL_GEN_POWER_ONOFF_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlGenPowerDefaultClStatusEvent_t),      /*!< MMDL_GEN_POWER_DEFAULT_CL_STATUS_EVENT */
  sizeof(mmdlGenPowerLastClStatusEvent_t),         /*!< MMDL_GEN_POWER_LAST_CL_STATUS_EVENT */
  sizeof(mmdlGenPowerLevelClStatusEvent_t),        /*!< MMDL_GEN_POWER_LEVEL_CL_STATUS_EVENT */
  sizeof(mmdlGenPowerRangeClStatusEvent_t),        /*!< MMDL_GEN_POWER_RANGE_CL_STATUS_EVENT */
  sizeof(mmdlGenPowerLevelSrCurrentState_t),       /*!< MMDL_GEN_POWER_DEFAULT_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenPowerLevelSrStateUpdate_t),        /*!< MMDL_GEN_POWER_DEFAULT_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlGenPowerLevelSrCurrentState_t),       /*!< MMDL_GEN_POWER_LAST_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenPowerLevelSrStateUpdate_t),        /*!< MMDL_GEN_POWER_LAST_SR_STATE_UPDATE_EVENT TODO useful?*/
  sizeof(mmdlGenPowerLevelSrCurrentState_t),       /*!< MMDL_GEN_POWER_LEVEL_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlGenPowerLevelSrStateUpdate_t),        /*!< MMDL_GEN_POWER_LEVEL_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlGenPowerLevelSrCurrentState_t),       /*!< MMDL_GEN_POWER_RANGE_SR_CURRENT_EVENT */
  sizeof(mmdlGenPowerLevelSrStateUpdate_t),        /*!< MMDL_GEN_POWER_RANGE_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightLightnessActualStatusParam_t),   /*!< MMDL_LIGHT_LIGHTNESS_CL_STATUS_EVENT */
  sizeof(mmdlLightLightnessDefaultStatusParam_t),  /*!< MMDL_LIGHT_LIGHTNESS_DEFAULT_CL_STATUS_EVENT */
  sizeof(mmdlLightLightnessLastStatusParam_t),     /*!< MMDL_LIGHT_LIGHTNESS_LAST_CL_STATUS_EVENT */
  sizeof(mmdlLightLightnessLinearStatusParam_t),   /*!< MMDL_LIGHT_LIGHTNESS_LINEAR_CL_STATUS_EVENT */
  sizeof(mmdlLightLightnessRangeStatusParam_t),    /*!< MMDL_LIGHT_LIGHTNESS_RANGE_CL_STATUS_EVENT */
  sizeof(mmdlLightLightnessSrCurrentState_t),      /*!< MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlLightLightnessSrStateUpdate_t),       /*!< MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightLightnessSrCurrentState_t),      /*!< MMDL_LIGHT_LIGHTNESS_LAST_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlLightLightnessSrCurrentState_t),      /*!< MMDL_LIGHT_LIGHTNESS_LINEAR_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlLightLightnessSrStateUpdate_t),       /*!< MMDL_LIGHT_LIGHTNESS_LINEAR_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightLightnessSrCurrentState_t),      /*!< MMDL_LIGHT_LIGHTNESS_RANGE_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlLightLightnessSrStateUpdate_t),       /*!< MMDL_LIGHT_LIGHTNESS_RANGE_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightLightnessSrStateUpdate_t),       /*!< MMDL_LIGHT_LIGHTNESS_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlLightLightnessSrCurrentState_t),      /*!< MMDL_LIGHT_LIGHTNESS_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightHslClDefStatusEvent_t),          /*!< MMDL_LIGHT_HSL_CL_DEF_STATUS_EVENT */
  sizeof(mmdlLightHslClHueStatusEvent_t),          /*!< MMDL_LIGHT_HSL_CL_HUE_STATUS_EVENT */
  sizeof(mmdlLightHslClRangeStatusEvent_t),        /*!< MMDL_LIGHT_HSL_CL_RANGE_STATUS_EVENT */
  sizeof(mmdlLightHslClSatStatusEvent_t),          /*!< MMDL_LIGHT_HSL_CL_SAT_STATUS_EVENT */
  sizeof(mmdlLightHslClStatusEvent_t),             /*!< MMDL_LIGHT_HSL_CL_STATUS_EVENT */
  sizeof(mmdlLightHslClStatusEvent_t),             /*!< MMDL_LIGHT_HSL_CL_TARGET_STATUS_EVENT */
  sizeof(mmdlLightHslHueSrStateUpdate_t),          /*!< MMDL_LIGHT_HSL_HUE_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightHslSatSrStateUpdate_t),          /*!< MMDL_LIGHT_HSL_SAT_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightHslSrStateUpdate_t),             /*!< MMDL_LIGHT_HSL_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightHslSrStateUpdate_t),             /*!< MMDL_LIGHT_HSL_SR_RANGE_STATE_UPDATE_EVENT */
  sizeof(mmdlSceneClRegStatusEvent_t),             /*!< MMDL_SCENE_CL_REG_STATUS_EVENT */
  sizeof(mmdlSceneClStatusEvent_t),                /*!< MMDL_SCENE_CL_STATUS_EVENT */
  sizeof(mmdlSchedulerClActionStatusEvent_t),      /*!< MMDL_SCHEDULER_CL_ACTION_STATUS_EVENT */
  sizeof(mmdlSchedulerClStatusEvent_t),            /*!< MMDL_SCHEDULER_CL_STATUS_EVENT */
  sizeof(mmdlSchedulerSrStartScheduleEvent_t),     /*!< MMDL_SCHEDULER_SR_START_SCHEDULE_EVENT */
  sizeof(mmdlSchedulerSrStopScheduleEvent_t),      /*!< MMDL_SCHEDULER_SR_STOP_SCHEDULE_EVENT */
  sizeof(mmdlTimeClDeltaStatusEvent_t),            /*!< MMDL_TIMEDELTA_CL_STATUS_EVENT */
  sizeof(mmdlTimeClRoleStatusEvent_t),             /*!< MMDL_TIMEROLE_CL_STATUS_EVENT */
  sizeof(mmdlTimeClZoneStatusEvent_t),             /*!< MMDL_TIMEZONE_CL_STATUS_EVENT */
  sizeof(mmdlTimeClStatusEvent_t),                 /*!< MMDL_TIME_CL_STATUS_EVENT */
  sizeof(mmdlTimeSrCurrentState_t),                /*!< MMDL_TIMEDELTA_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlTimeSrStateUpdate_t),                 /*!< MMDL_TIMEDELTA_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlTimeSrCurrentState_t),                /*!< MMDL_TIMEROLE_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlTimeSrStateUpdate_t),                 /*!< MMDL_TIMEROLE_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlTimeSrCurrentState_t),                /*!< MMDL_TIMEZONE_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlTimeSrStateUpdate_t),                 /*!< MMDL_TIMEZONE_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlTimeSrCurrentState_t),                /*!< MMDL_TIME_SR_CURRENT_STATE_EVENT */
  sizeof(mmdlTimeSrStateUpdate_t),                 /*!< MMDL_TIME_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightCtlClDefStatusEvent_t),          /*!< MMDL_LIGHT_CTL_CL_DEF_STATUS_EVENT */
  sizeof(mmdlLightCtlClTemperatureStatusEvent_t),  /*!< MMDL_LIGHT_CTL_CL_TEMP_STATUS_EVENT */
  sizeof(mmdlLightCtlClRangeStatusEvent_t),        /*!< MMDL_LIGHT_CTL_CL_RANGE_STATUS_EVENT */
  sizeof(mmdlLightCtlClStatusEvent_t),             /*!< MMDL_LIGHT_CTL_CL_STATUS_EVENT */
  sizeof(mmdlLightCtlTempSrStateUpdate_t),         /*!< MMDL_LIGHT_CTL_TEMP_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightCtlSrStateUpdate_t),             /*!< MMDL_LIGHT_CTL_SR_STATE_UPDATE_EVENT */
  sizeof(mmdlLightCtlSrStateUpdate_t)              /*!< MMDL_LIGHT_CTL_SR_RANGE_STATE_UPDATE_EVENT */
};

static const uint8_t mmdlEventOffsets[] =
{
  MESH_HT_CL_OFFSET,                               /*!< MESH_MMDL_HT_CL_EVENT */
  MESH_HT_SR_OFFSET,                               /*!< MESH_MMDL_HT_SR_EVENT */
  MMDL_GEN_BATTERY_CL_OFFSET,                      /*!< MESH_MMDL_GEN_BATTERY_CL_EVENT */
  MMDL_GEN_BATTERY_SR_OFFSET,                      /*!< MESH_MMDL_GEN_BATTERY_SR_EVENT */
  MMDL_GEN_DEFAULT_TRANS_CL_OFFSET,                /*!< MESH_MMDL_GEN_DEFAULT_TRANS_CL_EVENT */
  MMDL_GEN_DEFAULT_TRANS_SR_OFFSET,                /*!< MESH_MMDL_GEN_DEFAULT_TRANS_SR_EVENT */
  MMDL_GEN_LEVEL_CL_OFFSET,                        /*!< MESH_MMDL_GEN_LEVEL_CL_EVENT */
  MMDL_GEN_LEVEL_SR_OFFSET,                        /*!< MESH_MMDL_GEN_LEVEL_SR_EVENT */
  MMDL_GEN_ONOFF_CL_OFFSET,                        /*!< MESH_MMDL_GEN_ONOFF_CL_EVENT */
  MMDL_GEN_ONOFF_SR_OFFSET,                        /*!< MESH_MMDL_GEN_ONOFF_SR_EVENT */
  MMDL_GEN_POWER_ONOFF_CL_OFFSET,                  /*!< MESH_MMDL_GEN_POWER_ONOFF_CL_EVENT */
  MMDL_GEN_POWER_ONOFF_SR_OFFSET,                  /*!< MESH_MMDL_GEN_POWER_ONOFF_SR_EVENT */
  MMDL_GEN_POWER_LEVEL_CL_OFFSET,                  /*!< MESH_MMDL_GEN_POWER_LEVEL_CL_EVENT */
  MMDL_GEN_POWER_LEVEL_SR_OFFSET,                  /*!< MESH_MMDL_GEN_POWER_LEVEL_SR_EVENT */
  MMDL_LIGHT_LIGHTNESS_CL_OFFSET,                  /*!< MESH_MMDL_LIGHT_LIGHTNESS_CL_EVENT */
  MMDL_LIGHT_LIGHTNESS_SR_OFFSET,                  /*!< MESH_MMDL_LIGHT_LIGHTNESS_SR_EVENT */
  MMDL_LIGHT_HSL_CL_OFFSET,                        /*!< MESH_MMDL_LIGHT_HSL_CL_EVENT */
  MMDL_LIGHT_HSL_SR_OFFSET,                        /*!< MESH_MMDL_LIGHT_HSL_SR_EVENT */
  MMDL_SCENE_CL_OFFSET,                            /*!< MESH_MMDL_SCENE_CL_EVENT */
  MMDL_SCHEDULER_CL_OFFSET,                        /*!< MESH_MMDL_SCHEDULER_CL_EVENT */
  MMDL_SCHEDULER_SR_OFFSET,                        /*!< MESH_MMDL_SCHEDULER_SR_EVENT */
  MMDL_TIME_CL_OFFSET,                             /*!< MESH_MMDL_TIME_CL_EVENT */
  MMDL_TIME_SR_OFFSET,                             /*!< MESH_MMDL_TIME_SR_EVENT */
  MMDL_LIGHT_CTL_CL_OFFSET,                        /*!< MESH_MMDL_LIGHT_CTL_CL_EVENT */
  MMDL_LIGHT_CTL_SR_OFFSET                         /*!< MESH_MMDL_LIGHT_CTL_SR_EVENT */
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Generic empty event callback, used by all models.
 *
 *  \param[in] pEvent  Pointer to model event
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlEmptyCback(const wsfMsgHdr_t *pEvent)
{
  (void)pEvent;
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Model callback event.
 *
 *  \param[in] pMeshCfgEvt  Mesh Model callback event.
 *
 *  \return    Size of Mesh Model callback event.
 */
/*************************************************************************************************/
uint16_t MmdlSizeOfEvt(wsfMsgHdr_t *pEvt)
{
  if ((pEvt->event >= MMDL_CBACK_START) && (pEvt->event <= MMDL_CBACK_END))
  {
    return mmdlEventCbackLen[mmdlEventOffsets[pEvt->event - MMDL_CBACK_START] + pEvt->param];
  }

  return 0;
}
