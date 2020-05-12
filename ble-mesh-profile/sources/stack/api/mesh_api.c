/*************************************************************************************************/
/*!
 *  \file   mesh_api.c
 *
 *  \brief  Main API implementation.
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
#include "wsf_assert.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_trace.h"
#include "sec_api.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"

#include "mesh_api.h"
#include "mesh_handler.h"
#include "mesh_main.h"

#include "mesh_security_toolbox.h"
#include "mesh_security.h"

#include "mesh_seq_manager.h"

#include "mesh_local_config_types.h"
#include "mesh_local_config.h"

#include "mesh_adv_bearer.h"
#include "mesh_gatt_bearer.h"
#include "mesh_bearer.h"

#include "mesh_network.h"
#include "mesh_network_beacon.h"
#include "mesh_network_mgmt.h"

#include "mesh_lower_transport.h"
#include "mesh_sar_rx.h"
#include "mesh_sar_tx.h"

#include "mesh_replay_protection.h"

#include "mesh_upper_transport.h"
#include "mesh_upper_transport_heartbeat.h"

#include "mesh_access.h"
#include "mesh_access_period_pub.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_cl.h"
#include "mesh_proxy_cl.h"

#include "mesh_utils.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh callback event length table */
static const uint16_t meshEvtCbackLen[] =
{
  sizeof(wsfMsgHdr_t),                /*!< MESH_CORE_RESET_EVENT */
  sizeof(wsfMsgHdr_t),                /*!< MESH_CORE_ERROR_EVENT */
  sizeof(wsfMsgHdr_t),                /*!< MESH_CORE_SEND_MSG_EVENT */
  sizeof(wsfMsgHdr_t),                /*!< MESH_CORE_PUBLISH_MSG_EVENT */
  sizeof(meshGattConnEvt_t),          /*!< MESH_CORE_GATT_CONN_ADD_EVENT */
  sizeof(meshGattConnEvt_t),          /*!< MESH_CORE_GATT_CONN_REMOVE_EVENT */
  sizeof(meshGattConnEvt_t),          /*!< MESH_CORE_GATT_CONN_CLOSE_EVENT */
  sizeof(meshGattConnEvt_t),          /*!< MESH_CORE_GATT_PROCESS_PROXY_PDU_EVENT */
  sizeof(meshGattConnEvt_t),          /*!< MESH_CORE_GATT_SIGNAL_IF_RDY_EVENT */
  sizeof(meshAdvIfEvt_t),             /*!< MESH_CORE_ADV_IF_ADD_EVENT */
  sizeof(meshAdvIfEvt_t),             /*!< MESH_CORE_ADV_IF_REMOVE_EVENT */
  sizeof(meshAdvIfEvt_t),             /*!< MESH_CORE_ADV_IF_CLOSE_EVENT */
  sizeof(meshAdvIfEvt_t),             /*!< MESH_CORE_ADV_PROCESS_PDU_EVENT */
  sizeof(meshAdvIfEvt_t),             /*!< MESH_CORE_ADV_SIGNAL_IF_RDY_EVENT */
  sizeof(meshAttentionEvt_t),         /*!< MESH_CORE_ATTENTION_SET_EVENT */
  sizeof(meshAttentionEvt_t),         /*!< MESH_CORE_ATTENTION_CHG_EVENT */
  sizeof(meshNodeStartedEvt_t),       /*!< MESH_CORE_NODE_STARTED_EVENT */
  sizeof(meshProxyServiceDataEvt_t),  /*!< MESH_CORE_PROXY_SERVICE_DATA_EVENT */
  sizeof(meshProxyFilterStatusEvt_t), /*!< MESH_CORE_PROXY_FILTER_STATUS_EVENT */
  sizeof(meshIvUpdtEvt_t),            /*!< MESH_CORE_IV_UPDATED_EVENT */
  sizeof(meshHbInfoEvt_t),            /*!< MESH_CORE_HB_INFO_EVENT */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Stack Control block */
meshCb_t meshCb;

/*! Mesh Configuration */
meshConfig_t *pMeshConfig;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Execute Mesh Stack event notification callback function.
 *
 *  \param[in] event   Event value. See ::meshEventValues
 *  \param[in] status  Event status value. See ::meshReturnValues
 *  \param[in] handle  Attribute handle.
 *  \param[in] status  Callback event status.
 *  \param[in] mtu     Negotiated MTU value.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshExecCback(uint8_t event, uint8_t status, uint16_t param)
{
  wsfMsgHdr_t evt;

  evt.event = event;
  evt.status = status;
  evt.param = param;

  (*meshCb.evtCback)((meshEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security all keys material derivation complete callback.
 *
 *  \param[in] isSuccess  TRUE if operation is successful.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecAllKeyMaterialRestoreCback(bool_t isSuccess)
{
  meshNodeStartedEvt_t evt;
  meshElementId_t elemId;
  meshAddress_t primaryElemAddr;
  uint8_t mdlIdx = 0;
  meshModelId_t mdlId;

  WSF_ASSERT(isSuccess);

  MeshLocalCfgGetAddrFromElementId(0, &primaryElemAddr);

  evt.hdr.event = MESH_CORE_EVENT;
  evt.hdr.param = MESH_CORE_NODE_STARTED_EVENT;
  evt.hdr.status = isSuccess ? MESH_SUCCESS : MESH_UNKNOWN_ERROR;
  evt.address = primaryElemAddr;
  evt.elemCnt = pMeshConfig->elementArrayLen;

  MESH_TRACE_INFO0("MESH API: Node Started!");

  /* Signal Network state changed. */
  MeshNwkBeaconHandleStateChanged();

  /* Signal Heartbeat module. */
  MeshHbPublicationStateChanged();
  MeshHbSubscriptionStateChanged();

  /* Notify Periodic publishing module. */
  for(elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    for(mdlIdx = 0; mdlIdx < pMeshConfig->pElementArray[elemId].numSigModels; mdlIdx++)
    {
      mdlId.isSigModel = TRUE;
      mdlId.modelId.sigModelId = pMeshConfig->pElementArray[elemId].pSigModelArray[mdlIdx].modelId;
      MeshAccPpChanged(elemId, &mdlId);
    }

    for(mdlIdx = 0; mdlIdx < pMeshConfig->pElementArray[elemId].numVendorModels; mdlIdx++)
    {
      mdlId.isSigModel = FALSE;
      mdlId.modelId.vendorModelId =
          pMeshConfig->pElementArray[elemId].pVendorModelArray[mdlIdx].modelId;
      MeshAccPpChanged(elemId, &mdlId);
    }
  }

  /* Send event to application. */
  meshCb.evtCback((meshEvt_t *)&evt);

  (void)isSuccess;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Stack empty event notification callback.
 *
 *  \param[in] pEvt  Mesh Stack callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshEmptyCback(meshEvt_t *pEvt)
{
  (void)pEvt;
  MESH_TRACE_WARN0("MESH API: Mesh event notification callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Stack empty event handler.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshEmptyHandler(wsfMsgHdr_t *pMsg)
{
  (void)pMsg;
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh API WSF message handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    Handler to process the WSF message.
 */
/*************************************************************************************************/
static void meshApiMsgCback(wsfMsgHdr_t *pMsg)
{
  switch (pMsg->event)
  {
    case MESH_MSG_API_INIT:
      break;

    case MESH_MSG_API_RESET:
      break;

    case MESH_MSG_API_SEND_MSG:
      MeshAccSendMessage(&(((meshSendMessage_t *)pMsg)->msgInfo),
                         ((meshSendMessage_t *)pMsg)->pMsgParam,
                         ((meshSendMessage_t *)pMsg)->msgParamLen,
                         ((meshSendMessage_t *)pMsg)->netKeyIndex,
                         0,
                         0);
      break;

    case MESH_MSG_API_PUBLISH_MSG:
      MeshAccPublishMessage(&(((meshPublishMessage_t *)pMsg)->pubMsgInfo),
                            ((meshPublishMessage_t *)pMsg)->pMsgParam,
                            ((meshPublishMessage_t *)pMsg)->msgParamLen);
      break;

    case MESH_MSG_API_ADD_GATT_CONN:
      MeshGattAddProxyConn(((meshAddGattProxyConn_t *)pMsg)->connId,
                            ((meshAddGattProxyConn_t *)pMsg)->maxProxyPdu);
      break;

    case MESH_MSG_API_REM_GATT_CONN:
      MeshGattRemoveProxyConn(((meshRemoveGattProxyConn_t *)pMsg)->connId);
      break;

    case MESH_MSG_API_PROC_GATT_MSG:
      MeshGattProcessPdu(((meshProcessGattProxyPdu_t *)pMsg)->connId,
                         ((meshProcessGattProxyPdu_t *)pMsg)->pProxyPdu,
                         ((meshProcessGattProxyPdu_t *)pMsg)->proxyPduLen);
      break;

    case MESH_MSG_API_SGN_GATT_IF_RDY:
      MeshGattSignalIfReady(((meshSignalGattProxyIfRdy_t *)pMsg)->connId);
      break;

    case MESH_MSG_API_ADD_ADV_IF:
      MeshAdvAddInterface(((meshAddAdvIf_t *)pMsg)->advIfId);
      break;

    case MESH_MSG_API_REM_ADV_IF:
      MeshAdvRemoveInterface(((meshRemoveAdvIf_t *)pMsg)->advIfId);
      break;

    case MESH_MSG_API_PROC_ADV_MSG:
      MeshAdvProcessPdu(((meshProcessAdvPdu_t *)pMsg)->advIfId,
                        ((meshProcessAdvPdu_t *)pMsg)->pAdvPdu,
                        ((meshProcessAdvPdu_t *)pMsg)->advPduLen);
      break;

    case MESH_MSG_API_SGN_ADV_IF_RDY:
      MeshAdvSignalInterfaceReady(((meshSignalAdvIfRdy_t *)pMsg)->advIfId);
      break;

    case MESH_MSG_API_PROXY_CFG_REQ:
      meshProxyClCb.msgHandlerCback(pMsg);
      break;

    case MESH_MSG_API_ATT_SET:
      MeshLocalCfgSetAttentionTimer(((meshAttentionSet_t *)pMsg)->elemId,
                                    ((meshAttentionSet_t *)pMsg)->attTimeSec);
      break;

    default:
      MESH_TRACE_WARN0("MESH API: Invalid event message received!");
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Returns WSF message handler based on event.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    Handler to process the WSF message.
 */
/*************************************************************************************************/
static meshWsfMsgHandlerCback_t meshWsfMsgToCback(wsfMsgHdr_t *pMsg)
{
  /* Select handler based on event. */
  if(pMsg->event >= MESH_LOCAL_CFG_MSG_START)
  {
    return meshCb.localCfgMsgCback;
  }

  if(pMsg->event >= MESH_GATT_PROXY_MSG_START)
  {
    return meshCb.gattProxyMsgCback;
  }

  if(pMsg->event >= MESH_PRV_BR_MSG_START)
  {
    return meshCb.prvBrMsgCback;
  }

  if(pMsg->event >= MESH_PRV_BEACON_MSG_START)
  {
    return meshCb.prvBeaconMsgCback;
  }

  if(pMsg->event >= MESH_NWK_BEACON_MSG_START)
  {
    return meshCb.nwkBeaconMsgCback;
  }

  if(pMsg->event >= MESH_NWK_MGMT_MSG_START)
  {
    return meshCb.nwkMgmtMsgCback;
  }

  if(pMsg->event >= MESH_NWK_MSG_START)
  {
    return meshCb.nwkMsgCback;
  }

  if(pMsg->event >= MESH_SAR_TX_MSG_START)
  {
    return meshCb.sarTxMsgCback;
  }

  if(pMsg->event >= MESH_SAR_RX_MSG_START)
  {
    return meshCb.sarRxMsgCback;
  }

  if(pMsg->event >= MESH_HB_MSG_START)
  {
    return meshCb.hbMsgCback;
  }

  if(pMsg->event >= MESH_ACC_MSG_START)
  {
    return meshCb.accMsgCback;
  }

  if(pMsg->event >= MESH_CFG_MDL_CL_MSG_START)
  {
    return meshCb.cfgMdlClMsgCback;
  }

  if(pMsg->event >= MESH_FRIENDSHIP_MSG_START)
  {
    return meshCb.friendshipMsgCback;
  }

  return meshCb.apiMsgCback;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Network.
 *
 *  \return    None
 */
/*************************************************************************************************/
void MeshHandlerInit(wsfHandlerId_t handlerId)
{
  /* Store handler ID. */
  meshCb.handlerId = handlerId;

  /* Initialize control block. */
  meshCb.initialized = FALSE;
  meshCb.evtCback = meshEmptyCback;
  meshCb.apiMsgCback = meshEmptyHandler;
  meshCb.friendshipMsgCback= meshEmptyHandler;
  meshCb.accMsgCback= meshEmptyHandler;
  meshCb.hbMsgCback= meshEmptyHandler;
  meshCb.sarRxMsgCback= meshEmptyHandler;
  meshCb.sarTxMsgCback= meshEmptyHandler;
  meshCb.nwkMsgCback= meshEmptyHandler;
  meshCb.nwkMgmtMsgCback= meshEmptyHandler;
  meshCb.nwkBeaconMsgCback= meshEmptyHandler;
  meshCb.prvBeaconMsgCback= meshEmptyHandler;
  meshCb.prvBrMsgCback = meshEmptyHandler;
  meshCb.gattProxyMsgCback = meshEmptyHandler;
  meshCb.localCfgMsgCback = meshEmptyHandler;

  meshCb.cfgMdlClMsgCback = meshCfgMdlClEmptyHandler;
}

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Memory required value in bytes if success or ::MESH_MEM_REQ_INVALID_CFG in case fail.
 */
/*************************************************************************************************/
uint32_t MeshGetRequiredMemory(void)
{
  uint32_t reqMemLocalCfg;
  uint32_t reqMemNwk;
  uint32_t reqMemRpl;
  uint32_t reqMemSarRx;
  uint32_t reqMemSarTx;
  uint32_t reqMemSec;
  uint32_t reqMemAcc;
  uint32_t reqMem = MESH_MEM_REQ_INVALID_CFG;

  /* Get Local Config module required memory. */
  reqMemLocalCfg = MeshLocalCfgGetRequiredMemory();
  /* Get Network module required memory. */
  reqMemNwk = MeshNwkGetRequiredMemory();
  /* Get Replay Protection module required memory. */
  reqMemRpl = MeshRpGetRequiredMemory();
  /* Get SAR Rx module required memory. */
  reqMemSarRx = MeshSarRxGetRequiredMemory();
  /* Get SAR TX module required memory. */
  reqMemSarTx = MeshSarTxGetRequiredMemory();
  /* Get Security module required memory. */
  reqMemSec = MeshSecGetRequiredMemory();
  /* Get Acces layer required memory. */
  reqMemAcc = MeshAccGetRequiredMemory();

  if ((reqMemLocalCfg != MESH_MEM_REQ_INVALID_CFG) &&
      (reqMemNwk != MESH_MEM_REQ_INVALID_CFG) &&
      (reqMemRpl != MESH_MEM_REQ_INVALID_CFG) &&
      (reqMemSec != MESH_MEM_REQ_INVALID_CFG) &&
      (reqMemAcc != MESH_MEM_REQ_INVALID_CFG) &&
      (reqMemSarRx != MESH_MEM_REQ_INVALID_CFG) &&
      (reqMemSarTx != MESH_MEM_REQ_INVALID_CFG))
  {
    reqMem = (reqMemLocalCfg + reqMemNwk + reqMemRpl + reqMemSec + reqMemAcc + reqMemSarRx +
              reqMemSarTx);

    MESH_TRACE_INFO1("MESH API: Mesh Stack required memory = %u", reqMem);

    return reqMem;
  }

  MESH_TRACE_ERR0("MESH API: Get required memory failed! Check for invalid memory configuration.");

  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh Core Stack.
 *
 *  \param[in] pFreeMem     Pointer to free memory.
 *  \param[in] freeMemSize  Size of pFreeMem.
 *
 *  \return    Amount of free memory consumed.
 */
/*************************************************************************************************/
uint32_t MeshInit(uint8_t *pFreeMem, uint32_t freeMemSize)
{
  uint32_t reqMem;

  /* Initialize control block but leave the handler ID. */
  meshCb.initialized = FALSE;
  meshCb.proxyIsServer = FALSE;
  meshCb.evtCback = meshEmptyCback;

  if ((pFreeMem == NULL) || (pMeshConfig->pMemoryConfig == NULL) ||
      (pMeshConfig->pElementArray == NULL))
  {
    MESH_TRACE_ERR0("MESH API: Mesh Stack initialization failed! Invalid configuration provided.");

    return 0;
  }

  /* Check if sufficient memory was provided. This function return 0 in case of fail. */
  reqMem = MeshGetRequiredMemory();

  /* Check if the provided memory is enough and aligned. */
  if ((reqMem != 0) && (reqMem <= freeMemSize) && MESH_UTILS_IS_ALIGNED(pFreeMem))
  {
    /* Store memory buffer pointer and size. */
    meshCb.pMemBuff = pFreeMem;
    meshCb.memBuffSize = reqMem;

    /* Initialize Security Toolbox. */
    MeshSecToolInit();

    /* Initialize sequence manager. */
    MeshSeqInit();

    /* Initialize local config */
    MeshLocalCfgInit();

    /* Initialize security. */
    MeshSecInit();

    /* Initialize advertising bearer. */
    MeshAdvInit();

    /* Initialize bearer. */
    MeshBrInit();

    /* Initialize Network. */
    MeshNwkInit();

    /* Initialize Secure Network Beacons. */
    MeshNwkBeaconInit();

    /* Initialize Network Management. */
    MeshNwkMgmtInit();

    /* Initialize Replay Protection. */
    MeshRpInit();

    /* Initialize Mesh Lower Transport. */
    MeshLtrInit();

    /* Initialize Mesh Upper Transport. */
    MeshUtrInit();

    /* Initialize Mesh Access Layer. */
    MeshAccInit();
    MeshAccPeriodicPubInit();

    /* Register WSF message handler. */
    meshCb.apiMsgCback = meshApiMsgCback;

    /* Set initialized flag. */
    meshCb.initialized = TRUE;

    return reqMem;
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Mesh Stack initialization failed! Invalid configuration provided.");
  }

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Mesh Core Stack events callback.
 *
 *  \param[in] meshCback  Callback function for Mesh Stack events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRegister(meshCback_t meshCback)
{
  if (meshCback != NULL)
  {
    meshCb.evtCback = meshCback;
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Invalid mesh callback registered!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Resets the node to unprovisioned device state.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshFactoryReset(void)
{
  if (meshCb.initialized == TRUE)
  {
    MESH_TRACE_INFO0("MESH API: Mesh Factory Reset not implemented!");
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Factory Reset failed, Mesh Stack not initialized!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Checks if a device is provisioned.
 *
 *  \return TRUE if device is provisioned. FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshIsProvisioned(void)
{
  meshAddress_t addr;

  if (meshCb.initialized == TRUE)
  {
    MeshLocalCfgGetAddrFromElementId(0, &addr);

    if (MESH_IS_ADDR_UNICAST(addr))
    {
      if (MeshLocalCfgCountNetKeys() != 0)
      {
        return TRUE;
      }
    }

    return FALSE;
  }

  MESH_TRACE_ERR0("MESH API: Is Provisioned failed, Mesh Stack not initialized!");

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Starts a device as node. The device needs to be already provisioned.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshStartNode(void)
{
  meshElementId_t elemId;
  meshSeqNumber_t seqNumberThresh;

  if (meshCb.initialized == TRUE)
  {
    /* Restore SEQ number to stored threshold for each element. */
    for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
    {
      /* Read the SEQ number threshold value. */
      seqNumberThresh = MeshLocalCfgGetSeqNumberThresh(elemId);

      /* Set the current SEQ number to the threshold value. */
      (void) MeshLocalCfgSetSeqNumber(elemId, seqNumberThresh);
    }

    /* Restore key material for keys stored in NVM. */
    MeshSecRestoreAllKeyMaterial(meshSecAllKeyMaterialRestoreCback);

    MESH_TRACE_INFO0("MESH API: Node Starting!");
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Start Node failed, Mesh Stack not initialized!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh message to a destination address.
 *
 *  \param[in] pMsgInfo       Pointer to a Mesh message information structure.
 *  \param[in] pMsgParam      Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen    Length of the message parameter list.
 *  \param[in] rndDelayMsMin  Minimum value for random send delay in ms. 0 for instant send.
 *  \param[in] rndDelayMsMax  Maximum value for random send delay in ms. 0 for instant send.
 *
 *  \return    None.
 *
 */
/*************************************************************************************************/
void MeshSendMessage(const meshMsgInfo_t *pMsgInfo, const uint8_t *pMsgParam, uint16_t msgParamLen,
                     uint32_t rndDelayMsMin, uint32_t rndDelayMsMax)
{
  meshElement_t *pElement;
  meshSendMessage_t *pMsg;
  meshModelId_t mdlId;
  uint16_t boundNetKeyIndex;

  if (meshCb.initialized == TRUE)
  {
    if ((pMsgInfo == NULL) || ((pMsgParam == NULL) && (msgParamLen > 0)))
    {
      MESH_TRACE_ERR0("MESH API: Send message failed, invalid parameters!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    /* Check if the source address belongs to an element. */
    MeshLocalCfgGetElementFromId(pMsgInfo->elementId, (const meshElement_t **)&pElement);

    if (pElement == NULL)
    {
      MESH_TRACE_ERR0("MESH API: Send message failed, invalid Element ID!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    /* Destination address validation. */
    if (MESH_IS_ADDR_UNASSIGNED(pMsgInfo->dstAddr) ||
        (MESH_IS_ADDR_VIRTUAL(pMsgInfo->dstAddr) && (pMsgInfo->pDstLabelUuid == NULL)))
    {
      MESH_TRACE_ERR0("MESH API: Send message failed, invalid destination address");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    /* Opcode validation. */
    if (MESH_OPCODE_IS_VALID(pMsgInfo->opcode) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Send message failed, invalid Opcode!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    /* Construct the generic model ID. */
    mdlId.isSigModel = !MESH_OPCODE_IS_VENDOR(pMsgInfo->opcode);
    if (mdlId.isSigModel)
    {
      mdlId.modelId.sigModelId = pMsgInfo->modelId.sigModelId;
    }
    else
    {
      mdlId.modelId.vendorModelId = pMsgInfo->modelId.vendorModelId;
    }

    /* Lock scheduler. */
    WsfTaskLock();

    /* Validate AppKey Index. */
    if (!MeshLocalCfgValidateModelToAppKeyBind(pMsgInfo->elementId, &mdlId, pMsgInfo->appKeyIndex))
    {
      /* Unlock scheduler. */
      WsfTaskUnlock();
      MESH_TRACE_ERR0("MESH API: Send message failed, AppKey not bound to model instance !");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }
    /* Get Bound NetKey Index. */
    if (MeshLocalCfgGetBoundNetKeyIndex(pMsgInfo->appKeyIndex, &boundNetKeyIndex) != MESH_SUCCESS)
    {
      /* Unlock scheduler. */
      WsfTaskUnlock();
      MESH_TRACE_ERR0("MESH API: Send message failed, NetKey not bound to AppKey !");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    /* Unlock scheduler. */
    WsfTaskUnlock();

    if (msgParamLen > (MESH_ACC_MAX_PDU_SIZE - MESH_OPCODE_SIZE(pMsgInfo->opcode)))
    {
      MESH_TRACE_ERR0("MESH API: Send message failed, invalid Opcode size!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    if (!MESH_TTL_IS_VALID(pMsgInfo->ttl))
    {
      MESH_TRACE_ERR0("MESH API: Send message failed, invalid TTL!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    if ((rndDelayMsMin >= rndDelayMsMax) && (rndDelayMsMin > 0))
    {
      MESH_TRACE_ERR0("MESH API: Send message failed, invalid delay interval!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_SEND_MSG_EVENT);
      return;
    }

    if ((rndDelayMsMin == 0) && (rndDelayMsMax == 0))
    {
      /* Allocate the Stack Message and additional size for message parameters. */
      if ((pMsg = MeshAccAllocMsg(pMsgInfo, pMsgParam, msgParamLen, boundNetKeyIndex)) != NULL)
      {
        /* Set event type. */
        pMsg->hdr.event = MESH_MSG_API_SEND_MSG;

        /* Send Message. */
        WsfMsgSend(meshCb.handlerId, pMsg);

        MESH_TRACE_INFO0("MESH API: Message sent to be processed.");
        return;
      }
      else
      {
        MESH_TRACE_ERR0("MESH API: Send message failed, Mesh Stack out of memory!");
        return;
      }
    }
    else
    {
      /* Send Message to be queued by the Access Layer. */
      MeshAccSendMessage(pMsgInfo, pMsgParam, msgParamLen, boundNetKeyIndex,
                         rndDelayMsMin, rndDelayMsMax);
    }

  }

  MESH_TRACE_ERR0("MESH API: Send message failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Mesh Message based on internal Model Publication State configuration.
 *
 *  \param[in] pPubMsgInfo  Pointer to a Mesh publish message information structure.
 *  \param[in] pMsgParam    Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen  Length of the message parameter list.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPublishMessage(meshPubMsgInfo_t *pPubMsgInfo, const uint8_t *pMsgParam, uint16_t msgParamLen)
{
  meshPublishMessage_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if ((pPubMsgInfo == NULL) || ((pMsgParam == NULL) && (msgParamLen > 0)))
    {
      MESH_TRACE_ERR0("MESH API: Publish message failed, invalid params!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_PUBLISH_MSG_EVENT);
      return;
    }

    /* Check if element identifier exceeds limit. */
    if (pPubMsgInfo->elementId >= pMeshConfig->elementArrayLen)
    {
      MESH_TRACE_ERR0("MESH API: Publish message failed, invalid Element ID!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_PUBLISH_MSG_EVENT);
      return;
    }

    if (MESH_OPCODE_IS_VALID(pPubMsgInfo->opcode) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Publish message failed, invalid Opcode!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_PUBLISH_MSG_EVENT);
      return;
    }

    if (msgParamLen > (MESH_ACC_MAX_PDU_SIZE - MESH_OPCODE_SIZE(pPubMsgInfo->opcode)))
    {
      MESH_TRACE_ERR0("MESH API: Publish message failed, invalid Opcode size!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_PUBLISH_MSG_EVENT);
      return;
    }

    /* Allocate the Stack Message and additional size for message parameters. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshPublishMessage_t) + msgParamLen)) != NULL)
    {

      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_PUBLISH_MSG;

      /* Copy the publish message identification data. */
      memcpy(&(pMsg->pubMsgInfo), pPubMsgInfo, sizeof(meshPubMsgInfo_t));

      /* Copy the message parameters at the end of the event structure. */
      memcpy((uint8_t *)pMsg + sizeof(meshPublishMessage_t), pMsgParam, msgParamLen);

      /* Add message parameters location address. */
      pMsg->pMsgParam = (uint8_t *)((uint8_t *)pMsg + sizeof(meshPublishMessage_t));

      /* Add message parameters length. */
      pMsg->msgParamLen = msgParamLen;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);

      MESH_TRACE_INFO0("MESH API: Publish message sent to be processed.");
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: Publish message failed, Mesh Stack out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Publish message failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Registers GATT proxy callback for PDU's sent by the stack to the bearer.
 *
 *  \param[in] cback  Callback used by the Mesh Stack to send a GATT Proxy PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRegisterGattProxyPduSendCback(meshGattProxyPduSendCback_t cback)
{
  if (cback != NULL)
  {
    WSF_CS_INIT(cs);
    WSF_CS_ENTER(cs);
    MeshGattRegisterPduSendCback(cback);
    WSF_CS_EXIT(cs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a new GATT Proxy connection into the bearer.
 *
 *  \param[in] connId       Unique identifier for the connection. Valid range 0x00 to 0x0F.
 *  \param[in] maxProxyPdu  Maximum size of the Proxy PDU, derived from ATT MTU.
 *
 *  \return    None.
 *
 *  \remarks   If GATT Proxy is supported and this the first connection, it also enables proxy.
 */
/*************************************************************************************************/
void MeshAddGattProxyConn(meshGattProxyConnId_t connId, uint16_t maxProxyPdu)
{
  meshAddGattProxyConn_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if (MESH_GATT_PROXY_CONN_ID_IS_VALID(connId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Proxy conn add failed, invalid conn ID!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_GATT_CONN_ADD_EVENT);
      return;
    }

    if (maxProxyPdu < MESH_GATT_PROXY_PDU_MIN_VALUE)
    {
      MESH_TRACE_ERR0("MESH API: Proxy conn add failed, invalid max proxy PDU !");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_GATT_CONN_ADD_EVENT);
      return;
    }

    /* Allocate the Stack Message. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshAddGattProxyConn_t))) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_ADD_GATT_CONN;

      /* Add Connection ID. */
      pMsg->connId = connId;

      /* Add Maximum Proxy PDU size. */
      pMsg->maxProxyPdu = maxProxyPdu;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: Proxy conn add failed. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Proxy conn add failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Removes a GATT Proxy connection from the bearer.
 *
 *  \param[in] connId  Unique identifier for a connection. Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 *
 *  \remarks   A connection removed event is received after calling this.
 */
/*************************************************************************************************/
void MeshRemoveGattProxyConn(meshGattProxyConnId_t connId)
{
  meshRemoveGattProxyConn_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if (MESH_GATT_PROXY_CONN_ID_IS_VALID(connId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Proxy conn remove failed, invalid conn ID!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_GATT_CONN_REMOVE_EVENT);
      return;
    }

    /* Allocate the Stack Message. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshRemoveGattProxyConn_t))) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_REM_GATT_CONN;

      /* Add Connection ID. */
      pMsg->connId = connId;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: Proxy conn remove failed. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Proxy conn remove failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief  Checks if GATT Proxy Feature is enabled.
 *
 *  \return TRUE if GATT Proxy Feature is enabled. FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshIsGattProxyEnabled(void)
{
  return (MeshLocalCfgGetGattProxyState() == MESH_GATT_PROXY_FEATURE_ENABLED);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a GATT Proxy PDU to the Mesh Stack for processing.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProcessGattProxyPdu(meshGattProxyConnId_t connId, const uint8_t *pProxyPdu,
                             uint16_t proxyPduLen)
{
  meshProcessGattProxyPdu_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if (pProxyPdu == NULL)
    {
      MESH_TRACE_ERR0("MESH API: Process proxy PDU failed, invalid PDU!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_GATT_PROCESS_PROXY_PDU_EVENT);
      return;
    }

    if (MESH_GATT_PROXY_CONN_ID_IS_VALID(connId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Process proxy PDU failed, invalid params!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_GATT_PROCESS_PROXY_PDU_EVENT);
      return;
    }

    if ((pMsg = WsfMsgAlloc(sizeof(meshProcessGattProxyPdu_t) + proxyPduLen)) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_PROC_GATT_MSG;

      /* Add connection ID. */
      pMsg->connId = connId;

      /* Copy the PDU at the end of the stack message. */
      memcpy((uint8_t *)pMsg + sizeof(meshProcessGattProxyPdu_t), pProxyPdu, proxyPduLen);

      /* Add Proxy PDU location address. */
      pMsg->pProxyPdu = (uint8_t *)((uint8_t *)pMsg + sizeof(meshProcessGattProxyPdu_t));

      /* Add Proxy PDU length. */
      pMsg->proxyPduLen = proxyPduLen;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: Process proxy PDU failed. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Process proxy PDU failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Signals the Mesh Stack that the GATT Proxy interface is ready to transmit packets.
 *
 *  \param[in] connId  Unique identifier for the connection on which the PDU was received.
 *                     Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSignalGattProxyIfRdy(meshGattProxyConnId_t connId)
{
  meshSignalGattProxyIfRdy_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if (MESH_GATT_PROXY_CONN_ID_IS_VALID(connId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Signal fail, invalid GATT interface!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_GATT_SIGNAL_IF_RDY_EVENT);
      return;
    }

    /* Allocate the Stack Message. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshSignalGattProxyIfRdy_t))) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_SGN_GATT_IF_RDY;

      /* Add Connection ID. */
      pMsg->connId = connId;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: GATT interface signal fail. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: GATT interface signal fail, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Registers advertising interface callback for PDU's sent by the stack to the bearer.
 *
 *  \param[in] cback  Callback used by the Mesh Stack to send an advertising PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRegisterAdvIfPduSendCback(meshAdvPduSendCback_t cback)
{
  if (cback != NULL)
  {
    WSF_CS_INIT(cs);
    WSF_CS_ENTER(cs);
    MeshAdvRegisterPduSendCback(cback);
    WSF_CS_EXIT(cs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a new advertising interface into the bearer.
 *
 *  \param[in] advIfId  Unique identifier for the interface. Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAddAdvIf(meshAdvIfId_t advIfId)
{
  meshAddAdvIf_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if ((MESH_ADV_IF_ID_IS_VALID(advIfId) == FALSE))
    {
      MESH_TRACE_ERR0("MESH API: Add ADV interface failed, invalid params!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_ADV_IF_ADD_EVENT);
      return;
    }

    /* Allocate the Stack Message. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshAddAdvIf_t))) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_ADD_ADV_IF;

      /* Add ADV Interface ID. */
      pMsg->advIfId = advIfId;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: Add ADV interface failed. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Add ADV interface failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Removes an advertising interface from the bearer.
 *
 *  \param[in] advIfId  Unique identifier for the interface. Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRemoveAdvIf(meshAdvIfId_t advIfId)
{
  meshRemoveAdvIf_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if (MESH_ADV_IF_ID_IS_VALID(advIfId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Remove ADV interface failed, invalid params!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_ADV_IF_REMOVE_EVENT);
      return;
    }

    /* Allocate the Stack Message. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshRemoveAdvIf_t))) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_REM_ADV_IF;

      /* Add ADV Interface ID. */
      pMsg->advIfId = advIfId;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: Remove ADV interface failed. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Remove ADV interface failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Sends an Advertising PDU to the Mesh Stack for processing.
 *
 *  \param[in] advIfId    Unique identifier for the interface on which the PDU was received.
 *                        Valid range is 0x00 to 0x1F.
 *  \param[in] pAdvPdu    Pointer to a buffer containing the Advertising PDU.
 *  \param[in] advPduLen  Size of the advertising PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProcessAdvPdu(meshAdvIfId_t advIfId, const uint8_t *pAdvPdu, uint8_t advPduLen)
{
  meshProcessAdvPdu_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if ((pAdvPdu == NULL) || (MESH_ADV_IF_ID_IS_VALID(advIfId) == FALSE))
    {
      MESH_TRACE_ERR0("MESH API: Process ADV PDU failed, invalid params!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_ADV_PROCESS_PDU_EVENT);
      return;
    }

    if ((advPduLen < MESH_ADV_IF_PDU_MIN_VALUE) || (advPduLen > MESH_ADV_IF_PDU_MAX_VALUE))
    {
      MESH_TRACE_ERR0("MESH API: Process ADV PDU failed, invalid PDU length!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_ADV_PROCESS_PDU_EVENT);
      return;
    }

    /* Allocate the Stack Message and aditional size for message parameters. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshProcessAdvPdu_t) + advPduLen)) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_PROC_ADV_MSG;

      /* Add ADV Interface ID. */
      pMsg->advIfId = advIfId;

      /* Copy the PDU at the end of the stack message. */
      memcpy((uint8_t *)pMsg + sizeof(meshProcessAdvPdu_t), pAdvPdu, advPduLen);

      /* Add ADV PDU location address. */
      pMsg->pAdvPdu = (uint8_t *)((uint8_t *)pMsg + sizeof(meshProcessAdvPdu_t));

      /* Add ADV PDU length. */
      pMsg->advPduLen = advPduLen;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);

      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: Process ADV PDU failed. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Process ADV PDU failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Signals the Mesh Stack that the advertising interface is ready to transmit packets.
 *
 *  \param[in] advIfId  Unique identifier for the connection.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSignalAdvIfRdy(meshAdvIfId_t advIfId)
{
  meshSignalAdvIfRdy_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if (MESH_ADV_IF_ID_IS_VALID(advIfId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Signal fail, invalid ADV interface!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_ADV_SIGNAL_IF_RDY_EVENT);
      return;
    }

    /* Allocate the Stack Message. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshSignalAdvIfRdy_t))) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_SGN_ADV_IF_RDY;

      /* Add ADV Interface ID. */
      pMsg->advIfId = advIfId;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      MESH_TRACE_ERR0("MESH API: ADV interface signal fail. Out of memory!");
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: ADV interface signal fail, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     WSF event handler for Mesh API.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  meshWsfMsgHandlerCback_t msgHandlerCback;

  /* Handle message. */
  if (pMsg != NULL)
  {
    /* Select handler. */
    msgHandlerCback = meshWsfMsgToCback(pMsg);

    /* Invoke handler. */
    msgHandlerCback(pMsg);
  }
  /* Handle events. */
  else if (event)
  {
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the provisioning and configuration data, either as a result of a completed
 *             Provisioning Procedure, or after reading the data from NVM if already provisioned.
 *
 *  \param[in] pPrvData  Pointer to a structure containing provisioning and configuration data.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLoadPrvData(const meshPrvData_t* pPrvData)
{
  wsfMsgHdr_t *pMsg;
  meshLocalCfgRetVal_t cfgRetVal;

  if (meshCb.initialized == TRUE)
  {
      if (pPrvData->pDevKey == NULL)
      {
        MESH_TRACE_ERR0("MESH API: Provisioning load fail. DevKey pointer NULL!");

        return;
      }

      if (pPrvData->pNetKey == NULL)
      {
        MESH_TRACE_ERR0("MESH API: Provisioning load fail. NetKey pointer NULL!");

        return;
      }

      /* Assign a unicast address to the primary element. */
      cfgRetVal = MeshLocalCfgSetPrimaryNodeAddress(pPrvData->primaryElementAddr);

      if (cfgRetVal != MESH_SUCCESS)
      {
        MESH_TRACE_WARN0("MESH API: Provisioning load fail. Unable to set Primary Element Address!");

        return;
      }

      /* Set DevKey. */
      MeshLocalCfgSetDevKey(pPrvData->pDevKey);

      /* Set the Network Key and the Network Key Index. */
      cfgRetVal = MeshLocalCfgSetNetKey(pPrvData->netKeyIndex, pPrvData->pNetKey);

      if (cfgRetVal != MESH_SUCCESS)
      {
        MESH_TRACE_ERR0("MESH API: Provisioning load fail. Unable to set Network Key!");

        return;
     }

      /* Set the IV index. */
      MeshLocalCfgSetIvIndex(pPrvData->ivIndex);

      /* Set the IV Update procedure state. */
      MeshLocalCfgSetIvUpdateInProgress((bool_t)(pPrvData->flags & 0x02));

      /* Set the Key Refresh Phase. */
      MeshLocalCfgSetKeyRefreshState(pPrvData->netKeyIndex, (bool_t)(pPrvData->flags & 0x01) ?
                                     MESH_KEY_REFRESH_SECOND_PHASE : MESH_KEY_REFRESH_NOT_ACTIVE);

      if (pPrvData->flags & 0x01)
      {
        MeshLocalCfgUpdateNetKey(pPrvData->netKeyIndex, pPrvData->pNetKey);
      }

      MESH_TRACE_INFO0("MESH API: Provisioning load success.");

      /* Send message to Network Management. */
      if((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t)))!= NULL)
      {
        pMsg->event = MESH_NWK_MGMT_MSG_PRV_COMPLETE;
        WsfMsgSend(meshCb.handlerId, pMsg);
      }

      return;
  }

  MESH_TRACE_ERR0("MESH API: Provisioning load fail. Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Sets attention timer for an element.
 *
 *  \param[in] elemId      Element identifier.
 *  \param[in] attTimeSec  Attention timer time in seconds. Set to 0 to disable.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAttentionSet(meshElementId_t elemId, uint8_t attTimeSec)
{
  meshAttentionSet_t *pMsg;

  if (meshCb.initialized == TRUE)
  {
    if (elemId >= pMeshConfig->elementArrayLen)
    {
      MESH_TRACE_ERR0("MESH API: Attention Set failed, invalid params!");
      meshExecCback(MESH_CORE_EVENT, MESH_INVALID_PARAM, MESH_CORE_ATTENTION_SET_EVENT);
      return;
    }

    /* Allocate the Stack Message. */
    if ((pMsg = WsfMsgAlloc(sizeof(meshAttentionSet_t))) != NULL)
    {
      /* Set event type. */
      pMsg->hdr.event = MESH_MSG_API_ATT_SET;

      /* Set event message parameters. */
      pMsg->elemId     = elemId;
      pMsg->attTimeSec = attTimeSec;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
      return;
    }
    else
    {
      return;
    }
  }

  MESH_TRACE_ERR0("MESH API: Attention set failed, Mesh Stack not initialized!");
}

/*************************************************************************************************/
/*!
 *  \brief     Gets attention timer remaining time in seconds for an element.
 *
 *  \param[in] elemId  Element identifier.
 *
 *  \return    Current Attention Timer time in seconds.
 */
/*************************************************************************************************/
uint8_t MeshAttentionGet(meshElementId_t elemId)
{
  uint8_t result = 0;

  /* Initialize critical section. */
  WSF_CS_INIT(cs);
  /* Enter critical section. */
  WSF_CS_ENTER(cs);

  /* Retrieve value. */
  result = MeshLocalCfgGetAttentionTimer(elemId);

  /* Exit critical section. */
  WSF_CS_EXIT(cs);

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh callback event.
 *
 *  \param[in] pDmEvt  Mesh callback event.
 *
 *  \return    Size of Mesh callback event.
 */
/*************************************************************************************************/
uint16_t MeshSizeOfEvt(meshEvt_t *pMeshEvt)
{
  uint16_t len;

  /* If a valid DM event ID */
  if ((pMeshEvt->hdr.event == MESH_CORE_EVENT) && (pMeshEvt->hdr.param <= MESH_CORE_MAX_EVENT))
  {
    len = meshEvtCbackLen[pMeshEvt->hdr.param];
  }
  else
  {
    len = sizeof(wsfMsgHdr_t);
  }

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the GATT proxy functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshGattProxyInit(void)
{
  /* Initialize GATT */
  MeshBrEnableGatt();
}
