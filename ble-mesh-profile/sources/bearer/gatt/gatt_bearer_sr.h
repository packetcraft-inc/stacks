/*************************************************************************************************/
/*!
 *  \file   gatt_bearer_sr.h
 *
 *  \brief  GATT Bearer Server module interface.
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

#ifndef GATT_BEARER_SR_H
#define GATT_BEARER_SR_H

#include "wsf_os.h"
#include "wsf_timer.h"
#include "dm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup MESH_BEARER_GATT GATT Bearer Adaptation Layer API
 *  \{ */

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Invalid GATT Bearer interface ID */
#define GATT_BEARER_INVALID_IF_ID              0xFF

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Configurable parameters for GATT Bearer */
typedef struct
{
  uint16_t intervalMin;   /*!< Minimum advertising interval in 0.625 ms units */
  uint16_t intervalMax;   /*!< Maximum advertising interval in 0.625 ms units */
  uint8_t  advType;       /*!< The advertising type */
} gattBearerSrCfg_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes GATT Bearer for the Mesh node.
 *
 *  \param[in] pGattBearerSrCfg  Configuration pointer for GATT Bearer Server.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrInit(gattBearerSrCfg_t *pGattBearerSrCfg);

/*************************************************************************************************/
/*!
 *  \brief     Configures the Advertising Data for the GATT Server hosting a Mesh Provisioning service.
 *
 *  \param[in] pDevUuid  Device UUID.
 *  \param[in] oobInfo   OOB information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrSetPrvSvcData(const uint8_t *pDevUuid, uint16_t oobInfo);

/*************************************************************************************************/
/*!
 *  \brief     Configures the Advertising Data for the GATT Server hosting a Mesh Proxy service.
 *
 *  \param[in] pSvcData    Service data.
 *  \param[in] svcDataLen  Service data length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrSetPrxSvcData(const uint8_t *pSvcData, uint8_t svcDataLen);

/*************************************************************************************************/
/*!
 *  \brief  Start Advertising on the GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GattBearerSrStart(void);

/*************************************************************************************************/
/*!
 *  \brief  Stops Advertising on the GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
bool_t GattBearerSrStop(void);

/*************************************************************************************************/
/*!
 *  \brief     Process DM messages for a Mesh node.  This function should be called
 *             from the application's event handler.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrProcDmMsg(dmEvt_t *pMsg);

/*! \} */    /* MESH_BEARER_ADV */

#ifdef __cplusplus
};
#endif

#endif /* GATT_BEARER_SR_H */
