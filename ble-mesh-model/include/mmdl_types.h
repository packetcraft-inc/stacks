/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  General Mesh Model type definitions.
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

/*! ***********************************************************************************************
 * \addtogroup ModelTypes Model Definitions and Types
 * @{
 *************************************************************************************************/

#ifndef MMDL_TYPES_H
#define MMDL_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_assert.h"
#include "mesh_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Use for destination address field to send to publication address */
#define MMDL_USE_PUBLICATION_ADDR      MESH_ADDR_TYPE_UNASSIGNED

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \breif Model events start */
#define MMDL_CBACK_START             0xC0

/*! \brief Model events */
enum mmdlEvents
{
  MESH_HT_CL_EVENT = MMDL_CBACK_START,  /*!< Health client event */
  MESH_HT_SR_EVENT,                     /*!< Health server event */
  MMDL_GEN_BATTERY_CL_EVENT,            /*!< Generic Battery client event */
  MMDL_GEN_BATTERY_SR_EVENT,            /*!< Generic Battery server event */
  MMDL_GEN_DEFAULT_TRANS_CL_EVENT,      /*!< Generic Default Transition client event */
  MMDL_GEN_DEFAULT_TRANS_SR_EVENT,      /*!< Generic Default Transition server event */
  MMDL_GEN_LEVEL_CL_EVENT,              /*!< Generic Level client event */
  MMDL_GEN_LEVEL_SR_EVENT,              /*!< Generic Level server event */
  MMDL_GEN_ONOFF_CL_EVENT,              /*!< Generic On Off client event */
  MMDL_GEN_ONOFF_SR_EVENT,              /*!< Generic On Off server event */
  MMDL_GEN_POWER_ONOFF_CL_EVENT,        /*!< Generic Power On Off client event */
  MMDL_GEN_POWER_ONOFF_SR_EVENT,        /*!< Generic Power On Off server event */
  MMDL_GEN_POWER_LEVEL_CL_EVENT,        /*!< Generic Power Level client event */
  MMDL_GEN_POWER_LEVEL_SR_EVENT,        /*!< Generic Power Level server event */
  MMDL_LIGHT_LIGHTNESS_CL_EVENT,        /*!< Light Lightness client event */
  MMDL_LIGHT_LIGHTNESS_SR_EVENT,        /*!< Light Lightness server event */
  MMDL_LIGHT_HSL_CL_EVENT,              /*!< Light HSL client event */
  MMDL_LIGHT_HSL_SR_EVENT,              /*!< Light HSL server event */
  MMDL_LIGHT_CTL_CL_EVENT,              /*!< Light CTL client event */
  MMDL_LIGHT_CTL_SR_EVENT,              /*!< Light CTL server event */
  MMDL_SCENE_CL_EVENT,                  /*!< Scene client event */
  MMDL_SCHEDULER_CL_EVENT,              /*!< Scheduler client event */
  MMDL_SCHEDULER_SR_EVENT,              /*!< Scheduler server event */
  MMDL_TIME_CL_EVENT,                   /*!< Time client event */
  MMDL_TIME_SR_EVENT                    /*!< Time server event */
};

#define MMDL_CBACK_END               MMDL_TIME_SR_EVENT  /*! Mesh callback event ending value */

/*! \brief Mesh Health client model events */
enum mmdlHealthClEventValues
{

  MESH_HT_CL_ATTENTION_STATUS_EVENT,                     /*!< Health Attention Status event */
  MESH_HT_CL_CURRENT_STATUS_EVENT,                       /*!< Health Current Status event */
  MESH_HT_CL_FAULT_STATUS_EVENT,                         /*!< Health Fault Status event */
  MESH_HT_CL_PERIOD_STATUS_EVENT,                        /*!< Health Period Status event */
  MESH_HT_CL_MAX_EVENT                                   /*!< Health client max event */
};

/*! \brief Mesh Health server model events */
enum mmdlHealthSrEventValues
{
  MESH_HT_SR_TEST_START_EVENT,                           /*!< Health Test Start event */
  MESH_HT_SR_MAX_EVENT                                   /*!< Health Test server max event */
};

/*! \brief Mesh Generic Battery client model events */
enum mmdlGenBattClEventValues
{
  MMDL_GEN_BATTERY_CL_STATUS_EVENT,                      /*!< Generic Battery Status event */
  MMDL_GEN_BATTERY_CL_MAX_EVENT                          /*!< Generic Battery client max event */
};

/*! \brief Mesh Generic Battery server model events */
enum mmdlGenBattSrEventValues
{
  MMDL_GEN_BATTERY_SR_CURRENT_STATE_EVENT,               /*!< Generic Battery Current State event */
  MMDL_GEN_BATTERY_SR_STATE_UPDATE_EVENT,                /*!< Generic Battery State Updated event */
  MMDL_GEN_BATTERY_SR_MAX_EVENT                          /*!< Generic Battery server max event */
};

/*! \brief Mesh Generic Default Transition Time client model events */
enum mmdlGenDefaultTransClEventValues
{
  MMDL_GEN_DEFAULT_TRANS_CL_STATUS_EVENT,                /*!< Generic Default Transition Status event */
  MMDL_GEN_DEFAULT_TRANS_CL_MAX_EVENT                    /*!< Generic Default Transition client max event */
};

/*! \brief Mesh Generic Default Transition Time server model events */
enum mmdlGenDefaultTransSrEventValues
{
  MMDL_GEN_DEFAULT_TRANS_SR_CURRENT_STATE_EVENT,          /*!< Generic Default Transition Current State event */
  MMDL_GEN_DEFAULT_TRANS_SR_STATE_UPDATE_EVENT,           /*!< Generic Default Transition State Updated event */
  MMDL_GEN_DEFAULT_TRANS_SR_MAX_EVENT                     /*!< Generic Default Transition server max event */
};

/*! \brief Mesh Generic Level client model events */
enum mmdlGenLevelClEventValues
{
  MMDL_GEN_LEVEL_CL_STATUS_EVENT,                        /*!< Generic Level Status event */
  MMDL_GEN_LEVEL_CL_MAX_EVENT                            /*!< Generic Level client max event */
};

/*! \brief Mesh Generic Level server model events */
enum mmdlGenLevelSrEventValues
{
  MMDL_GEN_LEVEL_SR_CURRENT_STATE_EVENT,                 /*!< Generic Level Current State event */
  MMDL_GEN_LEVEL_SR_STATE_UPDATE_EVENT,                  /*!< Generic Level State Updated event */
  MMDL_GEN_LEVEL_SR_MAX_EVENT                            /*!< Generid Level server max event */
};

/*! \brief Mesh Generic On Off client model events */
enum mmdlGenOnOffClEventValues
{
  MMDL_GEN_ONOFF_CL_STATUS_EVENT,                        /*!< Generic OnOff Status event */
  MMDL_GEN_ONOFF_CL_MAX_EVENT                            /*!< Generic OnOff client max event */
};

/*! \brief Mesh Generic On Off server model events */
enum mmdlGenOnOffSrEventValues
{
  MMDL_GEN_ONOFF_SR_CURRENT_STATE_EVENT,                 /*!< Generic OnOff Current State event */
  MMDL_GEN_ONOFF_SR_STATE_UPDATE_EVENT,                  /*!< Generic OnOff State Updated event */
  MMDL_GEN_ONOFF_SR_MAX_EVENT                            /*!< Generic OnOff server max event */
};

/*! \brief Mesh Generic Power client model events */
enum mmdlGenPowerOnOffClEventValues
{
  MMDL_GEN_POWER_ONOFF_CL_STATUS_EVENT,                  /*!< Generic Power On Off Status event */
  MMDL_GEN_POWER_ONOFF_CL_MAX_EVENT                      /*!< Generic Power On Off client max event */
};

/*! \brief Mesh Generic Power server model events */
enum mmdlGenPowerOnOffSrEventValues
{
  MMDL_GEN_POWER_ONOFF_SR_CURRENT_STATE_EVENT,           /*!< Generic Power On Off Current State event */
  MMDL_GEN_POWER_ONOFF_SR_STATE_UPDATE_EVENT,            /*!< Generic Power On Off State Updated event */
  MMDL_GEN_POWER_ONOFF_SR_MAX_EVENT                      /*!< Generic Power On Off server max event */
};

/*! \brief Mesh Generic Power Level client model events */
enum mmdlPowerLevelClEventValues
{
  MMDL_GEN_POWER_DEFAULT_CL_STATUS_EVENT,                /*!< Generic Power Default Status event */
  MMDL_GEN_POWER_LAST_CL_STATUS_EVENT,                   /*!< Generic Power Last Status event */
  MMDL_GEN_POWER_LEVEL_CL_STATUS_EVENT,                  /*!< Generic Power Level Status event */
  MMDL_GEN_POWER_RANGE_CL_STATUS_EVENT,                  /*!< Generic Power Range Status event */
  MMDL_GEN_POWER_CL_MAX_EVENT                            /*!< Generic Power client max event */
};

/*! \brief Mesh Generic Power Level server model events */
enum mmdlPowerLevelSrEventValues
{
  MMDL_GEN_POWER_DEFAULT_SR_CURRENT_STATE_EVENT,         /*!< Generic Power Level Default Current State event */
  MMDL_GEN_POWER_DEFAULT_SR_STATE_UPDATE_EVENT,          /*!< Generic Power Level Default State event */
  MMDL_GEN_POWER_LAST_SR_CURRENT_STATE_EVENT,            /*!< Generic Power Level Last Current State event */
  MMDL_GEN_POWER_LAST_SR_STATE_UPDATE_EVENT,             /*!< Generic Power Level Last State event */
  MMDL_GEN_POWER_LEVEL_SR_CURRENT_STATE_EVENT,           /*!< Generic Power Level Actual Current State event */
  MMDL_GEN_POWER_LEVEL_SR_STATE_UPDATE_EVENT,            /*!< Generic Power Level Actual State event */
  MMDL_GEN_POWER_RANGE_SR_CURRENT_EVENT,                 /*!< Generic Power Level Range Current event */
  MMDL_GEN_POWER_RANGE_SR_STATE_UPDATE_EVENT,            /*!< Generic Power Level Range State event */
  MMDL_GEN_POWER_SR_MAX_EVENT                            /*!< Generic Power Level server max event */
};

/*! \brief Mesh Light Lightness client model events */
enum mmdlLightLightnessClEventValues
{
  MMDL_LIGHT_LIGHTNESS_CL_STATUS_EVENT,                   /*!< Light Lightness Status event */
  MMDL_LIGHT_LIGHTNESS_DEFAULT_CL_STATUS_EVENT,           /*!< Light Lightness Default Status event */
  MMDL_LIGHT_LIGHTNESS_LAST_CL_STATUS_EVENT,              /*!< Light Lightness Last Status event */
  MMDL_LIGHT_LIGHTNESS_LINEAR_CL_STATUS_EVENT,            /*!< Light Lightness Linear Status event */
  MMDL_LIGHT_LIGHTNESS_RANGE_CL_STATUS_EVENT,             /*!< Light Lightness Range Status event */
  MMDL_LIGHT_LIGHTNESS_CL_MAX_EVENT                       /*!< Light Lightness client max event */
};

/*! \brief Mesh Light Lightness server model events */
enum mmdlLightLightnessSrEventValues
{
  MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_CURRENT_STATE_EVENT,    /*!< Light Lightness Default Current State event */
  MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_STATE_UPDATE_EVENT,     /*!< Light Lightness Default State Updated event */
  MMDL_LIGHT_LIGHTNESS_LAST_SR_CURRENT_STATE_EVENT,       /*!< Light Lightness Last Current State event */
  MMDL_LIGHT_LIGHTNESS_LINEAR_SR_CURRENT_STATE_EVENT,     /*!< Light Lightness Linear Current State event */
  MMDL_LIGHT_LIGHTNESS_LINEAR_SR_STATE_UPDATE_EVENT,      /*!< Light Lightness Linear State Updated event */
  MMDL_LIGHT_LIGHTNESS_RANGE_SR_CURRENT_STATE_EVENT,      /*!< Light Lightness Range Current State event */
  MMDL_LIGHT_LIGHTNESS_RANGE_SR_STATE_UPDATE_EVENT,       /*!< Light Lightness Range State Updated event */
  MMDL_LIGHT_LIGHTNESS_SR_CURRENT_STATE_EVENT,            /*!< Light Lightness Current State event */
  MMDL_LIGHT_LIGHTNESS_SR_STATE_UPDATE_EVENT,             /*!< Light Lightness State Updated event */
  MMDL_LIGHT_LIGHTNESS_SR_MAX_EVENT                       /*!< Light Lightness server max event */
};

/*! \brief Mesh Light HSL client model events */
enum mmdlLightHslClEventValues
{
  MMDL_LIGHT_HSL_CL_DEF_STATUS_EVENT,                    /*!< Light HSL Default Status event */
  MMDL_LIGHT_HSL_CL_HUE_STATUS_EVENT,                    /*!< Light HSL Hue Status event */
  MMDL_LIGHT_HSL_CL_RANGE_STATUS_EVENT,                  /*!< Light HSL Range Status event */
  MMDL_LIGHT_HSL_CL_SAT_STATUS_EVENT,                    /*!< Light HSL Saturation Status event */
  MMDL_LIGHT_HSL_CL_STATUS_EVENT,                        /*!< Light HSL Status event */
  MMDL_LIGHT_HSL_CL_TARGET_STATUS_EVENT,                 /*!< Light HSL Target Status event */
  MMDL_LIGHT_HSL_CL_MAX_EVENT                            /*!< Light HSL client max event */
};

/*! \brief Mesh Light HSL server model events */
enum mmdlLightHslSrEventValues
{
  MMDL_LIGHT_HSL_HUE_SR_STATE_UPDATE_EVENT,              /*!< Light HSL Hue State Update event */
  MMDL_LIGHT_HSL_SAT_SR_STATE_UPDATE_EVENT,              /*!< Light HSL Saturation State Update event */
  MMDL_LIGHT_HSL_SR_STATE_UPDATE_EVENT,                  /*!< Light HSL State Update event */
  MMDL_LIGHT_HSL_SR_RANGE_STATE_UPDATE_EVENT,            /*!< Light HSL Range State Updated event */
  MMDL_LIGHT_HSL_SR_MAX_EVENT                            /*!< Light HSL server max event */
};

/*! \brief Mesh Light CTL client model events */
enum mmdlLightCtlClEventValues
{
  MMDL_LIGHT_CTL_CL_DEF_STATUS_EVENT,                    /*!< Light CTL Default Status event */
  MMDL_LIGHT_CTL_CL_TEMP_STATUS_EVENT,                   /*!< Light CTL Temperature Status event */
  MMDL_LIGHT_CTL_CL_RANGE_STATUS_EVENT,                  /*!< Light CTL Temperature Range Status event */
  MMDL_LIGHT_CTL_CL_STATUS_EVENT,                        /*!< Light CTL Status event */
  MMDL_LIGHT_CTL_CL_MAX_EVENT                            /*!< Light CTL client max event */
};

/*! \brief Mesh Light CTL server model events */
enum mmdlLightCtlSrEventValues
{
  MMDL_LIGHT_CTL_TEMP_SR_STATE_UPDATE_EVENT,             /*!< Light CTL Temperature State Update event */
  MMDL_LIGHT_CTL_SR_STATE_UPDATE_EVENT,                  /*!< Light CTL State Update event */
  MMDL_LIGHT_CTL_SR_RANGE_STATE_UPDATE_EVENT,            /*!< Light CTL Range State Updated event */
  MMDL_LIGHT_CTL_SR_MAX_EVENT                            /*!< Light CTL server max event */
};

/*! \brief Mesh Scene client model events */
enum mmdlSceneClEventValues
{
  MMDL_SCENE_CL_REG_STATUS_EVENT,                        /*!< Scenes Register Status event */
  MMDL_SCENE_CL_STATUS_EVENT,                            /*!< Scenes Status event */
  MMDL_SCENE_CL_MAX_EVENT                                /*!< Scenes client max event */
};

/*! \brief Mesh Scheduler client model events */
enum mmdlSchedulerClEventValues
{
  MMDL_SCHEDULER_CL_ACTION_STATUS_EVENT,                 /*!< Scheduler Action Status event */
  MMDL_SCHEDULER_CL_STATUS_EVENT,                        /*!< Scheduler Status event */
  MMDL_SCHEDULER_CL_MAX_EVENT                            /*!< Scheduler client max event */
};

/*! \brief Mesh Scheduler server model events */
enum mmdlSchedulerSrEventValues
{
  MMDL_SCHEDULER_SR_START_SCHEDULE_EVENT,                /*!< Scheduler Server Start Schedule event */
  MMDL_SCHEDULER_SR_STOP_SCHEDULE_EVENT,                 /*!< Scheduler Server Stop Schedule event */
  MMDL_SCHEDULER_SR_MAX_EVENT                            /*!< Scheduler server max event */
};

/*! \brief Mesh Time client model events */
enum mmdlTimeClEventValues
{
  MMDL_TIMEDELTA_CL_STATUS_EVENT,                        /*!< Time Delta Status event */
  MMDL_TIMEROLE_CL_STATUS_EVENT,                         /*!< Time Role Status event */
  MMDL_TIMEZONE_CL_STATUS_EVENT,                         /*!< Time Zone Status event */
  MMDL_TIME_CL_STATUS_EVENT,                             /*!< Time Status event */
  MMDL_TIME_CL_MAX_EVENT                                 /*!< Time client max event */
};

/*! \brief Mesh Time server model events */
enum mmdlTimeSrEventValues
{
  MMDL_TIMEDELTA_SR_CURRENT_STATE_EVENT,                 /*!< Time Delta Current State event */
  MMDL_TIMEDELTA_SR_STATE_UPDATE_EVENT,                  /*!< Time Delta State Updated event */
  MMDL_TIMEROLE_SR_CURRENT_STATE_EVENT,                  /*!< Time Role Current State event */
  MMDL_TIMEROLE_SR_STATE_UPDATE_EVENT,                   /*!< Time Role State Updated event */
  MMDL_TIMEZONE_SR_CURRENT_STATE_EVENT,                  /*!< Time Zone Current State event */
  MMDL_TIMEZONE_SR_STATE_UPDATE_EVENT,                   /*!< Time Zone State Updated event */
  MMDL_TIME_SR_CURRENT_STATE_EVENT,                      /*!< Time Current State event */
  MMDL_TIME_SR_STATE_UPDATE_EVENT,                       /*!< Time State Updated event */
  MMDL_TIME_SR_MAX_EVENT                                 /*!< Time server max event */
};

/*! \brief Model internal callback events */
#define MMDL_INTERNAL_CBACK_START              (MESH_MODEL_EVT_PERIODIC_PUB + 1)

/*! \brief Model internal callback events */
enum mmdlInternalEventValues
{
  HT_SR_EVT_TMR_CBACK = MMDL_INTERNAL_CBACK_START,
  MMDL_GEN_ON_OFF_SR_EVT_TMR_CBACK,
  MMDL_GEN_ON_OFF_SR_MSG_RCVD_TMR_CBACK,
  MMDL_GEN_LEVEL_SR_EVT_TMR_CBACK,
  MMDL_GEN_LEVEL_SR_MSG_RCVD_TMR_CBACK,
  MMDL_GEN_POWER_LEVEL_SR_EVT_TMR_CBACK,
  MMDL_GEN_POWER_LEVEL_SR_MSG_RCVD_TMR_CBACK,
  MMDL_GEN_POWER_LEVELSETUP_SR_EVT_TMR_CBACK,
  MMDL_GEN_POWER_LEVELSETUP_SR_MSG_RCVD_TMR_CBACK,
  MMDL_SCENE_SR_EVT_TMR_CBACK,
  MMDL_SCENE_SR_MSG_RCVD_TMR_CBACK,
  MMDL_LIGHT_LIGHTNESS_SR_EVT_TMR_CBACK,
  MMDL_LIGHT_LIGHTNESS_SR_MSG_RCVD_TMR_CBACK,
  MMDL_LIGHT_LIGHTNESSSETUP_SR_EVT_TMR_CBACK,
  MMDL_LIGHT_LIGHTNESSSETUP_SR_MSG_RCVD_TMR_CBACK,
  MMDL_LIGHT_HSL_SR_EVT_TMR_CBACK,
  MMDL_LIGHT_HSL_SR_MSG_RCVD_TMR_CBACK,
  MMDL_LIGHT_HSL_HUE_SR_EVT_TMR_CBACK,
  MMDL_LIGHT_HSL_HUE_SR_MSG_RCVD_TMR_CBACK,
  MMDL_LIGHT_HSL_SAT_SR_EVT_TMR_CBACK,
  MMDL_LIGHT_HSL_SAT_SR_MSG_RCVD_TMR_CBACK,
  MMDL_LIGHT_CTL_SR_EVT_TMR_CBACK,
  MMDL_LIGHT_CTL_SR_MSG_RCVD_TMR_CBACK,
  MMDL_LIGHT_CTL_TEMP_SR_EVT_TMR_CBACK,
  MMDL_LIGHT_CTL_TEMP_SR_MSG_RCVD_TMR_CBACK,
  MMDL_SCHEDULER_SR_EVT_TMR_CBACK,
};

/*! Mesh models return value (status) enumeration.
 *
 *  Values from 0x00 to 0x7F are shared by all models.
 *  Values from 0x80 to 0xFF are unique to and defined by each model.
 */
enum mmdlReturnValues
{
  MMDL_SUCCESS            = 0x00, /*!< Operation completed without errors */
  MMDL_OUT_OF_MEMORY,             /*!< Memory allocation failed */
  MMDL_INVALID_PARAM,             /*!< Invalid parameter(s) in request */
  MMDL_BOUND_NOT_FOUND,           /*!< No bound was found in the bind table */
  MMDL_NO_BOUND_RESOLVER,         /*!< The bind has no resolver function */
  MMDL_INVALID_ELEMENT,           /*!< Invalid element event status */
};

/*! \brief Model return value or status. See ::mmdlReturnValues */
typedef uint8_t mmdlRetVal_t;

/*! \brief Mesh models state update source values. */
enum mmdlStateUpdateSrcValues
{
  MMDL_STATE_UPDATED_BY_CL    = 0x00,  /*!< State updated from a remote client */
  MMDL_STATE_UPDATED_BY_APP,           /*!< State updated locally by the application */
  MMDL_STATE_UPDATED_BY_BIND,          /*!< State updated locally by a bind */
  MMDL_STATE_UPDATED_BY_SCENE          /*!< State updated locally by a scene recall */
};

/*! \brief Mesh stack return value or status. See ::mmdlStateUpdateSrcValues */
typedef uint8_t mmdlStateUpdateSrc_t;

/*! \brief Mesh model scene identifier definition */
typedef uint16_t mmdlSceneNumber_t;

/*! \brief Generic OnOff state values. See values ::mmdlGenOnOffStates */
typedef uint8_t mmdlGenOnOffState_t;

/*! \brief Generic Level state values. */
typedef int16_t mmdlGenLevelState_t;

/*! \brief Generic Default Transition state values. See values ::mmdlTransitionNumberOfSteps and ::mmdlTransitionStepResolution */
typedef uint8_t mmdlGenDefaultTransState_t;

/*! \brief Generic Level Delta value. */
typedef int32_t mmdlGenDelta_t;

/*! \brief Generic OnPowerUp state values. See values ::mmdlGenOnPowerUpStates */
typedef uint8_t mmdlGenOnPowerUpState_t;

/*! \brief Generic Level state values. */
typedef uint16_t mmdlGenPowerLevelState_t;

/*! \brief Light Lightness state values. */
typedef uint16_t mmdlLightLightnessState_t;

/*! \brief Light HSL state definition */
typedef struct mmdlLightHslState_tag
{
  uint16_t  ltness;       /*!< Lightness value */
  uint16_t  hue;          /*!< Hue value */
  uint16_t  saturation;   /*!< Saturation value */
} mmdlLightHslState_t;

/*! \brief Light HSL Range state definition */
typedef struct mmdlLightHslRangeState_tag
{
  uint16_t    minHue;         /*!< Hue range minimum */
  uint16_t    maxHue;         /*!< Hue range maximum */
  uint16_t    minSaturation;  /*!< Saturation range minimum */
  uint16_t    maxSaturation;  /*!< Saturation range maximum */
} mmdlLightHslRangeState_t;

/*! \brief Light Lightness Range state values. */
typedef struct mmdlLightLightnessRangeState_tag
{
  mmdlLightLightnessState_t rangeMin; /*!< Range minimum. */
  mmdlLightLightnessState_t rangeMax; /*!< Range maximum. */
} mmdlLightLightnessRangeState_t;

/*! \brief Light CTL state definition */
typedef struct mmdlLightCtlState_tag
{
  uint16_t  ltness;       /*!< Lightness value */
  uint16_t  temperature;  /*!< Temperature value */
  uint16_t  deltaUV;      /*!< Delta UV value */
} mmdlLightCtlState_t;

/*! \brief Light CTL Range state definition */
typedef struct mmdlLightCtlRangeState_tag
{
  uint16_t  rangeMin;       /*!< Range minimum. */
  uint16_t  rangeMax;       /*!< Range maximum. */
} mmdlLightCtlRangeState_t;

/*! \brief Generic Battery state values. */
typedef struct mmdlGenBatteryState_tag
{
  uint8_t   batteryLevel;         /*!< Battery level state */
  uint32_t  timeToDischarge;      /*!< Battery time to discharge */
  uint32_t  timeToCharge;         /*!< Battery time to charge */
  uint8_t   flags;                /*!< Batter flag state */
} mmdlGenBatteryState_t;

/*! \brief Generic Power Range state values. See value ::mmdlGenPowerRangeState_t */
typedef struct mmdlGenPowerRangeState_tag
{
  mmdlGenPowerLevelState_t rangeMin;  /*!< Range minimum. */
  mmdlGenPowerLevelState_t rangeMax;  /*!< Range maximum. */
} mmdlGenPowerRangeState_t;

/*! \brief Generic Power Range status values. See values ::mmdlRangeStatus */
typedef uint8_t mmdlGenPowerRangeStatus_t;

/*! \brief Time state values. */
typedef struct mmdlTimeState_tag
{
  uint64_t taiSeconds;      /*!< Current TAI time in seconds */
  uint8_t  subSecond;       /*!< Sub-second time */
  uint8_t  uncertainty;     /*!< Time Authority */
  uint8_t  timeAuthority;   /*!< TAI-UTC Delta */
  int16_t  taiUtcDelta;     /*!< Current difference between TAI and UTC in seconds */
  int8_t   timeZoneOffset;  /*!< Local Time Zone Offset in 15-minutes increments */
} mmdlTimeState_t;

/*! \brief Time Zone state values. */
typedef struct mmdlTimeZoneState_tag
{
  int8_t   offsetNew;      /*!< Upcoming local time zone offset */
  uint64_t taiZoneChange;  /*!< TAI Seconds time of the upcoming Time Zone Offset change */
} mmdlTimeZoneState_t;

/*! \brief Time TAI_UTC Delta state values */
typedef struct mmdlTimeDeltaState_tag
{
  int16_t  deltaNew;      /*!< Upcoming diff between TAI and UTC in seconds */
  uint64_t deltaChange;   /*!< TAI Seconds time of the upcoming TAI-UTC Delat change */
} mmdlTimeDeltaState_t;

/*! \brief Time Role state values. See values ::mmdlTimeRoleStates */
typedef struct mmdlTimeRoleState_tag
{
  uint8_t timeRole;    /*!< Time Role for element */
} mmdlTimeRoleState_t;

/*! \brief Scene status values. See values ::mmdlSceneStatus */
typedef uint8_t mmdlSceneStatus_t;

/*! Bit-field representation for the months of the occurrences of the scheduler event.
 *  See ::mmdlSchedulerRegisterMonthBitfieldValues
 */
typedef uint16_t mmdlSchedulerRegisterMonthBf_t;

/*! \brief Day of the month the scheduler event occurs. See ::mmdlSchedulerRegisterDayValues */
typedef uint8_t mmdlSchedulerRegisterDay_t;

/*! \brief Hour of the occurrence of the scheduler event. See ::mmdlSchedulerRegisterHourValues */
typedef uint8_t mmdlSchedulerRegisterHour_t;

/*! \brief Minute of the occurrence of the scheduler event. See ::mmdlSchedulerRegisterMinuteValues */
typedef uint8_t mmdlSchedulerRegisterMinute_t;

/*! \brief Second of the occurrence of the scheduler event. See ::mmdlSchedulerRegisterSecondValues */
typedef uint8_t mmdlSchedulerRegisterSecond_t;

/*! Bit-field representation for the days of the week of the occurrences of the scheduler event.
 *  See ::mmdlSchedulerRegisterDayOfWeekBitfieldValues
 */
typedef uint16_t mmdlSchedulerRegisterDayOfWeekBf_t;

/*! \brief Action to be executed for a scheduler event. See ::mmdlSchedulerRegisterActionValues */
typedef uint8_t  mmdlSchedulerRegisterAction_t;

/*! \brief Scheduler Register State entry definition. */
typedef struct mmdlSchedulerRegisterEntry_tag
{
  uint8_t                             year;        /*!< Last two digits for the Year of the event
                                                    *   or ::MMDL_SCHEDULER_REGISTER_YEAR_ALL
                                                    */
  mmdlSchedulerRegisterMonthBf_t      months;      /*!< Bit-field for months. A value of 1 for bit N
                                                    *   means event occurs in month N
                                                    */
  mmdlSchedulerRegisterDay_t          day;         /*!< Day of the event */
  mmdlSchedulerRegisterHour_t         hour;        /*!< Hour of the event */
  mmdlSchedulerRegisterMinute_t       minute;      /*!< Minute of the event */
  mmdlSchedulerRegisterSecond_t       second;      /*!< Second of the event */
  mmdlSchedulerRegisterDayOfWeekBf_t  daysOfWeek;  /*!< Bit-field for days of week. A value of 1 for bit N
                                                    *   means event occurs in day N
                                                    */
  mmdlSchedulerRegisterAction_t       action;      /*!< Action triggered by the event */
  mmdlGenDefaultTransState_t          transTime;   /*!< Transition time in milliseconds */
  mmdlSceneNumber_t                   sceneNumber; /*!< Scene to be recalled. */
} mmdlSchedulerRegisterEntry_t;

/*************************************************************************************************/
/*!
 *  \brief     Defines the function that recalls the scene to the stored configuration.
 *
 *  \param[in] elementId     Identifier of the element implementing the model instance.
 *  \param[in] sceneIdx      Identifier of the recalled scene.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlSceneRecall_t)(meshElementId_t elementId, uint8_t sceneIdx,
                                  uint32_t transitionMs);

/*************************************************************************************************/
/*!
 *  \brief     Defines the function that stores the states to be recalled later in a scene.
 *
 *  \param[in] pDesc    Model instance descriptor.
 *  \param[in] sceneIdx Identifier of the stored scene.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlSceneStore_t)(void* pDesc, uint8_t sceneIdx);

/*************************************************************************************************/
/*!
 *  \brief     Model Event Callback
 *
 *  \param[in] pEvent  Pointer to Model Event
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlEventCback_t)(const wsfMsgHdr_t *pEvent);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Model message handler type definition
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlModelHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Model instance Save to Non-Volatile memory function definition.
 *
 *  \param[in] elementId  Element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlNvmSaveHandler_t )(meshElementId_t elementId);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MMDL_TYPES_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
