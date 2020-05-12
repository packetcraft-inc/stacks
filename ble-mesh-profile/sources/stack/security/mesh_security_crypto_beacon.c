/*************************************************************************************************/
/*!
 *  \file   mesh_security_crypto_beacon.c
 *
 *  \brief  Security implementation for beacons.
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

#include "mesh_defs.h"
#include "mesh_network_beacon_defs.h"
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

static meshSecRetVal_t meshSecTryNextAuthParams(meshSecNwkBeaconAuthReq_t *pReq);

/*************************************************************************************************/
/*!
 *  \brief     Implementation of the CMAC callback used for computing Beacon Authentication Value.
 *
 *  \param[in] pCmacResult  Pointer to the 128-bit CMAC result.
 *  \param[in] pParam       Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecBeaconCompCback(const uint8_t *pCmacResult, void *pParam)
{
  /* Recover request. */
  meshSecNwkBeaconComputeAuthReq_t *pReq;
  meshSecBeaconComputeAuthCback_t  cback;

  /* Recover request. */
  pReq = (meshSecNwkBeaconComputeAuthReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Store user callback. */
  cback = pReq->cback;

  /* Clear callback to make request available. */
  pReq->cback = NULL;

  /* Handle success. */
  if (pCmacResult != NULL)
  {
    /* Copy authentication bytes. */
    memcpy(&(pReq->pSecBeacon[MESH_NWK_BEACON_NUM_BYTES - MESH_NWK_BEACON_AUTH_NUM_BYTES]),
           pCmacResult,
           MESH_NWK_BEACON_AUTH_NUM_BYTES);
  }

   /* Invoke user callback. */
  cback(pCmacResult != NULL, pReq->pSecBeacon, pReq->netKeyIndex, pReq->pParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Implementation of the CMAC callback used for verifying Beacon Authentication Value.
 *
 *  \param[in] pCmacResult  Pointer to the 128-bit CMAC result
 *  \param[in] pParam       Generic parameter provided in the request
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecBeaconVerificationCback(const uint8_t *pCmacResult, void *pParam)
{
  /* Recover request. */
  meshSecNwkBeaconAuthReq_t *pReq;
  meshSecBeaconAuthCback_t cback;
  bool_t isSuccess = FALSE;

  /* Recover request. */
  pReq = (meshSecNwkBeaconAuthReq_t *)pParam;

  /* Check if module is reinitialized. */
  if (pReq->cback == NULL)
  {
    return;
  }

  /* Store user callback. */
  cback = pReq->cback;

  /* Handle success. */
  if (pCmacResult != NULL)
  {
    /* Verify if computed authentication values matches Beacon Authentication value. */
    if(memcmp(&pReq->pSecBeacon[MESH_NWK_BEACON_NUM_BYTES - MESH_NWK_BEACON_AUTH_NUM_BYTES],
              pCmacResult, MESH_NWK_BEACON_AUTH_NUM_BYTES) == 0x00)
    {
      /* Clear callback to make request available. */
      pReq->cback = NULL;

      /* Invoke user callback. */
      cback(TRUE, pReq->newKeyUsed, pReq->pSecBeacon, pReq->netKeyIndex, pReq->pParam);

      isSuccess = TRUE;
    }
    else
    {
      /* Try next NetKey Index having a match on Network ID. */
      if (meshSecTryNextAuthParams(pReq) == MESH_SUCCESS)
      {
        isSuccess = TRUE;
      }
    }
  }

  if (!isSuccess)
  {
    /* Clear callback to make request available. */
    pReq->cback = NULL;

    /* Invoke user callback for error. */
    cback(FALSE, FALSE, pReq->pSecBeacon, MESH_SEC_INVALID_KEY_INDEX, pReq->pParam);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Finds matching Beacon Key and attemps authentication based on fields of the Secure
 *             Network Beacon.
 *
 *  \param[in] pCmacResult  Pointer to the 128-bit CMAC result.
 *  \param[in] pParam       Generic parameter provided in the request.
 *
 *  \return    MESH_SUCCESS if an attempt to authenticate starts, error otherwise.
 *
 *  \see meshReturnValues.
 */
/*************************************************************************************************/
static meshSecRetVal_t meshSecTryNextAuthParams(meshSecNwkBeaconAuthReq_t *pReq)
{
  meshSecNetKeyInfo_t *pNetKeyInfo = NULL;
  meshKeyRefreshStates_t state;
  uint16_t keyInfoId;
  uint8_t  entryId = 0;

  /* Iterate through Network Key information. */
  for (; pReq->keySearchIndex < MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.netKeyInfoListSize;
       pReq->keySearchIndex++)
  {

    keyInfoId = (uint8_t)(pReq->keySearchIndex >> (MESH_SEC_KEY_MAT_PER_INDEX - 1));

    /* Get index in material array. */
    entryId = pReq->keySearchIndex & (MESH_SEC_KEY_MAT_PER_INDEX - 1);
    pNetKeyInfo = &secMatLocals.pNetKeyInfoArray[keyInfoId];

    /* Check if key material is required and it does not exist. */
    if (!(pNetKeyInfo->hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      continue;
    }

    /* Check if the Key Refresh Phase allows use of updated material. Key Refresh Read shoud
     * never fail for keys that are in sync with security key information.
     */
    state = MeshLocalCfgGetKeyRefreshPhaseState(pNetKeyInfo->hdr.keyIndex);

    /* 3.10.4.2, 3.10.4.3 In phase 2 (and 3) a node shall only receive Secure Network beacons
     * secured using the new NetKey.
     */
    if ((state > MESH_KEY_REFRESH_FIRST_PHASE) && (entryId == pNetKeyInfo->hdr.crtKeyId))
    {
      continue;
    }

    /* Check if search hits updated key and material is not available. */
    if ((entryId != pNetKeyInfo->hdr.crtKeyId) &&
        !(pNetKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE))
    {
      continue;
    }

    /* Check Network ID match. */
    if(memcmp(&(pReq->pSecBeacon[MESH_NWK_BEACON_NWK_ID_START_BYTE]),
              pNetKeyInfo->keyMaterial[entryId].networkID,
              MESH_NWK_ID_NUM_BYTES) != 0x00)
    {
      continue;
    }

    /* Copy Beacon Key. */
    memcpy(pReq->bk, pNetKeyInfo->keyMaterial[entryId].beaconKey, MESH_SEC_TOOL_AES_BLOCK_SIZE);
    break;
  }

  if (pReq->keySearchIndex == (MESH_SEC_KEY_MAT_PER_INDEX * secMatLocals.netKeyInfoListSize))
  {
    return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
  }

  /* Set new Key used flag based on what key is used. */
  pReq->newKeyUsed = (entryId != pNetKeyInfo->hdr.crtKeyId);

  /* Set NetKey Index. */
  pReq->netKeyIndex = pNetKeyInfo->hdr.keyIndex;

  /* Increment key search index for the following requests. */
  ++(pReq->keySearchIndex);

  /* Call Toolbox to compute authentication value. */
  return (meshSecRetVal_t)MeshSecToolCmacCalculate(pReq->bk,
                                                   &pReq->pSecBeacon[MESH_NWK_BEACON_FLAGS_BYTE_POS],
                                                   MESH_SEC_BEACON_AUTH_INPUT_NUM_BYTES,
                                                   meshSecBeaconVerificationCback,
                                                   (void *)pReq);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes Secure Network Beacon authentication value.
 *
 *  \param[in] pSecNwkBeacon         Pointer to 22 byte buffer storing the Secure Network Beacon.
 *  \param[in] netKeyIndex           Global Network Key identifier.
 *  \param[in] useNewKey             TRUE if new key should be used for computation.
 *  \param[in] secNwkBeaconGenCback  Callback used to signal operation complete.
 *  \param[in] pParam                Pointer to generic callback parameter.
 *
 *  \retval MESH_SUCCESS                     All validation is performed. Calculation starts.
 *  \retval MESH_SEC_INVALID_PARAMS          Invalid parameters (E.g.: NULL input ).
 *  \retval MESH_SEC_KEY_MATERIAL_NOT_FOUND  Either there is no material available for the NetKey
 *                                           Index or the Network ID field of the beacon has no
 *                                           match with the Network ID(s) associated to the index.
 *  \retval MESH_SEC_OUT_OF_MEMORY           There are no resources to process the request.
 *
 *  \see meshSecBeaconComputeAuthCback_t
 *
 *  \remarks This function computes the authentication value and stores it in the last 8 bytes
 *           of the buffer referenced by pSecNwkBeacon. Also copies the Network ID associated to
 *           the Network Key used for computing the authentication value.
 *
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecBeaconComputeAuth(uint8_t *pSecNwkBeacon, uint16_t netKeyIndex,
                                         bool_t useNewKey,
                                         meshSecBeaconComputeAuthCback_t secNwkBeaconGenCback,
                                         void *pParam)
{
  meshSecNetKeyInfo_t  *pNetKeyInfo = NULL;
  uint8_t              *pLocalNwkId = NULL;
  meshSecRetVal_t      retVal       = MESH_SUCCESS;
  uint16_t             idx          = 0;

  /* Validate parameters. */
  if((pSecNwkBeacon == NULL) ||
     (netKeyIndex > MESH_SEC_MAX_KEY_INDEX) ||
     (secNwkBeaconGenCback == NULL))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Check if module is busy. */
  if (secCryptoReq.beaconCompAuthReq.cback != NULL)
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Search for material matching input NetKey Index. */
  for (idx = 0; idx < secMatLocals.netKeyInfoListSize; idx++)
  {
    if((secMatLocals.pNetKeyInfoArray[idx].hdr.keyIndex == netKeyIndex) &&
       (secMatLocals.pNetKeyInfoArray[idx].hdr.flags & MESH_SEC_KEY_CRT_MAT_AVAILABLE))
    {
      pNetKeyInfo = &secMatLocals.pNetKeyInfoArray[idx];
      break;
    }
  }

  /* Check if no NetKey Index matched. */
  if (pNetKeyInfo == NULL)
  {
    return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
  }

  /* Check if updated material exists in case new key should be used. */
  if (useNewKey)
  {
    if (!(pNetKeyInfo->hdr.flags & MESH_SEC_KEY_UPDT_MAT_AVAILABLE))
    {
      return MESH_SEC_KEY_MATERIAL_NOT_FOUND;
    }
  }

  /* Additional validation of Network ID since the Beacon module reads Network ID from the security
   * module.
   */
  pLocalNwkId = useNewKey ? pNetKeyInfo->keyMaterial[1 - pNetKeyInfo->hdr.crtKeyId].networkID :
                            pNetKeyInfo->keyMaterial[pNetKeyInfo->hdr.crtKeyId].networkID;

  /* Copy Network ID. */
  memcpy(&(pSecNwkBeacon[MESH_NWK_BEACON_NWK_ID_START_BYTE]), pLocalNwkId, MESH_NWK_ID_NUM_BYTES);

  /* Copy Beacon key. */
  if (useNewKey)
  {
    memcpy(secCryptoReq.beaconCompAuthReq.bk,
           pNetKeyInfo->keyMaterial[1 - pNetKeyInfo->hdr.crtKeyId].beaconKey,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);
  }
  else
  {
    memcpy(secCryptoReq.beaconCompAuthReq.bk,
           pNetKeyInfo->keyMaterial[pNetKeyInfo->hdr.crtKeyId].beaconKey,
           MESH_SEC_TOOL_AES_BLOCK_SIZE);
  }

  /* Configure request parameters. */
  secCryptoReq.beaconCompAuthReq.pSecBeacon  = pSecNwkBeacon;
  secCryptoReq.beaconCompAuthReq.netKeyIndex = netKeyIndex;

  retVal = (meshSecRetVal_t)MeshSecToolCmacCalculate(secCryptoReq.beaconCompAuthReq.bk,
                                                     &pSecNwkBeacon[MESH_NWK_BEACON_FLAGS_BYTE_POS],
                                                     MESH_SEC_BEACON_AUTH_INPUT_NUM_BYTES,
                                                     meshSecBeaconCompCback,
                                                     (void *)&(secCryptoReq.beaconCompAuthReq));

  if (retVal == MESH_SUCCESS)
  {
    /* Set user callback and parameter. */
    secCryptoReq.beaconCompAuthReq.cback = secNwkBeaconGenCback;
    secCryptoReq.beaconCompAuthReq.pParam   = pParam;
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Authenticates a Secure Network Beacon.
 *
 *  \param[in] pSecNwkBeacon          Pointer to received Secure Network Beacon.
 *  \param[in] secNwkBeaconAuthCback  Callback used to signal operation complete.
 *  \param[in] pParam                 Pointer to generic callback parameter.
 *
 *  \retval    MESH_SUCCESS              All validation is performed. Authentication starts.
 *  \retval    MESH_SEC_INVALID_PARAMS   Invalid parameters (E.g.: NULL input).
 *  \retval    MESH_SEC_KEY_NOT_FOUND    No matching NetKey index is found.
 *  \retval    MESH_SEC_OUT_OF_MEMORY    There are no resources to process the request.
 *
 *  \remarks This function computes the authentication value and compares it with the last 8 bytes
 *           of the buffer referenced by pSecNwkBeacon.
 *
 *  \see meshSecBeaconAuthCback_t
 */
/*************************************************************************************************/
meshSecRetVal_t MeshSecBeaconAuthenticate(uint8_t *pSecNwkBeacon,
                                          meshSecBeaconAuthCback_t secNwkBeaconAuthCback,
                                          void *pParam)
{
  meshSecRetVal_t retVal = MESH_SUCCESS;

  /* Validate parameters. */
  if ((pSecNwkBeacon == NULL) || (secNwkBeaconAuthCback == NULL))
  {
    return MESH_SEC_INVALID_PARAMS;
  }

  /* Check if request is in progress. */
  if (secCryptoReq.beaconAuthReq.cback != NULL)
  {
    return MESH_SEC_OUT_OF_MEMORY;
  }

  /* Reset key search index. */
  secCryptoReq.beaconAuthReq.keySearchIndex = 0;

  /* Store beacon parameter. */
  secCryptoReq.beaconAuthReq.pSecBeacon = pSecNwkBeacon;

  /* Start searching Network Key material and authenticate beacon. */
  retVal = meshSecTryNextAuthParams(&(secCryptoReq.beaconAuthReq));

  if (retVal == MESH_SUCCESS)
  {
    /* Set user callback and request parameter. */
    secCryptoReq.beaconAuthReq.cback = secNwkBeaconAuthCback;
    secCryptoReq.beaconAuthReq.pParam   = pParam;
  }

  return retVal;
}
