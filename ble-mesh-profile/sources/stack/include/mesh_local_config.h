/*************************************************************************************************/
/*!
*  \file   mesh_local_config.h
*
*  \brief  Local Configuration module interface.
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
 *  \addtogroup LOCAL_CONFIGURATION Local Configuration API
 *  @{
 */
/*************************************************************************************************/

#ifndef MESH_LOCAL_CONFIG_H
#define MESH_LOCAL_CONFIG_H

#include "mesh_api.h"
#include "mesh_local_config_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Local Config return value. See ::meshReturnValues
 *  for codes starting at ::MESH_LOCAL_CFG_RETVAL_BASE
 */
typedef uint16_t meshLocalCfgRetVal_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Local Config Friend Subscription event notification callback function pointer
 *             type.
 *
 *  \param[in] event         Reason the callback is being invoked. See ::meshLocalCfgSubscrEvent_t
 *  \param[in] pEventParams  Pointer to the event parameters structure passed to the function.
 *                           See ::meshLocalCfgEventParams_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshLocalCfgFriendSubscrEventNotifyCback_t) (meshLocalCfgFriendSubscrEvent_t event,
                                                            const meshLocalCfgFriendSubscrEventParams_t *pEventParams);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshLocalCfgGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Local Configuration module and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshLocalCfgInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the LPN events callback.
 *
 *  \param[in] friendSubscrEventCback  Friend subscription event notification callback.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgRegisterLpn(meshLocalCfgFriendSubscrEventNotifyCback_t friendSubscrEventCback);

/*************************************************************************************************/
/*!
 *  \brief     Sets the address for the primary node.
 *
 *  \param[in] address  Primary element address.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPrimaryNodeAddress(meshAddress_t address);

/*************************************************************************************************/
/*!
 *  \brief      Gets a pointer to a constant local Mesh element based on element address.
 *
 *  \param[in]  elementAddress  Local element address.
 *  \param[out] ppOutElement    Pointer to store address of a constant local element or
 *                              NULL if element not found.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetElementFromAddress(meshAddress_t elementAddress,
                                                       const meshElement_t **ppOutElement);

/*************************************************************************************************/
/*!
 *  \brief      Gets a pointer to a constant local Mesh element based on element identifier in
 *              list.
 *
 *  \param[in]  elementId     Local element identifier.
 *  \param[out] ppOutElement  Pointer to store address of a constant local element or
 *                            NULL if element not found.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetElementFromId(meshElementId_t elementId,
                                                  const meshElement_t **ppOutElement);

/*************************************************************************************************/
/*!
 *  \brief      Gets the Mesh Address from an element identifier.
 *
 *  \param[in]  elementId    Local element identifier.
 *  \param[out] pOutAddress  Pointer to store the address on success.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetAddrFromElementId(meshElementId_t elementId,
                                                      meshAddress_t *pOutAddress);

/*************************************************************************************************/
/*!
 *  \brief      Gets the Mesh element identifier from an unicast address.
 *
 *  \param[in]  elementAddress  Local element identifier.
 *  \param[out] pOutElementId   Pointer to memory where to store the id on success.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetElementIdFromAddr(meshAddress_t elementAddress,
                                                      meshElementId_t *pOutElementId);

/*************************************************************************************************/
/*!
 *  \brief      Gets generated Virtual Address associated to a Label UUID.
 *
 *  \param[in]  pLabelUuid        Pointer to the Label UUID location.
 *  \param[out] pPOutVirtualAddr  Pointer to store the virtual address or MESH_ADDR_TYPE_UNASSIGNED
 *                                if Label UUID not found.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetVirtualAddrFromLabelUuid(const uint8_t *pLabelUuid,
                                                             meshAddress_t *pOutVirtualAddr);

/*************************************************************************************************/
/*!
 *  \brief     Sets product information.
 *
 *  \param[in] pProdInfo  Pointer to product information structure. See ::meshProdInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetProductInformation(const meshProdInfo_t *pProdInfo);

/*************************************************************************************************/
/*!
 *  \brief      Gets product information.
 *
 *  \param[out] pOutProdInfo  Pointer to memory where to store the product information
 *                            (CID/PID/VID). See ::meshProdInfo_t
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshLocalCfgGetProductInformation(meshProdInfo_t *pOutProdInfo);

/*************************************************************************************************/
/*!
 *  \brief  Gets supported features from the stack.
 *
 *  \return Bit field feature support value. See ::meshFeatures_t.
 */
/*************************************************************************************************/
meshFeatures_t MeshLocalCfgGetSupportedFeatures(void);

/*************************************************************************************************/
/*!
 *  \brief     Determines if model instance exists on node.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    TRUE if element contains an instance of the model.
*/
/*************************************************************************************************/
bool_t MeshLocalCfgModelExists(meshElementId_t elementId, const meshModelId_t *pModelId);

/*************************************************************************************************/
/*!
 *  \brief     Sets publish non-virtual address based on a model ID.
 *
 *  \param[in] elementId       Local element identifier.
 *  \param[in] pModelId        Pointer to generic model identifier structure.
 *  \param[in] publishAddress  Publish address.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishAddress(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   meshAddress_t publishAddress);

/*************************************************************************************************/
/*!
 *  \brief      Gets publish address based on a model ID.
 *
 *  \param[in]  elementId           Local element identifier.
 *  \param[in]  pModelId            Pointer to generic model identifier structure.
 *  \param[out] pOutPublishAddress  Pointer to memory where to store the publish address.
 *  \param[out] ppOutLabelUuid      Pointer to memory where to store the memory address of the Label
 *                                  UUID if address is virtual.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishAddress(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   meshAddress_t *pOutPublishAddress,
                                                   uint8_t **ppOutLabelUuid);

/*************************************************************************************************/
/*!
 *  \brief     Sets publish virtual address based on a model ID.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] pLabelUuid   Pointer to Label UUID.
 *  \param[in] virtualAddr  Virtual address for the Label UUID.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishVirtualAddr(meshElementId_t elementId,
                                                       const meshModelId_t *pModelId,
                                                       const uint8_t *pLabelUuid,
                                                       meshAddress_t virtualAddr);

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish period in number of steps and step resolution.
 *
 *  \param[in] elementId       Local element identifier.
 *  \param[in] pModelId        Pointer to generic model identifier structure.
 *  \param[in] numberOfSteps   Number of steps. See ::meshPublishPeriodNumSteps_t
 *  \param[in] stepResolution  The resolution of the number of steps field.
 *                             See ::meshPublishPeriodStepRes_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishPeriod(meshElementId_t elementId,
                                                  const meshModelId_t *pModelId,
                                                  meshPublishPeriodNumSteps_t numberOfSteps,
                                                  meshPublishPeriodStepRes_t stepResolution);

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish period in number of steps and step resolution.
 *
 *  \param[in]  elementId           Local element identifier.
 *  \param[in]  pModelId            Pointer to generic model identifier structure.
 *  \param[out] pOutNumberOfSteps   Pointer to store the number of steps.
 *                                  See ::meshPublishPeriodNumSteps_t
 *  \param[out] pOutStepResolution  Pointer to store the resolution of the number of steps field.
 *                                  See :: meshPublishPeriodStepRes_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishPeriod(meshElementId_t elementId,
                                                  const meshModelId_t *pModelId,
                                                  meshPublishPeriodNumSteps_t *pOutNumberOfSteps,
                                                  meshPublishPeriodStepRes_t *pOutStepResolution);

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish AppKey Index.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  AppKey Index for model publication.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishAppKeyIndex(meshElementId_t elementId,
                                                       const meshModelId_t *pModelId,
                                                       uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Un-sets model publish AppKey Index.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgMdlClearPublishAppKeyIndex(meshElementId_t elementId, const meshModelId_t *pModelId);

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish AppKey Index.
 *
 *  \param[in]  elementId        Local element identifier.
 *  \param[in]  pModelId         Pointer to generic model identifier structure.
 *  \param[out] pOutAppKeyIndex  Pointer to memory where to store publish AppKeyIndex.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishAppKeyIndex(meshElementId_t elementId,
                                                       const meshModelId_t *pModelId,
                                                       uint16_t *pOutAppKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish friendship security credential flag.
 *
 *  \param[in] elementId           Local element identifier.
 *  \param[in] pModelId            Pointer to generic model identifier structure.
 *  \param[in] friendshipCredFlag  Publish friendship security credential flag.
 *                                 See ::meshPublishFriendshipCred_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishFriendshipCredFlag(meshElementId_t elementId,
                                                              const meshModelId_t *pModelId,
                                                              meshPublishFriendshipCred_t friendshipCredFlag);

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish friendship security credential flag.
 *
 *  \param[in]  elementId               Local element identifier.
 *  \param[in]  pModelId                Pointer to generic model identifier structure.
 *  \param[out] pOutFriendshipCredFlag  Pointer to memory where to store publish friendship credential
 *                                      flag. See ::meshPublishFriendshipCred_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishFriendshipCredFlag(meshElementId_t elementId,
                                                              const meshModelId_t *pModelId,
                                                              meshPublishFriendshipCred_t *pOutFriendshipCredFlag);

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish TTL.
 *
 *  \param[in] elementId   Local element identifier.
 *  \param[in] pModelId    Pointer to generic model identifier structure.
 *  \param[in] publishTtl  Publish TTL value.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishTtl(meshElementId_t elementId,
                                               const meshModelId_t *pModelId,
                                               uint8_t publishTtl);

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish TTL.
 *
 *  \param[in]  elementId       Local element identifier.
 *  \param[in]  pModelId        Pointer to generic model identifier structure.
 *  \param[out] pOutPublishTtl  Pointer to memory where to store publish TTL.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishTtl(meshElementId_t elementId,
                                               const meshModelId_t *pModelId,
                                               uint8_t *pOutPublishTtl);

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish retransmit count.
 *
 *  \param[in] elementId     Local element identifier.
 *  \param[in] pModelId      Pointer to generic model identifier structure
 *  \param[in] retransCount  Publish retransmit count value. See ::meshPublishRetransCount_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishRetransCount(meshElementId_t elementId,
                                                        const meshModelId_t *pModelId,
                                                        meshPublishRetransCount_t retransCount);

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish retransmit count.
 *
 *  \param[in]  elementId         Local element identifier.
 *  \param[in]  pModelId          Pointer to generic model identifier structure.
 *  \param[out] pOutRetransCount  Point to publish retransmit count value.
 *                                See ::meshPublishRetransCount_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishRetransCount(meshElementId_t elementId,
                                                        const meshModelId_t *pModelId,
                                                        meshPublishRetransCount_t *pOutRetransCount);

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish retransmission interval steps.
 *
 *  \param[in] elementId     Local element identifier.
 *  \param[in] pModelId      Pointer to generic model identifier structure.
 *  \param[in] retransSteps  Publish retransmit interval steps.
 *                           See ::meshPublishRetransIntvlSteps_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks   The retransmission interval is calculated using the formula
 *             retransmitInterval = (publishRetransSteps + 1) * 50.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishRetransIntvlSteps(meshElementId_t elementId,
                                                             const meshModelId_t *pModelId,
                                                             meshPublishRetransIntvlSteps_t retransSteps);

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish retransmission interval steps.
 *
 *  \param[in]  elementId         Local element identifier.
 *  \param[in]  pModelId          Pointer to generic model identifier structure.
 *  \param[out] pOutRetransSteps  Pointer to publish retransmit interval steps value.
 *                                See ::meshPublishRetransIntvlSteps_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks    The retransmission interval is calculated using the formula
 *              retransmitInterval = (publishRetransSteps + 1) * 50.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishRetransIntvlSteps(meshElementId_t elementId,
                                                             const meshModelId_t *pModelId,
                                                             meshPublishRetransIntvlSteps_t *pOutRetransSteps);

/*************************************************************************************************/
/*!
 *  \brief     Adds non-virtual address to the subscription list for an element.
 *
 *  \param[in] elementId     Local element identifier.
 *  \param[in] pModelId      Pointer to generic model identifier structure.
 *  \param[in] subscAddress  Subscription address.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgAddAddressToSubscrList(meshElementId_t elementId,
                                                        const meshModelId_t *pModelId,
                                                        meshAddress_t subscrAddress);

/*************************************************************************************************/
/*!
 *  \brief         Gets address entry from a specific index in the subscription list for an element.
 *
 *  \param[in]     elementId         Local element identifier.
 *  \param[in]     pModelId          Pointer to generic model identifier structure.
 *  \param[out]    pOutSubscAddress  Pointer to the memory where to store the subscription address.
 *  \param[in,out] pInOutStartIndex  Pointer to an indexing variable. Must be intialized with 0 on
 *                                   to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves as an iterator, fetching one address at a time.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextAddressFromSubscrList(meshElementId_t elementId,
                                                              const meshModelId_t *pModelId,
                                                              meshAddress_t *pOutSubscAddress,
                                                              uint8_t *pInOutStartIndex);

/*************************************************************************************************/
/*!
 *  \brief     Checks if a model instance is subscribed to an address.
 *
 *  \param[in] elementId   Local element identifier.
 *  \param[in] pModelId    Pointer to generic model identifier structure.
 *  \param[in] subscrAddr  Subscription address.
 *  \param[in] pLabelUuid  Pointer to Label UUID for virtual subscription addresses.
 *
 *  \return    TRUE if address is found, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgFindAddrInModelSubscrList(meshElementId_t elementId,
                                             const meshModelId_t *pModelId,
                                             meshAddress_t subscrAddr,
                                             const uint8_t *pLabelUuid);

/*************************************************************************************************/
/*!
 *  \brief     Removes an address from the subscription list for an element.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *  \param[in] address    Address to be removed.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveAddressFromSubscrList(meshElementId_t elementId,
                                                             const meshModelId_t *pModelId,
                                                             meshAddress_t address);

/*************************************************************************************************/
/*!
 *  \brief     Adds multiple virtual address entries to the subscription list for an element.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] pLabelUuid   Pointer to Label UUID.
 *  \param[in] virtualAddr  Virtual address for the Label UUID.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgAddVirtualAddrToSubscrList(meshElementId_t elementId,
                                                            const meshModelId_t *pModelId,
                                                            const uint8_t *pLabelUuid,
                                                            meshAddress_t virtualAddr);

/*************************************************************************************************/
/*!
 *  \brief     Removes an Label UUID from the subscription list for an element.
 *
 *  \param[in] elementId   Local element identifier.
 *  \param[in] pModelId    Pointer to generic model identifier structure.
 *  \param[in] pLabelUuid  Pointer to the Label UUID to be removed.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveVirtualAddrFromSubscrList(meshElementId_t elementId,
                                                                 const meshModelId_t *pModelId,
                                                                 const uint8_t *pLabelUuid);

/*************************************************************************************************/
/*!
 *  \brief     Removes all entries from the subscription list for an element.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveAllFromSubscrList(meshElementId_t elementId,
                                                         const meshModelId_t *pModelId);

/*************************************************************************************************/
/*!
 *  \brief      Gets the subscription list available size and maximum size for an model instance.
 *
 *  \param[in]  elementId          Local element identifier.
 *  \param[in]  pModelId           Pointer to generic model identifier structure.
 *  \param[out] pOutNumAddr        Pointer to store the number of addresses in subscription list
 *                                 for the given element.
 *  \param[out] pOutTotalSize      Pointer to store the maximum size of the subscription list for
 *                                 the given element.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetSubscrListSize(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   uint8_t *pOutNumAddr,
                                                   uint8_t *pOutTotalSize);

/*************************************************************************************************/
/*!
 *  \brief     Searches for an address in the subscription lists.
 *
 *  \param[in] subscrAddr  Subscription Address to be searched.
 *
 *  \return    TRUE if the address is in any subscription list or FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgFindSubscrAddr(meshAddress_t subscrAddr);

/*************************************************************************************************/
/*!
 *  \brief  Checks if Subscription Address List is not empty.
 *
 *  \return TRUE if not empty, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgSubscrAddressListIsNotEmpty(void);

/*************************************************************************************************/
/*!
 *  \brief  Checks if Subscription Virtual Address List is not empty.
 *
 *  \return TRUE if not empty, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgSubscrVirtualAddrListIsNotEmpty(void);

/*************************************************************************************************/
/*!
 *  \brief         Gets subscription address entry from a specific index in the address list.
 *
 *  \param[out]    pOutSubscAddress  Pointer to the memory where to store the subscription address.
 *  \param[in,out] pInOutStartIndex  For input the index from where to start the search.
 *                                   For output the index where the search stopped.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextSubscrAddress(meshAddress_t *pOutSubscAddress,
                                                      uint16_t *pInOutStartIndex);

/*************************************************************************************************/
/*!
 *  \brief         Gets virtual subscription address entry from a specific index in the virtual
 *                 address list.
 *
 *  \param[out]    pOutSubscAddress  Pointer to the memory where to store the subscription address.
 *  \param[in,out] pInOutStartIndex  For input the index from where to start the search.
 *                                   For output the index where the search stopped.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextSubscrVirtualAddr(meshAddress_t *pOutSubscAddress,
                                                          uint16_t *pInOutStartIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Device key.
 *
 *  \param[in] pDevKey  Pointer to the Device Key.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetDevKey(const uint8_t *pDevKey);

/*************************************************************************************************/
/*!
 *  \brief      Gets the Device key.
 *
 *  \param[out] pOutDevKey  Pointer to store the Device Key.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshLocalCfgGetDevKey(uint8_t *pOutDevKey);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Network key and key index.
 *
 *  \param[in] netKeyIndex  Network key index.
 *  \param[in] pNetKey      Pointer to the Network Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetNetKey(uint16_t netKeyIndex, const uint8_t *pNetKey);

/*************************************************************************************************/
/*!
 *  \brief     Sets the new Network key for a key index.
 *
 *  \param[in] netKeyIndex  Network key index.
 *  \param[in] pNetKey      Pointer to the Network Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgUpdateNetKey(uint16_t netKeyIndex, const uint8_t *pNetKey);

/*************************************************************************************************/
/*!
 *  \brief     Removes the Network key for a key index.
 *
 *  \param[in] netKeyIndex       Network key index.
 *  \param[in] removeOldKeyOnly  If TRUE, NetKey Old is replaced by NetKey New
 *                               If FALSE, the NetKey is removed from the NetKey List.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveNetKey(uint16_t netKeyIndex, bool_t removeOldKeyOnly);

/*************************************************************************************************/
/*!
 *  \brief      Gets the Network key for a key index.
 *
 *  \param[in]  netKeyIndex  Network key index.
 *  \param[out] pOutNetKey   Pointer to store the Network key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNetKey(uint16_t netKeyIndex, uint8_t *pOutNetKey);

/*************************************************************************************************/
/*!
 *  \brief      Gets the updated Network key for a key index.
 *
 *  \param[in]  netKeyIndex  Network key index.
 *  \param[out] pOutNetKey   Pointer to store the updated Network key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetUpdatedNetKey(uint16_t netKeyIndex, uint8_t *pOutNetKey);

/*************************************************************************************************/
/*!
 *  \brief  Counts number of NetKeys on the node.
 *
 *  \return Number of NetKeys on the node.
 */
/*************************************************************************************************/
uint16_t MeshLocalCfgCountNetKeys(void);

/*************************************************************************************************/
/*!
 *  \brief         Gets next NetKey Index.
 *
 *  \param[out]    pOutNetKeyIndex  Pointer to variable where the next NetKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be intialized with 0 on
 *                                  to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves as an iterator, fetching one key index at a time.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextNetKeyIndex(uint16_t *pOutNetKeyIndex,
                                                    uint16_t *pInOutIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Application key and key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] pAppKey      Pointer to the Application Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetAppKey(uint16_t appKeyIndex, const uint8_t *pAppKey);

/*************************************************************************************************/
/*!
 *  \brief     Sets the new Application key for a key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] pAppKey      Pointer to the Application Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgUpdateAppKey(uint16_t appKeyIndex, const uint8_t *pAppKey);

/*************************************************************************************************/
/*!
 *  \brief     Removes the Application key for a key index.
 *
 *  \param[in] appKeyIndex       Application key index.
 *  \param[in] removeOldKeyOnly  If TRUE, AppKey Old is replaced by AppKey New
 *                               If FALSE, the AppKey is removed from the AppKey List.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveAppKey(uint16_t appKeyIndex, bool_t removeOldKeyOnly);

/*************************************************************************************************/
/*!
 *  \brief         Gets next AppKey Index.
 *
 *  \param[out]    pOutAppKeyIndex  Pointer to variable where the next AppKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be initialized with 0 on
 *                                   to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves as an iterator, fetching one key index at a time.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextAppKeyIndex(uint16_t *pOutAppKeyIndex,
                                                    uint16_t *pInOutIndex);

/*************************************************************************************************/
/*!
 *  \brief      Gets the Application key for a key index.
 *
 *  \param[in]  appKeyIndex  Application key index.
 *  \param[out] pOutAppKey   Pointer to store the Application key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks    Any of the output parameters can be omitted. Set NULL if not requested.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetAppKey(uint16_t appKeyIndex, uint8_t *pOutAppKey);

/*************************************************************************************************/
/*!
 *  \brief      Gets the updated Application key for a key index.
 *
 *  \param[in]  appKeyIndex  Application key index.
 *  \param[out] pOutAppKey   Pointer to store the updated Application key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks    Any of the output parameters can be omitted. Set NULL if not requested.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetUpdatedAppKey(uint16_t appKeyIndex, uint8_t *pOutAppKey);

/*************************************************************************************************/
/*!
 *  \brief     Binds an Application Key index to a Model ID.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  Application key index.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgBindAppKeyToModel(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Unbinds an Application key index from a Model ID.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  Application key index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgUnbindAppKeyFromModel(meshElementId_t elementId, const meshModelId_t *pModelId,
                                       uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Validates that an Application Key is bound to a Model instance.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  Application key index.
 *
 *  \return    TRUE if bind exists, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgValidateModelToAppKeyBind(meshElementId_t elementId, meshModelId_t *pModelId,
                                             uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Counts number of Application Keys bound to a Model instance.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    TRUE if bind exists, FALSE otherwise.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgCountModelBoundAppKeys(meshElementId_t elementId,
                                           const meshModelId_t *pModelId);

/*************************************************************************************************/
/*!
 *  \brief         Gets next AppKey Index bound to a model instance.
 *
 *  \param[in]     elementId        Local element identifier.
 *  \param[in]     pModelId         Pointer to generic model identifier structure.
 *  \param[out]    pOutAppKeyIndex  Pointer to variable where the next AppKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be intialized with 0 on
 *                                  to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves like an iterator. Fetching one key index at a time.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextModelBoundAppKey(meshElementId_t elementId,
                                                         const meshModelId_t *pModelId,
                                                         uint16_t *pOutAppKeyIndex,
                                                         uint8_t *pInOutIndex);

/*************************************************************************************************/
/*!
 *  \brief     Binds an Application key index to a Network key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] netKeyIndex  Network key index.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgBindAppKeyToNetKey(uint16_t appKeyIndex, uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Unbinds an Application key index from a Network key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] netKeyIndex  Network key index.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgUnbindAppKeyToNetKey(uint16_t appKeyIndex, uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief      Gets NetKey Index of Network Key bound to Application Key identified by appKeyIndex.
 *
 *  \param[in]  appKeyIndex      Application Key index.
 *  \param[out] pOutNetKeyIndex  Pointer to memory where the NetKey Index is stored if successful.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetBoundNetKeyIndex(uint16_t appKeyIndex,
                                                     uint16_t *pOutNetKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Counts number of AppKeys bound to a NetKey.
 *
 *  \param[in] netKeyIndex  Network Key index.
 *
 *  \return    Number of bound AppKeys or 0 on error.
 */
/*************************************************************************************************/
uint16_t MeshLocalCfgCountBoundAppKeys(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief         Gets next AppKey Index bound to a NetKey Index.
 *
 *  \param[in]     netKeyIndex      Network Key index.
 *  \param[out]    pOutAppKeyIndex  Pointer to variable where the next AppKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be intialized with 0 on
 *                                  to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves as an iterator,fetching one key index at a time.
 *
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextBoundAppKey(uint16_t netKeyIndex, uint16_t *pOutAppKeyIndex,
                                                    uint16_t *pInOutIndex);

/*************************************************************************************************/
/*!
 *  \brief     Validates if a Network-Application Key binding exists.
 *
 *  \param[in] netKeyIndex  Network Key index.
 *  \param[in] appKeyIndex  Application Key index.
 *
 *  \return    TRUE if bind exists. FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgValidateNetToAppKeyBind(uint16_t netKeyIndex, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief      Gets a read only copy of the Virtual Address Table.
 *
 *  \param[out] ppVtadList  Pointer to memory where the address of the virtual address information
 *                          structure is stored.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshLocalCfgGetVtadList(const meshLocalCfgVirtualAddrListInfo_t **ppVtadList);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Default TTL value.
 *
 *  \param[in] defaultTtl  Default TTL value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetDefaultTtl(uint8_t defaultTtl);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Default TTL value.
 *
 *  \return Default TTL value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetDefaultTtl(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Relay state value.
 *
 *  \param[in] relayState  Relay state value. See ::meshRelayStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetRelayState(meshRelayStates_t relayState);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Relay state value.
 *
 *  \return Relay state value. See ::meshRelayStates_t.
 */
/*************************************************************************************************/
meshRelayStates_t MeshLocalCfgGetRelayState(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Attention Timer value.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] timerVal   Attention Timer value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetAttentionTimer(meshElementId_t elementId, uint8_t timerVal);

/*************************************************************************************************/
/*!
 *  \brief     Gets the Attention Timer value.
 *
 *  \param[in] elementId  Local element identifier.
 *
 *  \return    Attention Timer value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetAttentionTimer(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Secure Network Beacon state value.
 *
 *  \param[in] beaconState  Secure Network Beacon state value. See ::meshBeaconStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetBeaconState(meshBeaconStates_t beaconState);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Secure Network Beacon state value.
 *
 *  \return Secure Network Beacon state value. See ::meshBeaconStates_t.
 */
/*************************************************************************************************/
meshBeaconStates_t MeshLocalCfgGetBeaconState(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the GATT Proxy state value.
 *
 *  \param[in] gattProxyState  GATT Proxy state value. See ::meshGattProxyStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetGattProxyState(meshGattProxyStates_t gattProxyState);

/*************************************************************************************************/
/*!
 *  \brief  Gets the GATT Proxy state value.
 *
 *  \return GATT Proxy state value. See ::meshGattProxyStates_t.
 */
/*************************************************************************************************/
meshGattProxyStates_t MeshLocalCfgGetGattProxyState(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Node Identity state value.
 *
 *  \param[in] netKeyIndex        NetKey Index to identify the subnet for the Node Identity.
 *  \param[in] nodeIdentityState  Node Identity state value. See ::meshNodeIdentityStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetNodeIdentityState(uint16_t netKeyIndex, meshNodeIdentityStates_t nodeIdentityState);

/*************************************************************************************************/
/*!
 *  \brief     Gets the Node Identity state value.
 *
 *  \param[in] netKeyIndex  NetKey Index to identify the subnet for the Node Identity.
 *
 *  \return    Node Identity state value for the given NetKey Index or
 *             ::MESH_NODE_IDENTITY_PROHIBITED_START if NetKey Index is invalid.
 */
/*************************************************************************************************/
meshNodeIdentityStates_t MeshLocalCfgGetNodeIdentityState(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Friend state value.
 *
 *  \param[in] friendState  Friend state value. See ::meshFriendStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetFriendState(meshFriendStates_t friendState);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Friend state value.
 *
 *  \return Friend state value. See ::meshFriendStates_t.
 */
/*************************************************************************************************/
meshFriendStates_t MeshLocalCfgGetFriendState(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Low Power state value.
 *
 *  \param[in] lowPowerState  Low Power state value. See ::meshLowPowerStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetLowPowerState(meshLowPowerStates_t lowPowerState);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Low Power state value.
 *
 *  \return Low Power state value. See ::meshLowPowerStates_t.
 */
/*************************************************************************************************/
meshLowPowerStates_t MeshLocalCfgGetLowPowerState(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Key Refresh Phase state value.
 *
 *  \param[in] netKeyIndex      NetKey Index to identify the NetKey in the list.
 *  \param[in] keyRefreshState  Key Refresh Phase state value. See ::meshKeyRefreshStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetKeyRefreshState(uint16_t netKeyIndex, meshKeyRefreshStates_t keyRefreshState);

/*************************************************************************************************/
/*!
 *  \brief     Gets the Key Refresh Phase state value.
 *
 *  \param[in] netKeyIndex  NetKey Index to identify the NetKey in the list.
 *
 *  \return    Key Refresh Phase state value for the given NetKey Index.
 */
/*************************************************************************************************/
meshKeyRefreshStates_t MeshLocalCfgGetKeyRefreshPhaseState(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication destination address.
 *
 *  \param[in] dstAddress  Destination address for Heartbeat messages.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbPubDst(meshAddress_t dstAddress);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication destination address.
 *
 *  \return Heartbeat Publication destination address.
 */
/*************************************************************************************************/
meshAddress_t MeshLocalCfgGetHbPubDst(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication Count Log state which controls the number of
 *             periodical Heartbeat transport control messages to be sent.
 *
 *  \param[in] countLog  Heartbeat Publication Count Log value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbPubCountLog(uint8_t countLog);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication Count Log value.
 *
 *  \return Heartbeat Publication Count Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbPubCountLog(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication Period Log state which controls the cadence of
 *             periodical Heartbeat transport control messages to be sent.
 *
 *  \param[in] periodLog  Heartbeat Publication Period Log value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbPubPeriodLog(uint8_t periodLog);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication Period Log value.
 *
 *  \return Heartbeat Publication Period Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbPubPeriodLog(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication TTL used when sending Heartbeat messages.
 *
 *  \param[in] pubTtl  Heartbeat Publication TTL value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbPubTtl(uint8_t pubTtl);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication TTL value.
 *
 *  \return Heartbeat Publication TTL value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbPubTtl(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication Features that trigger sending Heartbeat messages.
 *
 *  \param[in] pubFeatures  Heartbeat Publication Features to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbPubFeatures(meshFeatures_t pubFeatures);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication Features set.
 *
 *  \return Heartbeat Publication Features.
 */
/*************************************************************************************************/
meshFeatures_t MeshLocalCfgGetHbPubFeatures(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication NetKey Index used when sending Heartbeat messages.
 *
 *  \param[in] netKeyIndex  Heartbeat Publication NetKey Index value to be set.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbPubNetKeyIndex(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief      Gets the Heartbeat Publication NetKey Index value.
 *
 *  \param[out] pOutNetKeyIndex  Pointer to store the Heartbeat Publication NetKey Index value.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetHbPubNetKeyIndex(uint16_t *pOutNetKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription source address.
 *
 *  \param[in] srcAddress  Source address for Heartbeat messages.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbSubSrc(meshAddress_t srcAddress);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription source address.
 *
 *  \return Heartbeat Subscription source address.
 */
/*************************************************************************************************/
meshAddress_t MeshLocalCfgGetHbSubSrc(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription destination address.
 *
 *  \param[in] dstAddress  Destination address for Heartbeat messages.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbSubDst(meshAddress_t dstAddress);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription destination address.
 *
 *  \return Heartbeat Subscription destination address.
 */
/*************************************************************************************************/
meshAddress_t MeshLocalCfgGetHbSubDst(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Count Log value.
 *
 *  \param[in] countLog  Heartbeat Subscription Count Log value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubCountLog(uint8_t countLog);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Count Log value.
 *
 *  \return Heartbeat Subscription Count Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubCountLog(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Period Log state which controls the period for
 *             processing periodical Heartbeat transport control messages.
 *
 *  \param[in] periodLog  Heartbeat Subscription Period Log value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubPeriodLog(uint8_t periodLog);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Period Log value.
 *
 *  \return Heartbeat Subscription Period Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubPeriodLog(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Min Hops value which determines the minimum hops
 *             registered when receiving Heartbeat messages since receiving the most recent.
 *
 *  \param[in] minHops  Heartbeat Subscription Min Hops value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubMinHops(uint8_t minHops);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Min Hops value.
 *
 *  \return Heartbeat Subscription Min Hops value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubMinHops(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Max Hops value which determines the maximum hops
 *             registered when receiving Heartbeat messages since receiving the most recent
 *             Config Heartbeat Subscription Set message.
 *
 *  \param[in] maxHops  Heartbeat Subscription Max Hops value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubMaxHops(uint8_t maxHops);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Max Hops value which determines the maximum hops
 *          registered when receiving Heartbeat messages since receiving the most recent
 *          Config Heartbeat Subscription Set message.
 *
 *  \return Heartbeat Subscription Max Hops value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubMaxHops(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Network Transmit Count value which controls the number of message
 *             transmissions of the Network PDU originating from the node.
 *
 *  \param[in] transCount  Network Transmit Count value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetNwkTransmitCount(uint8_t transCount);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Network Transmit Count value.
 *
 *  \return Network Transmit Count value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetNwkTransmitCount(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Network Transmit Interval Steps value representing the number of 10ms
 *             steps that controls the interval between transmissions of Network PDUs originating
 *             from the node.
 *
 *  \param[in] intvlSteps  Network Transmit Interval Steps value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetNwkTransmitIntvlSteps(uint8_t intvlSteps);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Network Transmit Interval Steps value.
 *
 *  \return Network Transmit Interval Steps value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetNwkTransmitIntvlSteps(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Relay Retransmit Count value which controls the number of retransmissions
 *             of the Network PDU relayed by the node.
 *
 *  \param[in] retransCount  Relay Retransmit Count value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetRelayRetransmitCount(uint8_t retransCount);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Relay Retransmit Count value.
 *
 *  \return Relay Retransmit Count value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetRelayRetransmitCount(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the Relay Retransmit Interval Steps value representing the number of 10ms
 *             steps that controls the interval between retransmissions of Network PDUs relayed
 *             by the node.
 *
 *  \param[in] intvlSteps  Relay Retransmit Interval Steps value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetRelayRetransmitIntvlSteps(uint8_t intvlSteps);

/*************************************************************************************************/
/*!
 *  \brief  Gets the Relay Retransmit Interval Steps value.
 *
 *  \return Relay Retransmit Interval Steps value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetRelayRetransmitIntvlSteps(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the SEQ number value.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] seqNumber  SEQ number value.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetSeqNumber(meshElementId_t elementId,
                                              meshSeqNumber_t seqNumber);

/*************************************************************************************************/
/*!
 *  \brief      Gets the SEQ number value.
 *
 *  \param[in]  elementId      Local element identifier.
 *  \param[out] pOutSeqNumber  Pointer to store the SEQ number value.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetSeqNumber(meshElementId_t elementId,
                                              meshSeqNumber_t *pOutSeqNumber);

/*************************************************************************************************/
/*!
 *  \brief     Sets the SEQ number threshold value.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] seqNumber  SEQ number threshold value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetSeqNumberThresh(meshElementId_t elementId, meshSeqNumber_t seqNumber);

/*************************************************************************************************/
/*!
 *  \brief     Gets the SEQ number Threshold value.
 *
 *  \param[in] elementId  Local element identifier.
 *
 *  \return    SEQ number threshold value.
 */
/*************************************************************************************************/
meshSeqNumber_t MeshLocalCfgGetSeqNumberThresh(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief      Gets the IV Index value and IV Index update in progress flag.
 *
 *  \param[out] pIvUpdtInProg  Pointer to store TRUE if IV update is in progress or FALSE otherwise.
 *
 *  \return     IV index value.
 */
/*************************************************************************************************/
uint32_t MeshLocalCfgGetIvIndex(bool_t *pIvUpdtInProg);

/*************************************************************************************************/
/*!
 *  \brief     Sets the IV index value.
 *
 *  \param[in] ivIndex  IV index value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetIvIndex(uint32_t ivIndex);

/*************************************************************************************************/
/*!
 *  \brief     Sets the IV Index update in progress flag.
 *
 *  \param[in] ivUpdtInProg  IV Index update in progress flag value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetIvUpdateInProgress(bool_t ivUpdtInProg);

  /*************************************************************************************************/
  /*!
   *  \brief  Erase configuration.
   *
   *  \return None.
   */
  /*************************************************************************************************/
  void MeshLocalCfgEraseNvm(void);

#ifdef __cplusplus
}
#endif

#endif /* MESH_LOCAL_CONFIG_H */

/*************************************************************************************************/
/*!
 *  @}
 */
/*************************************************************************************************/
