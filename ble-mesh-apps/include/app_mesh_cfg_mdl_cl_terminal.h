/*************************************************************************************************/
/*!
 *  \file   app_mesh_cfg_mdl_cl_terminal.h
 *
 *  \brief  Common Mesh Config Client Terminal handler.
 *
 *  Copyright (c) 2015-2018 Arm Ltd. All Rights Reserved.
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

#ifndef APP_MESH_CFG_MDL_CL_TERMINAL_H
#define APP_MESH_CFG_MDL_CL_TERMINAL_H

#include "mesh_cfg_mdl_cl_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Mesh Config Client terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void appMeshCfgMdlClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Process messages from the Config Client.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void appMeshCfgMdlClTerminalProcMsg(meshCfgMdlClEvt_t* pMsg);

#ifdef __cplusplus
}
#endif

#endif /* APP_MESH_CFG_MDL_CL_TERMINAL_H */
