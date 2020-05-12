/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Time Client Model API.
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

/*! ***********************************************************************************************
 * \addtogroup ModelTimeCl Time Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_TIME_CL_API_H
#define MMDL_TIME_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Time Client Set parameters structure */
typedef struct mmdlTimeSetParam_tag
{
  mmdlTimeState_t          state;          /*!< New Time State */
} mmdlTimeSetParam_t;

/*! \brief Model Time Client Zone Set parameters structure */
typedef struct mmdlTimeZoneSetParam_tag
{
  mmdlTimeZoneState_t      state;          /*!< New Time Zone State */
} mmdlTimeZoneSetParam_t;

/*! \brief Model Time Client Delta Set parameters structure */
typedef struct mmdlTimeDeltaSetParam_tag
{
  mmdlTimeDeltaState_t     state;          /*!< New Time Delta State */
} mmdlTimeDeltaSetParam_t;

/*! \brief Model Time Client Role Set parameters structure */
typedef struct mmdlTimeRoleSetParam_tag
{
  mmdlTimeRoleState_t      state;          /*!< New Time Role State */
} mmdlTimeRoleSetParam_t;

/*! \brief Time Client Model Status event structure */
typedef struct mmdlTimeClStatusEvent_tag
{
  wsfMsgHdr_t              hdr;            /*!< WSF message header */
  meshElementId_t          elementId;      /*!< Element ID */
  meshAddress_t            serverAddr;     /*!< Server Address */
  mmdlTimeState_t          state;          /*!< Received state */
} mmdlTimeClStatusEvent_t;

/*! \brief Time Client Model Zone Status event structure */
typedef struct mmdlTimeClZoneStatusEvent_tag
{
  wsfMsgHdr_t              hdr;            /*!< WSF message header */
  meshElementId_t          elementId;      /*!< Element ID */
  meshAddress_t            serverAddr;     /*!< Server Address */
  uint8_t                  offsetCurrent;  /*!< Current local time zone offset */
  uint8_t                  offsetNew;      /*!< Upcoming local time zone offset */
  uint64_t                 taiZoneChange;  /*!< TAI Seconds time of the upcoming
                                            * Time Zone Offset change
                                            */
} mmdlTimeClZoneStatusEvent_t;

/*! \brief Time Client Model Delta Status event structure */
typedef struct mmdlTimeClDeltaStatusEvent_tag
{
  wsfMsgHdr_t              hdr;            /*!< WSF message header */
  meshElementId_t          elementId;      /*!< Element ID */
  meshAddress_t            serverAddr;     /*!< Server Address */
  uint16_t                 deltaCurrent;   /*!< Current local time zone offset */
  uint16_t                 deltaNew;       /*!< Upcoming diff between TAI and UTC in seconds */
  uint64_t                 deltaChange;    /*!< TAI Seconds time of the upcoming TAI-UTC Delat change */
} mmdlTimeClDeltaStatusEvent_t;

/*! \brief Time Client Model Role Status event structure */
typedef struct mmdlTimeClRoleStatusEvent_tag
{
  wsfMsgHdr_t              hdr;            /*!< WSF message header */
  meshElementId_t          elementId;      /*!< Element ID */
  meshAddress_t            serverAddr;     /*!< Server Address */
  uint8_t                  timeRole;       /*!< Time Role for element */
} mmdlTimeClRoleStatusEvent_t;

/*! \brief Time Client Model event callback parameters structure */
typedef union mmdlTimeClEvent_tag
{
  wsfMsgHdr_t                  hdr;              /*!< WSF message header */
  mmdlTimeClStatusEvent_t      statusEvent;      /*!< State updated event. Used for
                                                  *   ::MMDL_TIME_CL_STATUS_EVENT.
                                                  */
  mmdlTimeClZoneStatusEvent_t  zoneStatusEvent;  /*!< State updated event. Used for
                                                  *   ::MMDL_TIMEZONE_CL_STATUS_EVENT.
                                                  */
  mmdlTimeClDeltaStatusEvent_t deltaStatusEvent; /*!< State updated event. Used for
                                                  *   ::MMDL_TIMEDELTA_CL_STATUS_EVENT.
                                                  */
  mmdlTimeClRoleStatusEvent_t  roleStatusEvent;  /*!< State updated event. Used for
                                                  *    ::MMDL_TIMEROLE_CL_STATUS_EVENT.
                                                  */
} mmdlTimeClEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlTimeClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlTimeClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Time Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Time Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClGet(meshElementId_t elementId, meshAddress_t serverAddr,  uint8_t ttl,
                   uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                   const mmdlTimeSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Zone Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClZoneGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Zone Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClZoneSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlTimeZoneSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Delta Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClDeltaGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Delta Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClDeltaSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        const mmdlTimeDeltaSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Role Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClRoleGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Role Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClRoleSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        const mmdlTimeRoleSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_TIME_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
