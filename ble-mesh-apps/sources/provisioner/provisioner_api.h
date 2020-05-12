/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Provisioner application API.
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
 */
/*************************************************************************************************/

#ifndef PROVISIONER_API_H
#define PROVISIONER_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/*! Provisioning device types. */
enum provisionerPrvDeviceTypes
{
  PROVISIONER_PRV_MASTER_SWITCH, /*!< Master Switch. */
  PROVISIONER_PRV_ROOM_SWITCH,   /*!< Room Switch. */
  PROVISIONER_PRV_LIGHT,         /*!< Light. */
  PROVISIONER_PRV_NONE,          /*!< Not provisioning. */
};

/*! \brief Provising States. */
enum provisionerPrvStates
{
  PROVISIONER_ST_PRV_START,      /*!< Start provisioning. */
  PROVISIONER_ST_PRV_ADV_INPRG,  /*!< Provisioning over PB-ADV in progress. */
  PROVISIONER_ST_PRV_GATT_INPRG, /*!< Provisioning over PB-GATT in progress. */
  PROVISIONER_ST_CC_ADV_INPRG,   /*!< Configuration Client configuration over Mesh Bearer in progress. */
  PROVISIONER_ST_CC_GATT_INPRG,  /*!< Configuration Client configuration over Proxy Bearer in progress. */
};

/* \brief Device type to provision. See ::provisionerPrvDeviceTypes. */
typedef uint8_t provisionerPrvDevType_t;

/* \brief Provisioning state.  See ::provisionerPrvStates. */
typedef uint8_t provisionerState_t;

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
void ProvisionerStart(void);

/*************************************************************************************************/
/*!
 *  \brief     Application handler init function called during system initialization.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \retval    None
 */
/*************************************************************************************************/
void ProvisionerHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     Initialize Mesh configuration for the application.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void ProvisionerConfigInit(void);

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
void ProvisionerHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

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
void ProvisionerStartGattCl(bool_t enableProv, uint16_t newAddress);

/*************************************************************************************************/
/*!
 *  \brief  Provisioner application User Interface initialization.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerUiInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Provisioner application provision device.
 *
 *  \param  deviceType  device type.  See ::provisionerPrvDeviceTypes.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerProvisionDevice(provisionerPrvDevType_t deviceType);

/*************************************************************************************************/
/*!
 *  \brief  Notify application menu of system events.
 *
 *  \param  status   status of event.
 *  \param  pUuid    Device UUID or NULL if not used.
 *  \param  pDevKey  Device key of provisioned node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerMenuHandleEvent(uint8_t status, uint8_t *pUuid, uint8_t *pDevKey);

/*************************************************************************************************/
/*!
 *  \brief  Provisioner cancel ongoing provisioning.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerCancelProvisioning(void);

#ifdef __cplusplus
}
#endif

#endif /* PROVISIONER_API_H */
