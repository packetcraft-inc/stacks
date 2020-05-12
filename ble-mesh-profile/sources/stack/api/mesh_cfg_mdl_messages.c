/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_messages.c
 *
 *  \brief  Configuration Messages module implementation.
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

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "mesh_types.h"
#include "mesh_utils.h"
#include "mesh_api.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_main.h"
#include "mesh_access.h"
#include "mesh_cfg_mdl_messages.h"

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Packs a single 12-bit key index as required by the spec (Section 4.3.1.1)
 *
 *  \param[out] pBuf      Pointer to buffer where to store the packed state.
 *  \param[in]  keyIndex  12-bit key index.
 *
 *  \return     Number of bytes occupied.
 */
/*************************************************************************************************/
uint8_t meshCfgMsgPackSingleKeyIndex(uint8_t *pBuf, uint16_t keyIndex)
{
  /* Store the LSBs of the key index in octet 0. */
  *pBuf     = keyIndex & 0xFF;
  /* Store the MSBs of the key index in octet 1, lower nibble. */
  *(pBuf+1) = (keyIndex >> 8) & 0x0F;

  return CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES;
}

/*************************************************************************************************/
/*!
 *  \brief      Packs two 12-bit key indexes as required by the spec (Section 4.3.1.1)
 *
 *  \param[out] pBuf       Pointer to buffer where to store the packed state.
 *  \param[in]  keyIndex1  12-bit key index.
 *  \param[in]  keyIndex2  12-bit key index.
 *
 *  \return     Number of bytes occupied.
 */
/*************************************************************************************************/
uint8_t meshCfgMsgPackTwoKeyIndex(uint8_t *pBuf, uint16_t keyIndex1, uint16_t keyIndex2)
{
  /* Store the LSBs of the first key index in octet 0. */
  *pBuf = keyIndex1 & 0xFF;
  /* Store the MSBs of the first key index in octet 1, lower nibble. */
  *(pBuf + 1) = (keyIndex1 >> 8) & 0x0F;

  /* Store the LSBs of the second key index in octet 1, higher nibble. */
  *(pBuf + 1) |= (keyIndex2 << 4) & 0xF0;
  /* Store the MSBs of the second key index in octet 2. */
  *(pBuf + 2) = (keyIndex2 >> 4) & 0xFF;

  return CFG_MDL_MSG_2KEY_PACKED_NUM_BYTES;
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks a 12-bit key indexes as required by the spec (Section 4.3.1.1)
 *
 *  \param[in]  pBuf       Pointer to buffer where the packed state is stored.
 *  \param[out] pKeyIndex  Pointer to 12-bit key index.
 *
 *  \return     Number of bytes occupied.
 */
/*************************************************************************************************/
uint8_t meshCfgMsgUnpackSingleKeyIndex(uint8_t *pBuf, uint16_t *pKeyIndex)
{
  /* Get the LSBs of the key index from octet 0. */
  *pKeyIndex = (*pBuf) & 0xFF;
  /* Get the MSBs of the key index from the lower nibble of octet 1. */
  (*pKeyIndex) |=  (((uint16_t)(*(pBuf+1) & 0x0F)) << 8);

  return CFG_MDL_MSG_1KEY_PACKED_NUM_BYTES;
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks two 12-bit key indexes as required by the spec (Section 4.3.1.1)
 *
 *  \param[in]  pBuf        Pointer to buffer where the packed state is stored.
 *  \param[out] pKeyIndex1  Pointer to 12-bit key index.
 *  \param[out] pKeyIndex2  Pointer to 12-bit key index.
 *
 *  \return     Number of bytes grabbed.
 */
/*************************************************************************************************/
uint8_t meshCfgMsgUnpackTwoKeyIndex(uint8_t *pBuf, uint16_t *pKeyIndex1, uint16_t *pKeyIndex2)
{
  /* Get the LSBs of the first key index from octet 0. */
  *pKeyIndex1 = (*pBuf) & 0xFF;
  /* Get the MSBs of the first key index from the lower nibble of octet 1. */
  (*pKeyIndex1) |= (((uint16_t)(*(pBuf + 1) & 0x0F)) << 8);

  /* Get the LSBs of the second key index from the higher nibble of octet 1. */
  *pKeyIndex2 = (*(pBuf + 1) & 0xF0) >> 4;
  /* Get the MSBs of the second key index from octet 2. */
  *pKeyIndex2 |= (((uint16_t)(*(pBuf + 2) & 0xFF)) << 4);

  return CFG_MDL_MSG_2KEY_PACKED_NUM_BYTES;
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Composition Data State.
 *
 *  \param[in]  pBuf        Pointer to buffer where the packed state is stored.
 *  \param[in]  dataLength  Value of the length of page data to be stored.
 *  \param[out] pCompData   Pointer to memory where to store a Composition Data State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackCompData(uint8_t *pBuf, uint16_t dataLength, meshCompData_t *pCompData)
{
  /* Get composition data page number. */
  pCompData->pageNumber = pBuf[0];

  pCompData->pageSize = dataLength;

  /* Get composition data for the page number. */
  pCompData->pPage = &pBuf[1];
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the number of bytes required to pack Page 0.
 *
 *  \return Number of sizes.
 */
/*************************************************************************************************/
uint16_t meshCfgMsgGetPackedCompDataPg0Size(void)
{
  meshElementId_t id = 0;
  uint16_t msgParamLen = CFG_MDL_MSG_COMP_DATA_PG0_EMPTY_NUM_BYTES;
  uint16_t nextLen;
  uint8_t numCoreSig = 0;
  uint8_t numCoreVendor = 0;

  /* Get number of core models for primary element. */
  MeshAccGetNumCoreModels(0, &numCoreSig, &numCoreVendor);

  /* Calculate number of bytes occupied by core models. */
  nextLen = (numCoreSig * sizeof(meshSigModelId_t)) ;

   /* Iterate through elements. */
  for (id = 0; id < pMeshConfig->elementArrayLen; id++)
  {
    /* Calculate next length when an element is added to page 0. */
    nextLen += CFG_MDL_MSG_COMP_DATA_PG0_ELEM_HDR_NUM_BYTES +
               (pMeshConfig->pElementArray[id].numSigModels * sizeof(meshSigModelId_t)) +
               (pMeshConfig->pElementArray[id].numVendorModels * sizeof(meshVendorModelId_t));

    /* Check if exceeds maximum number of bytes. */
    if (msgParamLen + nextLen > CFG_MDL_MSG_COMP_DATA_PG0_MAX_NUM_BYTES)
    {
      break;
    }
    msgParamLen += nextLen;
    nextLen = 0;
  }

  return msgParamLen;
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Composition Data State with Page 0 if required.
 *
 *  \param[out] pBuf        Pointer to buffer where to store the packed state.
 *  \param[in]  pageNumber  Page number.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackCompData(uint8_t *pBuf, uint8_t pageNumber)
{
  uint8_t *ptr;
  meshElementId_t id = 0;
  uint8_t mdlIdx;
  uint16_t msgParamLen = 0;
  uint16_t nextLen;
  meshProdInfo_t prodInfo;
  uint8_t numCoreSig = 0;
  uint8_t numCoreVendor = 0;

  msgParamLen = CFG_MDL_MSG_COMP_DATA_PG0_EMPTY_NUM_BYTES;

  /* Get number of core models. */
  MeshAccGetNumCoreModels(0, &numCoreSig, &numCoreVendor);

  /* Sanity check: there are no core vendor models */
  WSF_ASSERT(numCoreVendor == 0);
  /* Sanity check:  there is at least one core SIG model */
  WSF_ASSERT(numCoreSig != 0);

  /* Implementation does not define any internal vendor models. */
  numCoreVendor = 0;

  /* Add number of bytes occupied by the core models. */
  nextLen = (numCoreSig * sizeof(meshSigModelId_t));

  /* Set composition data page number. */
  pBuf[0] = pageNumber;

  /* For Page 0, start packing. */
  if (pageNumber != 0)
  {
    return;
  }
  /* Start building page 0. */
  ptr = &pBuf[1];

  /* Read product information. */
  MeshLocalCfgGetProductInformation(&prodInfo);

  /* Pack company id. */
  UINT16_TO_BSTREAM(ptr, prodInfo.companyId);

  /* Pack product id. */
  UINT16_TO_BSTREAM(ptr, prodInfo.productId);

  /* Pack version id. */
  UINT16_TO_BSTREAM(ptr, prodInfo.versionId);

  /* Pack CRPL. */
  UINT16_TO_BSTREAM(ptr, pMeshConfig->pMemoryConfig->rpListSize);

  /* Pack features. */
  UINT16_TO_BSTREAM(ptr, MeshLocalCfgGetSupportedFeatures());

  /* Iterate through elements. */
  for (id = 0; id < pMeshConfig->elementArrayLen; id++)
  {
    nextLen += CFG_MDL_MSG_COMP_DATA_PG0_ELEM_HDR_NUM_BYTES +
               (pMeshConfig->pElementArray[id].numSigModels * sizeof(meshSigModelId_t)) +
               (pMeshConfig->pElementArray[id].numVendorModels * sizeof(meshVendorModelId_t));

    if (msgParamLen + nextLen > CFG_MDL_MSG_COMP_DATA_PG0_MAX_NUM_BYTES)
    {
      return;
    }

    msgParamLen += nextLen;
    nextLen = 0;

    /* Pack element "header". */
    UINT16_TO_BSTREAM(ptr, pMeshConfig->pElementArray[id].locationDescriptor);
    UINT8_TO_BSTREAM(ptr, pMeshConfig->pElementArray[id].numSigModels + numCoreSig);
    UINT8_TO_BSTREAM(ptr, pMeshConfig->pElementArray[id].numVendorModels + numCoreVendor);
    /* Pack core SIG models on element 0. */
    if (numCoreSig)
    {
      meshSigModelId_t *pCoreSigMdlIds;

      /* Allocate space to store all core models */
      if ((pCoreSigMdlIds = (meshSigModelId_t *) WsfBufAlloc(numCoreSig * sizeof(meshSigModelId_t))) == NULL)
      {
        return;
      }

      /* Populate Core models structure. */
      MeshAccGetCoreSigModelsIds(id, pCoreSigMdlIds, numCoreSig);

      /* Pack core SIG models. */
      for (mdlIdx = 0; mdlIdx < numCoreSig; mdlIdx++)
      {
        UINT16_TO_BSTREAM(ptr, pCoreSigMdlIds[mdlIdx]);
      }
      /* Implementation only has core models on element 0. */
      numCoreSig = 0;

      /* Free core models structure. */
      WsfBufFree(pCoreSigMdlIds);
    }
    /* Pack SIG models. */
    for (mdlIdx = 0; mdlIdx < pMeshConfig->pElementArray[id].numSigModels; mdlIdx++)
    {
      UINT16_TO_BSTREAM(ptr, pMeshConfig->pElementArray[id].pSigModelArray[mdlIdx].modelId);
    }

    /* Pack Vendor models. */
    for (mdlIdx = 0; mdlIdx < pMeshConfig->pElementArray[id].numVendorModels; mdlIdx++)
    {
      VEND_MDL_TO_BSTREAM(ptr, pMeshConfig->pElementArray[id].pVendorModelArray[mdlIdx].modelId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Relay Composite State.
 *
 *  \param[out] pBuf          Pointer to buffer where to store the packed state.
 *  \param[in]  pRelayState   Pointer to a Relay State.
 *  \param[in]  pRetranState  Pointer to a Relay Retransmit State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackRelay(uint8_t *pBuf, meshRelayStates_t *pRelayState,
                         meshRelayRetransState_t *pRetranState)
{
  /* Set relay state. */
  pBuf[CFG_MDL_MSG_RELAY_COMP_STATE_RELAY_OFFSET] = *pRelayState;

  /* Set relay retransmit count. */
  MESH_UTILS_BF_SET(pBuf[CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_OFFSET], pRetranState->retransCount,
                    CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_SHIFT,
                    CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_SIZE);

  /* Set relay retransmit interval steps. */
  MESH_UTILS_BF_SET(pBuf[CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_OFFSET], pRetranState->retransIntervalSteps10Ms,
                    CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_SHIFT,
                    CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_SIZE);
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Relay Composite State.
 *
 *  \param[in]  pBuf          Pointer to buffer where the packed state is stored.
 *  \param[out] pRelayState   Pointer to memory where to store a Relay State.
 *  \param[out] pRetranState  Pointer to memory where to store a Relay Retransmit State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackRelay(uint8_t *pBuf, meshRelayStates_t *pRelayState,
                           meshRelayRetransState_t *pRetranState)
{
  /* Get relay state. */
  *pRelayState = pBuf[CFG_MDL_MSG_RELAY_COMP_STATE_RELAY_OFFSET];

  /* Get relay retransmit count. */
  pRetranState->retransCount = MESH_UTILS_BF_GET(pBuf[CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_OFFSET],
                                                 CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_SHIFT,
                                                 CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_SIZE);

  /* Get relay retransmit interval steps. */
  pRetranState->retransIntervalSteps10Ms =
    MESH_UTILS_BF_GET(pBuf[CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_OFFSET],
                      CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_SHIFT,
                      CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_SIZE);
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Model Publication Get message.
 *
 *  \param[out] pBuf      Pointer to buffer where to store the packed message.
 *  \param[in]  elemAddr  Address of element.
 *  \param[in]  sigId     SIG model identifier.
 *  \param[in]  vendorId  Vendor model identifier.
 *  \param[in]  isSig     TRUE if SIG model identifier is used, FALSE for vendorId.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackModelPubGet(uint8_t *pBuf, meshAddress_t elemAddr,
                               meshSigModelId_t sigId, meshVendorModelId_t vendorId,
                               bool_t isSig)
{
  uint8_t *pOffset = pBuf;

  /* Set element address. */
  UINT16_TO_BSTREAM(pOffset, elemAddr);

  /* Set model identifier. */
  if (isSig)
  {
    UINT16_TO_BSTREAM(pOffset, sigId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(pOffset, vendorId);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Model Publication Get message.
 *
 *  \param[in]  pBuf       Pointer to buffer where to store the packed message.
 *  \param[out] pElemAddr  Pointer address of element.
 *  \param[out] pSigId     Pointer to SIG model identifier.
 *  \param[out] pVendorId  Pointer to vendor model identifier.
 *  \param[in]  isSig      TRUE if SIG model identifier is unpacked, FALSE for vendorId.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackModelPubGet(uint8_t *pBuf, meshAddress_t *pElemAddr,
                                 meshSigModelId_t *pSigId, meshVendorModelId_t *pVendorId,
                                 bool_t isSig)
{
  uint8_t *pOffset = pBuf;

  /* Get element address. */
  BSTREAM_TO_UINT16(*pElemAddr, pOffset);

  /* Get model identifier. */
  if (isSig)
  {
    BSTREAM_TO_UINT16(*pSigId, pOffset);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(*pVendorId, pOffset);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Model Publication parameters used for Set and Status.
 *
 *  \param[out] pBuf      Pointer to buffer where to store the packed message.
 *  \param[in]  pParams   Pointer to Model Publication parameters.
 *  \param[in]  sigId     SIG model identifier.
 *  \param[in]  vendorId  Vendor model identifier.
 *  \param[in]  isSig     TRUE if SIG model identifier is used, FALSE for vendorId.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackModelPubParam(uint8_t *pBuf, meshModelPublicationParams_t *pParams,
                                 meshSigModelId_t sigId, meshVendorModelId_t vendorId,
                                 bool_t isSig)
{
  uint8_t *pOffset = pBuf;
  uint8_t tempByte = 0;

  /* Pack AppKey Index. */
  meshCfgMsgPackSingleKeyIndex(pOffset, pParams->publishAppKeyIndex);

  pOffset++;

  /* Clear RFU bits. */
  MESH_UTILS_BITMASK_CLR(*pOffset, CFG_MDL_MSG_MODEL_PUB_RFU_MASK);

  /* Set Credential Flag. */
  MESH_UTILS_BF_SET(*pOffset, pParams->publishFriendshipCred,
                    CFG_MDL_MSG_MODEL_PUB_FRIEND_CRED_SHIFT, CFG_MDL_MSG_MODEL_PUB_FRIEND_CRED_SIZE);

  pOffset++;

  /* Set Publish TTL. */
  UINT8_TO_BSTREAM(pOffset, pParams->publishTtl);

  /* Pack Publish Period. */
  MESH_UTILS_BF_SET(tempByte, pParams->publishPeriodNumSteps,
                    CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_SHIFT,
                    CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_SIZE);
  MESH_UTILS_BF_SET(tempByte, pParams->publishPeriodStepRes,
                    CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_SHIFT,
                    CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_SIZE);

  /* Set Publish Period. */
  UINT8_TO_BSTREAM(pOffset, tempByte);

  /* Pack Publish Retransmit states. */
  MESH_UTILS_BF_SET(tempByte, pParams->publishRetransCount,
                    CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_SHIFT,
                    CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_SIZE);
  MESH_UTILS_BF_SET(tempByte, pParams->publishRetransSteps50Ms,
                    CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_SHIFT,
                    CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_SIZE);

  /* Set Publish Retransmit states. */
  UINT8_TO_BSTREAM(pOffset, tempByte);

  /* Set model identifier. */
  if (isSig)
  {
    UINT16_TO_BSTREAM(pOffset, sigId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(pOffset, vendorId);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Model Publication parameters used for Set and Status.
 *
 *  \param[in]  pBuf       Pointer to buffer where to store the packed message.
 *  \param[out] pParam     Pointer to Model Publication parameters.
 *  \param[out] pSigId     Pointer to SIG model identifier.
 *  \param[out] pVendorId  Pointer to vendor model identifier.
 *  \param[in]  isSig      TRUE if SIG model identifier is unpacked, FALSE for vendorId.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackModelPubParam(uint8_t *pBuf, meshModelPublicationParams_t *pParams,
                                   meshSigModelId_t *pSigId, meshVendorModelId_t *pVendorId,
                                   bool_t isSig)
{
  uint8_t *pOffset = pBuf;
  uint8_t tempByte = 0;

  /* Get AppKeyIndex. */
  meshCfgMsgUnpackSingleKeyIndex(pOffset++, &(pParams->publishAppKeyIndex));

  /* Get second byte of the AppKey Index containing RFU and publish credentials flag */
  BSTREAM_TO_UINT8(tempByte, pOffset);

  /* Get Credential Flag. */
  pParams->publishFriendshipCred = MESH_UTILS_BF_GET(tempByte, CFG_MDL_MSG_MODEL_PUB_FRIEND_CRED_SHIFT,
                                                     CFG_MDL_MSG_MODEL_PUB_FRIEND_CRED_SIZE);

  /* Get Publish TTL. */
  BSTREAM_TO_UINT8(pParams->publishTtl, pOffset);

  /* Get Publish Period. */
  BSTREAM_TO_UINT8(tempByte, pOffset);

  /* Extract number of steps and step resolution. */
  pParams->publishPeriodNumSteps = MESH_UTILS_BF_GET(tempByte,
                                                     CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_SHIFT,
                                                     CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_SIZE);
  pParams->publishPeriodStepRes = MESH_UTILS_BF_GET(tempByte,
                                                    CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_SHIFT,
                                                    CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_SIZE);

  /* Get Publish Retransmit count and intervals steps. */
  BSTREAM_TO_UINT8(tempByte, pOffset);

  /* Extract count and interval steps. */
  pParams->publishRetransCount = MESH_UTILS_BF_GET(tempByte,
                                                   CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_SHIFT,
                                                   CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_SIZE);
  pParams->publishRetransSteps50Ms = MESH_UTILS_BF_GET(tempByte,
                                                       CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_SHIFT,
                                                       CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_SIZE);

  /* Get model identifier. */
  if (isSig)
  {
    BSTREAM_TO_UINT16(*pSigId, pOffset);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(*pVendorId, pOffset);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Model AppKey Index binding.
 *
 *  \param[out] pBuf           Pointer to buffer where to store the packed state.
 *  \param[in]  elemAddr       Element address.
 *  \param[in]  appKeyIndex    Global AppKey identifier.
 *  \param[in]  sigModelId     SIG model identifier.
 *  \param[in]  vendorModelId  Vendor model identifier.
 *  \param[in]  isSig          TRUE means sigModelId should be used, FALSE - vendorModelId.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackModelAppBind(uint8_t *pBuf, meshAddress_t elemAddr,
                                uint16_t appKeyIndex, meshSigModelId_t sigModelId,
                                meshVendorModelId_t vendorModelId, bool_t isSig)
{
  uint8_t *ptr = pBuf;

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, elemAddr);
  /* Pack AppKey Index. */
  ptr += meshCfgMsgPackSingleKeyIndex(ptr, appKeyIndex);

  /* Pack model identifier. */
  if (isSig)
  {
    UINT16_TO_BSTREAM(ptr, sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(ptr, vendorModelId);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the the Model AppKey Index binding.
 *
 *  \param[in]  pBuf            Pointer to buffer where the packed state is stored.
 *  \param[out] pElemAddr       Element address.
 *  \param[out] pAppKeyIndex    Global AppKey identifier.
 *  \param[out] pSigModelId     SIG model identifier.
 *  \param[out] pVendorModelId  Vendor model identifier.
 *  \param[in]  isSig           TRUE if SIG model identifier should be unpacked, FALSE otherwise
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackModelAppBind(uint8_t *pBuf, meshAddress_t *pElemAddr,
                                  uint16_t *pAppKeyIndex, meshSigModelId_t *pSigModelId,
                                  meshVendorModelId_t *pVendorModelId, bool_t isSig)
{
  uint8_t *ptr = pBuf;

  /* Unpack element address. */
  BSTREAM_TO_UINT16(*pElemAddr, ptr);

  /* Unpack AppKeyIndex. */
  ptr += meshCfgMsgUnpackSingleKeyIndex(ptr, pAppKeyIndex);

  /* Unpack model identifier. */
  if (isSig)
  {
    BSTREAM_TO_UINT16(*pSigModelId, ptr);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(*pVendorModelId, ptr);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Heartbeat Publication State.
 *
 *  \param[out] pBuf    Pointer to buffer where to store the packed state.
 *  \param[in]  pHbPub  Pointer to a Heartbeart Publication State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackHbPub(uint8_t *pBuf, meshHbPub_t *pHbPub)
{
  uint8_t *ptr = pBuf;

  /* Pack Heartbeat Publication data. */
  UINT16_TO_BSTREAM(ptr, pHbPub->dstAddr);
  UINT8_TO_BSTREAM(ptr, pHbPub->countLog);
  UINT8_TO_BSTREAM(ptr, pHbPub->periodLog);
  UINT8_TO_BSTREAM(ptr, pHbPub->ttl);
  UINT16_TO_BSTREAM(ptr, pHbPub->features);
  /* NetKeyIndex field shall be encoded as defined in Section 4.3.1.1. */
  meshCfgMsgPackSingleKeyIndex(ptr, pHbPub->netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Heartbeat Publication State.
 *
 *  \param[in]  pBuf    Pointer to buffer where the packed state is stored.
 *  \param[out] pHbPub  Pointer to memory where to store a Heartbeat Publication State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackHbPub(uint8_t *pBuf, meshHbPub_t *pHbPub)
{
  uint8_t *ptr = pBuf;
  /* Get Heartbeat Publication data. */
  BSTREAM_TO_UINT16(pHbPub->dstAddr, ptr);
  BSTREAM_TO_UINT8(pHbPub->countLog, ptr);
  BSTREAM_TO_UINT8(pHbPub->periodLog, ptr);
  BSTREAM_TO_UINT8(pHbPub->ttl, ptr);
  BSTREAM_TO_UINT16(pHbPub->features, ptr);
  /* NetKeyIndex field shall be encoded as defined in Section 4.3.1.1. */
  meshCfgMsgUnpackSingleKeyIndex(ptr, &(pHbPub->netKeyIndex));
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Heartbeat Subscription Set message.
 *
 *  \param[out] pBuf    Pointer to buffer where to store the packed state.
 *  \param[in]  pHbSub  Pointer to a Heartbeat Subscription State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackHbSubSet(uint8_t *pBuf, meshHbSub_t *pHbSub)
{
  uint8_t *ptr = pBuf;

  /* Set Heartbeat Subscription data. */
  UINT16_TO_BSTREAM(ptr, pHbSub->srcAddr);
  UINT16_TO_BSTREAM(ptr, pHbSub->dstAddr);
  UINT8_TO_BSTREAM(ptr, pHbSub->periodLog);
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Heartbeat Subcription State.
 *
 *  \param[in]  pBuf    Pointer to buffer where the packed state is stored.
 *  \param[out] pHbSub  Pointer to memory where to store a Heartbeart Subscription State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackHbSubSet(uint8_t *pBuf, meshHbSub_t *pHbSub)
{
  uint8_t *ptr = pBuf;

  /* Set Heartbeat Subscription Set data. */
  BSTREAM_TO_UINT16(pHbSub->srcAddr, ptr);
  BSTREAM_TO_UINT16(pHbSub->dstAddr, ptr);
  BSTREAM_TO_UINT8(pHbSub->periodLog, ptr);
}

/*************************************************************************************************/
/*!
 *  \brief      Pack the Heartbeat Subcription State Status.
 *
 *  \param[out] pBuf    Pointer to buffer where to store the packed state.
 *  \param[in]  pHbSub  Pointer to a Heartbeart Subscription State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackHbSubState(uint8_t *pBuf, meshHbSub_t *pHbSub)
{
  uint8_t *ptr = pBuf;

  /* Set Heartbeat Subscription data. */
  UINT16_TO_BSTREAM(ptr, pHbSub->srcAddr);
  UINT16_TO_BSTREAM(ptr, pHbSub->dstAddr);
  UINT8_TO_BSTREAM(ptr, pHbSub->periodLog);
  UINT8_TO_BSTREAM(ptr, pHbSub->countLog);
  UINT8_TO_BSTREAM(ptr, pHbSub->minHops);
  UINT8_TO_BSTREAM(ptr, pHbSub->maxHops);
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Heartbeat Subcription State Status.
 *
 *  \param[in]  pBuf    Pointer to buffer where the packed state is stored.
 *  \param[out] pHbSub  Pointer to memory where to store a Heartbeart Subscription State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackHbSubState(uint8_t *pBuf, meshHbSub_t *pHbSub)
{
  uint8_t *ptr = pBuf;

  /* Set Heartbeat Subscription data. */
  BSTREAM_TO_UINT16(pHbSub->srcAddr, ptr);
  BSTREAM_TO_UINT16(pHbSub->dstAddr, ptr);
  BSTREAM_TO_UINT8(pHbSub->periodLog, ptr);
  BSTREAM_TO_UINT8(pHbSub->countLog, ptr);
  BSTREAM_TO_UINT8(pHbSub->minHops, ptr);
  BSTREAM_TO_UINT8(pHbSub->maxHops, ptr);
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the LPN PollTimeout State.
 *
 *  \param[out] pBuf  Pointer to buffer where to store the packed state.
 *  \param[in]  addr  LPN address.
 *  \param[in]  time  PollTimeout timer value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackLpnPollTimeout(uint8_t *pBuf, meshAddress_t addr, uint32_t time)
{
  uint8_t *ptr = pBuf;

  /* Pack address. */
  UINT16_TO_BSTREAM(ptr, addr);

  /* Pack timer. */
  UINT24_TO_BSTREAM(ptr, time);
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the LPN PollTimeout State.
 *
 *  \param[in]  pBuf   Pointer to buffer where the packed state is stored.
 *  \param[out] pAddr  Pointer to memory where to store the LPN address.
 *  \param[out] pTime  Pointer to memory where to store the PollTimeout timer value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackLpnPollTimeout(uint8_t *pBuf, meshAddress_t *pAddr, uint32_t *pTime)
{
  uint8_t *ptr = pBuf;

  /* Pack address. */
  BSTREAM_TO_UINT16(*pAddr, ptr);

  /* Pack timer. */
  BSTREAM_TO_UINT24(*pTime, ptr);
}

/*************************************************************************************************/
/*!
 *  \brief      Packs the Network Transmit State.
 *
 *  \param[out] pBuf    Pointer to buffer where to store the packed state.
 *  \param[in]  pState  Pointer to a Network Transmit State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgPackNwkTrans(uint8_t *pBuf, meshNwkTransState_t *pState)
{

  /* Set network transmit count. */
  MESH_UTILS_BF_SET(pBuf[0], pState->transCount,
                    CFG_MDL_MSG_NWK_TRANS_STATE_CNT_SHIFT,
                    CFG_MDL_MSG_NWK_TRANS_STATE_CNT_SIZE);

  /* Set network transmit interval steps. */
  MESH_UTILS_BF_SET(pBuf[0], pState->transIntervalSteps10Ms,
                    CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_SHIFT,
                    CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_SIZE);
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks the Network Transmit State.
 *
 *  \param[in]  pBuf    Pointer to buffer where the packed state is stored.
 *  \param[out] pState  Pointer to memory where to store a Network Transmit State.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshCfgMsgUnpackNwkTrans(uint8_t *pBuf, meshNwkTransState_t *pState)
{
  /* Get network transmit count. */
  pState->transCount = MESH_UTILS_BF_GET(pBuf[0],
                                         CFG_MDL_MSG_NWK_TRANS_STATE_CNT_SHIFT,
                                         CFG_MDL_MSG_NWK_TRANS_STATE_CNT_SIZE);

  /* Get network transmit interval steps. */
  pState->transIntervalSteps10Ms =  MESH_UTILS_BF_GET(pBuf[0],
                                                      CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_SHIFT,
                                                      CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_SIZE);
}
