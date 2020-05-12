/*************************************************************************************************/
/*!
 *  \file   app_mesh_terminal.h
 *
 *  \brief  Common Mesh application Terminal handler.
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

#ifndef APP_MESH_TERMINAL_H
#define APP_MESH_TERMINAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Mesh Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Prints menu messages.
 *
 *  \param[in] pMenu  Pointer to constant strings array.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppMeshPrintMenu(char * const pMenu[]);

#ifdef __cplusplus
}
#endif

#endif /* APP_MESH_TERMINAL_H */
