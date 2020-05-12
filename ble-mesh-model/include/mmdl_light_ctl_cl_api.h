/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light CTL Client Model API.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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
 * \addtogroup ModelLightCtlCl Light CTL Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_LIGHT_CTL_CL_API_H
#define MMDL_LIGHT_CTL_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Light CTL parameters structure */
typedef struct mmdlLightCtlParam_tag
{
  uint16_t    lightness;      /*!< Lightness */
  uint16_t    temperature;    /*!< Temperature */
  uint16_t    deltaUV;        /*!< DeltaUV */
} mmdlLightCtlParam_t;

/*! \brief Light CTL Set parameters structure */
typedef struct mmdlLightCtlSetParam_tag
{
  uint16_t    lightness;      /*!< Lightness */
  uint16_t    temperature;    /*!< Temperature */
  uint16_t    deltaUV;        /*!< DeltaUV */
  uint8_t     tid;            /*!< Transaction Identifier */
  uint8_t     transitionTime; /*!< Transition time */
  uint8_t     delay;          /*!< Delay in steps of 5 ms  */
} mmdlLightCtlSetParam_t;

/*! \brief Light CTL Temperature Set parameters structure */
typedef struct mmdlLightCtlTemperatureSetParam_tag
{
  uint16_t    temperature;    /*!< Light Temperature */
  uint16_t    deltaUV;        /*!< Delta UV */
  uint8_t     tid;            /*!< Transaction Identifier */
  uint8_t     transitionTime; /*!< Transition time */
  uint8_t     delay;          /*!< Delay in steps of 5 ms  */
} mmdlLightCtlTemperatureSetParam_t;

/*! \brief Light CTL Range Set parameters structure */
typedef struct mmdlLightCtlRangeSetParam_tag
{
  uint16_t    minTemperature;         /*!< Temperature range minimum */
  uint16_t    maxTemperature;         /*!< Temperature range maximum */
} mmdlLightCtlRangeSetParam_t;

/*! \brief Light CTL Client Model Status event structure */
typedef struct mmdlLightCtlClStatusEvent_tag
{
  wsfMsgHdr_t       hdr;                /*!< WSF message header */
  meshElementId_t   elementId;          /*!< Element ID */
  meshAddress_t     serverAddr;         /*!< Server Address */
  uint16_t          presentLightness;   /*!< Lightness */
  uint16_t          presentTemperature; /*!< Light Temperature */
  uint16_t          targetLightness;    /*!< Lightness */
  uint16_t          targetTemperature;  /*!< Light Temperature */
  uint8_t           remainingTime;      /*!< Remaining time until the transition completes */
} mmdlLightCtlClStatusEvent_t;

/*! \brief Light CTL Client Model Temperature Status event structure */
typedef struct mmdlLightCtlClTemperatureStatusEvent_tag
{
  wsfMsgHdr_t       hdr;                /*!< WSF message header */
  meshElementId_t   elementId;          /*!< Element ID */
  meshAddress_t     serverAddr;         /*!< Server Address */
  uint16_t          presentTemperature; /*!< Present Temperature */
  uint16_t          presentDeltaUV;     /*!< Present Delta UV */
  uint16_t          targetTemperature;  /*!< Target Temperature */
  uint16_t          targetDeltaUV;      /*!< Target Delta UV */
  uint8_t           remainingTime;      /*!< Remaining time until the transition completes */
} mmdlLightCtlClTemperatureStatusEvent_t;

/*! \brief Light CTL Client Model Default Status event structure */
typedef struct mmdlLightCtlClDefStatusEvent_tag
{
  wsfMsgHdr_t       hdr;            /*!< WSF message header */
  meshElementId_t   elementId;      /*!< Element ID */
  meshAddress_t     serverAddr;     /*!< Server Address */
  uint16_t          lightness;      /*!< Lightness */
  uint16_t          temperature;    /*!< Temperature */
  uint16_t          deltaUV;        /*!< DeltaUV */
} mmdlLightCtlClDefStatusEvent_t;

/*! \brief Light CTL Client Model Range Status event structure */
typedef struct mmdlLightCtlClRangeStatusEvent_tag
{
  wsfMsgHdr_t       hdr;              /*!< WSF message header */
  meshElementId_t   elementId;        /*!< Element ID */
  meshAddress_t     serverAddr;       /*!< Server Address */
  uint8_t           opStatus;         /*!< Operation status */
  uint16_t          minTemperature;   /*!< Temperature range minimum */
  uint16_t          maxTemperature;   /*!< Temperature range maximum */
} mmdlLightCtlClRangeStatusEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightCtlClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlLightCtlClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light CTL Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light CTL Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex, const mmdlLightCtlSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlLightCtlSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Temperature Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClTemperatureGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                  uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Temperature Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClTemperatureSet(meshElementId_t elementId, meshAddress_t serverAddr,
                                  uint8_t ttl, uint16_t appKeyIndex,
                                  const mmdlLightCtlTemperatureSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Temperature Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClTemperatureSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                                       uint8_t ttl, uint16_t appKeyIndex,
                                       const mmdlLightCtlTemperatureSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Default Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClDefGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Default Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClDefSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightCtlParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Default Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClDefSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightCtlParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Range Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClRangeGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Range Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClRangeSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlLightCtlRangeSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Range Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClRangeSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const mmdlLightCtlRangeSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_CTL_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
