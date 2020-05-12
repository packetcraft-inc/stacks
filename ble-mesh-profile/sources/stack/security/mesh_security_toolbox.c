/*************************************************************************************************/
/*!
 *  \file   mesh_security_toolbox.c
 *
 *  \brief  Security toolbox implementation.
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
#include "sec_api.h"
#include "hci_api.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_handler.h"
#include "mesh_api.h"
#include "mesh_utils.h"

#include "mesh_security_toolbox.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Number of CMAC operations required by K1 */
#define MESH_SEC_TOOL_K1_CMAC_COUNT 2

/*! Number of CMAC operations required by K2 */
#define MESH_SEC_TOOL_K2_CMAC_COUNT 4

/*! Number of CMAC operations required by K3 */
#define MESH_SEC_TOOL_K3_CMAC_COUNT 2

/*! Number of CMAC operations required by K4 */
#define MESH_SEC_TOOL_K4_CMAC_COUNT 2

/*! Maximum size accepted for a CCM operation over input/output buffer. */
#define MESH_SEC_TOOL_CCM_MAX_BUFF  500

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh security WSF events */
enum meshSecToolEvents
{
  MESH_SEC_AES_EVENT        = 0x00,     /*!< AES complete stack message */
  MESH_SEC_CMAC_EVENT       = 0x01,     /*!< CMAC complete stack message */
  MESH_SEC_CCM_ENC_EVENT    = 0x02,     /*!< CCM encrypt complete stack message */
  MESH_SEC_CCM_DEC_EVENT    = 0x03,     /*!< CCM decrypt complete stack message */
  MESH_SEC_ECC_GEN_EVENT    = 0x04,     /*!< ECC key generated stack message */
  MESH_SEC_ECDH_EVENT       = 0x05,     /*!< ECDH shared secret calculated stack message */
};

/*! Mesh security toolbox enumeration of key derivation functions */
enum meshSecToolKxTypeValues
{
  MESH_SEC_TOOL_DERIV_K1 = 0,  /*!< Kx type is K1 */
  MESH_SEC_TOOL_DERIV_K2 = 1,  /*!< Kx type is K2 */
  MESH_SEC_TOOL_DERIV_K3 = 2,  /*!< Kx type is K3 */
  MESH_SEC_TOOL_DERIV_K4 = 3,  /*!< Kx type is K4 */
};

/*! Mesh security toolbox key derivation type identifier*/
typedef uint8_t meshSecToolKxType_t;

/*! AES request that can be enqueued */
typedef struct meshSecToolAesQueueElem_tag
{
  void                  *pNext;   /*!< Pointer to next element */
  meshSecToolAesCback_t cback;    /*!< AES complete callback */
  void                  *pParam;  /*!< Generic callback parameter */
  uint8_t               *pKey;    /*!< Pointer to buffer containing an AES-128 key */
  uint8_t               *pPlain;  /*!< Pointer to buffer containing an AES-128 input block */
} meshSecToolAesQueueElem_t;

/*! CMAC request that can be enqueued */
typedef struct meshSecToolCmacQueueElem_tag
{
  void                   *pNext;   /*!< Pointer to next element */
  meshSecToolCmacCback_t cback;    /*!< CMAC complete callback */
  void                   *pParam;  /*!< Generic callback parameter */
  uint8_t                *pKey;    /*!< Pointer to buffer containing an AES-128 key */
  uint8_t                *pIn;     /*!< Pointer to buffer containing CMAC input data */
  uint16_t               len;      /*!< Length of the CMAC input data */
} meshSecToolCmacQueueElem_t;

/*! CCM request that can be enqueued */
typedef struct meshSecToolCcmQueueElem_tag
{
  void                   *pNext;     /*!< Pointer to next element */
  meshSecToolCcmCback_t  cback;      /*!< CCM complete callback */
  void                   *pParam;    /*!< Generic callback parameter */
  meshSecToolCcmParams_t ccmParams;  /*!< CCM configuration parameters */
  bool_t                 isEncrypt;  /*!< CCM operation type. TRUE for encrypt, FALSE for decrypt */
} meshSecToolCcmQueueElem_t;

/*! Container for CCM queue elements to be used with Mesh Queues */
typedef struct meshSecToolCcmQueueElemContainer_tag
{

  meshSecToolCcmQueueElem_t elem;    /*!< Queue element */
} meshSecToolCcmQueueElemContainer_t;

/*! K1 or K2 or K3 or K4 request that can be enqueued */
typedef struct meshSecToolKxQueueElem_tag
{
  void                             *pNext;        /*!< Pointer to next element */
  meshSecToolKeyDerivationCback_t  cback;         /*!< Key derivation complete callback */
  void                             *pParam;       /*!< Generic callback parameter */
  uint8_t                          *pPlainText;   /*!< Pointer to plaintext buffer
                                                   *   (P - K1, K2, N - K3, K4)
                                                   */
  uint8_t                          *pTemp;        /*!< Pointer to second buffer (N - K1, K2 ) */
  uint8_t                          *pSalt;        /*!< Pointer to 16 byte salt buffer
                                                   *   (SALT - K1)
                                                   */
  meshSecToolCmacQueueElem_t       cmacElem;      /*!< CMAC slot used by Kx to provide enqueueable
                                                   *   request
                                                   */
  uint16_t                         plainTextLen;  /*!< Length of the plaintext (where needed) */
  uint16_t                         tempLen;       /*!< Length of the second buffer
                                                   *   (where needed)
                                                   */
  meshSecToolKxType_t              kxType;        /*!< Type of key derivation */
  uint8_t                          cmacCount;     /*!< Count of number of CMACs processed */
} meshSecToolKxQueueElem_t;

/*! Mesh Security Toolbox local data structure */
typedef struct meshSecToolLocals_tag
{
  meshSecToolAesQueueElem_t     aesQueuePool[MESH_SEC_TOOL_AES_REQ_QUEUE_SIZE];   /*!< AES pool */
  meshSecToolCmacQueueElem_t    cmacQueuePool[MESH_SEC_TOOL_CMAC_REQ_QUEUE_SIZE]; /*!< CMAC pool */
  meshSecToolCcmQueueElem_t     ccmQueuePool[MESH_SEC_TOOL_CCM_REQ_QUEUE_SIZE];   /*!< CCM pool */
  meshSecToolKxQueueElem_t      kxQueuePool[MESH_SEC_TOOL_KX_REQ_QUEUE_SIZE];     /*!< Kx pool */
  uint8_t                       kxTempBuff[MESH_SEC_TOOL_K2_CMAC_COUNT *
                                           MESH_SEC_TOOL_AES_BLOCK_SIZE];  /*!< Kx temp buffer */
  wsfQueue_t                    aesQueue;      /*!< AES queue */
  wsfQueue_t                    cmacQueue;     /*!< CMAC queue */
  wsfQueue_t                    ccmQueue;      /*!< CCM queue */
  wsfQueue_t                    kxQueue;       /*!< Kx derivation queue */

  meshSecToolAesQueueElem_t     *pCrtAes;      /*!< Current AES in progress */
  meshSecToolCmacQueueElem_t    *pCrtCmac;     /*!< Current CMAC in progress */
  meshSecToolCcmQueueElem_t     *pCrtCcm;      /*!< Current CCM in progress */
  meshSecToolKxQueueElem_t      *pCrtKx;       /*!< Current K derivation in progress */

  meshSecToolEccKeyGenCback_t   eccGenCback;   /*!< Callback to for ECC key generation */
  meshSecToolEcdhCback_t        ecdhCback;     /*!< Callback for ECDH calculation */
  meshSecToolCmacCback_t        kxCmacCback;   /*!< Internal CMAC callback used for Kx */

  meshSecToolAlgoBitfield_t     algos;         /*!< Bitfield of initialized algorithms */
  bool_t                        isInitialized; /*!< TRUE if toolbox is initialized */
  wsfHandlerId_t                handlerId;     /*!< WSF handler id for Mesh Security Handler */
} meshSecToolLocals_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Security Toolbox internal data */
static meshSecToolLocals_t secToolLocals = { 0 };

/*! Definition of an all 0's AES-128 Key */
static const uint8_t meshSecToolZkey[MESH_SEC_TOOL_AES_BLOCK_SIZE] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*! Precalculated S1(smk2) */
static const uint8_t meshSecToolS1Smk2[MESH_SEC_TOOL_AES_BLOCK_SIZE] =
{
  0x4f, 0x90, 0x48, 0x0c, 0x18, 0x71, 0xbf, 0xbf, 0xfd, 0x16, 0x97, 0x1f, 0x4d, 0x8d, 0x10, 0xb1
};

/*! Precalculated S1(smk3) */
static const uint8_t meshSecToolS1Smk3[MESH_SEC_TOOL_AES_BLOCK_SIZE] =
{
  0x00, 0x36, 0x44, 0x35, 0x03, 0xf1, 0x95, 0xcc, 0x8a, 0x71, 0x6e, 0x13, 0x62, 0x91, 0xc3, 0x02
};

/*! Precalculated S1(smk4) */
static const uint8_t meshSecToolS1Smk4[MESH_SEC_TOOL_AES_BLOCK_SIZE] =
{
  0x0e, 0x9a, 0xc1, 0xb7, 0xce, 0xfa, 0x66, 0x87, 0x4c, 0x97, 0xee, 0x54, 0xac, 0x5f, 0x49, 0xbe
};

/*! String id64 concatenated with hex 0x01*/
static const uint8_t meshSecToolId64[] =
{
  0x69, 0x64, 0x36, 0x34, 0x01
};

/*! String id6 concatenated with hex 0x01*/
static const uint8_t meshSecToolId6[] =
{
  0x69, 0x64, 0x36, 0x01
};

/*! CCM decrypt shadow buffer used to protect */
static uint8_t ccmResultBuff[MESH_SEC_TOOL_CCM_MAX_BUFF];

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Schedules a CMAC element to run be either starting calculation or enqueueing it.
 *
 *  \param[in] pElem  Pointer to the queue element of the CMAC operation to be scheduled.
 *
 *  \return    Success or error code. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecToolRetVal_t meshSecToolInternalCmacCalculate(meshSecToolCmacQueueElem_t *pElem)
{
  /* If no request is in progress, request calculation, else enqueue */
  if (secToolLocals.pCrtCmac == NULL)
  {
    secToolLocals.pCrtCmac = pElem;

    /* Call PAL CMAC function */
    if (!SecCmac(secToolLocals.pCrtCmac->pKey, secToolLocals.pCrtCmac->pIn,
                 secToolLocals.pCrtCmac->len, secToolLocals.handlerId, 0, MESH_SEC_CMAC_EVENT))
    {
      /* Reset slot by setting callback to NULL */
      secToolLocals.pCrtCmac->cback = NULL;

      /* Set current request to NULL*/
      secToolLocals.pCrtCmac = NULL;

      return MESH_SEC_TOOL_UNKNOWN_ERROR;
    }
  }
  else
  {
    /* Enqueue element from container */
    WsfQueueEnq(&(secToolLocals.cmacQueue), (void *)pElem);
  }

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Initiates key derivation by requesting the first CMAC calculation.
 *
 *  \param[in] pElem  Pointer to the queue element of the key derivation to be initiated.
 *
 *  \return    Success or error code. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecToolRetVal_t meshSecToolStartDerivation(meshSecToolKxQueueElem_t *pElem)
{
  /* Determine first CMAC operation */
  switch (pElem->kxType)
  {
    case MESH_SEC_TOOL_DERIV_K1:
      pElem->cmacElem.pKey = pElem->pSalt;
      pElem->cmacElem.pIn = pElem->pTemp;
      pElem->cmacElem.len = pElem->tempLen;
      break;
    case MESH_SEC_TOOL_DERIV_K2:
      pElem->cmacElem.pKey = (uint8_t *)meshSecToolS1Smk2;
      pElem->cmacElem.pIn = pElem->pTemp;
      pElem->cmacElem.len = MESH_SEC_TOOL_AES_BLOCK_SIZE;
      break;
    case MESH_SEC_TOOL_DERIV_K3:
      pElem->cmacElem.pKey = (uint8_t *)meshSecToolS1Smk3;
      pElem->cmacElem.pIn = pElem->pPlainText;
      pElem->cmacElem.len = MESH_SEC_TOOL_AES_BLOCK_SIZE;
      break;
    case MESH_SEC_TOOL_DERIV_K4:
      pElem->cmacElem.pKey = (uint8_t *)meshSecToolS1Smk4;
      pElem->cmacElem.pIn = pElem->pPlainText;
      pElem->cmacElem.len = MESH_SEC_TOOL_AES_BLOCK_SIZE;
      break;
    default:
      break;
  }
  /* Set callback for CMAC */
  pElem->cmacElem.cback = secToolLocals.kxCmacCback;

  /* Trigger first CMAC operation. This also reserves a CMAC slot */
  return meshSecToolInternalCmacCalculate(&(pElem->cmacElem));
}

/*************************************************************************************************/
/*!
 *  \brief     Implementation of the CMAC callback used for derivation functions (Kx).
 *
 *  \param[in] pCmacResult  Pointer to the 128-bit CMAC result.
 *  \param[in] pParam       Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecToolKxCmacCback(const uint8_t *pCmacResult, void *pParam)
{
  (void)pParam;

  meshSecToolKxQueueElem_t *pTemp = secToolLocals.pCrtKx;
  meshSecToolKeyDerivationCback_t cback = NULL;
  uint8_t  *pResult = NULL;
  uint8_t  *pDstBuf = NULL;
  uint8_t  resultSize = 0;
  bool_t   dequeueNext = FALSE;

  if (pTemp == NULL)
  {
    /* Shoud never happen */
    WSF_ASSERT(pTemp != NULL);
    return;
  }

  /* Increment CMAC count property */
  pTemp->cmacCount++;

  /* Handle CMAC failure */
  if (pCmacResult == NULL)
  {
    dequeueNext = TRUE;
    pResult     = NULL;
    resultSize   = 0;
  }
  else
  {
    /* Copy each result at MESH_SEC_TOOL_AES_BLOCK_SIZE distance in the temp buffer */
    pDstBuf = &(secToolLocals.kxTempBuff[(pTemp->cmacCount - 1) * MESH_SEC_TOOL_AES_BLOCK_SIZE]);
    memcpy(pDstBuf, pCmacResult, MESH_SEC_TOOL_AES_BLOCK_SIZE);

    switch (pTemp->kxType)
    {
      case MESH_SEC_TOOL_DERIV_K1:
      {
        if (pTemp->cmacCount < MESH_SEC_TOOL_K1_CMAC_COUNT)
        {
          /* Key T for K1 is pDstBuf*/
          pTemp->cmacElem.pKey = pDstBuf;
          pTemp->cmacElem.pIn  = pTemp->pPlainText;
          pTemp->cmacElem.len  = pTemp->plainTextLen;
        }
        else
        {
          /* Set result pointer */
          pResult = pDstBuf;

          /* Set result length */
          resultSize = MESH_SEC_TOOL_K1_RESULT_SIZE;

          dequeueNext = TRUE;
        }
      }
      break;
      case MESH_SEC_TOOL_DERIV_K2:
      {
        if (pTemp->cmacCount < MESH_SEC_TOOL_K2_CMAC_COUNT)
        {
          /* Key T for K2 is always at the start of the temporary buffer */
          pTemp->cmacElem.pKey = secToolLocals.kxTempBuff;

          /* pIn is pDstBuf[0-15] | pPlainText[0 - (plainTextLen - 1)] | cmacCount
           * (which is 1,2 or 3) except for first T1 calculation
           */
          pTemp->cmacElem.pIn = pDstBuf;

          /* calculate length */
          pTemp->cmacElem.len = MESH_SEC_TOOL_AES_BLOCK_SIZE + pTemp->plainTextLen + 1;

          if (pTemp->cmacCount == 1)
          {
            pTemp->cmacElem.pIn += MESH_SEC_TOOL_AES_BLOCK_SIZE;
            pTemp->cmacElem.len -= MESH_SEC_TOOL_AES_BLOCK_SIZE;
          }

          /* Advance pDstBuf at the end of Ti-1 */
          pDstBuf += MESH_SEC_TOOL_AES_BLOCK_SIZE;

          /* Copy P from pPlainext to pDstBuf */
          memcpy(pDstBuf, pTemp->pPlainText, pTemp->plainTextLen);

          /* Concatenate cmacCount to obtain (Ti-1 || P || i) */
          pDstBuf[pTemp->plainTextLen] = pTemp->cmacCount;
        }
        else
        {
          /* Set result pointer */
          pResult = &(secToolLocals.kxTempBuff[(MESH_SEC_TOOL_AES_BLOCK_SIZE * 2) - 1]);

          /* Set result length */
          resultSize = MESH_SEC_TOOL_K2_RESULT_SIZE;

          dequeueNext = TRUE;
        }
      }
      break;
      case MESH_SEC_TOOL_DERIV_K3:
      {
        if (pTemp->cmacCount < MESH_SEC_TOOL_K3_CMAC_COUNT)
        {
          /* Key T for K3 is pDstBuf*/
          pTemp->cmacElem.pKey = pDstBuf;
          pTemp->cmacElem.pIn  = (uint8_t *)meshSecToolId64;
          pTemp->cmacElem.len  = sizeof(meshSecToolId64);
        }
        else
        {
          /* Set result pointer */
          pResult = pDstBuf + (MESH_SEC_TOOL_AES_BLOCK_SIZE - MESH_SEC_TOOL_K3_RESULT_SIZE);

          /* Set result length */
          resultSize = MESH_SEC_TOOL_K3_RESULT_SIZE;

          dequeueNext = TRUE;
        }
      }
      break;
      case MESH_SEC_TOOL_DERIV_K4:
      {
        if (pTemp->cmacCount < MESH_SEC_TOOL_K4_CMAC_COUNT)
        {
          /* Key T for K4 is pDstBuf*/
          pTemp->cmacElem.pKey = pDstBuf;
          pTemp->cmacElem.pIn = (uint8_t *)meshSecToolId6;
          pTemp->cmacElem.len = sizeof(meshSecToolId6);
        }
        else
        {
          /* Set result pointer */
          pResult = pDstBuf + (MESH_SEC_TOOL_AES_BLOCK_SIZE - MESH_SEC_TOOL_K4_RESULT_SIZE);

          /* Set result length */
          resultSize = MESH_SEC_TOOL_K4_RESULT_SIZE;

          dequeueNext = TRUE;
        }
      }
      break;
      default:
        WSF_ASSERT(secToolLocals.pCrtKx->kxType <= MESH_SEC_TOOL_DERIV_K4);
        break;
    }
  }
  /* No dequeue next means that operation is still in progress */
  if (!dequeueNext)
  {
    /* Set the CMAC callback to this function */
    pTemp->cmacElem.cback = secToolLocals.kxCmacCback;

    /* Push the CMAC request at the beginning of the queue so it gets dequeued first */
    WsfQueuePush(&(secToolLocals.cmacQueue), &(pTemp->cmacElem));
  }
  else
  {
    /* Extract Kx callback */
    cback = pTemp->cback;

    /* Reset slot by setting callback to NULL */
    pTemp->cback = NULL;

    /* Invoke callback with either valid or invalid result */
    cback(pResult, resultSize, pTemp->pParam);

    /* Dequeue next request */
    secToolLocals.pCrtKx = WsfQueueDeq(&(secToolLocals.kxQueue));
  }
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an incoming AES complete stack message.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecToolHandleAesComplete(secAes_t * pMsg)
{
  meshSecToolAesCback_t cback = NULL;
  void *pParam = NULL;

  /* Validate current request */
  if (secToolLocals.pCrtAes == NULL)
  {
    /* Should never happen */
    WSF_ASSERT(secToolLocals.pCrtAes != NULL);
    return;
  }

  /* Copy callback and generic parameter */
  cback = secToolLocals.pCrtAes->cback;
  pParam   = secToolLocals.pCrtAes->pParam;

  /* Mark entry as free by setting callback to NULL */
  secToolLocals.pCrtAes->cback = NULL;

  cback(pMsg->pCiphertext, pParam);

  while(1)
  {
    /* Dequeue next element */
    secToolLocals.pCrtAes = WsfQueueDeq(&(secToolLocals.aesQueue));

    /* If queue is empty, break */
    if (secToolLocals.pCrtAes == NULL)
    {
      break;
    }

    /* Request AES encryption */
    if (SecAesRev(secToolLocals.pCrtAes->pKey, secToolLocals.pCrtAes->pPlain,
                  secToolLocals.handlerId, 0, MESH_SEC_AES_EVENT) != SEC_TOKEN_INVALID)
    {
      break;
    }
    else
    {
      /* Copy callback and generic parameter */
      cback = secToolLocals.pCrtAes->cback;
      pParam = secToolLocals.pCrtAes->pParam;

      /* Mark entry as free by setting callback to NULL */
      secToolLocals.pCrtAes->cback = NULL;

      /* Signal error by setting parameter to NULL */
      cback(NULL, pParam);
    }
  }
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an incoming CMAC complete stack message.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecToolHandleCmacComplete(secCmacMsg_t * pMsg)
{
  meshSecToolCmacCback_t cback = NULL;
  void *pParam = NULL;

  /* Validate current request */
  if (secToolLocals.pCrtCmac == NULL)
  {
    /* Should never happen */
    WSF_ASSERT(secToolLocals.pCrtCmac != NULL);
    return;
  }

  /* Copy callback and generic parameter */
  cback = secToolLocals.pCrtCmac->cback;
  pParam = secToolLocals.pCrtCmac->pParam;

  /* Mark entry as free by setting callback to NULL */
  secToolLocals.pCrtCmac->cback = NULL;

  /* Invoke callback */
  cback(pMsg->pCiphertext, pParam);

  while (1)
  {
    /* Dequeue next element */
    secToolLocals.pCrtCmac = WsfQueueDeq(&(secToolLocals.cmacQueue));

    if (secToolLocals.pCrtCmac == NULL)
    {
      break;
    }

    /* Request CMAC calculation */
    if (SecCmac(secToolLocals.pCrtCmac->pKey, secToolLocals.pCrtCmac->pIn,
                secToolLocals.pCrtCmac->len, secToolLocals.handlerId, 0, MESH_SEC_CMAC_EVENT))
    {
      break;
    }
    else
    {
      /* Copy callback and generic parameter */
      cback = secToolLocals.pCrtCmac->cback;
      pParam = secToolLocals.pCrtCmac->pParam;

      /* Mark entry as free by setting callback to NULL */
      secToolLocals.pCrtCmac->cback = NULL;

      /* Signal error by setting parameter to NULL */
      cback(NULL, pParam);
    }
  }
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an incoming CCM complete stack message.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecToolHandleCcmComplete(secMsg_t *pMsg)
{
  meshSecToolCcmCback_t cback = NULL;
  meshSecToolCcmResult_t result;
  meshSecToolCcmParams_t *pOpParams;
  void *pParam = NULL;
  bool_t ccmRes = FALSE;

  /* Validate current request */
  if (secToolLocals.pCrtCcm == NULL)
  {
    /* Should never happen */
    WSF_ASSERT(secToolLocals.pCrtCcm != NULL);
    return;
  }

  /* Copy callback and generic parameter */
  cback = secToolLocals.pCrtCcm->cback;
  pParam = secToolLocals.pCrtCcm->pParam;

  /* Mark entry as free by setting callback to NULL */
  secToolLocals.pCrtCcm->cback = NULL;

  result.op = secToolLocals.pCrtCcm->isEncrypt ? MESH_SEC_TOOL_CCM_ENCRYPT : MESH_SEC_TOOL_CCM_DECRYPT;

  if (secToolLocals.pCrtCcm->isEncrypt)
  {
    /* Set result. */
    result.results.encryptResult.pCipherText = secToolLocals.pCrtCcm->ccmParams.pOut;
    result.results.encryptResult.cipherTextSize = secToolLocals.pCrtCcm->ccmParams.inputLen;
    result.results.encryptResult.pCbcMac = secToolLocals.pCrtCcm->ccmParams.pCbcMac;
    result.results.encryptResult.cbcMacSize = secToolLocals.pCrtCcm->ccmParams.cbcMacSize;

    /* Copy encrypted data from WSF event. */
    memcpy(result.results.encryptResult.pCipherText,
           pMsg->ccmEnc.pCiphertext + secToolLocals.pCrtCcm->ccmParams.authDataLen,
           result.results.encryptResult.cipherTextSize);
    /* Copy MIC from WSF event. */
    memcpy(result.results.encryptResult.pCbcMac,
           pMsg->ccmEnc.pCiphertext + secToolLocals.pCrtCcm->ccmParams.authDataLen +
           secToolLocals.pCrtCcm->ccmParams.inputLen,
           result.results.encryptResult.cbcMacSize);
  }
  else
  {
    result.results.decryptResult.pPlainText = secToolLocals.pCrtCcm->ccmParams.pOut;
    result.results.decryptResult.plainTextSize = secToolLocals.pCrtCcm->ccmParams.inputLen;
    result.results.decryptResult.isAuthSuccess = pMsg->ccmDec.success;

    /* Copy from WSF event on successful authentication. */
    if (result.results.decryptResult.isAuthSuccess)
    {
      /* Copy decrypted data from WSF event. */
      memcpy(result.results.decryptResult.pPlainText,
             pMsg->ccmDec.pText,
             result.results.decryptResult.plainTextSize);
    }
  }

  /* Invoke callback */
  cback(&result, pParam);

  while (1)
  {
    /* Dequeue next element */
    secToolLocals.pCrtCcm = WsfQueueDeq(&(secToolLocals.ccmQueue));

    if (secToolLocals.pCrtCcm == NULL)
    {
      break;
    }

    pOpParams = &secToolLocals.pCrtCcm->ccmParams;

    /* Check what API to use for CCM. */
    if (secToolLocals.pCrtCcm->isEncrypt)
    {
      /* Encrypt. */
      ccmRes = SecCcmEnc(pOpParams->pCcmKey, pOpParams->pNonce, pOpParams->pIn, pOpParams->inputLen,
                         pOpParams->pAuthData, pOpParams->authDataLen, pOpParams->cbcMacSize,
                         ccmResultBuff, secToolLocals.handlerId, 0, MESH_SEC_CCM_ENC_EVENT);
    }
    else
    {
      /* Decrypt. */
      ccmRes = SecCcmDec(pOpParams->pCcmKey, pOpParams->pNonce, pOpParams->pIn, pOpParams->inputLen,
                         pOpParams->pAuthData, pOpParams->authDataLen, pOpParams->pCbcMac,
                         pOpParams->cbcMacSize, ccmResultBuff, secToolLocals.handlerId, 0,
                         MESH_SEC_CCM_DEC_EVENT);
    }
    if (ccmRes)
    {
      break;
    }
    else
    {
      /* Copy callback and generic parameter */
      cback = secToolLocals.pCrtCcm->cback;
      pParam = secToolLocals.pCrtCcm->pParam;

      /* Mark entry as free by setting callback to NULL */
      secToolLocals.pCrtCcm->cback = NULL;

      /* Signal error by setting parameter to NULL */
      cback(NULL, pParam);
    }
  }
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an incoming ECC generation complete stack message.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecToolHandleEccGenComplete(secEccMsg_t *pMsg)
{
  meshSecToolEccKeyGenCback_t cback = NULL;

  if (secToolLocals.eccGenCback == NULL)
  {
    /* Should never happen */
    WSF_ASSERT(secToolLocals.eccGenCback != NULL);
    return;
  }
  cback = secToolLocals.eccGenCback;

  /* Reset slot */
  secToolLocals.eccGenCback = NULL;

  /* Invoke callback */
  cback(pMsg->data.key.pubKey_x,
        pMsg->data.key.pubKey_y,
        pMsg->data.key.privKey);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an incoming ECDH complete stack message.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecToolHandleEcdhComplete(secEccMsg_t *pMsg)
{
  meshSecToolEcdhCback_t cback = NULL;

  if (secToolLocals.ecdhCback == NULL)
  {
    /* Should never happen */
    WSF_ASSERT(secToolLocals.ecdhCback != NULL);
    return;
  }
  cback = secToolLocals.ecdhCback;

  /* Reset slot */
  secToolLocals.ecdhCback = NULL;

  /* Invoke callback */
  cback((pMsg->hdr.status == HCI_SUCCESS), pMsg->data.sharedSecret.secret);
}

/*************************************************************************************************/
/*!
 *  \brief     Starts or enqueues a key derivation request.
 *
 *  \param[in] kxType               Type of key derivation function to be used.
 *  \param[in] pPlainText           Pointer to plaintext data (P - K1,K2; N - K3, K4).
 *  \param[in] plainTextSize        Size of the plaintext.
 *  \param[in] pSalt                Pointer to 128-bit Salt (K1).
 *  \param[in] pTempKeyMaterial     Pointer to plaintext used to compute the T key (N - K1, K2).
 *  \param[in] tempKeyMaterialSize  Size of the plaintext used to compute the T key.
 *  \param[in] derivCompleteCback   Callback invoked after derivation is complete.
 *  \param[in] pParam               Pointer to generic callback parameter.
 *
 *  \return    Success or error code. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshSecToolRetVal_t meshSecToolKxDerive(meshSecToolKxType_t kxType, uint8_t *pPlainText,
                                               uint16_t plainTextSize, uint8_t *pSalt,
                                               uint8_t *pTempKeyMaterial, uint16_t tempKeyMaterialSize,
                                               meshSecToolKeyDerivationCback_t derivCompleteCback,
                                               void *pParam)
{
  meshSecToolKxQueueElem_t *pKxElem = NULL;
  meshSecToolRetVal_t      retVal   = MESH_SUCCESS;
  uint8_t idx                       = 0;

  /* Get empty Kx slot */
  for (idx = 0; idx < MESH_SEC_TOOL_KX_REQ_QUEUE_SIZE; idx++)
  {
    /* An entry which has a NULL callback is empty since it does not return to anything */
    if (secToolLocals.kxQueuePool[idx].cback == NULL)
    {
      break;
    }
  }

  /* If no slot is found, return error */
  if (idx == MESH_SEC_TOOL_KX_REQ_QUEUE_SIZE)
  {
    return MESH_SEC_TOOL_OUT_OF_MEMORY;
  }

  /* Store pointer to slot */
  pKxElem = &(secToolLocals.kxQueuePool[idx]);

  /* Configure Kx parameters */
  pKxElem->cback = derivCompleteCback;
  pKxElem->pParam = pParam;
  pKxElem->pPlainText = pPlainText;
  pKxElem->plainTextLen = plainTextSize;
  pKxElem->pTemp = pTempKeyMaterial;
  pKxElem->tempLen = tempKeyMaterialSize;
  pKxElem->pSalt = pSalt;
  pKxElem->kxType = kxType;
  pKxElem->cmacCount = 0;

  /* Start derivation to add CMAC slot to CMAC queue */
  retVal = meshSecToolStartDerivation(pKxElem);
  if (retVal != MESH_SUCCESS)
  {
    /* Reset slot by setting callback to NULL */
    pKxElem->cback = NULL;

    return retVal;
  }

  /* If no request is in progress, set current derivation pointer, else enqueue */
  if (secToolLocals.pCrtKx == NULL)
  {
    secToolLocals.pCrtKx = pKxElem;
  }
  else
  {
    /* Enqueue element from container */
    WsfQueueEnq(&(secToolLocals.kxQueue), pKxElem);
  }

  return MESH_SUCCESS;
}
/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Mesh Security Toolbox Init.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSecToolInit(void)
{
  /* Set internal CMAC callback used for key derivation */
  secToolLocals.kxCmacCback = meshSecToolKxCmacCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh Security WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Security.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSecurityHandlerInit(wsfHandlerId_t handlerId)
{
  /* Reset locals */
  memset(&secToolLocals, 0, sizeof(secToolLocals));

  /* Store Handler Id. */
  secToolLocals.handlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF event handler for Mesh Security.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSecurityHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
    case MESH_SEC_AES_EVENT:
      meshSecToolHandleAesComplete((secAes_t *)pMsg);
      break;
    case MESH_SEC_CMAC_EVENT:
      meshSecToolHandleCmacComplete((secCmacMsg_t *)pMsg);
      break;
    case MESH_SEC_CCM_ENC_EVENT:
    case MESH_SEC_CCM_DEC_EVENT:
      /* Use generic security message and determine type inside. */
      meshSecToolHandleCcmComplete((secMsg_t *)pMsg);
      break;
    case MESH_SEC_ECC_GEN_EVENT:
      meshSecToolHandleEccGenComplete((secEccMsg_t *)pMsg);
      break;
    case MESH_SEC_ECDH_EVENT:
      meshSecToolHandleEcdhComplete((secEccMsg_t *)pMsg);
      break;
    default:
      break;
    }
  }
  /* Handle event */
  else if (event)
  {

  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox AES-128 primitive.
 *
 *  \param[in] pAesKey          Pointer to 128-bit AES Key.
 *  \param[in] pPlainTextBlock  Pointer to 128-bit plaintext block.
 *  \param[in] aesCback         Callback invoked after encryption is complete.
 *  \param[in] pParam           Pointer to generic callback parameter.
 *
 *  \see       meshSecToolAesCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolAesEncrypt(uint8_t *pAesKey, uint8_t *pPlainTextBlock,
                                          meshSecToolAesCback_t aesCback, void *pParam)
{
  uint8_t idx = 0;

  /* Validate input parameters */
  if (pAesKey == NULL || pPlainTextBlock == NULL || aesCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Get empty slot */
  for (idx = 0; idx < MESH_SEC_TOOL_AES_REQ_QUEUE_SIZE; idx++)
  {
    /* An entry which has a NULL callback is empty since it does not return to anything */
    if (secToolLocals.aesQueuePool[idx].cback == NULL)
    {
      break;
    }
  }

  /* If no slot is found, return error */
  if (idx == MESH_SEC_TOOL_AES_REQ_QUEUE_SIZE)
  {
    return MESH_SEC_TOOL_OUT_OF_MEMORY;
  }

  /* Configure parameters */
  secToolLocals.aesQueuePool[idx].cback = aesCback;
  secToolLocals.aesQueuePool[idx].pParam   = pParam;
  secToolLocals.aesQueuePool[idx].pKey     = pAesKey;
  secToolLocals.aesQueuePool[idx].pPlain   = pPlainTextBlock;

  /* If no request is in progress, request encryption, else enqueue */
  if (secToolLocals.pCrtAes == NULL)
  {
    secToolLocals.pCrtAes = &(secToolLocals.aesQueuePool[idx]);

    /* Call AES function */
    if (SecAesRev(pAesKey, pPlainTextBlock, secToolLocals.handlerId, 0, MESH_SEC_AES_EVENT) ==
        SEC_TOKEN_INVALID)
    {
      /* Reset slot by setting callback to NULL */
      secToolLocals.pCrtAes->cback = NULL;

      /* Set current request to NULL*/
      secToolLocals.pCrtAes = NULL;

      return MESH_SEC_TOOL_UNKNOWN_ERROR;
    }
  }
  else
  {
    /* Enqueue element from container */
    WsfQueueEnq(&(secToolLocals.aesQueue), (void *)(&(secToolLocals.aesQueuePool[idx])));
  }

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox AES-CMAC primitive.
 *
 *  \param[in] pKey          Pointer to 128-bit AES Key.
 *  \param[in] pPlainText    Pointer to plain text array.
 *  \param[in] plainTextLen  Length of the plain text.
 *  \param[in] cmacCback     Callback invoked after CMAC calculation is complete.
 *  \param[in] pParam        Pointer to generic parameter for the callback.
 *
 *  \see       meshSecToolCmacCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolCmacCalculate(uint8_t *pKey, uint8_t *pPlainText, uint16_t plainTextLen,
                                             meshSecToolCmacCback_t cmacCback, void *pParam)
{
  uint8_t idx = 0;

  if (pKey == NULL || (pPlainText == NULL && plainTextLen != 0) || cmacCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Get empty slot */
  for (idx = 0; idx < MESH_SEC_TOOL_CMAC_REQ_QUEUE_SIZE; idx++)
  {
    /* An entry which has a NULL callback is empty since it does not return to anything */
    if (secToolLocals.cmacQueuePool[idx].cback == NULL)
    {
      break;
    }
  }

  /* If no slot is found, return error */
  if (idx == MESH_SEC_TOOL_CMAC_REQ_QUEUE_SIZE)
  {
    return MESH_SEC_TOOL_OUT_OF_MEMORY;
  }

  /* Configure parameters */
  secToolLocals.cmacQueuePool[idx].cback = cmacCback;
  secToolLocals.cmacQueuePool[idx].pParam   = pParam;
  secToolLocals.cmacQueuePool[idx].pKey     = pKey;
  secToolLocals.cmacQueuePool[idx].pIn      = pPlainText;
  secToolLocals.cmacQueuePool[idx].len      = plainTextLen;

  return meshSecToolInternalCmacCalculate(&(secToolLocals.cmacQueuePool[idx]));
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox AES-CCM primitive.
 *
 *  \param[in] opType     Type of operation (encrypt or decrypt).
 *  \param[in] pOpParams  Pointer to structure holding the parameters to configure CCM.
 *  \param[in] ccmCback   Callback invoked after CCM operation is complete.
 *  \param[in] pParam     Pointer to generic parameter for the callback.
 *
 *  \see       meshSecToolCcmCback_t
 *  \see       meshSecToolCcmOperation_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  Usage:
 *  \code
 *  //global variables
 *  uint8_t ccmKey[16] = { 0x00, 0x01, 0x02 };
 *  uint8_t plain[5] = {'P','L','A','I','N'}, cipher[5] = { 0 };
 *  uint8_t authData[3] = { 0x00, 0x02, 0x03 };
 *  uint8_t cbcMac[8] = { 0 };
 *  uint8_t nonce[13] = { 0x01, 0x02, 0x02, 0x01, 0x04, 0x08,
 *                        0xFE, 0xAB, 0xAA, 0x90, 0x11, 0x22, 0x05 };
 *
 *  void ccmCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
 *  {
 *    if(pCcmResult->op == (meshSecToolCcmOperation_t)MESH_SEC_TOOL_CCM_ENCRYPT)
 *    {
 *      //pCipherText points to cipher array
 *      PRINT_ARRAY(pCcmResult->results.encryptResult.pCipherText,
 *                  pCcmResult->results.encryptResult.cipherTextLen);
 *
 *      //pCbcMac points to cbcMac array
 *      PRINT_ARRAY(pCcmResult->results.encryptResult.pCbcMac,
 *                  pCcmResult->results.encryptResult.cbcMacSize);
 *    }
 *    else
 *    {
 *      //pPlainText points to plain array
 *      PRINT_ARRAY(pCcmResult->results.decryptResult.pPlainText,
 *                  pCcmResult->results.decryptResult.plainTextSize);
 *
 *      PRINT("AUTH has %s",
 *            (pCcmResult->results.decryptResult.isAuthSuccess) ? "SUCCEEDED":"FAILED");
 *    }
 *  }
 *  void myFunc(void)
 *  {
 *      meshSecToolCcmParams_t params;
 *      meshSecToolCcmOperation_t opType;
 *
 *      params.pNonce = nonce;
 *      params.pCcmKey = ccmKey;
 *      params.pAuthData = authData;
 *      params.authDataLen = sizeof(authData);
 *      params.inputLen = sizeof(plain); //sizeof(cipher);
 *      params.pCbcMac = cbcMac;
 *      params.cbcMacSize = sizeof(cbcMac);
 *
 *      if(encrypt)
 *      {
 *        params.pIn = plain;
 *        params.pOut = cipher;
 *        opType = (meshSecToolCcmOperation_t)MESH_SEC_TOOL_CCM_ENCRYPT;
 *      }
 *      else //ciphertext located in cipher, cbcMac already contains 8 byte MAC
 *      {
 *        params.pIn = cipher;
 *        params.pOut = plain;
 *        opType = (meshSecToolCcmOperation_t)MESH_SEC_TOOL_CCM_DECRYPT;
 *      }
 *      retVal = MeshSecToolCcmEncryptDecrypt(opType, &params, ccmCback, NULL);
 *      //check retval
 *  }
 *  \endcode
 *
 *  \remarks The caller should not overwrite the memory referenced by input pointers until the
 *           callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolCcmEncryptDecrypt(meshSecToolCcmOperation_t opType,
                                                 meshSecToolCcmParams_t *pOpParams,
                                                 meshSecToolCcmCback_t ccmCback, void *pParam)
{
  uint8_t idx = 0;
  bool_t ccmRes = FALSE;

  if ((opType != MESH_SEC_TOOL_CCM_ENCRYPT && opType != MESH_SEC_TOOL_CCM_DECRYPT) ||
      ccmCback == NULL ||
      pOpParams == NULL   ||
      pOpParams->pIn == NULL   ||
      pOpParams->pOut == NULL  ||
      pOpParams->pCbcMac == NULL ||
      pOpParams->pNonce  == NULL ||
      (pOpParams->pAuthData != NULL && pOpParams->authDataLen == 0) ||
      (pOpParams->pAuthData == NULL && pOpParams->authDataLen != 0) ||
      pOpParams->cbcMacSize < 4 ||
      pOpParams->cbcMacSize > MESH_SEC_TOOL_AES_BLOCK_SIZE ||
      (pOpParams->cbcMacSize & 0x01) ||
      (pOpParams->inputLen > MESH_SEC_TOOL_CCM_MAX_BUFF) ||
      pOpParams->inputLen == 0)

  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Get empty slot */
  for (idx = 0; idx < MESH_SEC_TOOL_CCM_REQ_QUEUE_SIZE; idx++)
  {
    /* An entry which has a NULL callback is empty since it does not return to anything */
    if (secToolLocals.ccmQueuePool[idx].cback == NULL)
    {
      break;
    }
  }

  /* If no slot is found, return error */
  if (idx == MESH_SEC_TOOL_CCM_REQ_QUEUE_SIZE)
  {
    return MESH_SEC_TOOL_OUT_OF_MEMORY;
  }

  /* Configure parameters */
  memcpy(&(secToolLocals.ccmQueuePool[idx].ccmParams), pOpParams,
         sizeof(meshSecToolCcmParams_t));
  secToolLocals.ccmQueuePool[idx].isEncrypt = (opType == MESH_SEC_TOOL_CCM_ENCRYPT);
  secToolLocals.ccmQueuePool[idx].cback  = ccmCback;
  secToolLocals.ccmQueuePool[idx].pParam = pParam;

  /* If no request is in progress, request operation, else enqueue */
  if (secToolLocals.pCrtCcm == NULL)
  {
    secToolLocals.pCrtCcm = &(secToolLocals.ccmQueuePool[idx]);

    /* Check what API to use for CCM. */
    if (secToolLocals.pCrtCcm->isEncrypt)
    {
      /* Encrypt. */
      ccmRes = SecCcmEnc(pOpParams->pCcmKey, pOpParams->pNonce, pOpParams->pIn, pOpParams->inputLen,
                         pOpParams->pAuthData, pOpParams->authDataLen, pOpParams->cbcMacSize,
                         ccmResultBuff, secToolLocals.handlerId, 0, MESH_SEC_CCM_ENC_EVENT);
    }
    else
    {
      /* Decrypt. */
      ccmRes = SecCcmDec(pOpParams->pCcmKey, pOpParams->pNonce, pOpParams->pIn, pOpParams->inputLen,
                         pOpParams->pAuthData, pOpParams->authDataLen, pOpParams->pCbcMac,
                         pOpParams->cbcMacSize, ccmResultBuff, secToolLocals.handlerId, 0,
                         MESH_SEC_CCM_DEC_EVENT);
    }
    if (!ccmRes)
    {
      /* Reset slot by setting callback to NULL */
      secToolLocals.pCrtCcm->cback = NULL;

      /* Set current request to NULL*/
      secToolLocals.pCrtCcm = NULL;

      return MESH_SEC_TOOL_UNKNOWN_ERROR;
    }
  }
  else
  {
    /* Enqueue element from container */
    WsfQueueEnq(&(secToolLocals.ccmQueue), (void *)(&(secToolLocals.ccmQueuePool[idx])));
  }

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox Generate P-256 ECC Key.
 *
 *  \param[in] eccKeyGenCback  Callback invoked after a fresh key has been generated.
 *
 *  \see       meshSecToolEccKeyGenCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolEccGenerateKey(meshSecToolEccKeyGenCback_t eccKeyGenCback)
{
  if (eccKeyGenCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Check if another request is in progress */
  if (secToolLocals.eccGenCback != NULL)
  {
    return MESH_SEC_TOOL_OUT_OF_MEMORY;
  }

  /* Trigger request to PAL */
  if (!SecEccGenKey(secToolLocals.handlerId, 0, MESH_SEC_ECC_GEN_EVENT))
  {
    return MESH_SEC_TOOL_UNKNOWN_ERROR;
  }

  /* Store callback */
  secToolLocals.eccGenCback = eccKeyGenCback;

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox compute ECDH shared secret.
 *
 *  \param[in] pPrivAPubB         Pointer to ECC Key containing remote public-local private keys.
 *  \param[in] sharedSecretCback  Callback invoked at the end of the shared secret calculation.
 *
 *  \see       meshSecToolEcdhCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 *  Usage:
 *  Declare a ::meshSecToolEccKey_t variable. Copy the contents of its private
 *  key to the priv key. Copy the received public key (x,y) pair to the pubX and pubY
 *  members and call this function passing the address of the variable and a callback as params.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolEccCompSharedSecret(const uint8_t *pPeerPubX,
                                                   const uint8_t *pPeerPubY,
                                                   const uint8_t *pLocalPriv,
                                                   meshSecToolEcdhCback_t sharedSecretCback)
{
  secEccKey_t eccKey;

  if (pPeerPubX == NULL || pPeerPubY == NULL ||
      pLocalPriv == NULL || sharedSecretCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Check if another request is in progress */
  if (secToolLocals.ecdhCback != NULL)
  {
    return MESH_SEC_TOOL_OUT_OF_MEMORY;
  }

  /* Copy ECC Key. */
  memcpy(eccKey.privKey,  pLocalPriv, MESH_SEC_TOOL_ECC_KEY_SIZE);
  memcpy(eccKey.pubKey_x, pPeerPubX, MESH_SEC_TOOL_ECC_KEY_SIZE);
  memcpy(eccKey.pubKey_y, pPeerPubY, MESH_SEC_TOOL_ECC_KEY_SIZE);

  /* Trigger request to PAL */
  if (!SecEccGenSharedSecret(&eccKey, secToolLocals.handlerId, 0, MESH_SEC_ECDH_EVENT))
  {
    return MESH_SEC_TOOL_UNKNOWN_ERROR;
  }

  /* Store callback */
  secToolLocals.ecdhCback = sharedSecretCback;

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox Salt s1 primitive.
 *
 *  \param[in] pPlainText    Pointer to non-zero length plaintext.
 *  \param[in] plainTextLen  Length of the plaintext.
 *  \param[in] saltCback     Callback invoked after salt is generated.
 *  \param[in] pParam        Pointer to generic callback parameter.
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \see       meshSecToolCmacCback_t
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolGenerateSalt(uint8_t *pPlainText, uint16_t plainTextLen,
                                            meshSecToolCmacCback_t saltCback, void *pParam)
{
  if (pPlainText == NULL || plainTextLen == 0)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  return MeshSecToolCmacCalculate((uint8_t *)meshSecToolZkey, pPlainText, plainTextLen, saltCback, pParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k1 derivation primitive.
 *
 *  Used to derive Device, Identity and Beacon Keys.
 *  Description:
 *  k1(N, SALT, P) = CMAC(T, P), T = CMAC(SALT, N), size(SALT) = 16B, size(N) >= 0B, size(P) >= 0B;
 *
 *  \param[in] pPlainText           Pointer to plaintext data (P).
 *  \param[in] plainTextSize        Size of the plaintext.
 *  \param[in] pSalt                Pointer to 128-bit Salt.
 *  \param[in] pTempKeyMaterial     Pointer to plaintext used to compute the T key (N).
 *  \param[in] tempKeyMaterialSize  Size of the plaintext used to compute the T key.
 *  \param[in] derivCompleteCback   Callback invoked after derivation is complete.
 *  \param[in] pParam               Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK1Derive(uint8_t *pPlainText, uint16_t plainTextSize, uint8_t *pSalt,
                                        uint8_t *pTempKeyMaterial, uint16_t tempKeyMaterialSize,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam)
{
  /* Validate parameters */
  if ((pPlainText == NULL && plainTextSize != 0) ||
      (pTempKeyMaterial == NULL && tempKeyMaterialSize != 0) ||
      pSalt == NULL ||
      derivCompleteCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Call generic derivation function */
  return meshSecToolKxDerive(MESH_SEC_TOOL_DERIV_K1,
                             pPlainText,
                             plainTextSize,
                             pSalt,
                             pTempKeyMaterial,
                             tempKeyMaterialSize,
                             derivCompleteCback,
                             pParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k2 derivation primitive.
 *
 *  Used to derive Encryption and Privacy Keys, and NID.
 *  Description:
 *  k2(N, P) = (T1 || T2 || T3) mod (1 << 263), size(N) = 16B, size(P) >= 1B
 *  T = CMAC(SALT, N), SALT = s1("smk2")
 *  T0 = empty string, Tn = CMAC(T, (Tn-1 || P || n)), 0 < n < 4
 *
 *  \param[in] pPlainText          Pointer to plaintext data (P).
 *  \param[in] plainTextSize       Size of the plaintext (at least 1 byte).
 *  \param[in] pTempKeyMaterial    Pointer to 128-bit plaintext used to compute the T key (N).
 *  \param[in] derivCompleteCback  Callback invoked after derivation is complete.
 *  \param[in] pParam              Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 *
 *  \note      This implementation does not allow plainTextSize larger than 15 bytes.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK2Derive(uint8_t *pPlainText, uint16_t plainTextSize,
                                        uint8_t *pTempKeyMaterial,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam)
{
  /* Validate parameters */
  if (pPlainText == NULL ||
      plainTextSize == 0 ||
      plainTextSize > (MESH_SEC_TOOL_AES_BLOCK_SIZE - 1) || /* Max allowed to fit temp Kx buffer */
      pTempKeyMaterial == NULL ||
      derivCompleteCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Call generic derivation function */
  return meshSecToolKxDerive(MESH_SEC_TOOL_DERIV_K2,
                             pPlainText,
                             plainTextSize,
                             NULL,
                             pTempKeyMaterial,
                             0,
                             derivCompleteCback,
                             pParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k3 derivation primitive.
 *
 *  Used to derive 64 bit Network ID
 *  Description:
 *  k3(N) = CMAC(T, ("id64" || 0x01)) mod (1 << 64); size(N) = 16B
 *  T = CMAC(SALT, N), SALT = s1("smk3")
 *
 *  \param[in] pPlainText          Pointer to 128-bit plaintext used to compute the T key (N).
 *  \param[in] derivCompleteCback  Callback invoked after derivation is complete.
 *  \param[in] pParam              Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK3Derive(uint8_t *pPlainText,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam)
{
  /* Validate parameters */
  if (pPlainText == NULL || derivCompleteCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Call generic derivation function */
  return meshSecToolKxDerive(MESH_SEC_TOOL_DERIV_K3,
                             pPlainText,
                             0,
                             NULL,
                             NULL,
                             0,
                             derivCompleteCback,
                             pParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k4 derivation primitive.
 *
 *  Used to derive 6 bits AID
 *  Description:
 *  k4(N) = CMAC(T, ("id6" || 0x01)) mod (1 << 6); size(N) = 16B
 *  T = CMAC(SALT, N), SALT = s1("smk4")
 *
 *  \param[in] pPlainText           Pointer to 128-bit plaintext used to compute the T key (N).
 *  \param[in] derivCompleteCaback  Callback invoked after derivation is complete.
 *  \param[in] pParam               Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK4Derive(uint8_t *pPlainText,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam)
{
  /* Validate parameters */
  if (pPlainText == NULL || derivCompleteCback == NULL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Call generic derivation function */
  return meshSecToolKxDerive(MESH_SEC_TOOL_DERIV_K4,
                             pPlainText,
                             0,
                             NULL,
                             NULL,
                             0,
                             derivCompleteCback,
                             pParam);
}
