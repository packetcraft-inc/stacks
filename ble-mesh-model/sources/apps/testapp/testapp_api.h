/*************************************************************************************************/
/*!
 *  \file   testapp_api.h
 *
 *  \brief  TestApp application API.
 *
 *  Copyright (c) 2010-2018 Arm Ltd.
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

#ifndef TESTAPP_API_H
#define TESTAPP_API_H

#ifdef __cplusplus
extern "C"
{
#endif

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
void TestAppStart(void);

/*************************************************************************************************/
/*!
 *  \brief     Application handler init function called during system initialization.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \retval    None
 */
/*************************************************************************************************/
void TestAppHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     Initialize Mesh configuration for the application.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void TestAppConfigInit(void);

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
void TestAppHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppInitPrvSr(void);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Client module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppInitPrvCl(void);

/*************************************************************************************************/
/*!
 *  \brief  Start the Gatt Server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppStartGattSr(void);

/*************************************************************************************************/
/*!
 *  \brief     Start the GATT Client feature.
 *
 *  \param[in] enableProv  Enable Provisioning Client.
 *  \param[in] newAddress  Unicast address to be assigned during provisioning.
 *                         Valid only if enableProv is TRUE.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppStartGattCl(bool_t enableProv, uint16_t newAddress);

#ifdef __cplusplus
}
#endif

#endif /* TESTAPP_API_H */
