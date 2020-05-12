/*************************************************************************************************/
/*!
 *  \file   mesh_access.h
 *
 *  \brief  Access module interface.
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
 *
 */
/*************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \addtogroup MESH_ACCESS Mesh Access Layer API
 *  @{
 */
/*************************************************************************************************/

#ifndef MESH_ACCESS_H
#define MESH_ACCESS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Callback definition for getting the Friend address for a subnet.
 *
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet.
 *
 *  \return    Friend address or MESH_ADDR_TYPE_UNASSIGNED if friendship is not established.
 */
/*************************************************************************************************/
typedef meshAddress_t (*meshAccFriendAddrFromSubnetCback_t)(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Callback definition for receiving Mesh messages for core models working with
 *             Device Key.
 *
 *  \param[in] opcodeIdx    Index of the opcode in the receive opcodes array registered.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] elemId       Destination element identifier.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshAccCoreMdlMsgRecvCback_t)(uint8_t opcodeIdx, uint8_t *pMsgParam,
                                             uint16_t msgParamLen, meshAddress_t src,
                                             meshElementId_t elemId, uint8_t ttl,
                                             uint16_t netKeyIndex);

/*! Structure containing Access Layer identification of models implemented by the Core stack. */
typedef struct meshAccCoreMdl_tag
{
  void                          *pNext;         /*!< Pointer to next core model. */
  meshAccCoreMdlMsgRecvCback_t  msgRecvCback;   /*!< Core model message received callback. */
  const meshMsgOpcode_t         *pOpcodeArray;  /*!< Pointer to opcode array for received messages.
                                                 */
  uint8_t                       opcodeArrayLen; /*!< Length of the opcode array. */
  meshElementId_t               elemId;         /*!< Identifier of the element containing the model.
                                                 */
  meshModelId_t                 mdlId;          /*!< Model identifier. */
} meshAccCoreMdl_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Gets memory required for configuration.
 *
 *  \return Configuration memory required or ::MESH_MEM_REQ_INVALID_CFG on error.
 */
/*************************************************************************************************/
uint32_t MeshAccGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Access layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAccInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers callback for LPN.
 *
 *  \param[in] cback  Callback to get the Friend address from subnet.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccRegisterLpn(meshAccFriendAddrFromSubnetCback_t cback);

/*************************************************************************************************/
/*!
 *  \brief     Registers the Access Layer information to multiplex received messages to core models.
 *
 *  \param[in] pCoreMdl  Pointer to linked list element containing information used by the Access
 *                       Layer to multiplex messages on the Rx data path.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAccRegisterCoreModel(meshAccCoreMdl_t *pCoreMdl);

/*************************************************************************************************/
/*!
 *  \brief      Gets number of core models contained by an element.
 *
 *  \param[in]  elemId         Element identifier.
 *  \param[out] pOutNumSig     Pointer to memory where number of SIG models is stored.
 *  \param[out] pOutNumVendor  Pointer to memory where number of Vendor models is stored.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshAccGetNumCoreModels(uint8_t elemId, uint8_t *pOutNumSig, uint8_t *pOutNumVendor);

/*************************************************************************************************/
/*!
 *  \brief      Gets core SIG models identifiers of models contained by an element.
 *
 *  \param[in]  elemId          Element identifier.
 *  \param[out] pOutMdlIdArray  Pointer to buffer store SIG model identifiers of core models.
 *  \param[in]  arraySize       Size of the array provided.
 *
 *  \return     Number of core SIG model identifiers contained by the element.
 */
/*************************************************************************************************/
uint8_t MeshAccGetCoreSigModelsIds(uint8_t elemId, meshSigModelId_t *pOutSigMdlIdArray,
                                   uint8_t arraySize);

/*************************************************************************************************/
/*!
 *  \brief     Allocate and build an WSF message for delaying or sending an Access message.
 *
 *  \param[in] pMsgInfo       Pointer to a Mesh message information structure. See ::meshMsgId_t
 *  \param[in] pMsgParam      Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen    Length of the message parameter list.
 *  \param[in] netKeyIndex    Global Network Key identifier.
 *
 *  \return    Pointer to meshSendMessage_t.
 */
/*************************************************************************************************/
meshSendMessage_t *MeshAccAllocMsg(const meshMsgInfo_t *pMsgInfo, const uint8_t *pMsgParam,
                                   uint16_t msgParamLen, uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh message to a destination address with a random delay.
 *
 *  \param[in] pMsgInfo       Pointer to a Mesh message information structure. See ::meshMsgId_t
 *  \param[in] pMsgParam      Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen    Length of the message parameter list.
 *  \param[in] netKeyIndex    Global Network Key identifier.
 *  \param[in] rndDelayMsMin  Minimum value for random send delay in ms. 0 for instant send.
 *  \param[in] rndDelayMsMax  Maximum value for random send delay in ms. 0 for instant send.
 *
 *  \return    None.
 *
 *  \remarks   pMsgInfo->appKeyIndex can also be ::MESH_APPKEY_INDEX_LOCAL_DEV_KEY or
 *             ::MESH_APPKEY_INDEX_REMOTE_DEV_KEY for local or remote Device keys.
 */
/*************************************************************************************************/
void MeshAccSendMessage(const meshMsgInfo_t *pMsgInfo, const uint8_t *pMsgParam,
                        uint16_t msgParamLen, uint16_t netKeyIndex, uint32_t rndDelayMsMin,
                        uint32_t rndDelayMsMax);

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Mesh Message based on internal Model Publication State configuration.
 *
 *  \param[in] pPubMsgInfo  Pointer to a published Mesh message information structure.
 *  \param[in] pMsgParam    Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen  Length of the message parameter list.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccPublishMessage(const meshPubMsgInfo_t *pPubMsgInfo, const uint8_t *pMsgParam,
                           uint16_t msgParamLen);

/*************************************************************************************************/
/*!
 *  \brief     Informs the Access Layer of the periodic publishing value of a model instance.
 *
 *  \param[in] elemId    Element identifier.
 *  \param[in] pModelId  Pointer to a model identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccPeriodPubChanged(meshElementId_t elemId, meshModelId_t *pModelId);

#ifdef __cplusplus
}
#endif

#endif /* MESH_ACCESS_H */

/*************************************************************************************************/
/*!
 *  @}
 */
/*************************************************************************************************/
