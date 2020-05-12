/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_default_trans_sr.h
 *
 *  \brief  Interface of the Generic Level Server model.
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

#ifndef MMDL_GEN_DEFAULT_TRANS_SR_H
#define MMDL_GEN_DEFAULT_TRANS_SR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/
/*! Gets the transition time steps */
#define TRANSITION_TIME_STEPS(time)                       ((time) & 0x3F)

/*! Sets the transition time steps */
#define SET_TRANSITION_TIME_RESOLUTION(time, res)         ((time) |= ((res) << 6))

/*! Gets the transition time resolution */
#define TRANSITION_TIME_RESOLUTION(time)                  ((time) >> 6)

/*! Transforms the Delay defined by the spec in milliseconds*/
#define DELAY_5MS_TO_MS(delay5Ms)                         ((delay5Ms) * 5)

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Get the default transition value on the specified element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    Default transition time in milliseconds, or 0 if undefined.
 */
/*************************************************************************************************/
uint32_t MmdlGenDefaultTransGetTime(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Converts the transition time to remaining time expressed in milliseconds.
 *
 *  \param[in] transitionTime  Transition time.
 *
 *  \return    Time in milliseconds.
 */
/*************************************************************************************************/
uint32_t MmdlGenDefaultTransTimeToMs(uint8_t transitionTime);

/*************************************************************************************************/
/*!
 *  \brief     Converts the remaining time expressed in milliseconds to transition time.
 *
 *  \param[in] remainingTimeMs  Remaining time in milliseconds.
 *
 *  \return    Transition time.
 */
/*************************************************************************************************/
uint8_t MmdlGenDefaultTimeMsToTransTime(uint32_t remainingTimeMs);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_DEFAULT_TRANS_SR_H */
