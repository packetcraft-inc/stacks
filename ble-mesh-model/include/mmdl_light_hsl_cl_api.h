/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light HSL Client Model API.
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
 * \addtogroup ModelLightHslCl Light HSL Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_LIGHT_HSL_CL_API_H
#define MMDL_LIGHT_HSL_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Light HSL parameters structure */
typedef struct mmdlLightHslParam_tag
{
  uint16_t    lightness;      /*!< Lightness */
  uint16_t    hue;            /*!< Hue */
  uint16_t    saturation;     /*!< Saturation */
} mmdlLightHslParam_t;

/*! \brief Light HSL Set parameters structure */
typedef struct mmdlLightHslSetParam_tag
{
  uint16_t    lightness;      /*!< Lightness */
  uint16_t    hue;            /*!< Hue */
  uint16_t    saturation;     /*!< Saturation */
  uint8_t     tid;            /*!< Transaction Identifier */
  uint8_t     transitionTime; /*!< Transition time */
  uint8_t     delay;          /*!< Delay in steps of 5 ms  */
} mmdlLightHslSetParam_t;

/*! \brief Light HSL Hue Set parameters structure */
typedef struct mmdlLightHslHueSetParam_tag
{
  uint16_t    hue;            /*!< Hue */
  uint8_t     tid;            /*!< Transaction Identifier */
  uint8_t     transitionTime; /*!< Transition time */
  uint8_t     delay;          /*!< Delay in steps of 5 ms  */
} mmdlLightHslHueSetParam_t;

/*! \brief Light HSL Saturation Set parameters structure */
typedef struct mmdlLightHslSatSetParam_tag
{
  uint16_t    saturation;     /*!< Saturation */
  uint8_t     tid;            /*!< Transaction Identifier */
  uint8_t     transitionTime; /*!< Transition time */
  uint8_t     delay;          /*!< Delay in steps of 5 ms  */
} mmdlLightHslSatSetParam_t;

/*! \brief Light HSL Range Set parameters structure */
typedef struct mmdlLightHslRangeSetParam_tag
{
  uint16_t    minHue;         /*!< Hue range minimum */
  uint16_t    maxHue;         /*!< Hue range maximum */
  uint16_t    minSaturation;  /*!< Saturation range minimum */
  uint16_t    maxSaturation;  /*!< Saturation range maximum */
} mmdlLightHslRangeSetParam_t;

/*! \brief Light HSL Client Model Status event structure */
typedef struct mmdlLightHslClStatusEvent_tag
{
  wsfMsgHdr_t       hdr;            /*!< WSF message header */
  meshElementId_t   elementId;      /*!< Element ID */
  meshAddress_t     serverAddr;     /*!< Server Address */
  uint16_t          lightness;      /*!< Lightness */
  uint16_t          hue;            /*!< Hue */
  uint16_t          saturation;     /*!< Saturation */
  uint8_t           remainingTime;  /*!< Remaining time until the transition completes */
} mmdlLightHslClStatusEvent_t;

/*! \brief Light HSL Client Model Hue Status event structure */
typedef struct mmdlLightHslClHueStatusEvent_tag
{
  wsfMsgHdr_t       hdr;            /*!< WSF message header */
  meshElementId_t   elementId;      /*!< Element ID */
  meshAddress_t     serverAddr;     /*!< Server Address */
  uint16_t          presentHue;     /*!< Present Hue */
  uint16_t          targetHue;      /*!< Target Hue */
  uint8_t           remainingTime;  /*!< Remaining time until the transition completes */
} mmdlLightHslClHueStatusEvent_t;

/*! \brief Light HSL Client Model Saturation Status event structure */
typedef struct mmdlLightHslClSatStatusEvent_tag
{
  wsfMsgHdr_t       hdr;            /*!< WSF message header */
  meshElementId_t   elementId;      /*!< Element ID */
  meshAddress_t     serverAddr;     /*!< Server Address */
  uint16_t          presentSat;     /*!< Present Saturation */
  uint16_t          targetSat;      /*!< Target Saturation */
  uint8_t           remainingTime;  /*!< Remaining time until the transition completes */
} mmdlLightHslClSatStatusEvent_t;

/*! \brief Light HSL Client Model Default Status event structure */
typedef struct mmdlLightHslClDefStatusEvent_tag
{
  wsfMsgHdr_t       hdr;            /*!< WSF message header */
  meshElementId_t   elementId;      /*!< Element ID */
  meshAddress_t     serverAddr;     /*!< Server Address */
  uint16_t          lightness;      /*!< Lightness */
  uint16_t          hue;            /*!< Hue */
  uint16_t          saturation;     /*!< Saturation */
} mmdlLightHslClDefStatusEvent_t;

/*! \brief Light HSL Client Model Range Status event structure */
typedef struct mmdlLightHslClRangeStatusEvent_tag
{
  wsfMsgHdr_t       hdr;            /*!< WSF message header */
  meshElementId_t   elementId;      /*!< Element ID */
  meshAddress_t     serverAddr;     /*!< Server Address */
  uint8_t           opStatus;       /*!< Operation status */
  uint16_t          minHue;         /*!< Hue range minimum */
  uint16_t          maxHue;         /*!< Hue range maximum */
  uint16_t          minSaturation;  /*!< Saturation range minimum */
  uint16_t          maxSaturation;  /*!< Saturation range maximum */
} mmdlLightHslClRangeStatusEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightHslClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlLightHslClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light HSL Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light HSL Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                    uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Set message to the destination address.
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
void MmdlLightHslClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex, const mmdlLightHslSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Unacknowledged message to the destination address.
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
void MmdlLightHslClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlLightHslSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Target Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClTargetGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                             uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Hue Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHueGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Hue Set message to the destination address.
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
void MmdlLightHslClHueSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslHueSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Hue Set Unacknowledged message to the destination address.
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
void MmdlLightHslClHueSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslHueSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Saturation Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClSatGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Saturation Set message to the destination address.
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
void MmdlLightHslClSatSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslSatSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Saturation Set Unacknowledged message to the destination address.
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
void MmdlLightHslClSatSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslSatSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Default Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClDefGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Default Set message to the destination address.
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
void MmdlLightHslClDefSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Default Set Unacknowledged message to the destination address.
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
void MmdlLightHslClDefSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Range Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClRangeGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Range Set message to the destination address.
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
void MmdlLightHslClRangeSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslRangeSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Range Set Unacknowledged message to the destination address.
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
void MmdlLightHslClRangeSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslRangeSetParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_HSL_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
