/*************************************************************************************************/
/*!
 *  \file   app_mesh_api.h
 *
 *  \brief  Mesh Application framework API.
 *
 *  Copyright (c) 2015-2019 Arm Ltd. All Rights Reserved.
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

#ifndef APP_MESH_API_H
#define APP_MESH_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_assert.h"
#include "dm_api.h"
#include "mesh_prv.h"
#include "mmdl_types.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh callback events start */
#define APP_MESH_CBACK_START                 0xF0 /*! Mesh App callback event starting value */

/*! Mesh App callback events */
enum appMeshEventValues
{
  APP_BR_TIMEOUT_EVT            = APP_MESH_CBACK_START, /*! App bearer timeout event */
  APP_MESH_NODE_IDENTITY_TIMEOUT_EVT,                   /*! ADV with Node Identity timeout event */
  APP_MESH_NODE_IDENTITY_USER_INTERACTION_EVT,          /*! ADV with Node Identity user interaction
                                                         *  event
                                                         */
  APP_MESH_NODE_RST_TIMEOUT_EVT,                        /*! Node Reset timeout event */
};

/*! Mesh App callback events end */
#define APP_MESH_CBACK_END                   APP_MESH_NODE_RST_TIMEOUT_EVT  /*! Mesh App callback
                                                                             *  event ending value
                                                                             */

/*! ADV with Node Identity timeout */
#define APP_MESH_NODE_IDENTITY_TIMEOUT_MS    60000

/*! Node Reset timeout */
#define APP_MESH_NODE_RST_TIMEOUT_MS         100

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Configurable parameters for Provisioning Server */
typedef struct meshPrvSrCfg_tag
{
  uint8_t   devUuid[MESH_PRV_DEVICE_UUID_SIZE]; /*!< Device UUID */
  uint32_t  pbAdvInterval;                      /*!< Provisioning Bearer advertising interval */
  uint8_t   pbAdvIfId;                          /*!< Provisioning Bearer ADV interface ID */
  bool_t    pbAdvRestart;                       /*!< Auto-restart Provisioning */
} meshPrvSrCfg_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Configuration pointer for Provisioning Server */
extern meshPrvSrCfg_t *pMeshPrvSrCfg;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize common Mesh Application functionality for a Mesh Node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshNodeInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Pointer to version number string.
 *
 *  \param[in] pVersion  Pointer to version string.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppMeshSetVersion(const char *pVersion);

/*************************************************************************************************/
/*!
 *  \brief  Gets the application version.
 *
 *  \return Pointer to version number string.
 */
/*************************************************************************************************/
const char *AppMeshGetVersion(void);

/*************************************************************************************************/
/*!
 *  \brief  Clears the NVM for Mesh Stack and models.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshClearNvm(void);

/*************************************************************************************************/
/*!
 *  \brief  Initiate software system reset.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshReset(void);

/*************************************************************************************************/
/*!
 *  \brief     Handle a scan report.
 *
 *  \param[in] pMsg     Pointer to DM callback event message.
 *  \param[in] pDevUuid Pointer to device UUID to search for.
 *
 *  \return   \ref TRUE if connection request was sent, else \ref FALSE.
 */
/*************************************************************************************************/
bool_t AppProxyScanReport(dmEvt_t *pMsg, uint8_t *pDevUuid);

#ifdef __cplusplus
}
#endif

#endif /* APP_MESH_API_H */
