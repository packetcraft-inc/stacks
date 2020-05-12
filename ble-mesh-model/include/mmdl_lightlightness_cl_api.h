/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light Lightness Client Model API.
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
 * \addtogroup ModelLightLightnessCl Light Lightness Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_LIGHT_LIGHTNESS_CL_API_H
#define MMDL_LIGHT_LIGHTNESS_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_lightlightness_defs.h"

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightLightnessClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlLightLightnessClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light Lightness Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light Lightness Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClGet(meshElementId_t elementId, meshAddress_t serverAddr,  uint8_t ttl,
                          uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Set message to the destination address.
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
void MmdlLightLightnessClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                             const mmdlLightLightnessSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Set Unacknowledged message to the destination address.
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
void MmdlLightLightnessClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                  const mmdlLightLightnessSetParam_t *pSetParam,
                                  uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Linear Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLinearClGet(meshElementId_t elementId, meshAddress_t serverAddr,
                                   uint8_t ttl, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Linear Set message to the destination address.
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
void MmdlLightLightnessLinearClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   const mmdlLightLightnessLinearSetParam_t *pSetParam,
                                   uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Linear Set Unacknowledged message to the destination address.
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
void MmdlLightLightnessLinearClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                                        uint8_t ttl,
                                        const mmdlLightLightnessLinearSetParam_t *pSetParam,
                                        uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Last Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLastClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Default Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessDefaultClGet(meshElementId_t elementId, meshAddress_t serverAddr,
                                    uint8_t ttl, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Default Set message to the destination address.
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
void MmdlLightLightnessDefaultClSet(meshElementId_t elementId, meshAddress_t serverAddr,
                                    uint8_t ttl,
                                    const mmdlLightLightnessDefaultSetParam_t *pSetParam,
                                    uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Default Set Unacknowledged message to the destination address.
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
void MmdlLightLightnessDefaultClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                                         uint8_t ttl,
                                         const mmdlLightLightnessDefaultSetParam_t *pSetParam,
                                         uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Ranger Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessRangeClGet(meshElementId_t elementId, meshAddress_t serverAddr,
                                  uint8_t ttl, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Range Set message to the destination address.
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
void MmdlLightLightnessRangeClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                  const mmdlLightLightnessRangeSetParam_t *pSetParam,
                                  uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Range Set Unacknowledged message to the destination address.
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
void MmdlLightLightnessRangeClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                                       uint8_t ttl,
                                       const mmdlLightLightnessRangeSetParam_t *pSetParam,
                                       uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_LIGHTNESS_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
