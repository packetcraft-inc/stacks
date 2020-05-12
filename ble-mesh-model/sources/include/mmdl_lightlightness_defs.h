/*************************************************************************************************/
/*!
 *  \file   mmdl_lightlightness_defs.h
 *
 *  \brief  Light Lightness Client Model API.
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

#ifndef MMDL_LIGHT_LIGHTNESS_DEFS_H
#define MMDL_LIGHT_LIGHTNESS_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light Lightness Set parameters structure */
typedef struct mmdlLightLightnessSetParam_tag
{
  mmdlLightLightnessState_t lightness;      /*!< Target value of Light Lightness Actual state. */
  uint8_t                   tid;            /*!< Transaction Identifier */
  uint8_t                   transitionTime; /*!< Transition time */
  uint8_t                   delay;          /*!< Delay in steps of 5 ms  */
} mmdlLightLightnessSetParam_t;

/*! Light Lightness Linear Set parameters structure */
typedef struct mmdlLightLightnessLinearSetParam_tag
{
  mmdlLightLightnessState_t lightness;      /*!< Target value of Light Lightness Linear state. */
  uint8_t                   tid;            /*!< Transaction Identifier */
  uint8_t                   transitionTime; /*!< Transition time */
  uint8_t                   delay;          /*!< Delay in steps of 5 ms  */
} mmdlLightLightnessLinearSetParam_t;

/*! Light Lightness Default Set parameters structure */
typedef struct mmdlLightLightnessDefaultSetParam_tag
{
  mmdlLightLightnessState_t lightness;      /*!< Target value of Light Lightness Default state. */
} mmdlLightLightnessDefaultSetParam_t;

/*! Light Lightness Range Set parameters structure */
typedef struct mmdlLightLightnessRangeSetParam_tag
{
  mmdlLightLightnessState_t rangeMin;       /*!< Minimum Light Lightness Range state */
  mmdlLightLightnessState_t rangeMax;       /*!< Maximum Light Lightness Range state */
} mmdlLightLightnessRangeSetParam_t;

/*! Light Lightness Actual Status Model structure */
typedef struct mmdlLightLightnessActualStatusParam_tag
{
  mmdlLightLightnessState_t presentLightness; /*!< Present value of Light Lightness Actual state */
  mmdlLightLightnessState_t targetLightness;  /*!< Target value of Light Lightness Actual state */
  uint8_t                   remainingTime;    /*!< Remaining time until the transition completes */
} mmdlLightLightnessActualStatusParam_t;

/*! Light Lightness Linear Status Model structure */
typedef struct mmdlLightLightnessLinearStatusParam_tag
{
  mmdlLightLightnessState_t presentLightness; /*!< Present value of Light Lightness Linear state */
  mmdlLightLightnessState_t targetLightness;  /*!< Target value of Light Lightness Linear state */
  uint8_t                   remainingTime;    /*!< Remaining time until the transition completes */
} mmdlLightLightnessLinearStatusParam_t;

/*! Light Lightness Last Status structure */
typedef struct mmdlLightLightnessLastStatusParam_tag
{
  mmdlLightLightnessState_t lightness;        /*!< Value of Light Lightness Last state. */
} mmdlLightLightnessLastStatusParam_t;

/*! Light Lightness Default Status structure */
typedef struct mmdlLightLightnessDefaultStatusParam_tag
{
  mmdlLightLightnessState_t lightness;        /*!< Value of Light Lightness Default state. */
} mmdlLightLightnessDefaultStatusParam_t;

/*! Light Lightness Range Status structure */
typedef struct mmdlLightLightnessRangeStatusParam_tag
{
  uint8_t                   statusCode;     /*!< Status Code */
  mmdlLightLightnessState_t rangeMin;       /*!< Minimum Light Lightness Range state */
  mmdlLightLightnessState_t rangeMax;       /*!< Maximum Light Lightness Range state */
} mmdlLightLightnessRangeStatusParam_t;

/*! Light Lightness status parameter type union. */
typedef union
{
  mmdlLightLightnessActualStatusParam_t   actualStatusEvent;  /*!< State updated event. Used for
                                                               *   ::MMDL_LIGHT_LIGHTNESS_CL_STATUS_EVENT.
                                                               */
  mmdlLightLightnessLinearStatusParam_t   linearStatusEvent;  /*!< State updated event. Used for
                                                                 *   ::MMDL_LIGHT_LIGHTNESS_LINEAR_CL_STATUS_EVENT.
                                                                 */
  mmdlLightLightnessLastStatusParam_t     lastStatusEvent;    /*!< State updated event. Used for
                                                                 *   ::MMDL_LIGHT_LIGHTNESS_LAST_CL_STATUS_EVENT.
                                                                 */
  mmdlLightLightnessDefaultStatusParam_t  defaultStatusEvent; /*!< State updated event. Used for
                                                                 *   ::MMDL_LIGHT_LIGHTNESS_DEFAULT_CL_STATUS_EVENT.
                                                                 */
  mmdlLightLightnessRangeStatusParam_t    rangeStatusEvent;   /*!< State updated event. Used for
                                                                 *   ::MMDL_LIGHT_LIGHTNESS_RANGE_CL_STATUS_EVENT.
                                                                 */
} mmdlLightLightnessStatusParam_t;

/*! Light Lightness Client Model callback parameters structure */
typedef struct mmdlLightLightnessCl_tag
{
  wsfMsgHdr_t                     hdr;         /*!< WSF message header. */
  meshElementId_t                 elementId;   /*!< Element ID. */
  meshAddress_t                   serverAddr;  /*!< Server Address. */
  mmdlLightLightnessStatusParam_t statusParam; /*!< Status Param. */
} mmdlLightLightnessClEvent_t;

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_LIGHTNESS_DEFS_H */
