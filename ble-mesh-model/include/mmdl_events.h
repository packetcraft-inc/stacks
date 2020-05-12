/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Model specification definitions.
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

#ifndef MMDL_EVENTS_H
#define MMDL_EVENTS_H

#ifdef __cplusplus
extern "C"
{
#endif

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
#include "mmdl_lightlightness_defs.h"
#include "mmdl_light_hsl_cl_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
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


typedef union mmdl_events_tag
{
  wsfMsgHdr_t                             hdr;
  meshHtClAttentionStatusEvt_t            meshHtClAttentionStatusEvt;            /*!< MESH_HT_CL_ATTENTION_STATUS_EVENT */
  meshHtClFaultStatusEvt_t                meshHtClFaultStatusEvt;                /*!< MESH_HT_CL_CURRENT_STATUS_EVENT
                                                                                      MESH_HT_CL_FAULT_STATUS_EVENT */
  meshHtClPeriodStatusEvt_t               meshHtClPeriodStatusEvt;               /*!< MESH_HT_CL_PERIOD_STATUS_EVENT */
  meshHtSrTestStartEvt_t                  meshHtSrTestStartEvt;                  /*!< MESH_HT_SR_TEST_START_EVENT */
  mmdlGenBatteryClStatusEvent_t           mmdlGenBatteryClStatusEvent;           /*!< MMDL_GEN_BATTERY_CL_STATUS_EVENT */
  mmdlGenBatterySrCurrentState_t          mmdlGenBatterySrCurrentState;          /*!< MMDL_GEN_BATTERY_SR_CURRENT_STATE_EVENT */
  mmdlGenBatterySrStateUpdate_t           mmdlGenBatterySrStateUpdate;           /*!< MMDL_GEN_BATTERY_SR_STATE_UPDATE_EVENT */
  mmdlGenDefaultTransClStatusEvent_t      mmdlGenDefaultTransClStatusEvent;      /*!< MMDL_GEN_DEFAULT_TRANS_CL_STATUS_EVENT */
  mmdlGenDefaultTransSrCurrentState_t     mmdlGenDefaultTransSrCurrentState;     /*!< MMDL_GEN_DEFAULT_TRANS_SR_CURRENT_STATE_EVENT */
  mmdlGenDefaultTransSrStateUpdate_t      mmdlGenDefaultTransSrStateUpdate;      /*!< MMDL_GEN_DEFAULT_TRANS_SR_STATE_UPDATE_EVENT */
  mmdlGenLevelClStatusEvent_t             mmdlGenLevelClStatusEvent;             /*!< MMDL_GEN_LEVEL_CL_STATUS_EVENT */
  mmdlGenLevelSrCurrentState_t            mmdlGenLevelSrCurrentState;            /*!< MMDL_GEN_LEVEL_SR_CURRENT_STATE_EVENT */
  mmdlGenLevelSrStateUpdate_t             mmdlGenLevelSrStateUpdate;             /*!< MMDL_GEN_LEVEL_SR_STATE_UPDATE_EVENT */
  mmdlGenOnOffClStatusEvent_t             mmdlGenOnOffClStatusEvent;             /*!< MMDL_GEN_ONOFF_CL_STATUS_EVENT */
  mmdlGenOnOffSrCurrentState_t            mmdlGenOnOffSrCurrentState;            /*!< MMDL_GEN_ONOFF_SR_CURRENT_STATE_EVENT */
  mmdlGenOnOffSrStateUpdate_t             mmdlGenOnOffSrStateUpdate;             /*!< MMDL_GEN_ONOFF_SR_STATE_UPDATE_EVENT */
  mmdlGenPowOnOffClStatusEvent_t          mmdlGenPowOnOffClStatusEvent;          /*!< MMDL_GEN_POWER_ONOFF_CL_STATUS_EVENT */
  mmdlGenPowOnOffSrCurrentState_t         mmdlGenPowOnOffSrCurrentState;         /*!< MMDL_GEN_POWER_ONOFF_SR_CURRENT_STATE_EVENT */
  mmdlGenPowOnOffSrStateUpdate_t          mmdlGenPowOnOffSrStateUpdate;          /*!< MMDL_GEN_POWER_ONOFF_SR_STATE_UPDATE_EVENT */
  mmdlGenPowerDefaultClStatusEvent_t      mmdlGenPowerDefaultClStatusEvent;      /*!< MMDL_GEN_POWER_DEFAULT_CL_STATUS_EVENT */
  mmdlGenPowerLastClStatusEvent_t         mmdlGenPowerLastClStatusEvent;         /*!< MMDL_GEN_POWER_LAST_CL_STATUS_EVENT */
  mmdlGenPowerLevelClStatusEvent_t        mmdlGenPowerLevelClStatusEvent;        /*!< MMDL_GEN_POWER_LEVEL_CL_STATUS_EVENT */
  mmdlGenPowerRangeClStatusEvent_t        mmdlGenPowerRangeClStatusEvent;        /*!< MMDL_GEN_POWER_RANGE_CL_STATUS_EVENT */
  mmdlGenPowerLevelSrCurrentState_t       mmdlGenPowerLevelSrCurrentState;       /*!< MMDL_GEN_POWER_DEFAULT_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_GEN_POWER_LAST_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_GEN_POWER_LEVEL_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_GEN_POWER_RANGE_SR_CURRENT_EVENT */
  mmdlGenPowerLevelSrStateUpdate_t        mmdlGenPowerLevelSrStateUpdate;        /*!< MMDL_GEN_POWER_DEFAULT_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_GEN_POWER_LAST_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_GEN_POWER_LEVEL_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_GEN_POWER_RANGE_SR_STATE_UPDATE_EVENT */
  mmdlLightLightnessClEvent_t             mmdlLightLightnessClEvent;             /*!< MMDL_LIGHT_LIGHTNESS_CL_STATUS_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_DEFAULT_CL_STATUS_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_LAST_CL_STATUS_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_LINEAR_CL_STATUS_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_RANGE_CL_STATUS_EVENT */
  mmdlLightLightnessSrCurrentState_t      mmdlLightLightnessSrCurrentState;      /*!< MMDL_LIGHT_LIGHTNESS_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_LAST_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_LINEAR_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_RANGE_SR_CURRENT_STATE_EVENT */
  mmdlLightLightnessSrStateUpdate_t       mmdlLightLightnessSrStateUpdate;       /*!< MMDL_LIGHT_LIGHTNESS_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_LINEAR_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_LIGHT_LIGHTNESS_RANGE_SR_STATE_UPDATE_EVENT */
  mmdlLightHslClDefStatusEvent_t          mmdlLightHslClDefStatusEvent;          /*!< MMDL_LIGHT_HSL_CL_DEF_STATUS_EVENT */
  mmdlLightHslClHueStatusEvent_t          mmdlLightHslClHueStatusEvent;          /*!< MMDL_LIGHT_HSL_CL_HUE_STATUS_EVENT */
  mmdlLightHslClRangeStatusEvent_t        mmdlLightHslClRangeStatusEvent;        /*!< MMDL_LIGHT_HSL_CL_RANGE_STATUS_EVENT */
  mmdlLightHslClSatStatusEvent_t          mmdlLightHslClSatStatusEvent;          /*!< MMDL_LIGHT_HSL_CL_SAT_STATUS_EVENT */
  mmdlLightHslClStatusEvent_t             mmdlLightHslClStatusEvent;             /*!< MMDL_LIGHT_HSL_CL_STATUS_EVENT */
  mmdlLightHslClStatusEvent_t             mmdlLightHslClTargetStatusEvent;       /*!< MMDL_LIGHT_HSL_CL_TARGET_STATUS_EVENT */
  mmdlLightHslHueSrStateUpdate_t          mmdlLightHslHueSrStateUpdate;          /*!< MMDL_LIGHT_HSL_HUE_SR_STATE_UPDATE_EVENT */
  mmdlLightHslSatSrStateUpdate_t          mmdlLightHslSatSrStateUpdate;          /*!< MMDL_LIGHT_HSL_SAT_SR_STATE_UPDATE_EVENT */
  mmdlLightHslSrStateUpdate_t             mmdlLightHslSrStateUpdate;             /*!< MMDL_LIGHT_HSL_SR_STATE_UPDATE_EVENT */
  mmdlSceneClRegStatusEvent_t             mmdlSceneClRegStatusEvent;             /*!< MMDL_SCENE_CL_REG_STATUS_EVENT */
  mmdlSceneClStatusEvent_t                mmdlSceneClStatusEvent;                /*!< MMDL_SCENE_CL_STATUS_EVENT */
  mmdlSchedulerClActionStatusEvent_t      mmdlSchedulerClActionStatusEvent;      /*!< MMDL_SCHEDULER_CL_ACTION_STATUS_EVENT */
  mmdlSchedulerClStatusEvent_t            mmdlSchedulerClStatusEvent;            /*!< MMDL_SCHEDULER_CL_STATUS_EVENT */
  mmdlSchedulerSrStartScheduleEvent_t     mmdlSchedulerSrStartScheduleEvent;     /*!< MMDL_SCHEDULER_SR_START_SCHEDULE_EVENT */
  mmdlSchedulerSrStopScheduleEvent_t      mmdlSchedulerSrStopScheduleEvent;      /*!< MMDL_SCHEDULER_SR_STOP_SCHEDULE_EVENT*/
  mmdlTimeClDeltaStatusEvent_t            mmdlTimeClDeltaStatusEvent;            /*!< MMDL_TIMEDELTA_CL_STATUS_EVENT */
  mmdlTimeClRoleStatusEvent_t             mmdlTimeClRoleStatusEvent;             /*!< MMDL_TIMEROLE_CL_STATUS_EVENT */
  mmdlTimeClZoneStatusEvent_t             mmdlTimeClZoneStatusEvent;             /*!< MMDL_TIMEZONE_CL_STATUS_EVENT */
  mmdlTimeClStatusEvent_t                 mmdlTimeClStatusEvent;                 /*!< MMDL_TIME_CL_STATUS_EVENT */
  mmdlTimeSrCurrentState_t                mmdlTimeSrCurrentState;                /*!< MMDL_TIMEDELTA_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_TIMEROLE_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_TIMEZONE_SR_CURRENT_STATE_EVENT
                                                                                      MMDL_TIME_SR_CURRENT_STATE_EVENT */
  mmdlTimeSrStateUpdate_t                 mmdlTimeSrStateUpdate;                 /*!< MMDL_TIMEDELTA_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_TIMEROLE_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_TIMEZONE_SR_STATE_UPDATE_EVENT
                                                                                      MMDL_TIME_SR_STATE_UPDATE_EVENT */
} mmdl_events_t;


#ifdef __cplusplus
}
#endif

#endif /* MMDL_TYPES_H */
