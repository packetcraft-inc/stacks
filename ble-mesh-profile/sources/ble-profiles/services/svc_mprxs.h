/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Example Mesh Proxy Service Server implementation.
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

#ifndef SVC_MPRXS_H
#define SVC_MPRXS_H

#include "att_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup MESH_PROXY_PROFILE Mesh Proxy Profile API
 *  \{ */

/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

/** \name Mesh Proxy Service handle range
 *
 */
/**@{*/
#define MPRXS_START_HDL               0x0510               /*!< \brief Start handle. */
#define MPRXS_END_HDL                 (MPRXS_MAX_HDL - 1)  /*!< \brief End handle. */
/**@}*/

/**************************************************************************************************
 Handles
**************************************************************************************************/

/*! Mesh Proxy Service handles */
enum
{
  MPRXS_SVC_HDL = MPRXS_START_HDL,      /*!< Mesh Proxy Server Service declaration */
  MPRXS_DIN_CH_HDL,                     /*!< Mesh Proxy Data In characteristic */
  MPRXS_DIN_HDL,                        /*!< Mesh Proxy Data In */
  MPRXS_DOUT_CH_HDL,                    /*!< Mesh Proxy Data Out characteristic */
  MPRXS_DOUT_HDL,                       /*!< Mesh Proxy Data Out */
  MPRXS_DOUT_CH_CCC_HDL,                /*!< Mesh Proxy Data Out Client Characteristic Configuration Descriptor */
  MPRXS_MAX_HDL                         /*!< Max handle */
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
void SvcMprxsAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprxsRemoveGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Register write callback for the service.
 *
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprxsRegister(attsWriteCback_t writeCback);

/*! \} */    /* MESH_PROXY_PROFILE */

#ifdef __cplusplus
};
#endif

#endif /* SVC_MPRXS_H */
