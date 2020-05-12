/*************************************************************************************************/
/*!
 *  \file   wsf_os_int.h
 *
 *  \brief  Software foundation OS platform-specific interface file.
 *
 *  Copyright (c) 2009-2019 Arm Ltd. All Rights Reserved.
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
#ifndef WSF_OS_INT_H
#define WSF_OS_INT_H

#include "wsf_types.h"
#include "wsf_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Init and handler function type-- for test purposes */
typedef void (*wsfTestInit_t)(wsfHandlerId_t handlerId);
typedef void (*wsfTestHandler_t)(wsfEventMask_t event, void *pMsg);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize OS.
 *
 *  \param  msPerTick Milliseconds per timer tick.
 *
 *  \return Event handler ID.
 */
/*************************************************************************************************/
void wsfOsInit(uint8_t msPerTick,
    uint16_t bufMemLen, uint8_t *pBufMem, uint8_t numPools, wsfBufPoolDesc_t *pDesc);

/*************************************************************************************************/
/*!
 *  \brief  Set the App event handler and init function for test purposes.
 *
 *  \param  handler     Event handler.
 *  \param  handlerInit Init function.
 *
 *  \note   This function must be called before wsfOsInit().
 */
/*************************************************************************************************/
void wsfOsSetAppHandler(wsfTestHandler_t handler, wsfTestInit_t handlerInit);

/*************************************************************************************************/
/*!
 *  \brief  Set the instance name of the device for NVM simulation.
 *
 *  \param  str        Instance string.
 *
 *  \note   This function is used to create unique files for storing NVM data on the win32 host
 *          tester.  This function should be called from host test scripts that use NVM.
 */
/*************************************************************************************************/
void WsfNvmSetInstanceStr(char *str);

/*************************************************************************************************/
/*!
 *  \brief  Called to enable/disable NVM simulation on the host tester.
 *
 *  \param  enabled    Enabled boolean.
 */
/*************************************************************************************************/
void WsfNvmEnableSimulation(bool_t enabled);

#ifdef __cplusplus
};
#endif

#endif /* WSF_OS_INT_H */
