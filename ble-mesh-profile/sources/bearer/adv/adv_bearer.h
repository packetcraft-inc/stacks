/*************************************************************************************************/
/*!
 *  \file   adv_bearer.h
 *
 *  \brief  Advertising Bearer Mesh module interface.
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

#ifndef ADV_BEARER_H
#define ADV_BEARER_H

#include "wsf_os.h"
#include "wsf_timer.h"
#include "dm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup MESH_BEARER_ADV Advertising Bearer Adaptation Layer API
 *  \{ */

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Invalid Advertising Bearer interface ID */
#define ADV_BEARER_INVALID_IF_ID              0xFF

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Configurable parameters for Mesh Advertising Bearer */
typedef struct
{
  uint16_t scanInterval;    /*!< The scan interval, in 0.625 ms units */
  uint16_t scanWindow;      /*!< The scan window, in 0.625 ms units. Must be less than or equal
                             *   to scan interval.
                             */
  uint8_t  discMode;        /*!< The GAP discovery mode (general, limited, or none) */
  uint8_t  scanType;        /*!< The scan type (active or passive) */
  uint16_t advDuration;     /*!< The advertising duration in ms */
  uint16_t intervalMin;     /*!< Minimum advertising interval, in 0.625 ms units */
  uint16_t intervalMax;     /*!< Maximum advertising interval, in 0.625 ms units*/
} advBearerCfg_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initialize Advertising Bearer for the Mesh node.
 *
 *  \param[in] pAdvBearerCfg  Configuration pointer for Mesh Advertising Bearer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AdvBearerInit(advBearerCfg_t *pAdvBearerCfg);

/*************************************************************************************************/
/*!
 *  \brief  Register Advertising Bearer for the Mesh node.
 *
 *  \param  advIfId  Advertising interface ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerRegisterIf(meshAdvIfId_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief  Deregister Advertising Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerDeregisterIf(void);

/*************************************************************************************************/
/*!
 *  \brief  Starts advertising bearer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerStart(void);

/*************************************************************************************************/
/*!
 *  \brief  Stops current advertising bearer.
 *
 *  \return Always \ref TRUE.
 */
/*************************************************************************************************/
bool_t AdvBearerStop(void);

/*************************************************************************************************/
/*!
 *  \brief  Send Mesh message on the Advertising Bearer.
 *
 *  \param  pEvt  Advertising interface PDU to send.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerSendPacket(meshAdvPduSendEvt_t *pEvt);

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
void AdvBearerProcDmMsg(dmEvt_t *pMsg);

/*! \} */    /* MESH_BEARER_ADV */

#ifdef __cplusplus
};
#endif

#endif /* ADV_BEARER_H */
