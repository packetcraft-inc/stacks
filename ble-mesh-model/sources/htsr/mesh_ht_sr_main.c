/*************************************************************************************************/
/*!
 *  \file   mesh_ht_sr_main.c
 *
 *  \brief  Implementation of the Health Server model.
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
#include "wsf_timer.h"
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_defs.h"
#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mesh_ht_mdl_api.h"
#include "mesh_ht_sr_api.h"
#include "mesh_ht_sr_main.h"

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t meshHtSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t meshHtSrRcvdOpcodes[] =
{
  { MESH_HT_FAULT_GET_OPCODE },
  { MESH_HT_FAULT_CLEAR_UNACK_OPCODE },
  { MESH_HT_FAULT_CLEAR_OPCODE },
  { MESH_HT_FAULT_TEST_OPCODE },
  { MESH_HT_FAULT_TEST_UNACK_OPCODE },
  { MESH_HT_PERIOD_GET_OPCODE },
  { MESH_HT_PERIOD_SET_UNACK_OPCODE },
  { MESH_HT_PERIOD_SET_OPCODE },
  { MESH_HT_ATTENTION_GET_OPCODE },
  { MESH_HT_ATTENTION_SET_OPCODE },
  { MESH_HT_ATTENTION_SET_UNACK_OPCODE },
};

/*! Generic On Off Server Control Block */
meshHtSrCb_t  htSrCb;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Number of operations handled by the Health Server */
static const uint8_t htSrNumOps = sizeof(meshHtSrRcvdOpcodes) / sizeof(meshMsgOpcode_t);

/*! Handler functions for supported opcodes */
static const meshHtSrHandleMsg_t meshHtSrHandleMsg[] =
{
  meshHtSrHandleFaultGet,
  meshHtSrHandleFaultClearUnack,
  meshHtSrHandleFaultClear,
  meshHtSrHandleFaultTest,
  meshHtSrHandleFaultTestUnack,
  meshHtSrHandlePeriodGet,
  meshHtSrHandlePeriodSetUnack,
  meshHtSrHandlePeriodSet,
  meshHtSrHandleAttentionGet,
  meshHtSrHandleAttentionSet,
  meshHtSrHandleAttentionSetUnack,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles the Health Server Current Health fast publication timer callback.
 *
 *  \param[in] elementId  Element identifier for which the fast publication timer is on.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtSrHandleCrtHtTmrCback(meshElementId_t elementId)
{
  meshHtSrDescriptor_t *pDesc = NULL;

  /* Get model instance descriptor */
  meshHtSrGetDesc(elementId, &pDesc);

  if(pDesc == NULL)
  {
    return;
  }

  /* Publish current health. */
  meshHtSrPublishCrtHt(elementId);

  /* Check if timer needs restart. */
  if((pDesc->fastPubOn) && (pDesc->fastPeriodDiv != 0) && (pDesc->pubPeriodMs != 0))
  {
    /* Restart publication timer. */
    WsfTimerStartMs(&(pDesc->fastPubTmr), FAST_PUB_TIME(pDesc));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Removes a fault or all faults from a fault state array based on company identifier.
 *
 *  \param[in] elementId     Local element identifier for a Health Server model instance.
 *  \param[in] companyId     Company identifier (16-bit).
 *  \param[in] recentTestId  ID of the most recently performed self-test.
 *  \param[in] faultId       Identifier of the fault to be removed.
 *  \param[in] removeAll     TRUE if all current faults should be removed.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtSrRemoveFault(meshElementId_t elementId, uint16_t companyId,
                                meshHtMdlTestId_t recentTestId, meshHtFaultId_t faultId,
                                bool_t removeAll)
{
  meshHtSrDescriptor_t *pDesc;
  uint8_t cidx, fidx;
  bool_t faultsPresent = FALSE;
  bool_t compMatch = FALSE;

  /* Get descriptor */
  meshHtSrGetDesc(elementId, &pDesc);

  if(pDesc == NULL)
  {
    MESH_TRACE_WARN0("HT SR: Fault remove invalid element id");
    return;
  }

  /* Search matching companyId. */
  for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
  {
    if((pDesc->faultStateArray[cidx].companyId == companyId) && !compMatch)
    {
      compMatch = TRUE;

      /* Set recent test identifier. */
      pDesc->faultStateArray[cidx].testId = recentTestId;

      if(!removeAll)
      {
        if(faultId != MESH_HT_MODEL_FAULT_NO_FAULT)
        {
          /* Search matching entries in current fault array. */
          for(fidx = 0; fidx < MESH_HT_SR_MAX_NUM_FAULTS; fidx++)
          {
            if(pDesc->faultStateArray[cidx].crtFaultIdArray[fidx] == faultId)
            {
              /* Clear fault. */
              pDesc->faultStateArray[cidx].crtFaultIdArray[fidx] = MESH_HT_MODEL_FAULT_NO_FAULT;
              break;
            }
          }
        }
      }
      else
      {
        /* Reset all. */
        memset(pDesc->faultStateArray[cidx].crtFaultIdArray, MESH_HT_MODEL_FAULT_NO_FAULT,
               MESH_HT_SR_MAX_NUM_FAULTS);
      }
      /* Check if fast publication is not on. This happens on calling this function
       * when no faults where initially present.
       */
      if(!(pDesc->fastPubOn))
      {
        return;
      }
    }
    if(!faultsPresent)
    {
      faultsPresent = htSrGetNumFaults(pDesc->faultStateArray[cidx].crtFaultIdArray) != 0;
    }
  }
  /* Check if fast publishing needs disabling. */
  if((pDesc->fastPubOn) && !faultsPresent)
  {
    pDesc->fastPubOn = FALSE;
    /* Stop timer. */
    WsfTimerStop(&(pDesc->fastPubTmr));
  }
  if(!compMatch)
  {
    MESH_TRACE_WARN0("HT SR: Remove fault, no matching company found");
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Health Server Handler.
 *
 *  \param[in] handlerId  WSF handler ID of the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID. */
  meshHtSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Health Server model.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshHtSrInit(void)
{
  meshHtSrDescriptor_t *pDesc;
  meshElementId_t elemId;
  uint16_t cidx;

  for(elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get descriptor. */
    meshHtSrGetDesc(elemId, &pDesc);

    if(pDesc == NULL)
    {
      if(elemId == 0)
      {
        MESH_TRACE_WARN0("HT SR: Specification mandates Health Server on primary element. ");
      }
      continue;
    }

    /* Reset internals. */
    pDesc->fastPeriodDiv = 0x00;
    pDesc->fastPubOn = FALSE;
    pDesc->fastPubTmr.handlerId = meshHtSrHandlerId;
    pDesc->fastPubTmr.msg.event = HT_SR_EVT_TMR_CBACK;
    pDesc->fastPubTmr.msg.param = elemId;
    for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
    {
      pDesc->faultStateArray[cidx].testId = 0x00;
      memset(pDesc->faultStateArray[cidx].crtFaultIdArray, MESH_HT_MODEL_FAULT_NO_FAULT,
             MESH_HT_SR_MAX_NUM_FAULTS);
      memset(pDesc->faultStateArray[cidx].regFaultIdArray, MESH_HT_MODEL_FAULT_NO_FAULT,
             MESH_HT_SR_MAX_NUM_FAULTS);
    }
  }

  /* Set event callback. */
  htSrCb.recvCback = MmdlEmptyCback;
}


/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback registered by the upper layer to receive event messages
 *                        from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback. */
  if (recvCback != NULL)
  {
    htSrCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Health Server model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrHandler(wsfMsgHdr_t *pMsg)
{
  meshHtSrDescriptor_t *pDesc;
  meshModelEvt_t *pModelMsg;
  uint8_t opcodeIdx;
  bool_t restartTmr = FALSE;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Match the received opcode. */
        for (opcodeIdx = 0; opcodeIdx < htSrNumOps; opcodeIdx++)
        {
          /* Validate opcode size and value */
          if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) !=
              MESH_OPCODE_SIZE(meshHtSrRcvdOpcodes[opcodeIdx]))
          {
            continue;
          }
          if (!memcmp(&meshHtSrRcvdOpcodes[opcodeIdx], &(pModelMsg->msgRecvEvt.opCode),
                      MESH_OPCODE_SIZE(meshHtSrRcvdOpcodes[opcodeIdx])))
          {
            /* Process message. */
            meshHtSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
            return;
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Get descriptor. */
        meshHtSrGetDesc(pModelMsg->periodicPubEvt.elementId, &pDesc);

        /* Unexpected. */
        if(pDesc == NULL)
        {
          return;
        }

        /* Check if new publication period differs from stored. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != pDesc->pubPeriodMs)
        {
          /* Copy new publication period. */
          pDesc->pubPeriodMs = pModelMsg->periodicPubEvt.nextPubTimeMs;
          restartTmr = TRUE;
        }

        /* Check if publication is not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Check if fast publication is on and divisor is non zero. */
          if((pDesc->fastPubOn) && (pDesc->fastPeriodDiv != 0))
          {
            if(restartTmr)
            {
              /* Publish current health at the beginning of the new fast period. */
              meshHtSrPublishCrtHt(pModelMsg->periodicPubEvt.elementId);

              /* Start fast publication timer. */
              WsfTimerStartMs(&(pDesc->fastPubTmr), FAST_PUB_TIME(pDesc));
            }
          }
          else
          {
            /* Publish current health on publication period expired event. */
            meshHtSrPublishCrtHt(pModelMsg->periodicPubEvt.elementId);
          }
        }
        else
        {
          /* Stop fast publication timer. */
          WsfTimerStop(&(pDesc->fastPubTmr));
        }
        break;

      case HT_SR_EVT_TMR_CBACK:
        meshHtSrHandleCrtHtTmrCback((meshElementId_t)(pMsg->param));
        break;

      default:
        MESH_TRACE_WARN0("HT SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets company ID of an entry in the Fault State array.
 *
 *  \param[in] elementId        Local element identifier for a Health Server model instance.
 *  \param[in] faultStateIndex  ID of the most recently performed self-test.
 *  \param[in] companyId        Company identifier (16-bit).
 *
 *  \remarks   Fault State Array is identified by the company ID. This means faultStateIndex should
 *             always be less than ::MESH_HT_SR_MAX_NUM_COMP.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrSetCompanyId(meshElementId_t elementId, uint8_t faultStateIndex, uint16_t companyId)
{
  meshHtSrDescriptor_t *pDesc;

  /* Get descriptor */
  meshHtSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    MESH_TRACE_WARN0("HT SR: Set Company ID, invalid element id");
    return;
  }

  /* Check for range. */
  if (faultStateIndex >= MESH_HT_SR_MAX_NUM_COMP)
  {
    MESH_TRACE_WARN0("HT SR: Set Company ID, invalid entry index");
    return;
  }

  /* Set company ID. */
  pDesc->faultStateArray[faultStateIndex].companyId = companyId;
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a fault ID to a fault state array based on company Identifier.
 *
 *  \param[in] elementId     Local element identifier for a Health Server model instance.
 *  \param[in] companyId     Company identifier (16-bit).
 *  \param[in] recentTestId  ID of the most recently performed self-test.
 *  \param[in] faultId       Identifier of the fault to be added.
 *
 *  \remarks   Call this function with ::MESH_HT_MODEL_FAULT_NO_FAULT fault identifier to update
 *             the most recent test id.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrAddFault(meshElementId_t elementId, uint16_t companyId,
                      meshHtMdlTestId_t recentTestId, meshHtFaultId_t faultId)
{
  meshHtSrDescriptor_t *pDesc;
  uint8_t cidx, fidx;
  uint8_t crtFaultEmptyPos = MESH_HT_SR_MAX_NUM_FAULTS;
  uint8_t regFaultEmptyPos = MESH_HT_SR_MAX_NUM_FAULTS;
  bool_t crtFaultExists = FALSE, regFaultExists = FALSE;

  /* Get descriptor */
  meshHtSrGetDesc(elementId, &pDesc);

  if(pDesc == NULL)
  {
    MESH_TRACE_WARN0("HT SR: Fault add invalid element id");
    return;
  }

  /* Search matching companyId. */
  for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
  {
    if(pDesc->faultStateArray[cidx].companyId == companyId)
    {
      /* Set recent test identifier. */
      pDesc->faultStateArray[cidx].testId = recentTestId;

      /* Do not add No Fault identifiers. */
      if(faultId == MESH_HT_MODEL_FAULT_NO_FAULT)
      {
        return;
      }

      /* Search empty entries in current and registered fault arrays. */
      for(fidx = 0; fidx < MESH_HT_SR_MAX_NUM_FAULTS; fidx++)
      {
        /* Check if duplicate exists. */
        if(pDesc->faultStateArray[cidx].crtFaultIdArray[fidx] == faultId)
        {
          crtFaultExists = TRUE;
        }

        /* Check if duplicate exists in registered fault. */
        if(pDesc->faultStateArray[cidx].regFaultIdArray[fidx] == faultId)
        {
          regFaultExists = TRUE;
        }

        /* Find empty position. */
        if((crtFaultEmptyPos == MESH_HT_SR_MAX_NUM_FAULTS) &&
           (pDesc->faultStateArray[cidx].crtFaultIdArray[fidx] == MESH_HT_MODEL_FAULT_NO_FAULT))
        {
          crtFaultEmptyPos = fidx;
        }
        /* Find empty position. */
        if((regFaultEmptyPos == MESH_HT_SR_MAX_NUM_FAULTS) &&
           (pDesc->faultStateArray[cidx].regFaultIdArray[fidx] == MESH_HT_MODEL_FAULT_NO_FAULT))
        {
          regFaultEmptyPos = fidx;
        }
      }

      /* Store current fault */
      if((crtFaultEmptyPos < MESH_HT_SR_MAX_NUM_FAULTS) && (!crtFaultExists))
      {
        pDesc->faultStateArray[cidx].crtFaultIdArray[crtFaultEmptyPos] = faultId;
      }
      else
      {
        MESH_TRACE_INFO0("HT SR: Add fault, current fault array full");
      }

      /* Store registered fault */
      if((regFaultEmptyPos < MESH_HT_SR_MAX_NUM_FAULTS) && (!regFaultExists))
      {
        pDesc->faultStateArray[cidx].regFaultIdArray[regFaultEmptyPos] = faultId;
      }
      else
      {
        MESH_TRACE_INFO0("HT SR: Add fault, registered fault array full");
      }

      /* Check if no fault condition was present. */
      if(!(pDesc->fastPubOn))
      {
        pDesc->fastPubOn = TRUE;

        if((pDesc->fastPeriodDiv != 0) && (pDesc->pubPeriodMs != 0))
        {
          /* Publish Current Health Status. */
          meshHtSrPublishCrtHt(elementId);
          /* Start publication timer. */
          WsfTimerStartMs(&(pDesc->fastPubTmr), FAST_PUB_TIME(pDesc));
        }
      }

      return;
    }
  }
  MESH_TRACE_WARN0("HT SR: Add fault, no matching company found");
}

/*************************************************************************************************/
/*!
 *  \brief     Removes a fault ID from a fault state array based on company identifier.
 *
 *  \param[in] elementId     Local element identifier for a Health Server model instance.
 *  \param[in] companyId     Company identifier (16-bit).
 *  \param[in] recentTestId  ID of the most recently performed self-test.
 *  \param[in] faultId       Identifier of the fault to be removed.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrRemoveFault(meshElementId_t elementId, uint16_t companyId,
                         meshHtMdlTestId_t recentTestId, meshHtFaultId_t faultId)
{
  meshHtSrRemoveFault(elementId, companyId, recentTestId, faultId, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Removes all fault IDs from a fault state array based on company identifier.
 *
 *  \param[in] elementId     Local element identifier for a Health Server model instance.
 *  \param[in] companyId     Company identifier (16-bit).
 *  \param[in] recentTestId  ID of the most recently performed self-test.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrClearFaults(meshElementId_t elementId, uint16_t companyId,
                         meshHtMdlTestId_t recentTestId)
{
  meshHtSrRemoveFault(elementId, companyId, recentTestId, MESH_HT_MODEL_FAULT_NO_FAULT, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Signals to a Health Client that a test has been performed.
 *
 *  \param[in] elementId     Health Server element identifier to determine instance.
 *  \param[in] companyId     Company identifier for determining fault state.
 *  \param[in] meshHtClAddr  Health Client address received in the ::MESH_HT_SR_TEST_START_EVENT.
 *  \param[in] appKeyIndex   AppKey identifier received in the ::MESH_HT_SR_TEST_START_EVENT.
 *  \param[in] useTtlZero    TTL flag received in the ::MESH_HT_SR_TEST_START_EVENT.
 *  \param[in] unicastReq    Unicast flag received in the ::MESH_HT_SR_TEST_START_EVENT.
 *
 *  \return    None.
 *
 *  \remarks   Upon receiving a ::MESH_HT_SR_TEST_START_EVENT event with the notifTestEnd parameter
 *             set to TRUE, the user shall call this API using the associated event parameters, but
 *             not before logging 0 or more faults so that the most recent test identifier is stored.
 *
 */
/*************************************************************************************************/
void MeshHtSrSignalTestEnd(meshElementId_t elementId, uint16_t companyId,
                           meshAddress_t meshHtClAddr, uint16_t appKeyIndex, bool_t useTtlZero,
                           bool_t unicastReq)
{
  /* Send status. */
  meshHtSrSendFaultStatus(companyId, elementId, meshHtClAddr, appKeyIndex,
                          useTtlZero ? 0 : MESH_USE_DEFAULT_TTL, unicastReq);
}
