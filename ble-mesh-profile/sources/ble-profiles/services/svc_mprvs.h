/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Example Mesh Provisioning Service Server implementation.
 *
 *  Copyright (c) 2016-2018 Arm Ltd. All Rights Reserved.
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

#ifndef SVC_MPRVS_H
#define SVC_MPRVS_H

#include "att_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup MESH_PROVISIONING_PROFILE
 *  \{ */

/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

/** \name Mesh Provisioning Service handle range
 *
 */
/**@{*/
#define MPRVS_START_HDL               0x0500              /*!< \brief Start handle. */
#define MPRVS_END_HDL                 (MPRVS_MAX_HDL - 1) /*!< \brief End handle. */
/**@}*/

/**************************************************************************************************
 Handles
**************************************************************************************************/

/*! Mesh Provisioning Service handles */
enum
{
  MPRVS_SVC_HDL = MPRVS_START_HDL,      /*!< Mesh Provisioning Server Service declaration */
  MPRVS_DIN_CH_HDL,                     /*!< Mesh Provisioning Data In characteristic */
  MPRVS_DIN_HDL,                        /*!< Mesh Provisioning Data In */
  MPRVS_DOUT_CH_HDL,                    /*!< Mesh Provisioning Data Out characteristic */
  MPRVS_DOUT_HDL,                       /*!< Mesh Provisioning Data Out */
  MPRVS_DOUT_CH_CCC_HDL,                /*!< Mesh Provisioning Data Out Client Characteristic Configuration Descriptor */
  MPRVS_MAX_HDL                         /*!< Max handle */
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprvsAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprvsRemoveGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Register write callback for the service.
 *
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprvsRegister(attsWriteCback_t writeCback);

/*! \} */    /* MESH_PROVISIONING_PROFILE */

#ifdef __cplusplus
};
#endif

#endif /* SVC_MPRVS_H */
