/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Power Level Client Model API.
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
 * \addtogroup ModelGenPowerLevelCl Generic Power Level Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_POWER_LEVEL_CL_API_H
#define MMDL_GEN_POWER_LEVEL_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Power Level Client Set parameters structure */
typedef struct mmdlGenPowerLevelSetParam_tag
{
  mmdlGenPowerLevelState_t state;          /*!< New Power State */
  uint8_t                  tid;            /*!< Transaction Identifier */
  uint8_t                  transitionTime; /*!< Transition time */
  uint8_t                  delay;          /*!< Delay in steps of 5 ms  */
} mmdlGenPowerLevelSetParam_t;

/*! \brief Model Power Range Client Set parameters structure */
typedef struct mmdlGenPowerDefaultSetParam_tag
{
  mmdlGenPowerLevelState_t state;          /*!< Default Power State */
} mmdlGenPowerDefaultSetParam_t;

/*! \brief Model Power Level Client Set parameters structure */
typedef struct mmdlGenPowerRangeSetParam_tag
{
  mmdlGenPowerLevelState_t powerMin;       /*!< Minimum Power Range State */
  mmdlGenPowerLevelState_t powerMax;       /*!< Maximum Power Range State */
} mmdlGenPowerRangeSetParam_t;

/*! \brief Generic Power Level Client Model Status event structure */
typedef struct mmdlGenPowerLevelClStatusEvent_tag
{
  wsfMsgHdr_t               hdr;           /*!< WSF message header */
  meshElementId_t           elementId;     /*!< Element ID */
  meshAddress_t             serverAddr;    /*!< Server Address */
  mmdlGenPowerLevelState_t  state;         /*!< Received published state */
  mmdlGenPowerLevelState_t  targetState;   /*!< Received published target state */
  uint8_t                   remainingTime; /*!< Remaining time until the transition completes */
} mmdlGenPowerLevelClStatusEvent_t;

/*! \brief Generic Power Last Client Model Status event structure */
typedef struct mmdlGenPowerLastClStatusEvent_tag
{
  wsfMsgHdr_t               hdr;           /*!< WSF message header */
  meshElementId_t           elementId;     /*!< Element ID */
  meshAddress_t             serverAddr;    /*!< Server Address */
  mmdlGenPowerLevelState_t  lastState;     /*!< Received published last state */
} mmdlGenPowerLastClStatusEvent_t;

/*! \brief Generic Power Default Client Model Status event structure */
typedef struct mmdlGenPowerDefaultClStatusEvent_tag
{
  wsfMsgHdr_t               hdr;           /*!< WSF message header */
  meshElementId_t           elementId;     /*!< Element ID */
  meshAddress_t             serverAddr;    /*!< Server Address */
  mmdlGenPowerLevelState_t  state;         /*!< Received published state */
} mmdlGenPowerDefaultClStatusEvent_t;

/*! \brief Generic Power Range Client Model Status event structure */
typedef struct mmdlGenPowerRangeClStatusEvent_tag
{
  wsfMsgHdr_t               hdr;           /*!< WSF message header */
  meshElementId_t           elementId;     /*!< Element ID */
  meshAddress_t             serverAddr;    /*!< Server Address */
  uint8_t                   statusCode;    /*!< Status Code */
  mmdlGenPowerLevelState_t  powerMin;      /*!< Minimum Power Range state */
  mmdlGenPowerLevelState_t  powerMax;      /*!< Maximum Power Range state */
} mmdlGenPowerRangeClStatusEvent_t;

/*! \brief Generic Power Level Client Model event callback parameters structure */
typedef union mmdlGenPowerLevelClEvent_tag
{
  wsfMsgHdr_t                        hdr;                 /*!< WSF message header */
  mmdlGenPowerLevelClStatusEvent_t   statusEvent;         /*!< State updated event. Used for
                                                           *   ::MMDL_GEN_POWER_LEVEL_CL_STATUS_EVENT.
                                                           */
  mmdlGenPowerLastClStatusEvent_t    lastStatusEvent;     /*!< State updated event. Used for
                                                           *    ::MMDL_GEN_POWER_LAST_CL_STATUS_EVENT.
                                                           */
  mmdlGenPowerDefaultClStatusEvent_t defaultStatusEvent;  /*!< State updated event. Used for
                                                           *   ::MMDL_GEN_POWER_DEFAULT_CL_STATUS_EVENT.
                                                           */
  mmdlGenPowerRangeClStatusEvent_t   rangeStatusEvent;    /*!< State updated event. Used for
                                                           *   ::MMDL_GEN_POWER_RANGE_CL_STATUS_EVENT.
                                                           */
} mmdlGenPowerLevelClEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenPowerLevelClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlGenPowerLevelClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for On Off Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Power Level Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerLevelGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClGet(meshElementId_t elementId, meshAddress_t serverAddr,  uint8_t ttl,
                            uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerLevelSet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlGenPowerLevelSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerLevelSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 const mmdlGenPowerLevelSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerLastGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLastClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerDefaultGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerDefaultSet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] powerLevel   Default power level.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex, mmdlGenPowerLevelState_t powerLevel);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerDefaultSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] powerLevel   Default power level.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   uint16_t appKeyIndex, mmdlGenPowerLevelState_t powerLevel);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerRangeGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerRangeSet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlGenPowerRangeSetParam_t *pSetParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerRangeSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const mmdlGenPowerRangeSetParam_t *pSetParam);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_POWER_LEVEL_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
