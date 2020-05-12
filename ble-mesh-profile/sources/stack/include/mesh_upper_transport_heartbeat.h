/*************************************************************************************************/
/*!
 *  \file   mesh_upper_transport_heartbeat.h
 *
 *  \brief  Heartbeat module interface.
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

#ifndef MESH_UPPER_TRANSPORT_HEARTBEAT_H
#define MESH_UPPER_TRANSPORT_HEARTBEAT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Heartbeat module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshHbInit(void);

/*************************************************************************************************/
/*!
 *  \brief   Config Model Server module calls this function whenever Heartbeat Subscription State
 *           value is changed.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshHbSubscriptionStateChanged(void);

/*************************************************************************************************/
/*!
 *  \brief   Config Model Server module calls this function whenever Heartbeat Publication State
 *           value is changed.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshHbPublicationStateChanged(void);

/*************************************************************************************************/
/*!
 *  \brief     Signals the Heartbeat module that at least one Feature State value is changed.
 *
 *  \param[in] features  Bitmask for the changed features.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHbFeatureStateChanged(meshFeatures_t features);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously processes the given Heartbeat message PDU.
 *
 *  \param[in] pHbPdu  Pointer to the structure holding the received transport control PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHbProcessHb(const meshLtrCtlPduInfo_t *pHbPdu);

#ifdef __cplusplus
}
#endif

#endif /* MESH_UPPER_TRANSPORT_HEARTBEAT_H */
