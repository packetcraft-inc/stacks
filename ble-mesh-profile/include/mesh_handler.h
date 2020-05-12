/*************************************************************************************************/
/*!
 *  \file   mesh_handler.h
 *
 *  \brief  WSF handler-related API.
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

/*! ***********************************************************************************************
 * @addtogroup MESH_CORE_API
 * @{
 *************************************************************************************************/

#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Network.
 *
 *  \return    None
 */
/*************************************************************************************************/
void MeshHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF event handler for Mesh API.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh Security WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Network.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSecurityHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF event handler for Mesh Security.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSecurityHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MESH_HANDLER_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
