/*************************************************************************************************/
/*!
 *  \file   mesh_security_crypto_utr.c
 *
 *  \brief  Security implementation for Upper Transport.
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
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"

#include "mesh_defs.h"
#include "mesh_security_defs.h"

#include "mesh_types.h"
#include "mesh_error_codes.h"

#include "mesh_utils.h"
#include "mesh_api.h"
#include "mesh_seq_manager.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_security_toolbox.h"

#include "mesh_security.h"
#include "mesh_security_main.h"
#include "mesh_security_crypto.h"

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static bool_t meshSecUtrDecSetNextLabelUuid(meshSecUtrDecReq_t *pReq);
static meshSecRetVal_t meshSecUtrDecSetNextAppKey(meshSecUtrDecReq_t *pReq);

/*************************************************************************************************/
/*!
 *  \brief     Upper transport encryption complete toolbox callback.
 *
 *  \param[in] pCcmResult  Pointer to CCM result.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecUtrEncCcmCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
{
  /* Recover request pointer */
  meshSecUtrEncReq_t       *pReq;
  meshSecUtrEncryptCback_t cback;

  /* Recover request. */
  pReq = (meshSecUtrEncReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Store callback. */
  cback = pReq->cback;

  /* Clear callback to make request available. */
  pReq->cback = NULL;

  /* Check if toolbox was successful. */
  if (pCcmResult != NULL)
  {
    /* This should never fail. */
    WSF_ASSERT(pCcmResult->op == MESH_SEC_TOOL_CCM_ENCRYPT);
  }

  /* Invoke user callback with either success or error code. */
  cback(pCcmResult != NULL, pReq->pEncUtrPdu, pReq->encUtrPduSize, pReq->pTransMic,
        pReq->transMicSize, pReq->aid, pReq->pParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Upper transport decryption complete toolbox callback for Application Keys.
 *
 *  \param[in] pCcmResult  Pointer to CCM result.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecUtrAppDecCcmCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
{
  /* Recover request pointer. */
  meshSecUtrDecReq_t *pReq;
  meshSecUtrDecryptCback_t cback;
  const meshSecToolCcmDecryptResult_t *pResult;
  bool_t resumeDec = FALSE;

  /* Recover request*/
  pReq = (meshSecUtrDecReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Set user callback. */
  cback = pReq->cback;

  /* Check if toolbox failed. */
  if (pCcmResult != NULL)
  {
    /* This should never fail. */
    WSF_ASSERT(pCcmResult->op == MESH_SEC_TOOL_CCM_DECRYPT);

    pResult = &(pCcmResult->results.decryptResult);

    /* Check if authentication is successful. */
    if (pResult->isAuthSuccess)
    {
      /* Clear callback to make request available. */
      pReq->cback = NULL;

      /* Invoke user callback. */
      cback(TRUE, pResult->pPlainText,
            (MESH_IS_ADDR_VIRTUAL(pReq->vtad) ? pReq->ccmParams.pAuthData : NULL),
            pResult->plainTextSize, pReq->appKeyIndex, pReq->netKeyIndex, pReq->pParam);

      return;
    }
    /* Else. */
    /* Authentication failed so move on to next request. */
    if (MESH_IS_ADDR_VIRTUAL(pReq->vtad))
    {
      /* Search for the next virtual address. */
      if (meshSecUtrDecSetNextLabelUuid(pReq))
      {
        /* New matching virtual address found so a new request can be triggered. */
        resumeDec = TRUE;
      }
      else
      {
        /* Reset search index for virtual addresses. */
        pReq->vtadSearchIdx = 0;

        /* Move to next Application Key. */
        if((meshSecUtrDecSetNextAppKey(pReq) == MESH_SUCCESS) &&
           (meshSecUtrDecSetNextLabelUuid(pReq)))
        {
          resumeDec = TRUE;
        }
      }
    }
    else
    {
      /* Move to next Application Key. */
      if(meshSecUtrDecSetNextAppKey(pReq) == MESH_SUCCESS)
      {
        resumeDec = TRUE;
      }
    }
  }

  /* Check if a new attempt should be started. */
  if (resumeDec)
  {
    /* Trigger new request to Toolbox. */
    if(MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_DECRYPT, &(pReq->ccmParams),
                                    meshSecUtrAppDecCcmCback, (void *)pReq)
       != MESH_SUCCESS)
    {
      /* Mark operation as complete with errors. */
      resumeDec = FALSE;
    }
  }

  if(!resumeDec)
  {
    /* Clear callback to make request available. */
    pReq->cback = NULL;

    /* Invoke user callback to signal error. */
    cback(FALSE, pReq->ccmParams.pOut, pReq->ccmParams.pAuthData, pReq->ccmParams.inputLen,
          MESH_SEC_INVALID_KEY_INDEX, pReq->netKeyIndex, pReq->pParam);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Upper transport decryption complete toolbox callback for Device Key.
 *
 *  \param[in] pCcmResult  Pointer to CCM result.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecUtrDevDecCcmCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
{
  /* Recover request pointer. */
  meshSecUtrDecReq_t *pReq;
  meshSecUtrDecryptCback_t cback;
  const meshSecToolCcmDecryptResult_t *pResult;
  bool_t decFail = FALSE;

  /* Recover request. */
  pReq = (meshSecUtrDecReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Store user callback. */
  cback = pReq->cback;

  /* Check if toolbox failed. */
  if (pCcmResult != NULL)
  {
    /* This should never fail. */
    WSF_ASSERT(pCcmResult->op == MESH_SEC_TOOL_CCM_DECRYPT);

    /* Extract decrypt result. */
    pResult = &(pCcmResult->results.decryptResult);

    /* Check if decryption was successful. If TRUE, execution stops after callback invocation. */
    if (pResult->isAuthSuccess)
    {
      /* Clear callback to make request available. */
      pReq->cback = NULL;

      /* Invoke user callback. */
      cback(TRUE, pResult->pPlainText, NULL, pResult->plainTextSize,
            (pReq->keySearchIdx == 0 ?
            MESH_APPKEY_INDEX_LOCAL_DEV_KEY : MESH_APPKEY_INDEX_REMOTE_DEV_KEY),
            pReq->netKeyIndex, pReq->pParam);
      return;
    }

    /* Else. */
    /* Check if decryption attempt was performed with the local Device Key. */
    if (pReq->keySearchIdx == 0)
    {
      /* Try to read the Device Key of the node. */
      if ((meshSecCb.secRemoteDevKeyReader != NULL) &&
          (meshSecCb.secRemoteDevKeyReader(((pReq->nonce[MESH_SEC_NONCE_SRC_POS] << 8) |
                                        (pReq->nonce[MESH_SEC_NONCE_SRC_POS + 1])), pReq->key)))
      {

        /* Mark second decrypt attempt which uses remote Device Key. */
        ++(pReq->keySearchIdx);

        /* Request toolbox to attempt decryption using remote Device Key. */
        if (MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_DECRYPT, &(pReq->ccmParams),
                                         meshSecUtrDevDecCcmCback, pParam)
            != MESH_SUCCESS)
        {
          /* Decrypt request failed for the second key. */
          decFail = TRUE;
        }
      }
      else
      {
        /* Remote Device Key cannot be read. */
        decFail = TRUE;
      }
    }
    else
    {
      /* Authentication failed for the remote Device Key as well. */
      decFail = TRUE;
    }
  }
  else
  {
    /* Toolbox decrypt failed. */
    decFail = TRUE;
  }

  if (decFail)
  {
    /* Clear callback to make request available. */
    pReq->cback = NULL;

    /* Invoke user callback to signal error. */
    cback(FALSE, pReq->ccmParams.pOut, NULL, pReq->ccmParams.inputLen,
          (pReq->keySearchIdx == 0 ?
          MESH_APPKEY_INDEX_LOCAL_DEV_KEY : MESH_APPKEY_INDEX_REMOTE_DEV_KEY),
          pReq->netKeyIndex, pReq->pParam);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the next Application Key into the decrypt request structure based on matching
 *             AID and key refresh state of the bound Network Key.
 *
 *  \param[in] pReq  Pointer to the active decrypt request.
 *
 *  \return    MESH_SUCCESS if another key was successfully loaded, error code otherwise.
 *             See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecUtrDecSetNextAppKey(meshSecUtrDecReq_t *pReq)
{
  meshSecAppKeyInfo_t    *pAppKeyInfo = NULL;
  meshKeyRefreshStates_t state;
  uint16_t               keyIdx;
  uint8_t                entryId;

  /* Get key refresh state of the Network Key. */
  state = MeshLocalCfgGetKeyRefreshPhaseState(pReq->netKeyIndex);

  /* Loop through application key material. */
  for (; pReq->keySearchIdx < MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.appKeyInfoListSize;
       pReq->keySearchIdx++)
  {
    /* Extract entry identifier for the material. */
    entryId = (pReq->keySearchIdx) & (MESH_SEC_KEY_MAT_PER_INDEX - 1);
    /* Extract index of the key info list. */
    keyIdx = (pReq->keySearchIdx) >> (MESH_SEC_KEY_MAT_PER_INDEX - 1);
    /* Get key information. */
    pAppKeyInfo = &(secMatLocals.pAppKeyInfoArray[keyIdx]);

    /* Check if there is no material available. */
    if (!(pAppKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      continue;
    }

    /* Check if a binding does not exist */
    if (!MeshLocalCfgValidateNetToAppKeyBind(pReq->netKeyIndex, pAppKeyInfo->hdr.keyIndex))
    {
      continue;
    }

    /* Check if updated material should be searched and if it exists. */
    if (entryId != pAppKeyInfo->hdr.crtKeyId)
    {
      if (!(pAppKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE))
      {
        continue;
      }
    }

    /* Check if AID does not match. */
    if (pReq->aid != pAppKeyInfo->keyMaterial[entryId].aid)
    {
      continue;
    }

    /* If updated key exists, follow key refresh rules. */
    if (pAppKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
    {
      /* If entryId is index of the current(old) key material. */
      if (entryId == pAppKeyInfo->hdr.crtKeyId)
      {
        /* Key refresh phase 3 does not allow use of old key. */
        if (state == MESH_KEY_REFRESH_THIRD_PHASE)
        {
          continue;
        }
      }
      else
      {
        /* Key refresh phases 1,2 and 3 allow use of new key. */
        if ((state < MESH_KEY_REFRESH_FIRST_PHASE) || (state > MESH_KEY_REFRESH_THIRD_PHASE))
        {
          continue;
        }
      }
    }

    /* Read Application Key. */
    if (pAppKeyInfo->hdr.crtKeyId != entryId)
    {
      if (MeshLocalCfgGetUpdatedAppKey(pAppKeyInfo->hdr.keyIndex, pReq->key) != MESH_SUCCESS)
      {
        WSF_ASSERT(MeshLocalCfgGetUpdatedAppKey(pAppKeyInfo->hdr.keyIndex, pReq->key)
                   == MESH_SUCCESS);
        continue;
      }
    }
    else
    {
      if (MeshLocalCfgGetAppKey(pAppKeyInfo->hdr.keyIndex, pReq->key) != MESH_SUCCESS)
      {
        WSF_ASSERT(MeshLocalCfgGetAppKey(pAppKeyInfo->hdr.keyIndex, pReq->key) == MESH_SUCCESS);
        continue;
      }
    }
    /* If code execution reaches this, it means that AID matched a valid entry of the AppKey info
     * array.
     */
    break;
  }

  /* Check no Key was found with matching AID and NetKey bind. */
  if (pReq->keySearchIdx == (MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.appKeyInfoListSize))
  {
    return MESH_SEC_KEY_NOT_FOUND;
  }

  /* Set AppKey Index. */
  pReq->appKeyIndex = pAppKeyInfo->hdr.keyIndex;

  /* Increment key search index for the following requests. */
  ++(pReq->keySearchIdx);

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the next Label UUID as authentication data to the Upper Transport decrypt
 *             request that matches the virtual address in the request.
 *
 *  \param[in] pReq  Pointer to the active decrypt request.
 *
 *  \return    TRUE if another Label UUID is set as authentication data, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshSecUtrDecSetNextLabelUuid(meshSecUtrDecReq_t *pReq)
{
  meshLocalCfgVirtualAddrListInfo_t *pVtadList = NULL;

  /* Get pointer to VTAD Table. */
  MeshLocalCfgGetVtadList((const meshLocalCfgVirtualAddrListInfo_t **) &pVtadList);

  for (; pReq->vtadSearchIdx < pVtadList->virtualAddrListSize; pReq->vtadSearchIdx++)
  {
    if (pVtadList->pVirtualAddrList[pReq->vtadSearchIdx].address == pReq->vtad)
    {
      break;
    }
  }

  /* Check if no result is found. */
  if (pReq->vtadSearchIdx == pVtadList->virtualAddrListSize)
  {
    return FALSE;
  }

  /* Populate authentication data parameters for CCM. */
  pReq->ccmParams.pAuthData   = pVtadList->pVirtualAddrList[pReq->vtadSearchIdx].labelUuid;
  pReq->ccmParams.authDataLen = MESH_LABEL_UUID_SIZE;

  /* Increment search index so it continues from the next entry. */
  ++(pReq->vtadSearchIdx);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Internal security handler for decrypting Upper Transport PDU's.
 *
 *  \param[in] pReq      Pointer to Security Upper Transport decrypt request.
 *  \param[in] ccmCback  Callback to be invoked by the Toolbox CCM decryption.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecInternalUtrDecrypt(meshSecUtrDecReq_t *pReq,
                                                 meshSecToolCcmCback_t ccmCback)
{
  meshSecRetVal_t retVal = MESH_SUCCESS;

  /* Handle Device Key based decryption. */
  if (pReq->aid == MESH_SEC_DEVICE_KEY_AID)
  {
    /* Read Device Key. */
    MeshLocalCfgGetDevKey(pReq->key);

    /* Set authentication data to NULL. */
    pReq->ccmParams.pAuthData   = NULL;
    pReq->ccmParams.authDataLen = 0;
  }
  else
  {
    /* Read first matching Application Key. */
    retVal = meshSecUtrDecSetNextAppKey(pReq);

    if (retVal == MESH_SUCCESS)
    {
      /* Check if destination address is virtual. */
      if (MESH_IS_ADDR_VIRTUAL(pReq->vtad))
      {
        /* Search for the next label UUID with same address and set it as authentication data. */
        retVal = meshSecUtrDecSetNextLabelUuid(pReq) ? MESH_SUCCESS : MESH_SEC_INVALID_PARAMS;
      }
      else
      {
        /* No authentication data needed for non-virtual addresses. */
        pReq->ccmParams.pAuthData   = NULL;
        pReq->ccmParams.authDataLen = 0;
      }
    }
  }

  /* Check if decryption configuration failed. */
  if (retVal != MESH_SUCCESS)
  {
    /* Set request callback to NULL to signal module is not busy decrypting UTR PDU's. */
    pReq->cback = NULL;

    return retVal;
  }

  /* Trigger request to toolbox. */
  retVal = (meshSecRetVal_t)MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_DECRYPT,
                                                         &(pReq->ccmParams),
                                                         ccmCback,
                                                         (void *)pReq);

  /* Validate toolbox return value. */
  if (retVal != MESH_SUCCESS)
  {
    /* Set request callback to NULL to signal module is not busy decrypting UTR PDU's. */
    pReq->cback = NULL;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets pointer to Application Key information and entry in material table.
 *
 *  \param[in]  netKeyIndex   Global identifier of the Network Key bound to the Application Key.
 *  \param[in]  appKeyIndex   Global identifier of the Application Key.
 *  \param[out] ppAppKeyInfo  Pointer to the address of the key information.
 *  \param[out] pOutEntryIdx  Pointer to variable storing the material to be used based on
 *                            key refresh phase of the Network Key and key availability.
 *
 *  \return     Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecUtrEncKeyIndexToAppInfo(uint16_t netKeyIndex, uint16_t appKeyIndex,
                                                      meshSecAppKeyInfo_t **ppAppKeyInfo,
                                                      uint8_t *pOutEntryIdx)
{
  meshSecAppKeyInfo_t    *pKeyInfo  = NULL;
  uint8_t                entryId    = MESH_SEC_KEY_MAT_PER_INDEX;
  meshKeyRefreshStates_t state;
  uint16_t               idx;

  /* Check if the Application key is bound to the Network Key. */
  if (!MeshLocalCfgValidateNetToAppKeyBind(netKeyIndex, appKeyIndex))
  {
    return MESH_SEC_KEY_NOT_FOUND;
  }

  /* Read Key refresh state. */
  state = MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex);

  /* Search for material with matching AppKey Index. */
  for (idx = 0; idx < secMatLocals.appKeyInfoListSize; idx++)
  {
    if ((secMatLocals.pAppKeyInfoArray[idx].hdr.keyIndex == appKeyIndex) &&
        (secMatLocals.pAppKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      pKeyInfo = &(secMatLocals.pAppKeyInfoArray[idx]);
      break;
    }
  }

  if (idx == secMatLocals.appKeyInfoListSize || pKeyInfo == NULL)
  {
    return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
  }

  /* If updated key exists, comply to Key Refresh rules. */
  if (pKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
  {
    /* Decide which entry in the key material to use based on state. */
    switch (state)
    {
      case MESH_KEY_REFRESH_NOT_ACTIVE:
      case MESH_KEY_REFRESH_FIRST_PHASE:
        entryId = pKeyInfo->hdr.crtKeyId;
        break;
      case MESH_KEY_REFRESH_SECOND_PHASE:
      case MESH_KEY_REFRESH_THIRD_PHASE:
        entryId = 1 - pKeyInfo->hdr.crtKeyId;
        break;
      default:
        break;
    }
  }
  else
  {
    entryId = pKeyInfo->hdr.crtKeyId;
  }

  *ppAppKeyInfo = pKeyInfo;
  *pOutEntryIdx = entryId;

  return MESH_SUCCESS;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Encrypts upper transport PDU.
 *
 *  \param[in] pReqParams               Pointer to encryption setup and storage parameters.
 *  \param[in] utrEncryptCompleteCback  Callback used to signal encryption complete.
 *  \param[in] pParam                   Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Encryption starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: Mic size invalid, or
 *                                           Label UUID NULL for virtual destination address).
 *  \retval MESH_SEC_KEY_NOT_FOUND           Application key index not found.
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no AID computed for the application key index.
 *                                           See the remarks section for more information.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \remarks The AID can be present for the old key in a Key Refresh Procedure phase,
 *           but not for the new key and the phase dictates the new key AID should be returned.
 *
 *  \see meshSecUtrEncryptParams_t
 *  \see meshSecUtrEncryptCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecUtrEncrypt(meshSecUtrEncryptParams_t *pReqParams,
                                  meshSecUtrEncryptCback_t utrEncryptCompleteCback, void *pParam)
{
  meshSecAppKeyInfo_t    *pKeyInfo        = NULL;
  meshSecToolCcmParams_t ccmParams;
  uint32_t               ivIndex          = 0;
  bool_t                 ivUpdtInProgress = FALSE;
  meshSecRetVal_t        retVal           = MESH_SUCCESS;
  uint8_t                matEntryIdx      = 0;

  /* Check if another UTR encryption is not in progress. */
  if (secCryptoReq.utrEncReq.cback != NULL)
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Validate parameters. Security should not care about addresses in general. Just virtual
   * destination so that can validate presence of Label UUID.
   */
  if ((pReqParams == NULL) ||
      (utrEncryptCompleteCback == NULL) ||
      (pReqParams->netKeyIndex > MESH_SEC_MAX_KEY_INDEX) ||
      (pReqParams->seqNo > MESH_SEQ_MAX_VAL) ||
      (!MESH_IS_ADDR_UNICAST(pReqParams->srcAddr)) ||
      (pReqParams->pAppPayload == NULL) ||
      (pReqParams->appPayloadSize == 0) ||
      (pReqParams->pEncAppPayload == NULL) ||
      (pReqParams->pTransMic == NULL) ||
      ((pReqParams->transMicSize != MESH_SEC_MIC_SIZE_32) &&
       (pReqParams->transMicSize != MESH_SEC_MIC_SIZE_64)) ||
      ((MESH_IS_ADDR_VIRTUAL(pReqParams->dstAddr)) && (pReqParams->pLabelUuid == NULL)))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Decide what key should be used. */
  if ((pReqParams->appKeyIndex == MESH_APPKEY_INDEX_LOCAL_DEV_KEY) ||
      (pReqParams->appKeyIndex == MESH_APPKEY_INDEX_REMOTE_DEV_KEY))
  {
    /* Set invalid aid. */
    secCryptoReq.utrEncReq.aid = MESH_SEC_DEVICE_KEY_AID;

    /* Check if local Device Key must be read. */
    if (pReqParams->appKeyIndex == MESH_APPKEY_INDEX_LOCAL_DEV_KEY)
    {
      /* Read Device Key. */
      MeshLocalCfgGetDevKey(secCryptoReq.utrEncReq.key);
    }
    else
    {
      /* Check if remote Device Key can be read. */
      if ((meshSecCb.secRemoteDevKeyReader == NULL) ||
          (!meshSecCb.secRemoteDevKeyReader(pReqParams->dstAddr, secCryptoReq.utrEncReq.key)))
      {
        return MESH_SEC_INVALID_PARAMS;
      }
    }
  }
  else
  {
    /* Check if the non-reserved AppKey Index is valid. */
    if (pReqParams->appKeyIndex > MESH_SEC_MAX_KEY_INDEX)
    {
      return MESH_SEC_INVALID_PARAMS;
    }

    /* Search for the key material based on binding and index. */
    retVal = meshSecUtrEncKeyIndexToAppInfo(pReqParams->netKeyIndex, pReqParams->appKeyIndex,
                                            &pKeyInfo, &matEntryIdx);
    /* Handle error. */
    if (retVal != MESH_SUCCESS)
    {
      return retVal;
    }

    /* Store AID. */
    secCryptoReq.utrEncReq.aid = pKeyInfo->keyMaterial[matEntryIdx].aid;

    /* Read old or new Key based on key refresh state. */
    if (pKeyInfo->hdr.crtKeyId != matEntryIdx)
    {
      retVal = MeshLocalCfgGetUpdatedAppKey(pReqParams->appKeyIndex, secCryptoReq.utrEncReq.key);
    }
    else
    {
      retVal = MeshLocalCfgGetAppKey(pReqParams->appKeyIndex, secCryptoReq.utrEncReq.key);
    }

    if (retVal != MESH_SUCCESS)
    {
      WSF_ASSERT(retVal == MESH_SUCCESS);

      return retVal;
    }
  }

  /* Read IV index. */
  ivIndex = MeshLocalCfgGetIvIndex(&ivUpdtInProgress);

  if (ivUpdtInProgress)
  {
    /* For IV update in progress procedures, the IV index must be decremented by 1.
     * Make sure IV index is not 0.
     */
    WSF_ASSERT(ivIndex != 0);
    if (ivIndex != 0)
    {
      --ivIndex;
    }
  }

  /* Build nonce. */
  meshSecBuildNonce((pReqParams->appKeyIndex > MESH_SEC_MAX_KEY_INDEX ?
                     MESH_SEC_NONCE_DEV : MESH_SEC_NONCE_APP),
                    ((pReqParams->transMicSize == MESH_SEC_MIC_SIZE_64 ? 1 : 0) << MESH_SEC_ASZMIC_SHIFT),
                    (pReqParams->srcAddr),
                    (pReqParams->dstAddr),
                    (pReqParams->seqNo),
                    (ivIndex),
                    (secCryptoReq.utrEncReq.nonce));

  /* Setup input and output params. */
  ccmParams.pIn         = pReqParams->pAppPayload;
  ccmParams.pOut        = pReqParams->pEncAppPayload;
  ccmParams.inputLen    = pReqParams->appPayloadSize;

  /* Setup CBC-MAC params. */
  ccmParams.pCbcMac     = pReqParams->pTransMic;
  ccmParams.cbcMacSize  = pReqParams->transMicSize;

  /* Setup Nonce. */
  ccmParams.pNonce      = secCryptoReq.utrEncReq.nonce;

  /* Setup Authentication Data if needed. */
  ccmParams.pAuthData   = (MESH_IS_ADDR_VIRTUAL(pReqParams->dstAddr) ?
                           pReqParams->pLabelUuid : NULL);
  ccmParams.authDataLen = (MESH_IS_ADDR_VIRTUAL(pReqParams->dstAddr) ?
                           MESH_LABEL_UUID_SIZE : 0);

  /* Setup encryption key. */
  ccmParams.pCcmKey     = secCryptoReq.utrEncReq.key;

  /* Call toolbox. */
  retVal = (meshSecRetVal_t)MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_ENCRYPT,
                                                         &ccmParams,
                                                         meshSecUtrEncCcmCback,
                                                         (void *)&(secCryptoReq.utrEncReq));

  if (retVal == MESH_SUCCESS)
  {
    /* Mark operation as in progress by setting callback. */
    secCryptoReq.utrEncReq.cback = utrEncryptCompleteCback;
    secCryptoReq.utrEncReq.pParam   = pParam;

    /* Setup caller pointers to encrypted buffer */
    secCryptoReq.utrEncReq.pEncUtrPdu    = ccmParams.pOut;
    secCryptoReq.utrEncReq.encUtrPduSize = ccmParams.inputLen;
    secCryptoReq.utrEncReq.pTransMic     = ccmParams.pCbcMac;
    secCryptoReq.utrEncReq.transMicSize  = ccmParams.cbcMacSize;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Decrypts upper transport PDU.
 *
 *  \param[in] pReqParams               Pointer to decryption setup and storage parameters.
 *  \param[in] utrDecryptCompleteCback  Callback used to signal decryption complete.
 *  \param[in] pParam                   Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                All validation is performed. Decryption starts.
 *  \retval MESH_SEC_INVALID_PARAMS     Invalid parameters (E.g.: Mic size invalid, or
 *                                      Label UUID NULL for virtual destination address).
 *  \retval MESH_SEC_KEY_NOT_FOUND      Application key index not found to match AID.
 *  \retval MESH_SEC_OUT_OF_MEMORY      There are no resources to process the request.
 *
 *  \see meshSecUtrDecryptParams_t
 *  \see meshSecUtrDecryptCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecUtrDecrypt(meshSecUtrDecryptParams_t *pReqParams,
                                  meshSecUtrDecryptCback_t utrDecryptCompleteCback, void *pParam)
{
  meshSecRetVal_t retVal = MESH_SUCCESS;

  /* Check if module is busy. */
  if (secCryptoReq.utrDecReq.cback != NULL)
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Validate parameters. */
  if((pReqParams == NULL) ||
     (utrDecryptCompleteCback == NULL) ||
     (pReqParams->netKeyIndex > MESH_SEC_MAX_KEY_INDEX) ||
     (!MESH_IS_ADDR_UNICAST(pReqParams->srcAddr)) ||
     (pReqParams->seqNo > MESH_SEQ_MAX_VAL) ||
     (pReqParams->pAppPayload == NULL) ||
     (pReqParams->pEncAppPayload == NULL) ||
     (pReqParams->pTransMic == NULL) ||
     (pReqParams->appPayloadSize == 0) ||
     ((pReqParams->aid != MESH_SEC_DEVICE_KEY_AID) &&
      (pReqParams->aid > (MESH_AID_MASK >> MESH_AID_SHIFT))) ||
     ((pReqParams->aid == MESH_SEC_DEVICE_KEY_AID) &&
      (MESH_IS_ADDR_VIRTUAL(pReqParams->dstAddr))) ||
     ((pReqParams->transMicSize != MESH_SEC_MIC_SIZE_32) &&
      (pReqParams->transMicSize != MESH_SEC_MIC_SIZE_64)))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Reset search counters. Includes searches in virtual address table and key material since
   * both virtual addresses and AID's can collide for different Label UUID's and Application Keys
   * respectively.
   */
  secCryptoReq.utrDecReq.keySearchIdx  = 0;
  secCryptoReq.utrDecReq.vtadSearchIdx = 0;

  /* Set vtad field (destination address but only if its type is virtual). */
  if (MESH_IS_ADDR_VIRTUAL(pReqParams->dstAddr))
  {
    secCryptoReq.utrDecReq.vtad = pReqParams->dstAddr;
  }
  else
  {
    secCryptoReq.utrDecReq.vtad = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Set NetKey index to which the matching key should be bound. */
  secCryptoReq.utrDecReq.netKeyIndex = pReqParams->netKeyIndex;

  /* Set AID. */
  secCryptoReq.utrDecReq.aid         = pReqParams->aid;

  /* Build nonce. */
  meshSecBuildNonce((pReqParams->aid ==
                     MESH_SEC_DEVICE_KEY_AID ? MESH_SEC_NONCE_DEV : MESH_SEC_NONCE_APP),
                     ((pReqParams->transMicSize == MESH_SEC_MIC_SIZE_64 ? 1 : 0) <<
                       MESH_SEC_ASZMIC_SHIFT),
                     (pReqParams->srcAddr),
                     (pReqParams->dstAddr),
                     (pReqParams->seqNo),
                     (pReqParams->recvIvIndex),
                     (secCryptoReq.utrDecReq.nonce));

  /* Setup input and output params. */
  secCryptoReq.utrDecReq.ccmParams.pIn        = pReqParams->pEncAppPayload;
  secCryptoReq.utrDecReq.ccmParams.pOut       = pReqParams->pAppPayload;
  secCryptoReq.utrDecReq.ccmParams.inputLen   = pReqParams->appPayloadSize;

  /* Setup CBC-MAC params. */
  secCryptoReq.utrDecReq.ccmParams.pCbcMac    = pReqParams->pTransMic;
  secCryptoReq.utrDecReq.ccmParams.cbcMacSize = pReqParams->transMicSize;

  /* Setup Nonce. */
  secCryptoReq.utrDecReq.ccmParams.pNonce     = secCryptoReq.utrDecReq.nonce;

  /* Setup pointer to key. */
  secCryptoReq.utrDecReq.ccmParams.pCcmKey    = secCryptoReq.utrDecReq.key;

  /* Trigger request. */
  retVal = meshSecInternalUtrDecrypt(&(secCryptoReq.utrDecReq),
                                     (pReqParams->aid == MESH_SEC_DEVICE_KEY_AID ?
                                      meshSecUtrDevDecCcmCback : meshSecUtrAppDecCcmCback));

  /* Check if request is successfully processed. */
  if (retVal == MESH_SUCCESS)
  {
    /* Mark module as busy by setting user callback into the request. */
    secCryptoReq.utrDecReq.cback = utrDecryptCompleteCback;
    secCryptoReq.utrDecReq.pParam = pParam;
  }

  return retVal;
}
