/*************************************************************************************************/
/*!
 *  \file   mesh_access_periodic_pub.c
 *
 *  \brief  Periodic Publishing module implementation.
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
 *
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_cs.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_seq_manager.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_lower_transport.h"
#include "mesh_upper_transport.h"
#include "mesh_utils.h"
#include "mesh_security.h"
#include "mesh_access.h"
#include "mesh_access_main.h"
#include "mesh_access_period_pub.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Periodic publishing queue element */
typedef struct meshAccPpQueueElem_tag
{
  void             *pNext;         /*!< Pointer to next queue element */
  wsfTimer_t       ppTmr;          /*!< Periodic publishing timer */
  meshElementId_t  elemId;         /*!< Element identifier for the model instance */
  uint8_t          modelEntryIdx;  /*!< Model entry index in either SIG or vendor model list */
  bool_t           isSig;          /*!< TRUE if SIG model list should be used */
} meshAccPpQueueElem_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Step resolution to millisecond conversion table */
static const uint32_t ppStepResToMsTable[] =
{
  100, 1000, 10000, 60000
};

/*! Periodic publishing module control block */
static struct meshAccPpCb_tag
{
  meshAccPpQueueElem_t *pPpElemPool;      /*!< Pool of for Periodic publishing elements */
  uint16_t             numPoolElem;       /*!< Number of pool elements */
  wsfQueue_t           idlePpElemQueue;   /*!< Queue of idle Periodic publishing elements */
  wsfQueue_t           activePpElemQueue; /*!< Queue of used Periodic publishing elements */
} meshAccPpCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Calculates total number of model instances on this node.
 *
 *  \return Total number of model instances.
 */
/*************************************************************************************************/
static uint16_t meshAccPpGetNumModels(void)
{
  uint16_t numModelInstances = 0;
  uint8_t elemId;

  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    numModelInstances += pMeshConfig->pElementArray[elemId].numSigModels;
    numModelInstances += pMeshConfig->pElementArray[elemId].numVendorModels;
  }

  return numModelInstances;
}

/*************************************************************************************************/
/*!
 *  \brief     Send WSF message with Periodic publishing timer expired event to a model instance.
 *
 *  \param[in] pQueueElem  Pointer to an active queue element of the periodic publishing queue.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccPpSendWsfPubEvt(meshAccPpQueueElem_t *pQueueElem, uint32_t timeToPublishMs)
{
  meshModelPeriodicPubEvt_t *pEvt;
  wsfHandlerId_t handlerId;

  /* Get handler id. */
  if (pQueueElem->isSig)
  {
    WSF_ASSERT(SIG_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).pHandlerId
               != NULL);
    handlerId = *(SIG_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).pHandlerId);
  }
  else
  {
    WSF_ASSERT(VENDOR_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).pHandlerId
               != NULL);
    handlerId = *(VENDOR_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).pHandlerId);
  }

  /* Allocate and send event. */
  if ((pEvt = (meshModelPeriodicPubEvt_t *)WsfMsgAlloc(sizeof(meshModelPeriodicPubEvt_t))) != NULL)
  {
    pEvt->hdr.event = MESH_MODEL_EVT_PERIODIC_PUB;
    pEvt->elementId = pQueueElem->elemId;
    /* Time to publish should have freshly calculated publish period in ms. */
    pEvt->nextPubTimeMs = timeToPublishMs;

    /* Set model ID. */
    pEvt->isVendorModel = !pQueueElem->isSig;
    if (pQueueElem->isSig)
    {
      pEvt->modelId.sigModelId = SIG_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).modelId;
    }
    else
    {
      pEvt->modelId.vendorModelId = VENDOR_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).modelId;
    }

    WsfMsgSend(handlerId, (void *)pEvt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Re-arms next publication time.
 *
 *  \param[in]  pQueueElem     Pointer to an active queue element of the periodic publishing queue.
 *  \param[out] pOutPubTimeMs  Pointer to memory where publication time in milliseconds is stored.
 *
 *  \return     TRUE if publication time is valid and can be read, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshAccPpReloadTime(meshAccPpQueueElem_t *pQueueElem, uint32_t *pOutPubTimeMs)
{
  meshPublishPeriodStepRes_t  stepRes = 0;
  meshPublishPeriodNumSteps_t numSteps = 0;
  meshModelId_t mdlId;

  /* Build generic model id. */
  mdlId.isSigModel = pQueueElem->isSig;
  if (pQueueElem->isSig)
  {
    mdlId.modelId.sigModelId =
      SIG_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).modelId;
  }
  else
  {
    mdlId.modelId.vendorModelId =
      VENDOR_MODEL_INSTANCE(pQueueElem->elemId, pQueueElem->modelEntryIdx).modelId;
  }

  /* Read publish period. */
  if (MeshLocalCfgGetPublishPeriod(pQueueElem->elemId, &mdlId, &numSteps, &stepRes) != MESH_SUCCESS)
  {
    return FALSE;
  }

  /* Validate state. */
  if((numSteps == MESH_PUBLISH_PERIOD_DISABLED_NUM_STEPS) ||
     (numSteps > MESH_PUBLISH_PERIOD_NUM_STEPS_MAX) ||
     (stepRes > MESH_PUBLISH_PERIOD_STEP_RES_10MIN))
  {
    MESH_TRACE_ERR0("Mesh ACC: Periodic publish state modified without notification. ");
    return FALSE;
  }

  /* Re-arm value. */
  *pOutPubTimeMs = ppStepResToMsTable[stepRes] * numSteps;

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Timer callback for the Periodic publishing module.
 *
 *  \param[in] tmrUid  Unique timer identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccPpTmrCback(uint16_t tmrUid)
{
  meshAccPpQueueElem_t *ptr, *prev;
  uint32_t timeToPublish = 0;

  /* Point to start of the queue. */
  ptr = (meshAccPpQueueElem_t *)(meshAccPpCb.activePpElemQueue.pHead);
  prev = NULL;

  /* Search active queue elements for match. */
  while (ptr != NULL)
  {
    /* Check if timer identifiers match. */
    if (ptr->ppTmr.msg.param == tmrUid)
    {
      /* Reset time to publish. */
      timeToPublish = 0;

      /* Read states and re-arm. */
      if (!meshAccPpReloadTime(ptr, &timeToPublish))
      {
        /* Remove from active queue. */
        WsfQueueRemove(&(meshAccPpCb.activePpElemQueue), ptr, prev);

        /* Add back to idle queue due to error. */
        WsfQueueEnq(&(meshAccPpCb.idlePpElemQueue), ptr);
        return;
      }

      if(timeToPublish != 0)
      {
        /* Start timer. */
        WsfTimerStartMs(&(ptr->ppTmr), timeToPublish);
      }

      /* Send WSF message with event. */
      meshAccPpSendWsfPubEvt(ptr, timeToPublish);

      return;
    }

    /* Move to next entry. */
    prev = ptr;
    ptr = (meshAccPpQueueElem_t *)(ptr->pNext);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler callback.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccPpWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type to handle timer expiration. */
  switch(pMsg->event)
  {
    case MESH_ACC_MSG_PP_TMR_EXPIRED:
      meshAccPpTmrCback(pMsg->param);
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Periodic publishing state changed on a model instance.
 *
 *  \param[in] elemId         Element identifier.
 *  \param[in] modelEntryIdx  Index in an element's model list (either SIG or vendor model list).
 *  \param[in] pModelId       Pointer to model identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccHandlePpChanged(meshElementId_t elemId, uint8_t modelEntryIdx,
                                   meshModelId_t *pModelId)
{
  uint8_t *pLabelUuid;
  meshAccPpQueueElem_t *ptr, *prev;
  uint32_t timeToPublish = 0;
  meshPublishPeriodStepRes_t stepRes = 0;
  meshPublishPeriodNumSteps_t numSteps = MESH_PUBLISH_PERIOD_DISABLED_NUM_STEPS;
  meshAddress_t pubAddr;

  /* Read publish address to see if publication is enabled. */
  if (MeshLocalCfgGetPublishAddress(elemId, pModelId, &pubAddr, &pLabelUuid) != MESH_SUCCESS)
  {
    return;
  }

  /* Check if publication is not disabled. */
  if (!MESH_IS_ADDR_UNASSIGNED(pubAddr))
  {
    /* Read params from Local config. */
    if (MeshLocalCfgGetPublishPeriod(elemId, pModelId, &numSteps, &stepRes) != MESH_SUCCESS)
    {
      return;
    }
  }

  /* Point to start of the queue. */
  ptr = (meshAccPpQueueElem_t *)(meshAccPpCb.activePpElemQueue.pHead);
  prev = NULL;

  /* Search active queue elements for match. */
  while (ptr != NULL)
  {
    /* If active element has publication state changed. */
    if ((ptr->elemId == elemId) && (ptr->modelEntryIdx == modelEntryIdx) &&
        (ptr->isSig == pModelId->isSigModel))
    {
      /* If periodic publishing is disabled, move to idle queue. */
      if (numSteps == MESH_PUBLISH_PERIOD_DISABLED_NUM_STEPS)
      {
        timeToPublish = 0;

        /* Stop timer. */
        WsfTimerStop(&(ptr->ppTmr));

        /* Remove from active queue. */
        WsfQueueRemove(&(meshAccPpCb.activePpElemQueue), ptr, prev);
        WsfQueueEnq(&(meshAccPpCb.idlePpElemQueue), ptr);
      }
      else
      {
        timeToPublish = ppStepResToMsTable[stepRes] * numSteps;

        /* Start timer. */
        WsfTimerStartMs(&(ptr->ppTmr), timeToPublish);
      }
      /* Send event. */
      meshAccPpSendWsfPubEvt(ptr, timeToPublish);
      return;
    }

    /* Move to next entry. */
    prev = ptr;
    ptr = (meshAccPpQueueElem_t *)(ptr->pNext);
  }

  /* No match found. This means there is a new model that has periodic publication started. */

  /* If periodic publishing is disabled, nothing to do. */
  if (numSteps == MESH_PUBLISH_PERIOD_DISABLED_NUM_STEPS)
  {
    return;
  }

  /* Dequeue element from idle queue. */
  ptr = (meshAccPpQueueElem_t *) WsfQueueDeq(&(meshAccPpCb.idlePpElemQueue));

  WSF_ASSERT(ptr != NULL);

  if (ptr != NULL)
  {
    ptr->elemId = elemId;
    ptr->modelEntryIdx = modelEntryIdx;
    ptr->isSig = pModelId->isSigModel;
    timeToPublish = ppStepResToMsTable[stepRes] * numSteps;

    /* If timer is not started. */
    WsfTimerStartMs(&(ptr->ppTmr), timeToPublish);

    /* Enqueue in active queue. */
    WsfQueueEnq(&(meshAccPpCb.activePpElemQueue), (void *)ptr);

    /* Send event. */
    meshAccPpSendWsfPubEvt(ptr, timeToPublish);
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Gets memory required for configuration.
 *
 *  \return Configuration memory required or ::MESH_MEM_REQ_INVALID_CFG on error.
 */
/*************************************************************************************************/
uint32_t MeshAccGetRequiredMemory(void)
{
  return MESH_UTILS_ALIGN(meshAccPpGetNumModels() * sizeof(meshAccPpQueueElem_t));
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the periodic publishing feature in the Access Layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAccPeriodicPubInit(void)
{
  uint16_t mdlInstanceIdx;

  /* Register feature callbacks. */
  meshAccCb.ppChangedCback = MeshAccPpChanged;
  meshAccCb.ppWsfMsgCback = meshAccPpWsfMsgHandlerCback;

  /* Get number of models. */
  meshAccPpCb.numPoolElem = meshAccPpGetNumModels();

  /* Allocate memory. */
  meshAccPpCb.pPpElemPool = (meshAccPpQueueElem_t *)meshCb.pMemBuff;
  /* Forward pointer. */
  meshCb.pMemBuff += MESH_UTILS_ALIGN(meshAccPpCb.numPoolElem * sizeof(meshAccPpQueueElem_t));
  /* Decrement used memory. */
  meshCb.memBuffSize -= MESH_UTILS_ALIGN(meshAccPpCb.numPoolElem * sizeof(meshAccPpQueueElem_t));

  /* Reset memory. */
  memset(meshAccPpCb.pPpElemPool, 0, meshAccPpCb.numPoolElem * sizeof(meshAccPpQueueElem_t));

  /* Initialize queues. */
  WSF_QUEUE_INIT(&(meshAccPpCb.idlePpElemQueue));
  WSF_QUEUE_INIT(&(meshAccPpCb.activePpElemQueue));

  /* Configure timers and add all to idle queue. */
  for (mdlInstanceIdx = 0; mdlInstanceIdx < meshAccPpCb.numPoolElem; mdlInstanceIdx++)
  {
    meshAccPpCb.pPpElemPool[mdlInstanceIdx].ppTmr.msg.event = MESH_ACC_MSG_PP_TMR_EXPIRED;
    meshAccPpCb.pPpElemPool[mdlInstanceIdx].ppTmr.msg.param = mdlInstanceIdx;
    meshAccPpCb.pPpElemPool[mdlInstanceIdx].ppTmr.handlerId = meshCb.handlerId;

    WsfQueueEnq(&(meshAccPpCb.idlePpElemQueue), &(meshAccPpCb.pPpElemPool[mdlInstanceIdx]));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Informs the module that the periodic publishing value of a model instance has
 *             changed.
 *
 *  \param[in] elemId    Element identifier.
 *  \param[in] pModelId  Pointer to a model identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccPpChanged(meshElementId_t elemId, meshModelId_t *pModelId)
{
  uint8_t mdlInstanceIdx;

  WSF_ASSERT(elemId < pMeshConfig->elementArrayLen);
  WSF_ASSERT(pModelId != NULL);

  /* Find entry in model idx. */
  if (pModelId->isSigModel)
  {
    for (mdlInstanceIdx = 0; mdlInstanceIdx < pMeshConfig->pElementArray[elemId].numSigModels;
         mdlInstanceIdx++)
    {
      if (pModelId->modelId.sigModelId == SIG_MODEL_INSTANCE(elemId,mdlInstanceIdx).modelId)
      {
        /* Handle Periodic publishing state changed. */
        meshAccHandlePpChanged(elemId, mdlInstanceIdx, pModelId);
        return;
      }
    }
  }
  else
  {
    for (mdlInstanceIdx = 0; mdlInstanceIdx < pMeshConfig->pElementArray[elemId].numVendorModels;
         mdlInstanceIdx++)
    {
      if (pModelId->modelId.vendorModelId == VENDOR_MODEL_INSTANCE(elemId, mdlInstanceIdx).modelId)
      {
        /* Handle Periodic publishing state changed. */
        meshAccHandlePpChanged(elemId, mdlInstanceIdx, pModelId);
        return;
      }
    }
  }

  MESH_TRACE_ERR0("MESH ACC: Invalid params for configuring periodic publishing ");
}
