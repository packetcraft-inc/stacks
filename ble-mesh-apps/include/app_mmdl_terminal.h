/*************************************************************************************************/
/*!
 *  \file   app_mmdl_terminal.h
 *
 *  \brief  Common Mesh Models application Terminal handler.
 *
 *  Copyright (c) 2015-2018 Arm Ltd. All Rights Reserved.
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
 */
/*************************************************************************************************/

#ifndef APP_MMDL_TERMINAL_H
#define APP_MMDL_TERMINAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic On Off Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGooClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Power On Off Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGpooClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Level Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGlvClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Light Lightness Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlLlClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Default Transition Time Client Model Application common
 *          terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGdttClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Battery Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGbatClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Time Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlTimClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Scene Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlSceneClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Power Level Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGenPowerLevelClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Light HSL Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlLightHslClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Light CTL Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlLightCtlClTerminalInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Registers the Scheduler Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlSchedulerClTerminalInit(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_MMDL_TERMINAL_H */
