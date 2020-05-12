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

#ifndef GATT_BEARER_CL_H
#define GATT_BEARER_CL_H

#include "wsf_os.h"
#include "wsf_timer.h"
#include "dm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup MESH_BEARER_GATT GATT Bearer Adaptation Layer API
 *  \{ */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Configurable parameters for GATT Bearer */
typedef struct
{
  uint16_t    scanInterval;                    /*!< The scan interval, in 0.625 ms units */
  uint16_t    scanWindow;                      /*!< The scan window, in 0.625 ms units.   Must be
                                                   less than or equal to scan interval. */
  uint8_t     discMode;                        /*!< The GAP discovery mode (general, limited, or none) */
  uint8_t     scanType;                        /*!< The scan type (active or passive) */
  uint16_t    serviceUuid;                     /*!< The searched service UUID */
} gattBearerClCfg_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief Configuration pointer for GATT Bearer Client */
extern gattBearerClCfg_t *pGattBearerClCfg;

/*! \brief Configuration pointer for GATT Bearer Client Connection */
extern hciConnSpec_t *pGattBearerClConnCfg;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GattBearerClInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Schedules the scanning on the GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GattBearerClStart(void);

/*************************************************************************************************/
/*!
 *  \brief  Stops the scanning on the GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
bool_t GattBearerClStop(void);

/*************************************************************************************************/
/*!
 *  \brief     Initiates a GATT connection to a Mesh node.
 *
 *  \param[in] addrType  Address Type of the peer device.
 *  \param[in] pAddr     Address of the peer device.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerClConnect(uint8_t addrType, uint8_t* pAddr);

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
void GattBearerClProcDmMsg(dmEvt_t *pMsg);

/*! \} */    /* MESH_BEARER_GATT */

#ifdef __cplusplus
};
#endif

#endif /* GATT_BEARER_CL_H */
