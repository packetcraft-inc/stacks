/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Power OnOff Setup Server Model API.
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

/*! ***********************************************************************************************
 * \addtogroup ModelGenPowerOnOffSr Generic Power OnOff Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_POWER_ONOFFSETUP_SR_API_H
#define MMDL_GEN_POWER_ONOFFSETUP_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenPowOnOffSetupSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlGenPowOnOffSetupSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Power OnOff Setup Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSetupSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Power OnOff Setup Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Power OnOff Setup Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSetupSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic Power OnOff Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSetupSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSetupSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_POWER_ONOFFSETUP_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
