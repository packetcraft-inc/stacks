/*************************************************************************************************/
/*!
 *  \file   mesh_access_period_pub.h
 *
 *  \brief  Periodic Publishing module interface.
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

#ifndef MESH_ACCESS_PERIOD_PUB_H
#define MESH_ACCESS_PERIOD_PUB_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Periodic publishing timer number of milliseconds per tick. */
#ifndef MESH_ACC_PP_TMR_TICK_MS
#define MESH_ACC_PP_TMR_TICK_MS  (100)
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the periodic publishing feature in the Access Layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAccPeriodicPubInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Informs the module that the periodic publishing value of a model instance has
 *             changed.
 *
 *  \param[in] elemId    Element identifier.
 *  \param[in] pModelId  Pointer to a model identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccPpChanged(meshElementId_t elemId, meshModelId_t *pModelId);

#ifdef __cplusplus
}
#endif

#endif /* MESH_ACCESS_PERIOD_PUB_H */
