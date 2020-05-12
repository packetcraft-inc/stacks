/*************************************************************************************************/
/*!
 *  \file   mmdl_bindings.h
 *
 *  \brief  Interface of the model bindings.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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
 *
 */
/*************************************************************************************************/

#ifndef MMDL_BINDINGS_H
#define MMDL_BINDINGS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Maximum number of bindings */
#ifndef MMDL_BINDINGS_MAX
#define MMDL_BINDINGS_MAX      30
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Enumeration describing the identifiers of the state assigned for the bind table */
enum mmdlBoundState
{
  MMDL_STATE_GEN_ONOFF,          /*!< Generic On Off State */
  MMDL_STATE_GEN_LEVEL,          /*!< Generic Level State */
  MMDL_STATE_GEN_POW_ACT,        /*!< Generic Power Actual State */
  MMDL_STATE_GEN_ONPOWERUP,      /*!< Generic OnPowerUp State */
  MMDL_STATE_LT_LTNESS_ACT,      /*!< Light Lightness Actual State */
  MMDL_STATE_LT_CTL,             /*!< Light CTL State */
  MMDL_STATE_LT_CTL_TEMP,        /*!< Light CTL Temperature State */
  MMDL_STATE_LT_HSL,             /*!< Light HSL State */
  MMDL_STATE_LT_HSL_HUE,         /*!< Light HSL HUE State */
  MMDL_STATE_LT_HSL_SATURATION,  /*!< Light HSL Saturation State */
  MMDL_STATE_LT_XYL,             /*!< Light xyL State */
  MMDL_STATE_LT_LC_LIGHT_ONOFF,  /*!< Light LC Light On Off State */
  MMDL_STATE_SCENE_REG,          /*!< Scene Register state */
  MMDL_STATE_SCH_REG,            /*!< Scheduler Register State Action */
};

/*! State identifiers assigned internally for the bind table. See values ::mmdlBoundState */
typedef uint16_t mmdlBoundState_t;

/*! Defines the function that checks if one of the states the model instance has a bind with
 *  another state and calls the function to resolve it.
 */
typedef void (*mmdlBindResolve_t)(meshElementId_t srcElementId, mmdlBoundState_t srcBoundState,
                                  void *pStateValue);

/*! The type of the function that resolves the bind between two states.*/
typedef void (*mmdlBindResolver_t)(meshElementId_t tgtElementId, void *pStateValue);

/*! Model bind entry structure */
typedef struct mmdlBind_tag
{
  mmdlBoundState_t   srcBoundState;      /*!< Identifier for the bound state that has changed
                                          */
  mmdlBoundState_t   tgtBoundState;      /*!< Identifier for the bound state that needs to be
                                          *   changed
                                          */
  meshElementId_t    srcElementId;       /*!< Identifier for the element that contains the model
                                          *   instance state that has changed
                                          */
  meshElementId_t    tgtElementId;       /*!< Identifier for the element that contains the model
                                          *   instance state that needs to be changed
                                          */
  mmdlBindResolver_t pBindResolverFunc;  /*!< Pointer to function that resolves the bind
                                          *   between source and target state, when source
                                          *   state has been changed
                                          */
} mmdlBind_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Checks if one of the states of the model instance has a bind with another state and
 *             calls the function to resolve it.
 *
 *  \param[in] srcElementId   Identifier for the element that contains the model instance state
 *                            that has changed.
 *  \param[in] srcBoundState  Identifier for the bound state that has changed.
 *
 *  \param[in] pStateValue    Pointer to the new state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlBindResolve(meshElementId_t srcElementId, mmdlBoundState_t srcBoundState,
                     void *pStateValue);

/*************************************************************************************************/
/*!
 *  \brief     Adds a bind to the binding table.
 *
 *  \param[in] srcState           Bound state that has changed.
 *  \param[in] tgtState           Bound state that needs to be changed.
 *  \param[in] srcElementId       Element that contains the state that has changed.
 *  \param[in] tgtElementId       Element that contains the state that needs to be changed.
 *  \param[in] pBindResolverFunc  Resolver function. See ::mmdlBindResolver_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlAddBind(mmdlBoundState_t srcState, mmdlBoundState_t tgtState,
                 meshElementId_t srcElementId, meshElementId_t tgtElementId,
                 mmdlBindResolver_t pBindResolverFunc);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_BINDINGS_H */
