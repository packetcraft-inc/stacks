/*************************************************************************************************/
/*!
 *  \file   mesh_security_deriv.c
 *
 *  \brief  Security derivation function implementation.
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
  Macros
**************************************************************************************************/

/*! Invalid value for Network Key index used to mark an entry for which friendship material is
 *  in progress of being generated.
 */
#define MESH_SEC_FRIEND_ENTRY_BUSY_IDX      0xFFFE

/*! Resets in progress flags */
#define MESH_SEC_RESET_IN_PROGRESS(flags)   ((flags) &= ~(MESH_SEC_KEY_CRT_IN_PROGESS | \
                                                          MESH_SEC_KEY_UPDT_IN_PROGRESS))

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! S1("nkbk") */
static const uint8_t secS1NkbkStr[] =
{
  0x2c, 0x24, 0x61, 0x9a, 0xb7, 0x93, 0xc1, 0x23, 0x3f, 0x6e, 0x22, 0x67, 0x38, 0x39, 0x3d, 0xec
};

/*!< S1("nkik") */
static const uint8_t secS1NkikStr[] =
{
  0xf8, 0x79, 0x5a, 0x1a, 0xab, 0xf1, 0x82, 0xe4, 0xf1, 0x63, 0xd8, 0x6e, 0x24, 0x5e, 0x19, 0xf4
};

/*! String "id128" concatenated with hex 0x01 */
static const uint8_t secId128Str[] =
{
  0x69, 0x64, 0x31, 0x32, 0x38, 0x01
};

/*! Mesh Security temporary storage used by derivation procedures. */
static struct meshSecTempData_tag
{
  uint8_t appKey[MESH_KEY_SIZE_128];        /*!< Application Key used by an ongoing
                                             *   derivation procedure
                                             */
  uint8_t nwkKey[MESH_KEY_SIZE_128];        /*!< Network Key buffer used by an ongoing
                                             *   derivation procedure
                                             */
  uint8_t nwkKeyFriend[MESH_KEY_SIZE_128];  /*!< Network Key buffer used by an ongoing
                                             *   derivation procedure using friendship
                                             *   credentials
                                             */
} secTempData;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Gets address of the friendship material based on the friend or LPN address and
 *              associated Network Key information.
 *
 *  \param[in]  pKeyInfo        Pointer to key information.
 *  \param[in]  entryId         Key material entry identifier corresponding to either old or new
 *                              Network Key
 *  \param[in]  searchAddr      Address that needs to match the remote device in the friendship
 *                              material (either friend or LPN)
 *  \param[out] ppOutFriendMat  Pointer to memory where the address of the friendship material is
 *                              stored on success.
 *
 *  \return     Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
meshSecRetVal_t meshSecNetKeyInfoAndAddrToFriendMat(meshSecNetKeyInfo_t *pKeyInfo, uint8_t entryId,
                                                    meshAddress_t searchAddr,
                                                    meshSecFriendMat_t **ppOutFriendMat)
{
  meshSecFriendMat_t *pFriendMat;
  meshSecNetKeyInfo_t *pTempNetKeyInfo;
  meshAddress_t addr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t idx;

  /* Get element 0 address. */
  meshLocalCfgRetVal_t retVal = MeshLocalCfgGetAddrFromElementId(0, &addr);

  if(retVal != MESH_SUCCESS)
  {
    WSF_ASSERT(retVal == MESH_SUCCESS);

    return retVal;
  }

  for(idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    /* Grab instance from friendship array. */
    if (secMatLocals.pFriendMatArray[idx].netKeyInfoIndex >= secMatLocals.netKeyInfoListSize)
    {
      continue;
    }
    pFriendMat = &secMatLocals.pFriendMatArray[idx];
    pTempNetKeyInfo = &secMatLocals.pNetKeyInfoArray[pFriendMat->netKeyInfoIndex];

    /* Check for NetKey Index match. */
    if((pTempNetKeyInfo->hdr.keyIndex != pKeyInfo->hdr.keyIndex) ||
       !(pTempNetKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE) ||
       !(pKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      continue;
    }

    /* Check if addresses match on either combination to avoid checking if node is friend or lpn.
      */
    if (((pFriendMat->friendAddres == addr) && (pFriendMat->lpnAddress == searchAddr)) ||
        ((pFriendMat->lpnAddress == addr) && (pFriendMat->friendAddres == searchAddr)))
    {
      /* Check if updated material is needed and if it exists. */
      if ((entryId != pKeyInfo->hdr.crtKeyId) && (!pFriendMat->hasUpdtMaterial))
      {
        return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
      }
      /* Set address to friendship material. */
      *ppOutFriendMat = pFriendMat;

      return MESH_SUCCESS;
    }
  }

  return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Clears the hasUpdtMaterial flag of all friendship materials associated to a Network
 *             Key as part of a key refresh phase transition.
 *
 *  \param[in] netKeyListIdx  Index in Network Key information list.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecClearFriendUpdtFlag(uint16_t netKeyListIdx)
{
  uint16_t idx;
  /* Clear update flag for friendship material. */
  for(idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    /* Grab instance from friendship array */
    if (secMatLocals.pFriendMatArray[idx].netKeyInfoIndex == netKeyListIdx)
    {
      secMatLocals.pFriendMatArray[idx].hasUpdtMaterial = FALSE;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Clears an entry containing derivation material based on a Network key.
 *
 *  \param[in] netKeyListIdx  Index in Network Key information list.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecRemoveNetKeyMaterial(uint16_t netKeyListIdx)
{
  uint16_t idx;

  /* Clear update flag for friendship material. */
  for (idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    /* Grab instance from friendship array */
    if (secMatLocals.pFriendMatArray[idx].netKeyInfoIndex == netKeyListIdx)
    {
      secMatLocals.pFriendMatArray[idx].netKeyInfoIndex = MESH_SEC_INVALID_ENTRY_INDEX;
      secMatLocals.pFriendMatArray[idx].hasUpdtMaterial = FALSE;
    }
  }

  /* Clear the material available flag. */
  secMatLocals.pNetKeyInfoArray[netKeyListIdx].hdr.flags = MESH_SEC_KEY_UNUSED;

  /* Clear the NetKey Index. */
  secMatLocals.pNetKeyInfoArray[netKeyListIdx].hdr.keyIndex = MESH_SEC_INVALID_KEY_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     Updates all the friendship material associated to a Network Key during a key update.
 *
 *  \param[in] pReq   Pointer to a Network Key derivation request.
 *  \param[in] cback  Callback to be invoked by the toolbox at the end of K2.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecFriendMatUpdate(meshSecNetKeyDerivReq_t *pReq,
                                   meshSecToolKeyDerivationCback_t cback)
{
  static uint8_t      k2PBuff[MESH_SEC_K2_P_FRIEND_SIZE] = { 0 };

  uint8_t             *pK2PBuff;
  meshSecFriendMat_t  *pFriendMat  = NULL;
  bool_t              procComplete = FALSE;
  bool_t              isSuccess    = FALSE;
  uint16_t            netKeyEntryIdx;

  /* Point to start of K2 P buff. */
  pK2PBuff = k2PBuff;

  /* Resume searching for entries to be updated starting from last index. */
  while (pReq->friendUpdtIdx < secMatLocals.friendMatListSize)
  {
    pFriendMat = &(secMatLocals.pFriendMatArray[pReq->friendUpdtIdx]);

    /* Check if friendship material has matching key entry index and is not already updated or in
     * progress of being updated.
     */
     if ((pFriendMat->netKeyInfoIndex < secMatLocals.netKeyInfoListSize) &&
         (pFriendMat->netKeyInfoIndex == pReq->netKeyListIdx) &&
         (pFriendMat->hasUpdtMaterial == FALSE))
    {
      break;
    }

    ++(pReq->friendUpdtIdx);
  }

  /* If maximum index is reached, finish procedure. */
  if (pReq->friendUpdtIdx == secMatLocals.friendMatListSize)
  {
    procComplete = TRUE;
    isSuccess    = TRUE;
  }
  else
  {
    /* Generate P buffer. */
    UINT8_TO_BSTREAM(pK2PBuff, 0x01);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->lpnAddress >> 8);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->lpnAddress);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->friendAddres >> 8);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->friendAddres);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->lpnCounter >> 8);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->lpnCounter);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->friendCounter >> 8);
    UINT8_TO_BSTREAM(pK2PBuff, pFriendMat->friendCounter);

    /* Send next request. */
    if (MeshSecToolK2Derive(k2PBuff, sizeof(k2PBuff), secTempData.nwkKey, cback, (void *)pReq)
        != MESH_SUCCESS)
    {
      isSuccess = FALSE;
      procComplete = TRUE;
    }
  }

  if (procComplete)
  {
    if (isSuccess)
    {
      /* Check delete current only flag indicating key refresh phase is complete and
       * updated material is now current key material.
       */
      if (secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx].hdr.flags & MESH_SEC_KEY_CRT_MAT_DELETE)
      {
        /* Advance current key index. */
        secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx].hdr.crtKeyId =
          1 - secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx].hdr.crtKeyId;

        /* Clear flag. */
        secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx].hdr.flags &= ~MESH_SEC_KEY_CRT_MAT_DELETE;

        /* Clear update flag for friendship material. */
        meshSecClearFriendUpdtFlag(pReq->netKeyListIdx);
      }
      else
      {
        /* Update operation completed. Mark keys as available. */
        secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx].hdr.flags |= MESH_SEC_KEY_UPDT_MAT_AVAILABLE;
      }
    }
    else
    {
      /* Update failed so there is no point in considering some friendships have materials updated.
       */
      meshSecClearFriendUpdtFlag(pReq->netKeyListIdx);
    }

    /* Clear update "in progress" flag. */
    secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx].hdr.flags &= ~MESH_SEC_KEY_UPDT_IN_PROGRESS;

    /* Copy index in Network Key material from request. */
    netKeyEntryIdx = pReq->netKeyListIdx;

    /* Reset request slot. */
    pReq->netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

    /* Invoke user callback. */
    pReq->cback(MESH_SEC_KEY_TYPE_NWK,
                secMatLocals.pNetKeyInfoArray[netKeyEntryIdx].hdr.keyIndex,
                isSuccess, pReq->isUpdate, pReq->pParam);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     K2 callback implementation for friendship material updated during Network Key
 *             material update.
 *
 *  \param[in] pResult     Pointer to K2 result.
 *  \param[in] resultSize  Size of the result in bytes.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecFriendMatUpdateCback(const uint8_t *pResult,
                                        uint8_t resultSize,
                                        void *pParam)
{
  meshSecNetKeyDerivReq_t *pReq;
  meshSecNetKeyInfo_t     *pKeyInfo;
  meshSecFriendMat_t      *pFriendMat;
  uint16_t                netKeyIndex;
  uint8_t                 entryId;
  bool_t                  isSuccess = FALSE;

  /* Should never happen. */
  WSF_ASSERT((resultSize == MESH_SEC_TOOL_K2_RESULT_SIZE) || (pResult == NULL));

  /* Recover request. */
  pReq = (meshSecNetKeyDerivReq_t *)pParam;

  /* Check if module is re-initialised. */
  if (pReq->netKeyListIdx == MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return;
  }

  /* Recover key info from request. */
  pKeyInfo = &secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx];

  /* Recover friendship material. */
  pFriendMat = &(secMatLocals.pFriendMatArray[pReq->friendUpdtIdx]);

  /* Set entry id as 1 - current entry ID since this is an update material callback. */
  entryId = (1 - pKeyInfo->hdr.crtKeyId);

  /* Store key index in case delete is pending. */
  netKeyIndex = pKeyInfo->hdr.keyIndex;

  /* Handle error or key removed or material removed during update. */
   if ((pResult == NULL) ||
       (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE) ||
       (pFriendMat->netKeyInfoIndex != pReq->netKeyListIdx))
  {
    /* Check if Key Material is subject to removal. */
    if (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE)
    {
      /* Handle removal. */
      meshSecRemoveNetKeyMaterial(pReq->netKeyListIdx);
    }
  }
  else
  {
    isSuccess = TRUE;

    /* Copy NID. */
    pFriendMat->keyMaterial[entryId].nid =
      MESH_UTILS_BF_GET(pResult[0], MESH_NID_SHIFT, MESH_NID_SIZE);
    /* Copy Ek. */
    memcpy(pFriendMat->keyMaterial[entryId].encryptKey, &pResult[1],
                    MESH_KEY_SIZE_128);
    /* Copy Pk. */
    memcpy(pFriendMat->keyMaterial[entryId].privacyKey, &pResult[1 + MESH_KEY_SIZE_128],
           MESH_KEY_SIZE_128);

    /* Mark update material as available. */
    pFriendMat->hasUpdtMaterial = TRUE;

    /* Continue update. */
    ++(pReq->friendUpdtIdx);

    meshSecFriendMatUpdate(pReq, meshSecFriendMatUpdateCback);
  }

  if (!isSuccess)
  {
    /* Reset update in progress flag. */
    pKeyInfo->hdr.flags &= ~MESH_SEC_KEY_UPDT_IN_PROGRESS;

    /* Clear update flag on friendships since the procedure failed. */
    meshSecClearFriendUpdtFlag(pReq->netKeyListIdx);

    /* Reset request slot. */
    pReq->netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

    /* Invoke user callback. */
    pReq->cback(MESH_SEC_KEY_TYPE_NWK, netKeyIndex, isSuccess,
                   pReq->isUpdate, pReq->pParam);
  }
  (void)resultSize;
}

/*************************************************************************************************/
/*!
 *  \brief     K2 callback implementation for handling generated master PDU security material
 *             (NID, Ek, Pk) as part of the state machine for Network Key derivation.
 *
 *  \param[in] pResult     Pointer to K2 result.
 *  \param[in] resultSize  Size of the result in bytes.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecPduSecDerivCback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  meshSecNetKeyDerivReq_t *pReq;
  meshSecNetKeyInfo_t     *pKeyInfo;
  uint16_t                netKeyIndex;
  uint8_t                 entryId;
  bool_t                  isSuccess    = FALSE;
  bool_t                  procComplete = FALSE;

  /* Should never happen. */
  WSF_ASSERT((resultSize == MESH_SEC_TOOL_K2_RESULT_SIZE) || (pResult == NULL));

  /* Recover request. */
  pReq = (meshSecNetKeyDerivReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->netKeyListIdx == MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return;
  }

  /* Recover NetKey information. */
  pKeyInfo = &secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx];

  /* Get material entry identifier. */
  entryId = pReq->isUpdate == FALSE ? (pKeyInfo->hdr.crtKeyId) : (1 - pKeyInfo->hdr.crtKeyId);

  /* Store key index in case delete is pending. */
  netKeyIndex = pKeyInfo->hdr.keyIndex;

  if ((pResult == NULL) || (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE))
  {
    /* Check if Key Material is subject to removal. */
    if (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE)
    {
      /* Handle removal. */
      meshSecRemoveNetKeyMaterial(pReq->netKeyListIdx);
    }

    /* Mark procedure as terminated. */
    procComplete = TRUE;
  }
  else
  {
    /* Mark procedure success. */
    isSuccess = TRUE;

    /* Copy NID. */
    pKeyInfo->keyMaterial[entryId].masterPduSecMat.nid = MESH_UTILS_BF_GET(pResult[0],
                                                                           MESH_NID_SHIFT,
                                                                           MESH_NID_SIZE);
    /* Copy Ek. */
    memcpy(pKeyInfo->keyMaterial[entryId].masterPduSecMat.encryptKey, &pResult[1],
           MESH_KEY_SIZE_128);
    /* Copy Pk. */
    memcpy(pKeyInfo->keyMaterial[entryId].masterPduSecMat.privacyKey,
           &pResult[1 + MESH_KEY_SIZE_128], MESH_KEY_SIZE_128);

    /* Check if operation is key update. */
    if (pReq->isUpdate)
    {
      /* Reset search index for the friendship material. */
      pReq->friendUpdtIdx = 0;

      /* Start friendship material derivation update. */
      meshSecFriendMatUpdate(pReq, meshSecFriendMatUpdateCback);
    }
    else
    {
      /* Network Key derivation stops with no friendship material update. */
      procComplete = TRUE;

      /* Set material available flag and finish since it is impossible to have friendship material
       * on a key that is just added.
       */
      pKeyInfo->hdr.flags |= MESH_SEC_KEY_CRT_MAT_AVAILABLE;
    }
  }

  /* Check if procedure continues with friendship material update.
   * Successful operation is marked by isSuccess value.
   */
  if (procComplete)
  {
    /* Clear all "in progress" flags to avoid verifying which procedure is in progress. */
    MESH_SEC_RESET_IN_PROGRESS(pKeyInfo->hdr.flags);

    /* Reset request slot. */
    pReq->netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

    /* Invoke user callback. */
    pReq->cback(MESH_SEC_KEY_TYPE_NWK, netKeyIndex, isSuccess,
                pReq->isUpdate, pReq->pParam);
  }
  (void)resultSize;
}

/*************************************************************************************************/
/*!
 *  \brief     K3 callback implementation for handling generated Network ID as part of the state
 *             machine for Network Key derivation.
 *
 *  \param[in] pResult     Pointer to K3 result.
 *  \param[in] resultSize  Size of the result in bytes.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecNwkIdDerivCback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  static uint8_t          k2PBuff[MESH_SEC_K2_P_MASTER_SIZE] = { 0 };
  meshSecNetKeyDerivReq_t *pReq;
  meshSecNetKeyInfo_t     *pKeyInfo;
  uint16_t                netKeyIndex;
  uint8_t                 entryId;
  bool_t                  isSuccess = FALSE;

  /* Should never happen. */
  WSF_ASSERT((resultSize == MESH_SEC_TOOL_K3_RESULT_SIZE) || (pResult == NULL));

  /* Recover request. */
  pReq = (meshSecNetKeyDerivReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->netKeyListIdx == MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return;
  }

  /* Recover key information. */
  pKeyInfo = &secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx];

  /* Get material entry identifier. */
  entryId = pReq->isUpdate == FALSE ? (pKeyInfo->hdr.crtKeyId) : (1 - pKeyInfo->hdr.crtKeyId);

  /* Store key index in case delete is pending. */
  netKeyIndex = pKeyInfo->hdr.keyIndex;

  if ((pResult == NULL) || (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE))
  {
    /* Check if Key Material is subject to removal. */
    if (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE)
    {
      /* Handle removal. */
      meshSecRemoveNetKeyMaterial(pReq->netKeyListIdx);
    }
  }
  else
  {
    /* Copy result. */
    memcpy(pKeyInfo->keyMaterial[entryId].networkID, pResult, MESH_SEC_TOOL_K3_RESULT_SIZE);

    /* Set k2 P buffer to 0 as master credentials are derived. */
    k2PBuff[0] = 0;

    /* Send next request. */
    if (MeshSecToolK2Derive(k2PBuff, sizeof(k2PBuff), secTempData.nwkKey,
                            meshSecPduSecDerivCback, pParam)
        == MESH_SUCCESS)
    {
      isSuccess = TRUE;
    }
  }

  if (!isSuccess)
  {
    /* Reset in progress flags. */
    MESH_SEC_RESET_IN_PROGRESS(pKeyInfo->hdr.flags);

    /* Reset request slot. */
    pReq->netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

    /* Invoke user callback. */
    pReq->cback(MESH_SEC_KEY_TYPE_NWK, netKeyIndex, isSuccess,
                pReq->isUpdate, pReq->pParam);
  }
  (void)resultSize;
}

/*************************************************************************************************/
/*!
 *  \brief     K1 callback implementation for handling generated Beacon Key as part of the state
 *             machine for Network Key derivation.
 *
 *  \param[in] pResult     Pointer to K1 result.
 *  \param[in] resultSize  Size of the result in bytes.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecBkDerivCback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  meshSecNetKeyDerivReq_t *pReq;
  meshSecNetKeyInfo_t     *pKeyInfo;
  uint16_t                netKeyIndex;
  uint8_t                 entryId;
  bool_t                  isSuccess = FALSE;

  /* Should never happen. */
  WSF_ASSERT((resultSize == MESH_SEC_TOOL_K1_RESULT_SIZE) || (pResult == NULL));

  /* Recover request. */
  pReq = (meshSecNetKeyDerivReq_t *)pParam;

  /* Check if module is re-initialised. */
  if (pReq->netKeyListIdx == MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return;
  }

  /* Recover key information. */
  pKeyInfo = &secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx];

  /* Get material entry identifier. */
  entryId = pReq->isUpdate == FALSE ? (pKeyInfo->hdr.crtKeyId) : (1 - pKeyInfo->hdr.crtKeyId);

  /* Store key index in case delete is pending. */
  netKeyIndex = pKeyInfo->hdr.keyIndex;

  if ((pResult == NULL) || (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE))
  {
    /* Check if Key Material is subject to removal. */
    if (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE)
    {
      /* Handle removal. */
      meshSecRemoveNetKeyMaterial(pReq->netKeyListIdx);
    }
  }
  else
  {
    /* Copy result. */
    memcpy(pKeyInfo->keyMaterial[entryId].beaconKey, pResult, MESH_SEC_TOOL_K1_RESULT_SIZE);

    /* Send next request. */
    if (MeshSecToolK3Derive(secTempData.nwkKey, meshSecNwkIdDerivCback, pParam) == MESH_SUCCESS)
    {
      isSuccess = TRUE;
    }
  }

  if (!isSuccess)
  {
    /* Reset in progress flags. */
    MESH_SEC_RESET_IN_PROGRESS(pKeyInfo->hdr.flags);

    /* Reset request slot. */
    pReq->netKeyListIdx = MESH_SEC_INVALID_KEY_INDEX;

    /* Invoke user callback. */
    pReq->cback(MESH_SEC_KEY_TYPE_NWK, netKeyIndex, isSuccess,
                pReq->isUpdate, pReq->pParam);
  }
  (void)resultSize;
}

/*************************************************************************************************/
/*!
 *  \brief     K1 callback implementation for handling generated Identity Key as part of the state
 *             machine for Network Key derivation.
 *
 *  \param[in] pResult     Pointer to K1 result.
 *  \param[in] resultSize  Size of the result in bytes.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecIkDerivCback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  meshSecNetKeyDerivReq_t *pReq;
  meshSecNetKeyInfo_t     *pKeyInfo;
  uint16_t                netKeyIndex;
  uint8_t                 entryId;
  bool_t                  isSuccess = FALSE;

  /* Should never happen. */
  WSF_ASSERT((resultSize == MESH_SEC_TOOL_K1_RESULT_SIZE) || (pResult == NULL));

  /* Recover request. */
  pReq = (meshSecNetKeyDerivReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->netKeyListIdx == MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return;
  }

  /* Recover key information. */
  pKeyInfo = &secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx];

  /* Get material entry identifier. */
  entryId = pReq->isUpdate == FALSE ? (pKeyInfo->hdr.crtKeyId) : (1 - pKeyInfo->hdr.crtKeyId);

  /* Store key index in case delete is pending. */
  netKeyIndex = pKeyInfo->hdr.keyIndex;

  if ((pResult == NULL) || (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE))
  {
    /* Check if key material is subject to removal. */
    if (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE)
    {
      /* Handle removal. */
      meshSecRemoveNetKeyMaterial(pReq->netKeyListIdx);
    }
  }
  else
  {
    /* Copy result. */
    memcpy(pKeyInfo->keyMaterial[entryId].identityKey, pResult, MESH_SEC_TOOL_K1_RESULT_SIZE);

    /* Send next request. */
    if (MeshSecToolK1Derive((uint8_t *)secId128Str,
                            sizeof(secId128Str),
                            (uint8_t *)secS1NkbkStr,
                            secTempData.nwkKey,
                            MESH_KEY_SIZE_128,
                            meshSecBkDerivCback, pParam)
        == MESH_SUCCESS)
    {
      isSuccess = TRUE;
    }
  }

  if (!isSuccess)
  {
    /* Reset in progress flags. */
    MESH_SEC_RESET_IN_PROGRESS(pKeyInfo->hdr.flags);

    /* Reset request slot. */
    pReq->netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

    /* Invoke user callback. */
    pReq->cback(MESH_SEC_KEY_TYPE_NWK, netKeyIndex, isSuccess,
                pReq->isUpdate, pReq->pParam);
  }
  (void)resultSize;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Network key derivation request.
 *
 *  \param[in] netKeyIndex  Global identifier of the key.
 *  \param[in] isUpdate     TRUE if derivation reason is key update.
 *  \param[in] cback        Callback to be invoked at the end of the procedure.
 *  \param[in] pParam       Generic parameter provided in the request.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecHandleNetKeyDerivation(uint16_t netKeyIndex, bool_t isUpdate,
                                                     meshSecKeyMaterialDerivCback_t cback,
                                                     void *pParam)
{
  meshSecNetKeyInfo_t  *pNetKeyInfo = NULL;
  meshSecToolRetVal_t  retVal       = MESH_SUCCESS;
  uint16_t             idx;

  /* Read key from local config and also validate NetKey Index. */
  if (isUpdate)
  {
    if (MeshLocalCfgGetUpdatedNetKey(netKeyIndex, secTempData.nwkKey) != MESH_SUCCESS)
    {
      return MESH_SEC_KEY_NOT_FOUND;
    }
  }
  else
  {
    if (MeshLocalCfgGetNetKey(netKeyIndex, secTempData.nwkKey) != MESH_SUCCESS)
    {
      return MESH_SEC_KEY_NOT_FOUND;
    }
  }

  /* Search the network key information array. */
  for (idx = 0; idx < secMatLocals.netKeyInfoListSize; idx++)
  {
    /* Check if same NetKey index exists. */
    if ((secMatLocals.pNetKeyInfoArray[idx].hdr.keyIndex == netKeyIndex) &&
        (secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      /* Check if an updated key has been added for the NetKeyIndex. */
      if (isUpdate)
      {
        /* Check if there is already updated key material. */
        if (secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
        {
          return MESH_SEC_KEY_MATERIAL_EXISTS;
        }
        /* Store entry and finish searching. */
        pNetKeyInfo = &(secMatLocals.pNetKeyInfoArray[idx]);
        pNetKeyInfo->hdr.flags |= MESH_SEC_KEY_UPDT_IN_PROGRESS;
        break;
      }
      else
      {
        /* Key material already exists. */
        return MESH_SEC_KEY_MATERIAL_EXISTS;
      }
    }
    /* Check if there is an empty entry for the Network Key derivation material. */
    else if ((secMatLocals.pNetKeyInfoArray[idx].hdr.flags == MESH_SEC_KEY_UNUSED) && (!isUpdate))
    {
      /* Store slot and configure key header information. */
      pNetKeyInfo = &(secMatLocals.pNetKeyInfoArray[idx]);
      pNetKeyInfo->hdr.keyIndex = netKeyIndex;
      pNetKeyInfo->hdr.crtKeyId = 0;
      pNetKeyInfo->hdr.flags |= MESH_SEC_KEY_CRT_IN_PROGESS;
      break;
    }
  }
  /* Check if previous search did not find any entry. */
  if (idx == secMatLocals.netKeyInfoListSize)
  {
    if (isUpdate)
    {
      return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
    }

    /* This should never happen since number of keys is in sync with key material, but guard
     * anyway.
     */
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Set request parameters. */
  secKeyDerivReq.netKeyDerivReq.cback         = cback;
  secKeyDerivReq.netKeyDerivReq.pParam        = pParam;
  secKeyDerivReq.netKeyDerivReq.netKeyListIdx = idx;
  secKeyDerivReq.netKeyDerivReq.isUpdate      = isUpdate;

  /* Start derivation with Identity Key generation. */
  retVal = MeshSecToolK1Derive((uint8_t *)secId128Str,
                               sizeof(secId128Str),
                               (uint8_t *)secS1NkikStr,
                               secTempData.nwkKey,
                               MESH_KEY_SIZE_128,
                               meshSecIkDerivCback,
                               (void *)&(secKeyDerivReq.netKeyDerivReq));

  if (retVal != MESH_SUCCESS)
  {
    /* Reset in progress flags. */
    /* Note:  pNetKeyInfo is NULL if (idx == secMatLocals.netKeyInfoListSize), but this is checked
     * above and function will return with error status thus precluding a NULL derefence from
     * occurring here.
     */
    /* coverity[var_deref_op] */
    MESH_SEC_RESET_IN_PROGRESS(pNetKeyInfo->hdr.flags);

    /* Reset request. */
    secKeyDerivReq.netKeyDerivReq.netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     K2 callback implementation for handling PDU security material (NID, Ek, Pk) as part
 *             of the state machine for adding friendship credentials.
 *
 *  \param[in] pResult     Pointer to K2 result.
 *  \param[in] resultSize  Size of the result in bytes.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static void meshSecFriendCredAddCback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  meshSecFriendDerivReq_t *pReq;
  meshSecNetKeyInfo_t     *pKeyInfo;
  meshSecFriendMat_t      *pFriendMat;
  uint8_t                 entryId;
  bool_t                  isSuccess    = FALSE;
  bool_t                  procComplete = FALSE;

  /* Should never happen. */
  WSF_ASSERT(resultSize == MESH_SEC_TOOL_K2_RESULT_SIZE);

  /* Recover request. */
  pReq = (meshSecFriendDerivReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->friendListIdx == MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return;
  }

  /* Recover key information. */
  pKeyInfo = &secMatLocals.pNetKeyInfoArray[pReq->netKeyListIdx];

  /* Recover empty friendship material entry. */
  pFriendMat = &secMatLocals.pFriendMatArray[pReq->friendListIdx];

  /* Get material entry identifier. */
  entryId = (pReq->doUpdate == FALSE) ? pKeyInfo->hdr.crtKeyId : (1 - pKeyInfo->hdr.crtKeyId);

  /* Check if procedure failed or key material is removed or credentials are removed. */
  if ((pResult == NULL) ||
      ((pKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE) == 0) ||
      ((pFriendMat->netKeyInfoIndex != MESH_SEC_FRIEND_ENTRY_BUSY_IDX)))
  {
    procComplete = TRUE;
  }
  else
  {
    /* Copy NID. */
    pFriendMat->keyMaterial[entryId].nid =
      MESH_UTILS_BF_GET(pResult[0], MESH_NID_SHIFT, MESH_NID_SIZE);
    /* Copy Ek. */
    memcpy(pFriendMat->keyMaterial[entryId].encryptKey, &pResult[1], MESH_KEY_SIZE_128);
    /* Copy Pk. */
    memcpy(pFriendMat->keyMaterial[entryId].privacyKey, &pResult[1 + MESH_KEY_SIZE_128],
           MESH_KEY_SIZE_128);

    if (!pReq->doUpdate)
    {
      /* Check if there is an updated key. */
      if (pKeyInfo->hdr.flags & (MESH_SEC_KEY_UPDT_IN_PROGRESS | MESH_SEC_KEY_UPDT_MAT_AVAILABLE))
      {
        /* Mark second operation as in progress. */
        pReq->doUpdate = TRUE;

        /* Read new key. This should never fail as there is another procedure using it. */
        if (MeshLocalCfgGetUpdatedNetKey(pKeyInfo->hdr.keyIndex, secTempData.nwkKeyFriend)
            == MESH_SUCCESS)
        {
          /* Call K2 derivation with friendship credentials. */
          if (MeshSecToolK2Derive(pReq->pK2PBuff,
                                  MESH_SEC_K2_P_FRIEND_SIZE,
                                  secTempData.nwkKeyFriend,
                                  meshSecFriendCredAddCback, (void *)pParam) != MESH_SUCCESS)
          {
            procComplete = TRUE;
          }
        }
        else
        {
          /* Trap if it does fail. */
          WSF_ASSERT(MeshLocalCfgGetUpdatedNetKey(pKeyInfo->hdr.keyIndex, secTempData.nwkKeyFriend)
                     == MESH_SUCCESS);

          procComplete = TRUE;
        }
      }
      else
      {
        procComplete = TRUE;
        isSuccess    = TRUE;
      }
    }
    else
    {
      procComplete = TRUE;
      isSuccess    = TRUE;
    }
  }

  /* Check if procedure continues with friendship material update.
   * Successful operation is marked by isSuccess value.
   */
  if (procComplete)
  {
    if (isSuccess)
    {
      /* Set friendship material as valid by assigning the valid NetKey Index. */
      pFriendMat->netKeyInfoIndex = pReq->netKeyListIdx;

      /* Mark updated material available or not based on number of K2's performed successfully. */
      pFriendMat->hasUpdtMaterial = pReq->doUpdate;
    }
    else
    {
      /* Reset friendship entry by setting invalid NetKey Index. */
      pFriendMat->netKeyInfoIndex = MESH_SEC_INVALID_ENTRY_INDEX;
    }

    /* Reset request slot. */
    pReq->friendListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

    /* Invoke user callback. */
    pReq->cback(pFriendMat->friendAddres, pFriendMat->lpnAddress, pReq->netKeyIndex, isSuccess,
                pReq->pParam);
  }
  (void)resultSize;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes key material derived from an Application Key and frees entry.
 *
 *  \param[in] appKeyEntryIdx  Index into the Application Key information list.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static inline void meshSecRemoveAppKeyMaterial(uint16_t appKeyEntryIdx)
{
  /* Clear the material available flag. */
  secMatLocals.pAppKeyInfoArray[appKeyEntryIdx].hdr.flags = 0;

  /* Clear the AppKey Index. */
  secMatLocals.pAppKeyInfoArray[appKeyEntryIdx].hdr.keyIndex = MESH_SEC_INVALID_KEY_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     K4 callback implementation for handling generated AID as part of the state machine
 *             for Application Key derivation.
 *
 *  \param[in] pResult     Pointer to K4 result.
 *  \param[in] resultSize  Size of the result in bytes.
 *  \param[in] pParam      Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecAppKeyDerivCback(const uint8_t *pResult, uint8_t resultSize, void *pParam)
{
  meshSecAppKeyDerivReq_t *pReq;
  meshSecAppKeyInfo_t     *pKeyInfo;
  uint16_t                appKeyIndex;
  uint8_t                 entryId;
  bool_t                  isSuccess = FALSE;

  /* Should never happen. */
  WSF_ASSERT(resultSize == MESH_SEC_TOOL_K4_RESULT_SIZE);

  /* Recover request. */
  pReq = (meshSecAppKeyDerivReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->appKeyListIdx == MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return;
  }

  /* Recover key information. */
  pKeyInfo = &secMatLocals.pAppKeyInfoArray[pReq->appKeyListIdx];

  /* Get material entry identifier. */
  entryId = pReq->isUpdate == FALSE ? (pKeyInfo->hdr.crtKeyId) : (1 - pKeyInfo->hdr.crtKeyId);

  /* Store key index in case delete occurs. */
  appKeyIndex = pKeyInfo->hdr.keyIndex;

  if ((pResult == NULL) || (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE))
  {
    /* Check if Key Material is subject to removal. */
    if (pKeyInfo->hdr.flags & MESH_SEC_KEY_ALL_DELETE)
    {
      /* Handle removal. */
      meshSecRemoveAppKeyMaterial(pReq->appKeyListIdx);
    }
  }
  else
  {
    /* Copy AID. */
    pKeyInfo->keyMaterial[entryId].aid =
      MESH_UTILS_BF_GET(pResult[0], MESH_AID_SHIFT, MESH_AID_SIZE);

    /* Check if operation is key update. */
    if (pReq->isUpdate)
    {
      /* Check delete current only flag indicating key refresh phase is complete and
       * current key material is now the updated material.
       */
      if (pKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_DELETE)
      {
        /* Advance current key index. */
        pKeyInfo->hdr.crtKeyId = entryId;

        /* Clear delete current material flag. */
        pKeyInfo->hdr.flags &= ~MESH_SEC_KEY_CRT_MAT_DELETE;
      }
      else
      {
        /* Update operation completed. Mark keys as available. */
        pKeyInfo->hdr.flags |= MESH_SEC_KEY_UPDT_MAT_AVAILABLE;
      }
    }
    else
    {
      /* Set material available flag and finish. */
      pKeyInfo->hdr.flags |= MESH_SEC_KEY_CRT_MAT_AVAILABLE;
    }

    isSuccess = TRUE;
  }

  /* Clear all "in progress" flags to avoid verifying which procedure is in progress. */
  MESH_SEC_RESET_IN_PROGRESS(pKeyInfo->hdr.flags);

  /* Reset request slot. */
  pReq->appKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

  /* Invoke user callback. */
  pReq->cback(MESH_SEC_KEY_TYPE_APP, appKeyIndex, isSuccess,
              pReq->isUpdate, pReq->pParam);
  (void)resultSize;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an Application Key derivation request.
 *
 *  \param[in] appKeyIndex  Global identifier for the key.
 *  \param[in] isUpdate     TRUE if the derivation reason is key update.
 *  \param[in] cback        Callback to be invoked at the end of the procedure.
 *  \param[in] pParam       Generic parameter provided in the request.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecHandleAppKeyDerivation(uint16_t appKeyIndex, bool_t isUpdate,
                                                     meshSecKeyMaterialDerivCback_t cback,
                                                     void *pParam)
{
  meshSecAppKeyInfo_t  *pAppKeyInfo = NULL;
  uint16_t             idx;
  meshSecToolRetVal_t  retVal = MESH_SUCCESS;

  /* Read key from local config and also validate AppKey Index. */
  if (isUpdate)
  {
    if (MeshLocalCfgGetUpdatedAppKey(appKeyIndex, secTempData.appKey) != MESH_SUCCESS)
    {
      return MESH_SEC_KEY_NOT_FOUND;
    }
  }
  else
  {
    if (MeshLocalCfgGetAppKey(appKeyIndex, secTempData.appKey) != MESH_SUCCESS)
    {
      return MESH_SEC_KEY_NOT_FOUND;
    }
  }

  /* Search the Application Key information array. */
  for (idx = 0; idx < secMatLocals.appKeyInfoListSize; idx++)
  {
    /* Check if same AppKey index exists. */
    if ((secMatLocals.pAppKeyInfoArray[idx].hdr.keyIndex == appKeyIndex) &&
        (secMatLocals.pAppKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      /* Check if an updated key has been added for the AppKey index. */
      if (isUpdate)
      {
        /* Check if there is already updated key material. */
        if (secMatLocals.pAppKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
        {
          return MESH_SEC_KEY_MATERIAL_EXISTS;
        }
        /* Store entry and finish searching. */
        pAppKeyInfo = &(secMatLocals.pAppKeyInfoArray[idx]);
        pAppKeyInfo->hdr.flags |= MESH_SEC_KEY_UPDT_IN_PROGRESS;
        break;
      }
      else
      {
        /* Key material already exists. */
        return MESH_SEC_KEY_MATERIAL_EXISTS;
      }
    }
    /* Check if there is an empty entry for the Application Key. */
  else if ((secMatLocals.pAppKeyInfoArray[idx].hdr.flags == MESH_SEC_KEY_UNUSED) && (!isUpdate))
    {
      /* Store slot and configure key header information. */
      pAppKeyInfo = &(secMatLocals.pAppKeyInfoArray[idx]);
      pAppKeyInfo->hdr.keyIndex = appKeyIndex;
      pAppKeyInfo->hdr.crtKeyId = 0;
      pAppKeyInfo->hdr.flags |= MESH_SEC_KEY_CRT_IN_PROGESS;
      break;
    }
  }
  /* Check if previous search did not find any entry. */
  if (idx == secMatLocals.appKeyInfoListSize || pAppKeyInfo == NULL)
  {
    if (isUpdate)
    {
      return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
    }

    /* This should never happen since number of keys is in sync with key material, but guard
     * anyway.
     */
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Store request parameters and Application Key info address. */
  secKeyDerivReq.appKeyDerivReq.cback = cback;
  secKeyDerivReq.appKeyDerivReq.pParam   = pParam;
  secKeyDerivReq.appKeyDerivReq.appKeyListIdx = idx;
  secKeyDerivReq.appKeyDerivReq.isUpdate = isUpdate;

  /* Start derivation for AID. */
  retVal = MeshSecToolK4Derive(secTempData.appKey,
                               meshSecAppKeyDerivCback,
                               (void *)&(secKeyDerivReq.appKeyDerivReq));

  if (retVal != MESH_SUCCESS)
  {
    /* Reset in progress flags. */
    MESH_SEC_RESET_IN_PROGRESS(pAppKeyInfo->hdr.flags);

    /* Reset request. */
    secKeyDerivReq.appKeyDerivReq.appKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security key material derivation complete callback.
 *
 *  \param[in] keyType     The type of the key.
 *  \param[in] keyIndex    Global key index for application and network key types.
 *  \param[in] keyUpdated  TRUE if key material was added for the new key of an existing key index.
 *  \param[in] isSuccess   TRUE if operation is successful.
 *  \param[in] pParam      Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 *
 *  \see meshSecKeyType_t
 */
/*************************************************************************************************/
static void meshSecRestoreKeyDerivCback(meshSecKeyType_t keyType, uint16_t keyIndex,
                                        bool_t isSuccess, bool_t keyUpdated, void *pParam)
{
  /* On fail, terminate */
  if(!isSuccess)
  {
    meshSecCb.restoreCback(FALSE);
    return;
  }

  /* Check if key is NetKey. */
  if(keyType == MESH_SEC_KEY_TYPE_NWK)
  {
    /* Check if the updated key was derived or needs deriving. */
    if((!keyUpdated) &&
       (meshSecHandleNetKeyDerivation(keyIndex, TRUE, meshSecRestoreKeyDerivCback, pParam)
        == MESH_SUCCESS))
    {
      return;
    }

    /* Either the key was updated or there is no updated key. Move to next. */
    if(MeshLocalCfgGetNextNetKeyIndex(&keyIndex, (uint16_t *)pParam) != MESH_SUCCESS)
    {
      /* Reset indexer and move to AppKeys. */
      *((uint16_t *)pParam) = 0;

      /* Get first AppKey Index. */
      if(MeshLocalCfgGetNextAppKeyIndex(&keyIndex, (uint16_t *)pParam) != MESH_SUCCESS)
      {
        /* Finish here. No AppKeys. */
        meshSecCb.restoreCback(TRUE);
      }
      /* Derive the AppKey material */
      else if(meshSecHandleAppKeyDerivation(keyIndex, FALSE, meshSecRestoreKeyDerivCback, pParam)
              != MESH_SUCCESS)
      {
        /* Finish here with error. */
        meshSecCb.restoreCback(FALSE);
      }
    }
    /* Derive next NetKey index. */
    else if(meshSecHandleNetKeyDerivation(keyIndex, FALSE, meshSecRestoreKeyDerivCback, pParam)
            != MESH_SUCCESS)
    {
      /* Finish here with error. */
      meshSecCb.restoreCback(FALSE);
    }
  }
  /* Check if key is AppKey. */
  else
  {
    /* Check if the updated key was derived. */
    if((!keyUpdated) &&
       (meshSecHandleAppKeyDerivation(keyIndex, TRUE, meshSecRestoreKeyDerivCback, pParam)
        == MESH_SUCCESS))
    {
      return;
    }

    /* Get next AppKey Index. */
    if(MeshLocalCfgGetNextAppKeyIndex(&keyIndex, (uint16_t *)pParam) != MESH_SUCCESS)
    {
      /* Finish here. No AppKeys left. */
      meshSecCb.restoreCback(TRUE);
    }
    /* Derive the AppKey material */
    else if(meshSecHandleAppKeyDerivation(keyIndex, FALSE, meshSecRestoreKeyDerivCback, pParam)
            != MESH_SUCCESS)
    {
      /* Finish here with error. */
      meshSecCb.restoreCback(FALSE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Security Material derived from the Network Key
 *
 *  \param[in] netKeyIndex  Netkey index.
 *
 *  \return    Pointer to Security Material, or NULL if not found.
 */
/*************************************************************************************************/
static meshSecNetKeyMaterial_t *meshSecNetKeyIndexToNetKeyMaterial(uint16_t netKeyIndex)
{
  meshSecNetKeyInfo_t *pKeyInfo = NULL;
  uint16_t idx;
  uint8_t                entryId   = MESH_SEC_KEY_MAT_PER_INDEX;
  meshKeyRefreshStates_t state;

  /* Validate parameters. */
  if (netKeyIndex > MESH_SEC_MAX_KEY_INDEX)
  {
    return NULL;
  }

  /* Search for matching NetKey Index. */
  for (idx = 0; idx < secMatLocals.netKeyInfoListSize; idx++)
  {
    if ((secMatLocals.pNetKeyInfoArray[idx].hdr.keyIndex == netKeyIndex) &&
        (secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      pKeyInfo = &(secMatLocals.pNetKeyInfoArray[idx]);
      break;
    }
  }

  /* Check if result is successful. */
  if (idx == secMatLocals.netKeyInfoListSize || pKeyInfo == NULL)
  {
    return NULL;
  }

  /* Read Key refresh state. */
  state = MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex);

  /* Decide which entry in the key material to use based on key refresh state. */
  switch (state)
  {
    case MESH_KEY_REFRESH_NOT_ACTIVE: /* Fallthrough */
    case MESH_KEY_REFRESH_FIRST_PHASE:
      entryId = pKeyInfo->hdr.crtKeyId;
      break;
    case MESH_KEY_REFRESH_SECOND_PHASE:
      if (pKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
      {
        entryId = 1 - pKeyInfo->hdr.crtKeyId;
      }
      break;
    default:
      break;
  }

  if (entryId == MESH_SEC_KEY_MAT_PER_INDEX)
  {
    return NULL;
  }

  /* Return result. */
  return &pKeyInfo->keyMaterial[entryId];
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Derives and stores Application or Network Key material.
 *
 *  \param[in] keyType                Type of the key for which derivation is requested.
 *  \param[in] keyIndex               Global key index for application or network keys.
 *  \param[in] isUpdate               Indicates if derivation material is needed for a new key
 *                                    of the same global key index. Used during Key Refresh
 *                                    Procedure transitions (add new key).
 *  \param[in] keyMaterialAddedCback  Callback invoked after key material is stored in the module.
 *  \param[in] pParam                 Pointer to generic parameter for the callback.
 *
 *  \retval    MESH_SUCCESS                     Key derivation started.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters.
 *  \retval    MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *  \retval    MESH_SEC_KEY_NOT_FOUND           An update is requested for a key index that is
 *                                              not stored.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  An update is requested for a key index that has no
 *                                              key material derived from the old key.
 *  \retval    MESH_SEC_KEY_MATERIAL_EXISTS     The key derivation material already exists.
 *
 *  \remarks   For key update procedures, this function also updates the friendship security
 *             credentials that already exist for a specific network key.
 *
 *  \see meshSecKeyType_t
 *  \see meshSecKeyMaterialDerivCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecAddKeyMaterial(meshSecKeyType_t keyType, uint16_t keyIndex, bool_t isUpdate,
                                      meshSecKeyMaterialDerivCback_t keyMaterialAddedCback,
                                      void *pParam)
{
  /* Validate parameters. */
  if (((keyType != MESH_SEC_KEY_TYPE_NWK) && (keyType != MESH_SEC_KEY_TYPE_APP)) ||
      (keyIndex > MESH_SEC_MAX_KEY_INDEX) ||
      (keyMaterialAddedCback == NULL))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Check if procedure is in progress. */
  if ((keyType == MESH_SEC_KEY_TYPE_NWK &&
       secKeyDerivReq.netKeyDerivReq.netKeyListIdx != MESH_SEC_INVALID_ENTRY_INDEX) ||
      (keyType == MESH_SEC_KEY_TYPE_APP &&
       secKeyDerivReq.appKeyDerivReq.appKeyListIdx != MESH_SEC_INVALID_ENTRY_INDEX))
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  if (keyType == MESH_SEC_KEY_TYPE_NWK)
  {
    /* Handle Network Key Derivation. */
    return meshSecHandleNetKeyDerivation(keyIndex, isUpdate, keyMaterialAddedCback, pParam);
  }

  /* Handle Application Key Derivation. */
  return meshSecHandleAppKeyDerivation(keyIndex, isUpdate, keyMaterialAddedCback, pParam);
}

/*************************************************************************************************/
/*!
 *  \brief   Derives and stores key material for all keys stored in Local Config.
 *
 *  \return  None.
 *
 *  \see     meshSecAllKeyMaterialRestoreCback_t
 *  \remarks This function should be called only once after Local Config restores the keys.
 */
/*************************************************************************************************/
void MeshSecRestoreAllKeyMaterial(meshSecAllKeyMaterialRestoreCback_t restoreCback)
{
  static uint16_t indexer = 0;
  uint16_t netKeyIndex;

  WSF_ASSERT(restoreCback != NULL);

  /* Set user restore callback. */
  meshSecCb.restoreCback = restoreCback;

  /* Initialize indexer for deriving NetKeys */
  indexer = 0;

  /* Start derivation of NetKeys. At least one key should always exist. */
  if((MeshLocalCfgGetNextNetKeyIndex(&netKeyIndex, &indexer) != MESH_SUCCESS) ||
     (meshSecHandleNetKeyDerivation(netKeyIndex, FALSE, meshSecRestoreKeyDerivCback,
                                    (void *)&indexer)
      != MESH_SUCCESS))
  {
    meshSecCb.restoreCback(FALSE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Removes key derivation material based on keyType and key index.
 *
 *  \param[in] keyType        Type of the key for which key material must be removed.
 *  \param[in] keyIndex       Global key index for application or network keys.
 *  \param[in] deleteOldOnly  Indicates if only old key derivation material is removed. Used
 *                            during Key Refresh Procedure transitions (back to normal operation).
 *
 *  \retval    MESH_SUCCESS                     Key material removed as configured by the
 *                                              parameters.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters. See the Remarks section.
 *  \retval    MESH_SEC_KEY_NOT_FOUND           The module has no material for the specified global
 *                                              key index.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  Caller requests to delete the old key material only,
 *                                              but there is only one set of key derivation
 *                                              material.
 *
 *  \remarks This function cleans up all security materials for a specific key.
 *           For network keys, to remove only the friendship security material use
 *           MeshSecRemoveFriendCredentials. If deleteOldOnly is TRUE, this function also updates
 *           the friendship security credentials that already exist for a specific network key.
 *
 *  \see meshSecKeyType_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecRemoveKeyMaterial(meshSecKeyType_t keyType, uint16_t keyIndex,
                                         bool_t deleteOldOnly)
{
  meshSecKeyInfoHdr_t *pHdr;
  uint16_t maxElements;
  uint16_t idx;


  if (((keyType != MESH_SEC_KEY_TYPE_NWK) && (keyType != MESH_SEC_KEY_TYPE_APP)) ||
       (keyIndex > MESH_SEC_MAX_KEY_INDEX))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Get number of elements in target array based on key type. */
  maxElements = (keyType == MESH_SEC_KEY_TYPE_NWK) ? secMatLocals.netKeyInfoListSize :
                                                     secMatLocals.appKeyInfoListSize;
  for (idx = 0; idx < maxElements; idx++)
  {
    pHdr = (keyType == MESH_SEC_KEY_TYPE_NWK) ? &(secMatLocals.pNetKeyInfoArray[idx].hdr) :
                                                &(secMatLocals.pAppKeyInfoArray[idx].hdr);
    /* If index does not match, search the next one. */
    if (pHdr->keyIndex != keyIndex)
    {
      continue;
    }

    if (deleteOldOnly)
    {
      /* Check if update material is available. */
      if (pHdr->flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE)
      {
        /* Current material identifier moves to new material entry. */
        pHdr->crtKeyId = 1 - pHdr->crtKeyId;

        /* Clear update material available flag. */
        pHdr->flags &= ~MESH_SEC_KEY_UPDT_MAT_AVAILABLE;

        if (keyType == MESH_SEC_KEY_TYPE_NWK)
        {
          /* Clear update flag for friendship material. */
          meshSecClearFriendUpdtFlag(idx);
        }
        return MESH_SUCCESS;
      }

      /* Check if an update is in progress. */
      if (pHdr->flags & MESH_SEC_KEY_UPDT_IN_PROGRESS)
      {
        /* Signal procedure in progress to also modify the material identifier. */
        pHdr->flags |= MESH_SEC_KEY_CRT_MAT_DELETE;

        return MESH_SUCCESS;
      }

      return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
    }
    else
    {
      /* Check if material is available. */
      if (pHdr->flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE)
      {
        /* Check if an update procedure is pending. */
        if (pHdr->flags & MESH_SEC_KEY_UPDT_IN_PROGRESS)
        {
          /* Signal procedure in progress to also remove the material. */
          pHdr->flags |= MESH_SEC_KEY_ALL_DELETE;
        }
        else
        {
          if (keyType == MESH_SEC_KEY_TYPE_NWK)
          {
            /* Remove material. */
            meshSecRemoveNetKeyMaterial(idx);
          }
          else
          {
            /* Remove material. */
            meshSecRemoveAppKeyMaterial(idx);
          }
        }
        return MESH_SUCCESS;
      }

      /* Check if material is available. */
      if (pHdr->flags & MESH_SEC_KEY_CRT_IN_PROGESS)
      {
        /* Signal procedure in progress to also remove the material. */
        pHdr->flags |= MESH_SEC_KEY_ALL_DELETE;

        return MESH_SUCCESS;
      }
    }
  }

  /* Search returned no results. */
  return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Generates and adds friendship credentials to network key derivation material.
 *
 *  \param[in] pFriendshipCred  Pointer to friendship credentials material information.
 *  \param[in] friendCredAdded  Callback used for procedure complete notification.
 *  \param[in] pParam           Pointer to generic parameter for the callback.
 *
 *  \retval    MESH_SUCCESS                     Friendship material derivation started.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters.
 *  \retval    MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no existing entry for the provided network
 *                                              key index.
 *  \retval    MESH_SEC_KEY_MATERIAL_EXISTS     The friendship credentials already exist.
 *
 *  \remarks   This function must be called when a friendship is established.
 *
 *  \see meshSecKeyType_t
 *  \see meshSecFriendCredDerivCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecAddFriendCred(const meshSecFriendshipCred_t *pFriendshipCred,
                                     meshSecFriendCredDerivCback_t friendCredAdded,
                                     void *pParam)
{
  static uint8_t k2PBuff[MESH_SEC_K2_P_FRIEND_SIZE] = { 0 };

  uint8_t *pK2PBuff;
  meshSecFriendMat_t *pFriendMat;
  uint16_t idx;
  meshSecToolRetVal_t retVal = MESH_SUCCESS;

  /* Validate parameters. */
  if ((pFriendshipCred == NULL) ||
      (friendCredAdded == NULL) ||
      (pFriendshipCred->netKeyIndex > MESH_SEC_MAX_KEY_INDEX) ||
      (!MESH_IS_ADDR_UNICAST(pFriendshipCred->friendAddres)) ||
      (!MESH_IS_ADDR_UNICAST(pFriendshipCred->lpnAddress)))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Check if another request is in progress. */
  if (secKeyDerivReq.friendMatDerivReq.friendListIdx != MESH_SEC_INVALID_ENTRY_INDEX)
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Search valid network derivation material for corresponding NetKey Index. */
  for (idx = 0; idx < secMatLocals.netKeyInfoListSize; idx++)
  {
    if ((secMatLocals.pNetKeyInfoArray[idx].hdr.keyIndex == pFriendshipCred->netKeyIndex) &&
        (secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      break;
    }
  }

  /* Check if no entry was found. */
  if (idx == secMatLocals.netKeyInfoListSize)
  {
    return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
  }

  /* Store key information list index. */
  secKeyDerivReq.friendMatDerivReq.netKeyListIdx = idx;

  /* Store NetKey index for user. */
  secKeyDerivReq.friendMatDerivReq.netKeyIndex = pFriendshipCred->netKeyIndex;

  /* Check if this request is a duplicate. */
  for (idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    /* Get element. */
    pFriendMat = &secMatLocals.pFriendMatArray[idx];

    /* Verify duplicate. */
    if ((pFriendMat->friendAddres == pFriendshipCred->friendAddres) &&
        (pFriendMat->lpnAddress == pFriendshipCred->lpnAddress) &&
        (pFriendMat->netKeyInfoIndex == secKeyDerivReq.friendMatDerivReq.netKeyListIdx))
    {
      return MESH_SEC_KEY_MATERIAL_EXISTS;
    }
  }

  /* Search empty friendship material slot. */
  for (idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    if (secMatLocals.pFriendMatArray[idx].netKeyInfoIndex == MESH_SEC_INVALID_ENTRY_INDEX)
    {
      break;
    }
  }

  if (idx == secMatLocals.friendMatListSize)
  {
    /* This should never happen since the slots should be in sync with friendship module slots. */
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Store index of empty entry. */
  secKeyDerivReq.friendMatDerivReq.friendListIdx = idx;

  /* Store request callback, parameter and pointer to formed P buffer. */
  secKeyDerivReq.friendMatDerivReq.cback      = friendCredAdded;
  secKeyDerivReq.friendMatDerivReq.pParam     = pParam;
  secKeyDerivReq.friendMatDerivReq.pK2PBuff   = k2PBuff;
  secKeyDerivReq.friendMatDerivReq.doUpdate   = FALSE;

  /* Store friendship security parameters. */
  secMatLocals.pFriendMatArray[idx].friendAddres    = pFriendshipCred->friendAddres;
  secMatLocals.pFriendMatArray[idx].lpnAddress      = pFriendshipCred->lpnAddress;
  secMatLocals.pFriendMatArray[idx].friendCounter   = pFriendshipCred->friendCounter;
  secMatLocals.pFriendMatArray[idx].lpnCounter      = pFriendshipCred->lpnCounter;
  secMatLocals.pFriendMatArray[idx].netKeyInfoIndex = MESH_SEC_FRIEND_ENTRY_BUSY_IDX;
  secMatLocals.pFriendMatArray[idx].hasUpdtMaterial = FALSE;

  /* Point to start of P buffer. */
  pK2PBuff = k2PBuff;

  /* Generate P buffer. */
  UINT8_TO_BSTREAM(pK2PBuff, 0x01);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].lpnAddress >> 8);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].lpnAddress);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].friendAddres >> 8);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].friendAddres);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].lpnCounter >> 8);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].lpnCounter);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].friendCounter >> 8);
  UINT8_TO_BSTREAM(pK2PBuff, secMatLocals.pFriendMatArray[idx].friendCounter);

  /* Read Network Key. This should never fail since there is key material stored
   * derived from it.
   */
  retVal = MeshLocalCfgGetNetKey(pFriendshipCred->netKeyIndex, secTempData.nwkKeyFriend);

  if (retVal != MESH_SUCCESS)
  {
    /* Trap in case it fails. */
    WSF_ASSERT(retVal == MESH_SUCCESS);
  }
  else
  {
    /* Start K2 derivation. */
    retVal = MeshSecToolK2Derive(k2PBuff,
                                 sizeof(k2PBuff),
                                 secTempData.nwkKeyFriend,
                                 meshSecFriendCredAddCback,
                                 (void *)(&secKeyDerivReq.friendMatDerivReq));
  }
  if (retVal != MESH_SUCCESS)
  {
    /* Reset slot. */
    secMatLocals.pFriendMatArray[idx].netKeyInfoIndex = MESH_SEC_INVALID_ENTRY_INDEX;

    /* Reset request. */
    secKeyDerivReq.friendMatDerivReq.netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes friendship credentials from existing network key derivation material.
 *
 *  \param[in] friendAddr   Friend address.
 *  \param[in] lpnAddr      LPN address.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *
 *  \retval    MESH_SUCCESS                     Key material removed as configured by the
 *                                              parameters.
 *  \retval    MESH_SEC_INVALID_PARAMS          Invalid parameters. See the Remarks section.
 *  \retval    MESH_SEC_KEY_NOT_FOUND           The module has no material for the specified global
 *                                              key index.
 *  \retval    MESH_SEC_KEY_MATERIAL_NOT_FOUND  There is no friendship material identified by the
 *                                              same parameters.
 *
 *  \remarks   This function must be called when a friendship is terminated.
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecRemoveFriendCred(meshAddress_t friendAddr, meshAddress_t lpnAddr,
                                        uint16_t netKeyIndex)
{
  meshSecFriendMat_t *pFriendMat;
  uint16_t idx;

  /* Validate parameters. */
  if ((!MESH_IS_ADDR_UNICAST(friendAddr)) || (!MESH_IS_ADDR_UNICAST(lpnAddr)) ||
      (netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Search the list for matching friendship parameters. */
  for (idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    pFriendMat = &secMatLocals.pFriendMatArray[idx];

    if ((pFriendMat->friendAddres == friendAddr) && (pFriendMat->lpnAddress == lpnAddr))
    {
      /* Check if material is already derived. */
      if((pFriendMat->netKeyInfoIndex < secMatLocals.netKeyInfoListSize) &&
         (secMatLocals.pNetKeyInfoArray[pFriendMat->netKeyInfoIndex].hdr.keyIndex == netKeyIndex))
      {
        /* Reset entry by setting netKeyIndex to an invalid value and dequeuing it from the
         * Network Key material reference list.
         */
        pFriendMat->netKeyInfoIndex = MESH_SEC_INVALID_ENTRY_INDEX;
        return MESH_SUCCESS;
      }
      /* Check if material is in the process of getting derived. */
      if((pFriendMat->netKeyInfoIndex == MESH_SEC_FRIEND_ENTRY_BUSY_IDX) &&
         (secKeyDerivReq.friendMatDerivReq.netKeyListIdx != MESH_SEC_INVALID_ENTRY_INDEX) &&
         (secKeyDerivReq.friendMatDerivReq.netKeyIndex == netKeyIndex))
      {
        /* Reset entry by setting netKeyIndex to an invalid value and dequeuing it from the
         * Network Key material reference list.
         */
        pFriendMat->netKeyInfoIndex = MESH_SEC_INVALID_ENTRY_INDEX;
        return MESH_SUCCESS;
      }
    }
  }

  return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Determines if the NID value exists for the existing network keys.
 *
 *  \param[in] nid  Network identifier to be searched.
 *
 *  \return    TRUE if at least one NID matched or FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSecNidExists(uint8_t nid)
{
  meshSecFriendMat_t *pFriendMat;
  uint16_t idx;
  uint8_t crtEntry = 0;

  for (idx = 0; idx < secMatLocals.netKeyInfoListSize; idx++)
  {
    /* If slot is empty search next one. */
    if (!(secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      continue;
    }

    /* Get current key entry. */
    crtEntry = secMatLocals.pNetKeyInfoArray[idx].hdr.crtKeyId;

    /* Check NID. */
    if ((secMatLocals.pNetKeyInfoArray[idx].keyMaterial[crtEntry].masterPduSecMat.nid == nid) ||
        ((secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE) &&
         (secMatLocals.pNetKeyInfoArray[idx].keyMaterial[1 - crtEntry].masterPduSecMat.nid == nid)))
    {
      return TRUE;
    }
  }

  /* Look in the friendship material. */
  for (idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    /* Skip empty or in progress entries. */
    if (secMatLocals.pFriendMatArray[idx].netKeyInfoIndex >= secMatLocals.netKeyInfoListSize)
    {
      continue;
    }

    pFriendMat = &secMatLocals.pFriendMatArray[idx];

    /* Check if current material has matching NID. */
    if (pFriendMat->keyMaterial[crtEntry].nid == nid)
    {
      return TRUE;
    }

    /* Check if updated material exists and has a matching NID. */
    if ((pFriendMat->hasUpdtMaterial) && (pFriendMat->keyMaterial[1 - crtEntry].nid == nid))
    {
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Network ID.
 *
 *  \param[in] netKeyIndex  Netkey index.
 *
 *  \return    Pointer to Network ID, or NULL if not found.
 */
/*************************************************************************************************/
uint8_t *MeshSecNetKeyIndexToNwkId(uint16_t netKeyIndex)
{
  meshSecNetKeyMaterial_t *pSecMaterial;

  /* Get security material */
  pSecMaterial = meshSecNetKeyIndexToNetKeyMaterial(netKeyIndex);

  if (pSecMaterial != NULL)
  {
    return pSecMaterial->networkID;
  }

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Identity Key.
 *
 *  \param[in] netKeyIndex  Netkey index.
 *
 *  \return    Pointer to Identity Key, or NULL if not found.
 */
/*************************************************************************************************/
uint8_t *MeshSecNetKeyIndexToIdentityKey(uint16_t netKeyIndex)
{
  meshSecNetKeyMaterial_t *pSecMaterial;

  /* Get security material */
  pSecMaterial = meshSecNetKeyIndexToNetKeyMaterial(netKeyIndex);

  if (pSecMaterial != NULL)
  {
    return pSecMaterial->identityKey;
  }

  return NULL;
}
