/*************************************************************************************************/
/*!
 *  \file   mesh_security_nwk.c
 *
 *  \brief  Security implementation for Network.
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

#include <string.h>

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "util/bstream.h"

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
#include "mesh_security_deriv.h"
#include "mesh_security_crypto.h"

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static meshSecRetVal_t meshSecSetNextFriendMatAndDeobf(meshSecNwkDeobfDecReq_t *pReq);
static meshSecRetVal_t meshSecSetNextNetKeyMatAndDeobf(meshSecNwkDeobfDecReq_t *pReq);

/*************************************************************************************************/
/*!
 *  \brief     Network obfuscation complete toolbox callback.
 *
 *  \param[in] pCipherTextBlock  Pointer to 128-bit AES result.
 *  \param[in] pParam            Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecNwkObfCback(const uint8_t *pCipherTextBlock, void *pParam)
{
  meshSecNwkEncObfReq_t *pReq;
  meshSecNwkEncObfCback_t cback;
  uint8_t idx = 0;

  /* Recover request. */
  pReq = (meshSecNwkEncObfReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Store user callback. */
  cback = pReq->cback;

  /* Clear callback to make request available. */
  pReq->cback = NULL;

  if (pCipherTextBlock != NULL)
  {
    /* PECB is pCipherTextBlock. XOR with Network PDU starting from CTL. */
    for (idx = 0; idx < (1 + MESH_ADDR_NUM_BYTES + MESH_SEQ_NUM_BYTES); idx++)
    {
      pReq->pEncObfNwkPdu[idx + MESH_CTL_TTL_POS] ^= pCipherTextBlock[idx];
    }
  }

  /* Invoke user callback. */
  cback(pCipherTextBlock != NULL,
        pReq->nonce[MESH_SEC_NONCE_TYPE_POS] == MESH_SEC_NONCE_PROXY,
        pReq->pEncObfNwkPdu,
        pReq->encObfNwkPduSize,
        pReq->pNetMic,
        pReq->netMicSize,
        pReq->pParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Network encryption complete toolbox callback.
 *
 *  \param[in] pCcmResult  Pointer to CCM result.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecNwkEncCcmCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
{
  /* Recover request pointer. */
  const meshSecToolCcmEncryptResult_t *pResult;
  uint8_t *pObfIn;
  meshSecNwkEncObfReq_t *pReq;
  meshSecNwkEncObfCback_t cback;
  uint8_t idx;
  bool_t isSuccess  = FALSE;

  /* Recover request. */
  pReq = (meshSecNwkEncObfReq_t *)pParam;

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
    WSF_ASSERT(pCcmResult->op == MESH_SEC_TOOL_CCM_ENCRYPT);

    /* Get encrypt result. */
    pResult = &(pCcmResult->results.encryptResult);

    /* Zero-in the obfuscation input buffer. */
    memset(pReq->obfIn, 0, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    /* Construct AES encrypt function input array starting from the end. */
    pObfIn = &(pReq->obfIn[MESH_SEC_TOOL_AES_BLOCK_SIZE - 1]);

    /* Copy the Privacy Random bytes. */
    for (idx = 0; idx < MESH_SEC_PRIV_RAND_SIZE; idx++)
    {
      *pObfIn-- = pResult->pCipherText[MESH_SEC_PRIV_RAND_SIZE - 1 - idx];
    }

    /* Copy the IV index bytes which are located at the end of the nonce. */
    for (idx = 0; idx < MESH_IV_NUM_BYTES; idx++)
    {
      *pObfIn-- = pReq->nonce[MESH_SEC_TOOL_CCM_NONCE_SIZE - 1 - idx];
    }

    if (MeshSecToolAesEncrypt(pReq->pK, pReq->obfIn, meshSecNwkObfCback, pParam) == MESH_SUCCESS)
    {
      isSuccess = TRUE;
    }
  }

  if (!isSuccess)
  {
    /* Clear callback to make request available. */
    pReq->cback = NULL;

    /* Invoke user callback. */
    cback(FALSE,
          pReq->nonce[0] == MESH_SEC_NONCE_PROXY,
          pReq->pEncObfNwkPdu,
          pReq->encObfNwkPduSize,
          pReq->pNetMic,
          pReq->netMicSize,
          pReq->pParam);
    return;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Network decryption complete toolbox callback.
 *
 *  \param[in] pCcmResult  Pointer to CCM result.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecNwkDecCcmCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
{
  meshSecNwkDeobfDecReq_t *pReq;
  meshSecNwkDeobfDecCback_t cback;
  meshAddress_t friendOrLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  meshAddress_t elem0Addr;
  bool_t isSuccess   = FALSE;
  uint8_t friendMatIdx;

  /* Recover request. */
  pReq = (meshSecNwkDeobfDecReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Store user callback. */
  cback = pReq->cback;

  /* Check if toolbox succeeded. */
  if (pCcmResult != NULL)
  {
    /* Should never happen. */
    WSF_ASSERT(pCcmResult->op == MESH_SEC_TOOL_CCM_DECRYPT);

    /* Handle successful decryption and authentication. */
    if (pCcmResult->results.decryptResult.isAuthSuccess)
    {
      /* Clear callback to signal module is ready. */
      pReq->cback = NULL;

      /* Copy CTL-TTL byte, Sequence number bytes and source address bytes. */
      memcpy(&pReq->pNwkPdu[MESH_CTL_TTL_POS], &pReq->nonce[MESH_SEC_NONCE_ASZ_CTL_PAD_POS],
             1 + MESH_SEQ_NUM_BYTES + MESH_ADDR_NUM_BYTES);

      if (pReq->nonce[MESH_SEC_NONCE_TYPE_POS] == MESH_SEC_NONCE_PROXY)
      {
        pReq->pNwkPdu[MESH_CTL_TTL_POS] = pReq->ctlTtl;
      }

      /* Calculate PDU length from the initial size excluding NetMic. */
      pReq->encObfNwkPduSize -=
                              (MESH_UTILS_BF_GET(pReq->ctlTtl, MESH_CTL_SHIFT, MESH_CTL_SIZE) == 0 ?
                               MESH_SEC_MIC_SIZE_32 : MESH_SEC_MIC_SIZE_64);

      /* Check if friendship material was used to decrypt. */
      if(pReq->searchInFriendshipMat)
      {
        /* Get element 0 address. */
        MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

        /* Go back one position on search index and extract friend material index. */
        friendMatIdx = (uint8_t)((pReq->keySearchIndex - 1) >> (MESH_SEC_KEY_MAT_PER_INDEX - 1));

        if(secMatLocals.pFriendMatArray[friendMatIdx].friendAddres == elem0Addr)
        {
          friendOrLpnAddr = secMatLocals.pFriendMatArray[friendMatIdx].lpnAddress;
        }
        else
        {
          /* Should at least one of the addresses should match this. */
          WSF_ASSERT(secMatLocals.pFriendMatArray[friendMatIdx].lpnAddress == elem0Addr);

          friendOrLpnAddr = secMatLocals.pFriendMatArray[friendMatIdx].friendAddres;
        }
      }

      cback(TRUE,
            pReq->nonce[MESH_SEC_NONCE_TYPE_POS] == MESH_SEC_NONCE_PROXY,
            pReq->pNwkPdu,
            pReq->encObfNwkPduSize,
            pReq->netKeyIndex,
            pReq->ivIndex,
            friendOrLpnAddr,
            pReq->pParam);

      /* Terminate state machine here. */
      return;
    }
    /* Else. */
    /* Check if the state machine looks for friendship material. */
    if (pReq->searchInFriendshipMat)
    {
      /* Request next attempt to start. */
      if (meshSecSetNextFriendMatAndDeobf(pReq) == MESH_SUCCESS)
      {
        isSuccess = TRUE;
      }
    }
    else
    {
      /* Request next attempt to start. */
      if (meshSecSetNextNetKeyMatAndDeobf(pReq) == MESH_SUCCESS)
      {
        isSuccess = TRUE;
      }
    }
  }

  if (!isSuccess)
  {
    /* Clear callback to signal module is ready. */
    pReq->cback = NULL;

    /* Invoke user callback to signal error. */
    cback(FALSE,
          pReq->nonce[MESH_SEC_NONCE_TYPE_POS] == MESH_SEC_NONCE_PROXY,
          pReq->pNwkPdu,
          pReq->encObfNwkPduSize,
          MESH_SEC_INVALID_KEY_INDEX, /* Undetermined in case of failures. */
          0, /* Not important in case of failures. */
          friendOrLpnAddr,
          pReq->pParam);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Network deobfuscation complete toolbox callback.
 *
 *  \param[in] pCipherTextBlock  Pointer to 128-bit AES result.
 *  \param[in] pParam            Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecDeobfCback(const uint8_t *pCipherTextBlock, void *pParam)
{
  meshSecNwkDeobfDecReq_t   *pReq;
  meshSecNwkDeobfDecCback_t cback;
  meshSecToolCcmParams_t    params;
  bool_t                    isSuccess = FALSE;
  uint32_t                  temp;

  /* Recover request. */
  pReq = (meshSecNwkDeobfDecReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Store user callback. */
  cback = pReq->cback;

  /* Handle Toolbox success. */
  if (pCipherTextBlock != NULL)
  {
    /* Configure CCM params. */
    params.pIn         = &(pReq->pEncObfNwkPdu[MESH_DST_ADDR_POS]);
    params.pOut        = &(pReq->pNwkPdu[MESH_DST_ADDR_POS]);
    params.pAuthData   = NULL;
    params.pNonce      = pReq->nonce;
    params.pCcmKey     = pReq->eK;
    params.authDataLen = 0;

    /* Recover TTL-CTL byte. */
    BSTREAM_TO_UINT8(pReq->ctlTtl, pCipherTextBlock);
    pReq->ctlTtl ^= pReq->pEncObfNwkPdu[MESH_CTL_TTL_POS];

    /* Recover Sequence Number. */
    BSTREAM_BE_TO_UINT24(pReq->seqNo, pCipherTextBlock);
    BYTES_BE_TO_UINT24(temp, &pReq->pEncObfNwkPdu[MESH_SEQ_POS]);
    pReq->seqNo ^= temp;

    /* Recover Source Address. */
    BSTREAM_BE_TO_UINT16(pReq->srcAddr, pCipherTextBlock);
    BYTES_BE_TO_UINT16(temp, &pReq->pEncObfNwkPdu[MESH_SRC_ADDR_POS]);
    pReq->srcAddr ^= (uint16_t)temp;

    /* Determine NetMic size. CTL PDU's have 64-bit NetMic. */
    params.cbcMacSize = MESH_UTILS_BF_GET(pReq->ctlTtl, MESH_CTL_SHIFT, MESH_CTL_SIZE) == 0 ?
                        MESH_SEC_MIC_SIZE_32 : MESH_SEC_MIC_SIZE_64;

    /* Check if address is not unicast or recovered NetMic size exceeds minimum PDU requirements. */
    if ((!MESH_IS_ADDR_UNICAST(pReq->srcAddr)) ||
        (pReq->encObfNwkPduSize < (MESH_DST_ADDR_POS + sizeof(meshAddress_t) + params.cbcMacSize)))
    {
      /* Move to next NID since source address should always be unicast. */
      ++(pReq->keySearchIndex);

      /* Check if the state machine looks for friendship material. */
      if (pReq->searchInFriendshipMat)
      {
        /* Request next attempt to start. */
        if (meshSecSetNextFriendMatAndDeobf(pReq) == MESH_SUCCESS)
        {
          isSuccess = TRUE;
        }
      }
      else
      {
        /* Request next attempt to start. */
        if (meshSecSetNextNetKeyMatAndDeobf(pReq) == MESH_SUCCESS)
        {
          isSuccess = TRUE;
        }
      }
    }
    else /* Continue with CCM since preliminary validations passed. */
    {
      /* Build nonce. */
      meshSecBuildNonce(pReq->nonce[MESH_SEC_NONCE_TYPE_POS],
                        pReq->ctlTtl,
                        pReq->srcAddr,
                        0x0000,
                        pReq->seqNo,
                        pReq->ivIndex,
                        pReq->nonce);

      /* Determine NetMic position in the PDU. */
      params.pCbcMac = (pReq->pEncObfNwkPdu + pReq->encObfNwkPduSize - params.cbcMacSize);

      /* Determine CCM input length by substracting NetMic size and offset from total length. */
      params.inputLen = pReq->encObfNwkPduSize - MESH_DST_ADDR_POS - params.cbcMacSize;

      /* Call Toolbox to decrypt CCM. */
      if (MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_DECRYPT, &params, meshSecNwkDecCcmCback,
                                       pParam)
          == MESH_SUCCESS)
      {
        isSuccess = TRUE;
      }
    }
  }

  if (!isSuccess)
  {
    /* Clear callback to signal module is ready. */
    pReq->cback = NULL;

    /* Invoke user callback to signal error. */
    cback(FALSE,
          pReq->nonce[MESH_SEC_NONCE_TYPE_POS] == MESH_SEC_NONCE_PROXY,
          pReq->pNwkPdu,
          pReq->encObfNwkPduSize,
          MESH_SEC_INVALID_KEY_INDEX, /* Undetermined in case of failures. */
          0, /* Not important in case of failures. */
          MESH_ADDR_TYPE_UNASSIGNED,
          pReq->pParam);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets Friendship material into the decryption request based on NID match
 *             and triggers AES for deobfuscation.
 *
 *  \param[in] pReq  Pointer to active network decryption request.
 *
 *  \return    MESH_SUCCESS if a NID match is found or error code otherwise.
 *
 *  \see meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecSetNextFriendMatAndDeobf(meshSecNwkDeobfDecReq_t *pReq)
{
  meshSecFriendMat_t     *pFriendMat  = NULL;
  meshSecNetKeyInfo_t    *pNetKeyInfo = NULL;
  meshKeyRefreshStates_t state;
  uint16_t frdnMatId;
  uint8_t  entryId = 0;
  uint8_t  nid;

  /* Extract NID. */
  nid = MESH_UTILS_BF_GET(pReq->pEncObfNwkPdu[MESH_IVI_NID_POS], MESH_NID_SHIFT, MESH_NID_SIZE);

  /* Search the totality of friendship materials (old and updated). */
  for (;
       pReq->keySearchIndex < MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.friendMatListSize;
       pReq->keySearchIndex++)
  {
    /* Get index in key info array. */
    frdnMatId = (uint8_t)(pReq->keySearchIndex >> (MESH_SEC_KEY_MAT_PER_INDEX - 1));

    /* Get index in material array. */
    entryId = pReq->keySearchIndex & (MESH_SEC_KEY_MAT_PER_INDEX - 1);

    pFriendMat = &secMatLocals.pFriendMatArray[frdnMatId];

    /* Check if NetKey Index is valid so garbage didn't pass the previous filter. */
    if (pFriendMat->netKeyInfoIndex >= secMatLocals.netKeyInfoListSize)
    {
      continue;
    }

    /* Check first if NID matches. */
    if (pFriendMat->keyMaterial[entryId].nid != nid)
    {
      continue;
    }

    /* Get key information. */
    pNetKeyInfo = &secMatLocals.pNetKeyInfoArray[pFriendMat->netKeyInfoIndex];

    if (!(pNetKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      /* Should never happen since friendship credentials must be in sync with network key info. */
      WSF_ASSERT(pNetKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE);
      continue;
    }

    /* Check if the Key Refresh Phase allows use of updated material. Key Refresh Read shoud
     * never fail for keys that are in sync with security key information.
     */
    state = MeshLocalCfgGetKeyRefreshPhaseState(pNetKeyInfo->hdr.keyIndex);

    /* Check if search hits updated key slots. */
    if (entryId != pNetKeyInfo->hdr.crtKeyId)
    {
      /* Check if updated material exists. */
      if (pFriendMat->hasUpdtMaterial)
      {
        /* Check if Key Refresh rules don't allow use of new keys. */
        if ((state < MESH_KEY_REFRESH_FIRST_PHASE) || (state > MESH_KEY_REFRESH_THIRD_PHASE))
        {
          continue;
        }
      }
      else
      {
        continue;
      }
    }
    else
    {
      /* Check if Key refresh rules allow use of old key. */
      if (state == MESH_KEY_REFRESH_THIRD_PHASE)
      {
        continue;
      }
    }

    /* If code reached this, NID is found, associated NetKey information is found and the
     * friendship material is available regardless of the Key Refresh Phase.
     */
    break;
  }

  if (pReq->keySearchIndex == (MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.friendMatListSize) ||
      pNetKeyInfo == NULL)
  {
    return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
  }

  /* Copy Ek. */
  memcpy(pReq->eK, pFriendMat->keyMaterial[entryId].encryptKey, MESH_SEC_TOOL_AES_BLOCK_SIZE);

  /* Copy Pk. */
  memcpy(pReq->pK, pFriendMat->keyMaterial[entryId].privacyKey, MESH_SEC_TOOL_AES_BLOCK_SIZE);

  /* Set NetKey Index. */
  pReq->netKeyIndex = pNetKeyInfo->hdr.keyIndex;

  /* Increment search index for the following requests. */
  ++(pReq->keySearchIndex);

  /* Call Toolbox for AES used in deobfuscation. */
  return (meshSecRetVal_t)MeshSecToolAesEncrypt(pReq->pK,
                                                pReq->obfIn,
                                                meshSecDeobfCback,
                                                (void *)pReq);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets Network Key material into the decryption request based on NID match
 *             and triggers AES for deobfuscation.
 *
 *  \param[in] pReq  Pointer to active network decryption request.
 *
 *  \return    MESH_SUCCESS if a NID match is found or error code otherwise.
 *
 *  \see meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecSetNextNetKeyMatAndDeobf(meshSecNwkDeobfDecReq_t *pReq)
{
  meshSecNetKeyInfo_t    *pNetKeyInfo = NULL;
  meshKeyRefreshStates_t state;
  uint8_t                entryId      = 0;
  uint8_t nid;
  uint16_t keyInfoId;

  /* Extract NID. */
  nid = MESH_UTILS_BF_GET(pReq->pEncObfNwkPdu[MESH_IVI_NID_POS], MESH_NID_SHIFT, MESH_NID_SIZE);

  /* Search the totality of key materials (old and updated). */
  for (;
       pReq->keySearchIndex < MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.netKeyInfoListSize;
       pReq->keySearchIndex++)
  {
    /* Get index in key info array. */
    keyInfoId = (uint8_t)(pReq->keySearchIndex >> (MESH_SEC_KEY_MAT_PER_INDEX - 1));

    /* Get index in material array. */
    entryId = pReq->keySearchIndex & (MESH_SEC_KEY_MAT_PER_INDEX - 1);
    pNetKeyInfo = &secMatLocals.pNetKeyInfoArray[keyInfoId];

    /* Check if search hits old key slots (for keys not updated). */
    if (!(pNetKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      continue;
    }

    /* Check if the NID matches */
    if (pNetKeyInfo->keyMaterial[entryId].masterPduSecMat.nid != nid)
    {
      continue;
    }

    /* Check if the Key Refresh Phase allows use of updated material. Key Refresh Read shoud
     * never fail.
     */
    state = MeshLocalCfgGetKeyRefreshPhaseState(pNetKeyInfo->hdr.keyIndex);

    /* Check if search hits updated key slots. */
    if (entryId != pNetKeyInfo->hdr.crtKeyId)
    {
      /* Check if updated material exists. */
      if (pNetKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
      {
        /* Check if state doesn't allow use of new keys. */
        if ((state < MESH_KEY_REFRESH_FIRST_PHASE) || (state > MESH_KEY_REFRESH_THIRD_PHASE))
        {
          continue;
        }
      }
      else
      {
        continue;
      }
    }
    else
    {
      /* Check if Key refresh rules allow use of old key. */
      if (state == MESH_KEY_REFRESH_THIRD_PHASE)
      {
        continue;
      }
    }

    /* If execution reaches here, it means that the NID matched a valid entry of the NetKey info
     * array.
     */
    break;
  }

  if (pReq->keySearchIndex == (MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.netKeyInfoListSize))
  {
    /* End here but not invoke user callback. Instead move to friendship security. */

    /* Instruct SM to continue in friendship security material. */
    pReq->searchInFriendshipMat = TRUE;

    /* Reset key search index. */
    pReq->keySearchIndex = 0;

    return meshSecSetNextFriendMatAndDeobf(pReq);
  }

  /* Copy Ek. */
  memcpy(pReq->eK, pNetKeyInfo->keyMaterial[entryId].masterPduSecMat.encryptKey,
         MESH_SEC_TOOL_AES_BLOCK_SIZE);

  /* Copy Pk. */
  memcpy(pReq->pK, pNetKeyInfo->keyMaterial[entryId].masterPduSecMat.privacyKey,
         MESH_SEC_TOOL_AES_BLOCK_SIZE);

  /* Set NetKey Index. */
  pReq->netKeyIndex = pNetKeyInfo->hdr.keyIndex;

  /* Increment search index for the following requests. */
  ++(pReq->keySearchIndex);

  /* Call Toolbox for AES used in deobfuscation. */
  return (meshSecRetVal_t)MeshSecToolAesEncrypt(pReq->pK, pReq->obfIn, meshSecDeobfCback,
                                                (void *)pReq);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Encrypts and obfuscates the network PDU.
 *
 *  \param[in] isProxyConfig        TRUE if the PDU is a Proxy Configuration Message.
 *  \param[in] pReqParams           Pointer to encryption and obfuscation setup and storage
 *                                  parameters.
 *  \param[in] encObfCompleteCback  Callback used to signal encryption and obfuscation complete.
 *  \param[in] pParam               Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Encryption starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: Invalid length of the
 *                                           network PDU).
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no key material associated to the Network Key
 *                                           Index. See the remarks section for more information.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \remarks The NID, Encryption, Privacy keys can be present for the old key during
 *           a Key Refresh Procedure phase, but not for the new key and the phase dictates
 *           the new key material should be returned.
 *
 *  \note    The successful operation of Network encrypt also sets IVI-NID byte in the first byte
 *           of the encrypted and obfuscated Network PDU.
 *
 *  \see meshSecNwkEncObfParams_t
 *  \see meshSecNwkEncObfCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecNwkEncObf(bool_t isProxyConfig, meshSecNwkEncObfParams_t *pReqParams,
                                 meshSecNwkEncObfCback_t encObfCompleteCback, void *pParam)
{
  meshSecToolCcmParams_t ccmParams;
  meshSecNwkEncObfReq_t  *pReq       = NULL;
  meshSecNetKeyInfo_t    *pKeyInfo   = NULL;
  meshSecFriendMat_t     *pFriendMat = NULL;
  meshSeqNumber_t        seqNo       = 0;
  uint16_t               idx         = 0;
  meshAddress_t          src         = 0;
  meshKeyRefreshStates_t state;
  meshSecToolRetVal_t    retVal      = MESH_SUCCESS;
  uint8_t                nid;
  uint8_t                entryId     = MESH_SEC_KEY_MAT_PER_INDEX;

  /* Validate parameters. */
  if((pReqParams == NULL) ||
     (encObfCompleteCback == NULL) ||
     (pReqParams->pNwkPduNoMic == NULL) ||
     (pReqParams->pObfEncNwkPduNoMic == NULL) ||
     (pReqParams->pNwkPduNetMic == NULL) ||
     ((pReqParams->netMicSize != MESH_SEC_MIC_SIZE_32) &&
      (pReqParams->netMicSize != MESH_SEC_MIC_SIZE_64)) ||
     ((pReqParams->nwkPduNoMicSize + pReqParams->netMicSize) < MESH_SEC_NWK_PDU_MIN_SIZE) ||
     (pReqParams->netKeyIndex > MESH_SEC_MAX_KEY_INDEX) ||
     ((pReqParams->friendOrLpnAddress != MESH_ADDR_TYPE_UNASSIGNED) &&
      (!MESH_IS_ADDR_UNICAST(pReqParams->friendOrLpnAddress))))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Check if is a encrypt request from Proxy*/
  if (isProxyConfig)
  {
    pReq = &(secCryptoReq.nwkEncObfReq[MESH_SEC_NWK_ENC_SRC_PROXY]);
  }
  else
  {
    /* Check if is an encrypt request from the Friendship module */
    pReq = (pReqParams->friendOrLpnAddress == MESH_ADDR_TYPE_UNASSIGNED) ?
                                      (&(secCryptoReq.nwkEncObfReq[MESH_SEC_NWK_ENC_SRC_NWK])) :
                                      (&(secCryptoReq.nwkEncObfReq[MESH_SEC_NWK_ENC_SRC_FRIEND]));
  }

  /* Check if request is in progress. */
  if (pReq->cback != NULL)
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Search for the key material. */
  for (idx = 0; idx < secMatLocals.netKeyInfoListSize; idx++)
  {
    if ((secMatLocals.pNetKeyInfoArray[idx].hdr.keyIndex == pReqParams->netKeyIndex) &&
        (secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      pKeyInfo = &(secMatLocals.pNetKeyInfoArray[idx]);
      break;
    }
  }

  /* Check if key index is found. */
  if (pKeyInfo == NULL)
  {
    return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
  }

  /* Read Key refresh state. */
  state = MeshLocalCfgGetKeyRefreshPhaseState(pReqParams->netKeyIndex);

  /* Decide which entry in the key material to use based on key refresh state. */
  switch (state)
  {
    case MESH_KEY_REFRESH_NOT_ACTIVE:
    case MESH_KEY_REFRESH_FIRST_PHASE:
      entryId = pKeyInfo->hdr.crtKeyId;
      break;
    case MESH_KEY_REFRESH_SECOND_PHASE:
    case MESH_KEY_REFRESH_THIRD_PHASE:
      if (pKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
      {
        entryId = 1 - pKeyInfo->hdr.crtKeyId;
      }
      break;
    default:
      break;
  }

  /* Check if correct material entry can be determined. */
  if (entryId == MESH_SEC_KEY_MAT_PER_INDEX)
  {
    return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
  }

  /* Check if master credential material should be used */
  if(pReqParams->friendOrLpnAddress == MESH_ADDR_TYPE_UNASSIGNED)
  {
    /* Copy encryption key and privacy key. This ensures that the encryption and obfuscation
     * are consistent in case key updates or removals happen in between.
     */
    memcpy(pReq->eK, pKeyInfo->keyMaterial[entryId].masterPduSecMat.encryptKey,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);
    memcpy(pReq->pK, pKeyInfo->keyMaterial[entryId].masterPduSecMat.privacyKey,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);
    /* Get NID */
    nid = pKeyInfo->keyMaterial[entryId].masterPduSecMat.nid;
  }
  else
  {
    /* Search friendship material based on NetKey Index, key refresh phase and address of friend
     * or LPN.
     */
    if (meshSecNetKeyInfoAndAddrToFriendMat(pKeyInfo, entryId, pReqParams->friendOrLpnAddress,
                                            &pFriendMat)
        != MESH_SUCCESS)
    {
      return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
    }

    /* Copy encryption key and privacy key. This ensures that the encryption and obfuscation
     * are consistent in case key updates or removals happen in between.
     */
    memcpy(pReq->eK, pFriendMat->keyMaterial[entryId].encryptKey,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);
    memcpy(pReq->pK, pFriendMat->keyMaterial[entryId].privacyKey,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);
    /* Get NID */
    nid = pFriendMat->keyMaterial[entryId].nid;
  }

  /* Reconstruct nonce parameters. */
  src = (((uint16_t)pReqParams->pNwkPduNoMic[MESH_SRC_ADDR_POS]) << 8) |
         ((uint16_t)pReqParams->pNwkPduNoMic[MESH_SRC_ADDR_POS + 1]);

  seqNo = (((meshSeqNumber_t)pReqParams->pNwkPduNoMic[MESH_SEQ_POS]) << 16) |
          (((meshSeqNumber_t)pReqParams->pNwkPduNoMic[MESH_SEQ_POS + 1]) << 8) |
           ((meshSeqNumber_t)pReqParams->pNwkPduNoMic[MESH_SEQ_POS + 2]);

  /* Build Nonce. */
  meshSecBuildNonce(isProxyConfig ?
                    MESH_SEC_NONCE_PROXY : MESH_SEC_NONCE_NWK,
                    isProxyConfig ?
                    0 : pReqParams->pNwkPduNoMic[MESH_CTL_TTL_POS],
                    src,
                    0x0000,
                    seqNo,
                    pReqParams->ivIndex,
                    pReq->nonce);

  /* Setup input and output params as offset to start of Network PDU. */
  ccmParams.pIn         = pReqParams->pNwkPduNoMic + MESH_DST_ADDR_POS;
  ccmParams.pOut        = pReqParams->pObfEncNwkPduNoMic + MESH_DST_ADDR_POS;
  ccmParams.inputLen    = pReqParams->nwkPduNoMicSize - MESH_DST_ADDR_POS;

  /* Setup CBC-MAC params. */
  ccmParams.pCbcMac     = pReqParams->pNwkPduNetMic;
  ccmParams.cbcMacSize  = pReqParams->netMicSize;

  /* Setup Nonce. */
  ccmParams.pNonce      = pReq->nonce;

  /* Setup Authentication Data if needed. */
  ccmParams.pAuthData   = NULL;
  ccmParams.authDataLen = 0;

  /* Setup encryption key. */
  ccmParams.pCcmKey     = pReq->eK;

  /* Call toolbox. */
  retVal = (meshSecRetVal_t)MeshSecToolCcmEncryptDecrypt(MESH_SEC_TOOL_CCM_ENCRYPT,
                                                         &ccmParams,
                                                         meshSecNwkEncCcmCback,
                                                         (void *)pReq);

  if (retVal == MESH_SUCCESS)
  {
    /* Mark operation as in progress by setting callback. */
    pReq->cback = encObfCompleteCback;
    pReq->pParam   = pParam;

    /* Set IVI-NID byte. Start by resetting the byte. */
    pReqParams->pObfEncNwkPduNoMic[MESH_IVI_NID_POS] = 0;

    /* Set LSb of IV index. */
    MESH_UTILS_BF_SET(pReqParams->pObfEncNwkPduNoMic[MESH_IVI_NID_POS],
                      (uint8_t)(pReqParams->ivIndex & 0x01),
                      MESH_IVI_SHIFT,
                      MESH_IVI_SIZE);
    /* Set NID. */
    MESH_UTILS_BF_SET(pReqParams->pObfEncNwkPduNoMic[MESH_IVI_NID_POS],
                      nid,
                      MESH_NID_SHIFT,
                      MESH_NID_SIZE);

    /* Copy PDU bytes starting from CTL-TTL to the destination buffer for obfuscation XOR. */
    memcpy(&(pReqParams->pObfEncNwkPduNoMic[MESH_CTL_TTL_POS]),
           &(pReqParams->pNwkPduNoMic[MESH_CTL_TTL_POS]),
           MESH_SEC_PRIV_RAND_SIZE - 1);

    /* Set pointer to destination PDU. */
    pReq->pEncObfNwkPdu = pReqParams->pObfEncNwkPduNoMic;

    /* Set pointer to NETMIC. */
    pReq->pNetMic = pReqParams->pNwkPduNetMic;

    /* Set destination PDU size. */
    pReq->encObfNwkPduSize = pReqParams->nwkPduNoMicSize;

    /* Set NETMIC size. */
    pReq->netMicSize = pReqParams->netMicSize;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Deobfuscates and decrypts a received network PDU.
 *
 *  \param[in] isProxyConfig     TRUE if PDU type is a Proxy Configuration Message.
 *  \param[in] pReqParams        Pointer to deobfuscation and decryption setup and storage
 *                               parameters.
 *  \param[in] nwkDeobfDecCback  Callback used to signal operation complete.
 *  \param[in] pParam            Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Deobfuscation starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: Invalid length of the
 *                                           received PDU).
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  No NetKey index matching NID.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \see meshSecNwkDeobfDecParams_t
 *  \see meshSecNwkDeobfDecCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecNwkDeobfDec(bool_t isProxyConfig, meshSecNwkDeobfDecParams_t *pReqParams,
                                   meshSecNwkDeobfDecCback_t nwkDeobfDecCback, void *pParam)
{
  meshSecNwkDeobfDecReq_t *pReq;
  uint8_t                 *pObfIn = NULL;
  uint32_t                ivIndex = 0;
  uint8_t                 idx     = 0;
  uint8_t                 ivi     = 0;
  meshSecRetVal_t         retVal  = MESH_SUCCESS;

  /* Validate parameters. */
  if((pReqParams == NULL) ||
     (nwkDeobfDecCback == NULL) ||
     (pReqParams->pNwkPduNoMic == NULL) ||
     (pReqParams->pObfEncAuthNwkPdu == NULL) ||
     (pReqParams->nwkPduSize < MESH_SEC_NWK_PDU_MIN_SIZE))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Set correct request source. */
  pReq = isProxyConfig ? (&secCryptoReq.nwkDeobfDecReq[MESH_SEC_NWK_DEC_SRC_PROXY]) :
                         (&secCryptoReq.nwkDeobfDecReq[MESH_SEC_NWK_DEC_SRC_NWK_FRIEND]);

  /* Check if resource is available. */
  if (pReq->cback != NULL)
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Configure request parameters. */
  pReq->encObfNwkPduSize = pReqParams->nwkPduSize;
  pReq->pEncObfNwkPdu    = pReqParams->pObfEncAuthNwkPdu;
  pReq->pNwkPdu          = pReqParams->pNwkPduNoMic;

  /* Read IV index. */
  ivIndex = MeshLocalCfgGetIvIndex(NULL);

  ivi = MESH_UTILS_BF_GET((pReqParams->pObfEncAuthNwkPdu[MESH_IVI_NID_POS]), MESH_IVI_SHIFT,
                          MESH_IVI_SIZE);

  /* Check if LSBit of IV index is different from IVI. */
  if(ivi != MESH_UTILS_BF_GET(ivIndex, 0, MESH_IVI_SIZE))
  {
    /* For accepted IV the IV index must be decremented by 1 when IVI and LSB of IV index are
     * different. Make sure IV index is not 0.
     */
    if (ivIndex == 0)
    {
      /* Probably the PDU is sent by a node which finished IV update so this means local node
       * never entered the IV update procedure.
       */
      return MESH_SEC_INVALID_PARAMS;
    }
    --ivIndex;
  }

  /* Store ivIndex as well to save some computation time. */
  pReq->ivIndex = ivIndex;

  /* Reset obfuscation buffer */
  memset(pReq->obfIn, 0, MESH_KEY_SIZE_128);

  /* Prepare input for deobfuscation starting from the back. */
  pObfIn = &(pReq->obfIn[MESH_SEC_TOOL_AES_BLOCK_SIZE - 1]);

  /* Copy the Privacy Random bytes. */
  for (idx = 0; idx < MESH_SEC_PRIV_RAND_SIZE; idx++)
  {
    *pObfIn-- = pReq->pEncObfNwkPdu[MESH_DST_ADDR_POS + MESH_SEC_PRIV_RAND_SIZE - 1 - idx];
  }

  /* Copy the IV index bytes which are located at the end of the nonce. */
  for (idx = 0; idx < MESH_IV_NUM_BYTES; idx++)
  {
    *pObfIn-- = (uint8_t)(ivIndex >> (8 * idx));
  }

  /* Reset key search index. */
  pReq->keySearchIndex = 0;

  /* Signal decryption SM to start in master credential material. */
  pReq->searchInFriendshipMat = FALSE;

  /* Set first byte of the nonce (nonce type) to avoid keeping another variable. */
  pReq->nonce[MESH_SEC_NONCE_TYPE_POS] = isProxyConfig ? MESH_SEC_NONCE_PROXY : MESH_SEC_NONCE_NWK;

  /* Try to find keys matching NID and request deobfuscation. */
  retVal = meshSecSetNextNetKeyMatAndDeobf(pReq);

  if(retVal == MESH_SUCCESS)
  {
    /* Mark resource as busy by setting the user callback. */
    pReq->cback = nwkDeobfDecCback;

    /* Set request parameters. */
    pReq->pParam = pParam;
  }

  return retVal;
}
