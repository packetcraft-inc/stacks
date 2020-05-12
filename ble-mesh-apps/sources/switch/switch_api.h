/*************************************************************************************************/
/*!
 *  \file   switch_api.h
 *
 *  \brief  Switch application API.
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

#ifndef SWITCH_API_H
#define SWITCH_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_types.h"
#include "switch_config.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic OnOff Server element control block structure */
typedef struct switchElemCb_tag
{
  mmdlGenOnOffState_t state;          /*!< New OnOff State */
  uint8_t             tid;            /*!< Transaction Identifier */
} switchElemCb_t;

/**************************************************************************************************
  Externs
**************************************************************************************************/

extern switchElemCb_t switchElemCb[SWITCH_ELEMENT_COUNT];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SwitchStart(void);

/*************************************************************************************************/
/*!
 *  \brief     Application handler init function called during system initialization.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \retval    None
 */
/*************************************************************************************************/
void SwitchHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     Initialize Mesh configuration for the application.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void SwitchConfigInit(void);

/*************************************************************************************************/
/*!
 *  \brief     WSF event handler for the application.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void SwitchHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* SWITCH_API_H */
