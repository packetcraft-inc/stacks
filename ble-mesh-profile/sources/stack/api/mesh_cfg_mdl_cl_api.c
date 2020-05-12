/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_cl_api.c
 *
 *  \brief  Configuration Client API implementation.
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
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_handler.h"

#include "mesh_main.h"
#include "mesh_security.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_access.h"

#include "mesh_utils.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_cl_api.h"

#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_cl.h"
#include "mesh_cfg_mdl_cl_main.h"

#include "mesh_cfg_mdl_messages.h"

#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Verifies if conditions are met for a local request */
#define MESH_CFG_MDL_CL_IS_REQ_LOCAL(addr,pDevKey) (((addr) == MESH_CFG_MDL_CL_LOCAL_NODE_SR)\
                                                    && ((pDevKey) == NULL))

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Definition of Core model to be registered in the Access Layer */
static meshAccCoreMdl_t cfgMdlClAccMdl =
{
  .msgRecvCback   = meshCfgMdlClAccMsgRcvCback,        /*! Message received callback */
  .pOpcodeArray   = meshCfgMdlSrOpcodes,               /*! Opcodes registered for Rx */
  .opcodeArrayLen = MESH_CFG_MDL_SR_MAX_OP,            /*! Number of opcodes */
  .elemId         = 0,                                 /*! Only primary element allowed for
                                                        *  Configuration Client
                                                        */
  .mdlId.isSigModel         = TRUE,                    /*! SIG model */
  .mdlId.modelId.sigModelId = MESH_CFG_MDL_CL_MODEL_ID /*! Configuration Client Model ID. */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Configuration Client control block */
meshCfgMdlClCb_t meshCfgMdlClCb =
{
  .cback                = meshCfgMdlClEmptyCback,
  .opTimeoutSec         = MESH_CFG_MDL_CL_OP_TIMEOUT_DEFAULT_SEC,
  .cfgMdlSrDbNumEntries = 0,
  .rspTmrUidGen         = 0
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Checks if server parameters provided in the request are valid.
 *
 *  \param[in]  addr         Address of the remote node.
 *  \param[in]  pDevKey      Pointer to the remote node's Device Key.
 *  \param[in]  netKeyIndex  Sub-net identifier on which the request is sent.
 *  \param[in]  pEvt         Pointer to event that is used if parameters are invalid.
 *
 *  \return     TRUE if parameters are valid, FALSE otherwise.
 *
 *  \remarks    If the server parameters are invalid this function will send an event to the
 *              upper layer.
 *
 */
/*************************************************************************************************/
static bool_t meshCfgMdlClCheckSrParamsAndNotify(meshAddress_t addr, const uint8_t *pDevKey,
                                                 uint16_t netKeyIndex, meshCfgMdlClEvt_t *pEvt)
{
  /*! Validates that Configuration Server address, device key and Network Key Index are in valid
   *  ranges for each API
   */

  if (!MESH_CFG_MDL_CL_IS_REQ_LOCAL((addr), (pDevKey)))
  {
    if (!MESH_IS_ADDR_UNICAST((addr)) || ((pDevKey) == NULL) ||
        ((netKeyIndex) > MESH_NET_KEY_INDEX_MAX_VAL))
    {
      /* Invoke user callback. */
      meshCfgMdlClCb.cback(pEvt);

      return FALSE;
    }
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      Callback implementation for registering in Security module to read remote Device
 *              Keys.
 *
 *  \param[in]  addr        Address of the remote node.
 *  \param[out] pOutDevKey  Pointer to buffer where the Device Key is copied.
 *
 *  \return     TRUE if Device Key exists and gets copied, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshCfgMdlClSecDeviceKeyReader(meshAddress_t addr, uint8_t *pOutDevKey)
{
  uint16_t dbIdx;

  /* Validate parameters. */
  if (!MESH_IS_ADDR_UNICAST(addr) || pOutDevKey == NULL)
  {
    return FALSE;
  }

  /* Initialize critical section. */
  WSF_CS_INIT(cs);

  /* Enter critical section. */
  WSF_CS_ENTER(cs);

  for (dbIdx = 0; dbIdx < meshCfgMdlClCb.cfgMdlSrDbNumEntries; dbIdx++)
  {
    /* Check if address matches an used entry. */
    if((addr == meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].cfgMdlSrAddr) &&
       (meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].refCount != 0))
    {
      /* Copy Device Key. */
      memcpy(pOutDevKey, meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].cfgMdlSrDevKey, MESH_KEY_SIZE_128);

      /* Exit critical section. */
      WSF_CS_EXIT(cs);

      return TRUE;
    }
  }

  /* Exit critical section. */
  WSF_CS_EXIT(cs);

  /* No matching address found. */
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Adds server to database under critical section.
 *
 *  \param[in]  cfgMdlSrAddr  Address of the remote server.
 *  \param[out] pDevKey    Pointer to Device Key.
 *
 *  \return     TRUE if successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClAddToSrDbSafe(meshAddress_t cfgMdlSrAddr, const uint8_t *pDevKey)
{
  uint16_t dbIdx;
  uint16_t emptyIdx = meshCfgMdlClCb.cfgMdlSrDbNumEntries;

  /* Initialize critical section. */
  WSF_CS_INIT(cs);

  /* Enter critical section. */
  WSF_CS_ENTER(cs);

  /* Iterate through database. */
  for (dbIdx = 0; dbIdx < meshCfgMdlClCb.cfgMdlSrDbNumEntries; dbIdx++)
  {
    /* Check if remote server exists. */
    if (meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].cfgMdlSrAddr == cfgMdlSrAddr)
    {
      /* Device Key must be the same. */
      WSF_ASSERT(memcmp(pDevKey, meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].cfgMdlSrDevKey,
                                 MESH_KEY_SIZE_128) == 0x00);

      /* Increment reference count because there is another request pending for this server. */
      meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].refCount++;

      /* Exit critical section. */
      WSF_CS_EXIT(cs);

      return TRUE;
    }

    /* Check if there is an empty entry. */
    if ((meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].cfgMdlSrAddr == MESH_ADDR_TYPE_UNASSIGNED) &&
        (emptyIdx == meshCfgMdlClCb.cfgMdlSrDbNumEntries))
    {
      emptyIdx = dbIdx;
    }
  }

  /* If server was not in the database and there is an empty entry, store address and key. */
  if (emptyIdx != meshCfgMdlClCb.cfgMdlSrDbNumEntries)
  {
    meshCfgMdlClCb.pCfgMdlSrDb[emptyIdx].cfgMdlSrAddr = cfgMdlSrAddr;
    memcpy(meshCfgMdlClCb.pCfgMdlSrDb[emptyIdx].cfgMdlSrDevKey, pDevKey, MESH_KEY_SIZE_128);

    /* Set reference count to 1 since there is one request pending. */
    meshCfgMdlClCb.pCfgMdlSrDb[emptyIdx].refCount = 1;

    /* Exit critical section. */
    WSF_CS_EXIT(cs);

    return TRUE;
  }

  /* Exit critical section. */
  WSF_CS_EXIT(cs);

  /* Server cannot be added. */
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Decrements reference count and removes server from database under critical section.
 *
 *  \param[in] cfgMdlSrAddr  Address of the remote server.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlClRemFromSrDbSafe(meshAddress_t cfgMdlSrAddr)
{
  uint16_t dbIdx;

  /* Initialize critical section. */
  WSF_CS_INIT(cs);

  /* Enter critical section. */
  WSF_CS_ENTER(cs);

  for (dbIdx = 0; dbIdx < meshCfgMdlClCb.cfgMdlSrDbNumEntries; dbIdx++)
  {
    if (meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].cfgMdlSrAddr == cfgMdlSrAddr)
    {
      WSF_ASSERT(meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].refCount > 0);

      /* Decrement reference count. */
      meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].refCount--;

      if (meshCfgMdlClCb.pCfgMdlSrDb[dbIdx].refCount == 0)
      {
        /* Reset internal memory on this index in the db. */
        memset(&(meshCfgMdlClCb.pCfgMdlSrDb[dbIdx]), 0, sizeof(meshCfgMdlClRemCfgMdlSrDbEntry_t));
      }

      /* Exit critical section. */
      WSF_CS_EXIT(cs);

      return;
    }
  }

  /* Exit critical section. */
  WSF_CS_EXIT(cs);

  return;
}

/*************************************************************************************************/
/*!
 *  \brief      Allocates a WSF msg containing API request and sets some of the parameters.
 *
 *  \param[in]  cfgMdlSrAddr         Address of the remote server.
 *  \param[in]  pCfgMdlSrDevKey      Pointer to server Device Key.
 *  \param[in]  cfgMdlSrNetKeyIndex  Global identifier for the Network Key of the sub-net on which
 *                                   the request is sent.
 *  \param[in]  msgParamLen          Length of the message parameters.
 *
 *  \return     Pointer to allocated request with allocated request parameters to be stored until
 *              response is sent.
 *
 *  \remarks    For non-local requests, this function also tries to add the address-key pair into
 *              the remote server database.
 */
/*************************************************************************************************/
static inline meshCfgMdlClOpReq_t *meshCfgMdlClAllocateRequest(meshAddress_t cfgMdlSrAddr,
                                                               const uint8_t *pCfgMdlSrDevKey,
                                                               uint16_t cfgMdlSrNetKeyIndex,
                                                               uint16_t msgParamLen)
{
  meshCfgMdlClOpReq_t *pReq;
  meshAddress_t elem0Addr;

  /* Check if request is local without explicit API request and convert. */
  if (MESH_IS_ADDR_UNICAST(cfgMdlSrAddr))
  {
    MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

    if (cfgMdlSrAddr == elem0Addr)
    {
      cfgMdlSrAddr = MESH_ADDR_TYPE_UNASSIGNED;
      pCfgMdlSrDevKey = NULL;
    }
  }

  /* Allocate API message. */
  if ((pReq = (meshCfgMdlClOpReq_t *)WsfMsgAlloc(sizeof(meshCfgMdlClOpReq_t) + msgParamLen)) != NULL)
  {
    /* Allocate operation request parameters. */
    if ((pReq->pReqParam = (meshCfgMdlClOpReqParams_t *)WsfBufAlloc(sizeof(meshCfgMdlClOpReqParams_t)))
        != NULL)
    {
      pReq->hdr.event = MESH_CFG_MDL_CL_MSG_API_SEND;

      /* Set pointer to message parameters and length. */
      if (msgParamLen == 0)
      {
        pReq->pMsgParam = NULL;
      }
      else
      {
        pReq->pMsgParam = (uint8_t *)pReq + sizeof(meshCfgMdlClOpReq_t);
      }
      pReq->msgParamLen = msgParamLen;

      /* Try to add remote server to database. */
      if (!MESH_CFG_MDL_CL_IS_REQ_LOCAL(cfgMdlSrAddr, pCfgMdlSrDevKey) &&
          !meshCfgMdlClAddToSrDbSafe(cfgMdlSrAddr, pCfgMdlSrDevKey))
      {
        WsfBufFree(pReq->pReqParam);
        WsfMsgFree(pReq);

        return NULL;
      }

      pReq->pReqParam->cfgMdlSrAddr = cfgMdlSrAddr;
      pReq->pReqParam->cfgMdlSrNetKeyIndex = cfgMdlSrNetKeyIndex;

      return pReq;
    }
    else
    {
      /* Free API message. */
      WsfMsgFree(pReq);
    }
  }

  return NULL;
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
uint32_t MeshCfgMdlClGetRequiredMemory(void)
{
  /* Calculate required memory and return it. */
  return MESH_UTILS_ALIGN(pMeshConfig->pMemoryConfig->cfgMdlClMaxSrSupported *
                          sizeof(meshCfgMdlClRemCfgMdlSrDbEntry_t));
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Configuration Client.
 *
 *  \param[in] pFreeMem     Pointer to free memory.
 *  \param[in] freeMemSize  Size of pFreeMem.
 *
 *  \return    Amount of free memory consumed.
 */
/*************************************************************************************************/
uint32_t MeshCfgMdlClInit(uint8_t *pFreeMem, uint32_t freeMemSize)
{
  uint32_t reqMem = MeshCfgMdlClGetRequiredMemory();

  /* Insufficient memory. */
  if (reqMem > freeMemSize)
  {
    WSF_ASSERT(FALSE);
    return 0;
  }

  /* Reserve configuration memory for remote Configuration Servers. */
  meshCfgMdlClCb.pCfgMdlSrDb = (meshCfgMdlClRemCfgMdlSrDbEntry_t *)pFreeMem;

  /* Store number of entries in the remote device database. */
  meshCfgMdlClCb.cfgMdlSrDbNumEntries = pMeshConfig->pMemoryConfig->cfgMdlClMaxSrSupported;

  /* Reset entries. */
  memset(meshCfgMdlClCb.pCfgMdlSrDb, 0,
         meshCfgMdlClCb.cfgMdlSrDbNumEntries * sizeof(meshCfgMdlClRemCfgMdlSrDbEntry_t));

  /* Initialize operation queue. */
  while (!WsfQueueEmpty(&(meshCfgMdlClCb.opQueue)))
  {
    WsfBufFree(WsfQueueDeq(&(meshCfgMdlClCb.opQueue)));
  }
  WSF_QUEUE_INIT(&(meshCfgMdlClCb.opQueue));

  /* Register the Configuration Client in the Access Layer. */
  MeshAccRegisterCoreModel(&cfgMdlClAccMdl);

  /* Register the WSF API message handler. */
  meshCb.cfgMdlClMsgCback = meshCfgMdlClWsfMsgHandlerCback;

  /* Register to default user callback. */
  meshCfgMdlClCb.cback = meshCfgMdlClEmptyCback;

  /* Set default timeout for operations. */
  meshCfgMdlClCb.opTimeoutSec = MESH_CFG_MDL_CL_OP_TIMEOUT_DEFAULT_SEC;

  /* Register Device Key Reader in the Security Module. */
  MeshSecRegisterRemoteDevKeyReader(meshCfgMdlClSecDeviceKeyReader);

  MESH_TRACE_INFO0("MESH CFG CL: init");
  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief     Installs the Configuration Client callback.
 *
 *  \param[in] meshCfgMdlClCback  Upper layer callback to notify operation results.
 *  \param[in] timeoutSeconds     Timeout for configuration operations, in seconds.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClRegister(meshCfgMdlClCback_t meshCfgMdlClCback, uint16_t timeoutSeconds)
{
  WSF_CS_INIT(cs);

  /* Check callback. */
  if (meshCfgMdlClCback != NULL)
  {
    WSF_CS_ENTER(cs);
    /* Set callback into control block. */
    meshCfgMdlClCb.cback = meshCfgMdlClCback;
    WSF_CS_EXIT(cs);
  }

  /* Check timeout. */
  if (timeoutSeconds != 0)
  {
    WSF_CS_ENTER(cs);
    meshCfgMdlClCb.opTimeoutSec = timeoutSeconds;
    WSF_CS_EXIT(cs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets a Secure Network Beacon state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_BEACON_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClBeaconGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                           const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  wsfMsgHdr_t evt =
  {
    .event = MESH_CFG_MDL_SR_EVENT,
    .param = MESH_CFG_MDL_BEACON_GET_EVENT,
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_BEACON_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_BEACON_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_BEACON_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_BEACON_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets a Secure Network Beacon state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] beaconState          New secure network beacon state.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_BEACON_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClBeaconSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                           const uint8_t *pCfgMdlSrDevKey, meshBeaconStates_t beaconState)
{
  meshCfgMdlClOpReq_t *pReq;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_BEACON_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate Beacon state. */
  if (!MESH_BEACON_STATE_IS_VALID(beaconState))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_BEACON_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Set Beacon state. */
    pReq->pMsgParam[0] = beaconState;

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_BEACON_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_BEACON_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_BEACON_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets a Composition Data Page. (Only Page 0 is supported at this time).
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] pageNumber           Requested page number.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_COMP_PAGE_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClCompDataGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                             const uint8_t *pCfgMdlSrDevKey, uint8_t pageNumber)
{
  meshCfgMdlClOpReq_t *pReq;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_COMP_PAGE_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_COMP_DATA_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Set Composition Data page field. */
    pReq->pMsgParam[0] = pageNumber;

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_COMP_DATA_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_COMP_DATA_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_COMP_PAGE_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Default TTL state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_DEFAULT_TTL_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClDefaultTtlGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                               const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_DEFAULT_TTL_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_DEFAULT_TTL_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_DEFAULT_TTL_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_DEFAULT_TTL_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_DEFAULT_TTL_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Default TTL state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] ttl                  New value for the Default TTL.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClDefaultTtlSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                               const uint8_t *pCfgMdlSrDevKey, uint8_t ttl)
{
  meshCfgMdlClOpReq_t *pReq;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };


  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate TTL state values. */
  if (!MESH_TTL_IS_VALID(ttl) || (ttl == MESH_TX_TTL_FILTER_VALUE) ||
      (ttl == MESH_USE_DEFAULT_TTL))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_DEFAULT_TTL_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Set Default TTL field. */
    pReq->pMsgParam[0] = ttl;

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_DEFAULT_TTL_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_DEFAULT_TTL_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Gatt Proxy state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_GATT_PROXY_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClGattProxyGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                              const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_GATT_PROXY_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_GATT_PROXY_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_GATT_PROXY_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_GATT_PROXY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_GATT_PROXY_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Gatt Proxy state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] gattProxyState       GATT Proxy State.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_GATT_PROXY_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClGattProxySet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                              const uint8_t *pCfgMdlSrDevKey, meshGattProxyStates_t gattProxyState)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_GATT_PROXY_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate GATT Proxy State. */
  if(gattProxyState >= MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED)
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_GATT_PROXY_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack Gatt Proxy state. */
    pReq->pMsgParam[0] = gattProxyState;

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_GATT_PROXY_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_GATT_PROXY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_GATT_PROXY_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets a Relay state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_RELAY_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClRelayGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                          const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_RELAY_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_RELAY_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_RELAY_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_RELAY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_RELAY_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets Relay and Relay Retransmit states.
 *
 *  \param[in] cfgMdlSrAddr           Primary element containing an instance of Configuration Server
 *                                    model.
 *  \param[in] cfgMdlSrNetKeyIndex    Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey        Pointer to the Device Key of the remote Configuration Server
 *                                    or NULL for local (cfgMdlSrAddr is
 *                                    MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] relayState             New value for the Relay state.
 *  \param[in] pRelayRetransState     Pointer to new value for the Relay Retransmit state.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_RELAY_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClRelaySet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                          const uint8_t *pCfgMdlSrDevKey, meshRelayStates_t relayState,
                          meshRelayRetransState_t *pRelayRetransState)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_RELAY_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  if ((pRelayRetransState == NULL) ||
      (relayState >= MESH_RELAY_FEATURE_NOT_SUPPORTED) ||
      (pRelayRetransState->retransCount >
      (CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_MASK >> CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_CNT_SHIFT)) ||
      (pRelayRetransState->retransIntervalSteps10Ms >
      (CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_MASK >> CFG_MDL_MSG_RELAY_COMP_STATE_RETRANS_INTVL_SHIFT)))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_RELAY_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack Relay composite state. */
    meshCfgMsgPackRelay(pReq->pMsgParam, &relayState, pRelayRetransState);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_RELAY_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_RELAY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_RELAY_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the publish address and parameters of an outgoing message that originates from a
 *             model instance.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] elemAddr             Address of the element containing the model.
 *  \param[in] sigModelId           SIG model identifier.
 *  \param[in] vendorModelId        Vendor model identifier.
 *  \param[in] isSig                TRUE if SIG model identifier should be used, FALSE for vendor.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_PUB_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClPubGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                        const uint8_t *pCfgMdlSrDevKey, meshAddress_t elemAddr,
                        meshSigModelId_t sigModelId, meshVendorModelId_t vendorModelId,
                        bool_t isSig)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_PUB_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate request parameters. */
  if (!MESH_IS_ADDR_UNICAST(elemAddr))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_MODEL_PUB_GET_NUM_BYTES(isSig));

  if (pReq != NULL)
  {
    /* Pack parameters. */
    meshCfgMsgPackModelPubGet(pReq->pMsgParam, elemAddr, sigModelId, vendorModelId, isSig);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_MODEL_PUB_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_MODEL_PUB_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_PUB_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Model Publication state of an outgoing message that originates from a model
 *             instance when a either a virtual or non-virtual publish address is used.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] elemAddr             Address of the element containing the model.
 *  \param[in] pubAddr              Publication address. Ignored when pointer to Label UUID is not
 *                                  NULL.
 *  \param[in] pLabelUuid           Pointer to Label UUID. Set to NULL to use pubAddr as publication
 *                                  address.
 *  \param[in] pPubParams           Pointer to model publication parameters.
 *  \param[in] sigModelId           SIG model identifier.
 *  \param[in] vendorModelId        Vendor model identifier.
 *  \param[in] isSig                TRUE if SIG model identifier should be used, FALSE for vendor.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_PUB_SET_EVENT, MESH_CFG_MDL_PUB_VIRT_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClPubSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                        const uint8_t *pCfgMdlSrDevKey, meshAddress_t elemAddr, meshAddress_t pubAddr,
                        const uint8_t *pLabelUuid, meshModelPublicationParams_t *pPubParams,
                        meshSigModelId_t sigModelId, meshVendorModelId_t vendorModelId,
                        bool_t isSig)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *pOffset;
  bool_t isVirtual = pLabelUuid != NULL;
  wsfMsgHdr_t evt =
  {
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };
  /* Set event type based on label UUID. */
  evt.event = isVirtual ? MESH_CFG_MDL_PUB_VIRT_SET_EVENT : MESH_CFG_MDL_PUB_SET_EVENT;
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate request parameters. */
  if (!MESH_IS_ADDR_UNICAST(elemAddr) || (pPubParams == NULL) ||
      (pPubParams->publishAppKeyIndex > MESH_APP_KEY_INDEX_MAX_VAL) ||
      !MESH_TTL_IS_VALID(pPubParams->publishTtl))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Validate composite states. */
  if((pPubParams->publishPeriodNumSteps >
      (CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_MASK >> CFG_MDL_MSG_MODEL_PUB_PERIOD_NUM_STEPS_SHIFT)) ||
     (pPubParams->publishPeriodStepRes >
      (CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_MASK >> CFG_MDL_MSG_MODEL_PUB_PERIOD_STEP_RES_SHIFT)) ||
     (pPubParams->publishRetransCount >
      (CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_MASK >> CFG_MDL_MSG_MODEL_PUB_RETRANS_CNT_SHIFT)) ||
     (pPubParams->publishRetransSteps50Ms >
      (CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_MASK >> CFG_MDL_MSG_MODEL_PUB_RETRANS_STEPS_SHIFT)))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Validate publication address. */
  if ((!isVirtual) && MESH_IS_ADDR_VIRTUAL(pubAddr))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     isVirtual ? CFG_MDL_MSG_MODEL_PUB_VIRT_SET_NUM_BYTES(isSig) :
                                                 CFG_MDL_MSG_MODEL_PUB_SET_NUM_BYTES(isSig));

  if (pReq != NULL)
  {
    pOffset = pReq->pMsgParam;

    /* Pack element address. */
    UINT16_TO_BSTREAM(pOffset, elemAddr);

    if (!isVirtual)
    {
      /* Pack Publish Address. */
      UINT16_TO_BSTREAM(pOffset, pubAddr);
    }
    else
    {
      /* Pack Label UUID. */
      memcpy(pOffset, pLabelUuid, MESH_LABEL_UUID_SIZE);
      pOffset += MESH_LABEL_UUID_SIZE;
    }

    /* Pack parameters. */
    meshCfgMsgPackModelPubParam(pOffset, pPubParams, sigModelId, vendorModelId, isSig);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = isVirtual ? MESH_CFG_MDL_CL_MODEL_PUB_VIRT_SET : MESH_CFG_MDL_CL_MODEL_PUB_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_MODEL_PUB_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = isVirtual ? MESH_CFG_MDL_PUB_VIRT_SET_EVENT :
                                          MESH_CFG_MDL_PUB_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Changes the Model Subscription List state of a model instance when a either a
 *             virtual or non-virtual subscription address is used.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] elemAddr             Address of the element containing the model.
 *  \param[in] opType               Type of operation applied to the subscription list.
 *  \param[in] subscrAddr           Subscription address. Ignored when pointer to Label UUID is not
 *                                  NULL.
 *  \param[in] pLabelUuid           Pointer to Label UUID. Set to NULL to use subscrAddr as
 *                                  subscription address.
 *  \param[in] sigModelId           SIG model identifier.
 *  \param[in] vendorModelId        Vendor model identifier.
 *  \param[in] isSig                TRUE if SIG model identifier should be used, FALSE for vendor.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_SUBSCR_ADD_EVENT, MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT,
 *       MESH_CFG_MDL_SUBSCR_DEL_EVENT, MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT,
 *       MESH_CFG_MDL_SUBSCR_OVR_EVENT, MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT,
 *       MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT
 *
 *  \remarks   If opType is ::MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT, both subscrAddr and pLabelUuid
 *             are ignored.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClSubscrListChg(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                               const uint8_t *pCfgMdlSrDevKey, meshAddress_t elemAddr,
                               meshCfgMdlClSubscrAddrOp_t opType, meshAddress_t subscrAddr,
                               const uint8_t *pLabelUuid, meshSigModelId_t sigModelId,
                               meshVendorModelId_t vendorModelId, bool_t isSig)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *pOffset;
  bool_t isVirtual = pLabelUuid != NULL;
  meshCfgMdlClOpId_t clOpId;
  uint8_t apiEvt;
  uint16_t msgParamLen;
  wsfMsgHdr_t evt =
  {
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };

  evt.param = cfgMdlSrAddr;

  /* Get operation ID, API event and message parameters length based on
   * label UUID, operation type and model type.
   */
  switch (opType)
  {
    case MESH_CFG_MDL_CL_SUBSCR_ADDR_ADD:
      if (isVirtual)
      {
        clOpId = MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_ADD;
        apiEvt = MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT;
        msgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_VIRT_ADD_NUM_BYTES(isSig);
      }
      else
      {
        clOpId = MESH_CFG_MDL_CL_MODEL_SUBSCR_ADD;
        apiEvt = MESH_CFG_MDL_SUBSCR_ADD_EVENT;
        msgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_ADD_NUM_BYTES(isSig);
      }
      break;
    case MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL:
      if (isVirtual)
      {
        clOpId = MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_DEL;
        apiEvt = MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT;
        msgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_VIRT_DEL_NUM_BYTES(isSig);
      }
      else
      {
        clOpId = MESH_CFG_MDL_CL_MODEL_SUBSCR_DEL;
        apiEvt = MESH_CFG_MDL_SUBSCR_DEL_EVENT;
        msgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_DEL_NUM_BYTES(isSig);
      }
      break;
    case MESH_CFG_MDL_CL_SUBSCR_ADDR_OVR:
      if (isVirtual)
      {
        clOpId = MESH_CFG_MDL_CL_MODEL_SUBSCR_VIRT_OVR;
        apiEvt = MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT;
        msgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_VIRT_OVR_NUM_BYTES(isSig);
      }
      else
      {
        clOpId = MESH_CFG_MDL_CL_MODEL_SUBSCR_OVR;
        apiEvt = MESH_CFG_MDL_SUBSCR_OVR_EVENT;
        msgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_OVR_NUM_BYTES(isSig);
      }
      break;
    case MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL_ALL:
      clOpId = MESH_CFG_MDL_CL_MODEL_SUBSCR_DEL_ALL;
      apiEvt = MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT;
      msgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_DEL_ALL_NUM_BYTES(isSig);
      break;
    default:
      return;
  }
  evt.event = apiEvt;
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate element address. */
  if (!MESH_IS_ADDR_UNICAST(elemAddr))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Validate subscription address. */
  if ((!isVirtual) && (opType != MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL_ALL) &&
      (MESH_IS_ADDR_VIRTUAL(subscrAddr) ||
       MESH_IS_ADDR_UNASSIGNED(subscrAddr) ||
       subscrAddr == MESH_ADDR_GROUP_ALL))

  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex, msgParamLen);

  if (pReq != NULL)
  {
    pOffset = pReq->pMsgParam;

    /* Pack element address. */
    UINT16_TO_BSTREAM(pOffset, elemAddr);

    /* Pack subscription address if the operation is not DELETE ALL. */
    if (opType != MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL_ALL)
    {
      if (!isVirtual)
      {
        /* Pack Subscription Address. */
        UINT16_TO_BSTREAM(pOffset, subscrAddr);
      }
      else
      {
        /* Pack Label UUID. */
        memcpy(pOffset, pLabelUuid, MESH_LABEL_UUID_SIZE);
        pOffset += MESH_LABEL_UUID_SIZE;
      }
    }

    /* Pack model ID. */
    if (isSig)
    {
      UINT16_TO_BSTREAM(pOffset, sigModelId);
    }
    else
    {
      VEND_MDL_TO_BSTREAM(pOffset, vendorModelId);
    }

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = clOpId;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = apiEvt;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Model Subscription List state of a model instance.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] elemAddr             Address of the element containing the model.
 *  \param[in] sigModelId           SIG model identifier.
 *  \param[in] vendorModelId        Vendor model identifier.
 *  \param[in] isSig                TRUE if SIG model identifier should be used, FALSE for vendor.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT,
 *       MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClSubscrListGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                               const uint8_t *pCfgMdlSrDevKey, meshAddress_t elemAddr,
                               meshSigModelId_t sigModelId,
                               meshVendorModelId_t vendorModelId, bool_t isSig)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *pOffset;
  wsfMsgHdr_t evt =
  {
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };

  /* Set event type. */
  evt.event = isSig ? MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT :
                      MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT;
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate element address. */
  if (!MESH_IS_ADDR_UNICAST(elemAddr))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     isSig ? CFG_MDL_MSG_MODEL_SUBSCR_SIG_GET_NUM_BYTES :
                                             CFG_MDL_MSG_MODEL_SUBSCR_VENDOR_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    pOffset = pReq->pMsgParam;

    /* Pack element address. */
    UINT16_TO_BSTREAM(pOffset, elemAddr);

    /* Pack model ID. */
    if (isSig)
    {
      UINT16_TO_BSTREAM(pOffset, sigModelId);
    }
    else
    {
      VEND_MDL_TO_BSTREAM(pOffset, vendorModelId);
    }

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = isSig ? MESH_CFG_MDL_CL_MODEL_SUBSCR_SIG_GET :
                                       MESH_CFG_MDL_CL_MODEL_SUBSCR_VENDOR_GET;
    pReq->pReqParam->rspOpId = isSig ? MESH_CFG_MDL_SR_MODEL_SUBSCR_SIG_LIST :
                                       MESH_CFG_MDL_SR_MODEL_SUBSCR_VENDOR_LIST;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = isSig ? MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT :
                                      MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Modifies a NetKey.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] netKeyIndex          Global identifier of the NetKey.
 *  \param[in] keyOp                Key Operation.
 *  \param[in] pNetKey              Pointer to memory holding the Network Key.
 *
 *  \see meshCfgMdlClEvent_t, MESH_CFG_MDL_NETKEY_ADD_EVENT, MESH_CFG_MDL_NETKEY_UPDT_EVENT,
 *                         MESH_CFG_MDL_NETKEY_DEL_EVENT
 *
 *  \return    None.
 *
 *  \remarks   If the operation is ::MESH_CFG_MDL_CL_KEY_DEL, pNetKey is ignored.
 *
 */
/*************************************************************************************************/
void MeshCfgMdlClNetKeyChg(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                           const uint8_t *pCfgMdlSrDevKey, uint16_t netKeyIndex, meshCfgMdlClKeyOp_t keyOp,
                           const uint8_t *pNetKey)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *pOffset;
  wsfMsgHdr_t evt =
  {
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };
  uint16_t msgLen;
  uint8_t apiEvt;
  meshCfgMdlClOpId_t clOpId;

  /* Set event type, message length, operation id and API event. */
  switch (keyOp)
  {
    case MESH_CFG_MDL_CL_KEY_ADD:
      apiEvt = MESH_CFG_MDL_NETKEY_ADD_EVENT;
      msgLen = CFG_MDL_MSG_NETKEY_ADD_NUM_BYTES;
      clOpId = MESH_CFG_MDL_CL_NETKEY_ADD;
      break;
    case MESH_CFG_MDL_CL_KEY_UPDT:
      apiEvt = MESH_CFG_MDL_NETKEY_UPDT_EVENT;
      msgLen = CFG_MDL_MSG_NETKEY_UPDT_NUM_BYTES;
      clOpId = MESH_CFG_MDL_CL_NETKEY_UPDT;
      break;
    case MESH_CFG_MDL_CL_KEY_DEL:
      apiEvt = MESH_CFG_MDL_NETKEY_DEL_EVENT;
      msgLen = CFG_MDL_MSG_NETKEY_DEL_NUM_BYTES;
      clOpId = MESH_CFG_MDL_CL_NETKEY_DEL;
      break;
    default:
      MESH_TRACE_ERR0("CFG CL: Out of bounds key operation type");
      return;
  }

  evt.event = apiEvt;
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate NetKey Index. */
  if ((netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL) ||
      ((keyOp != MESH_CFG_MDL_CL_KEY_DEL) && (pNetKey == NULL)))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex, msgLen);

  if (pReq != NULL)
  {
    pOffset = pReq->pMsgParam;

    /* Pack key binding. */
    pOffset += meshCfgMsgPackSingleKeyIndex(pOffset, netKeyIndex);

    /* If operation is not delete, pack the key. */
    if (keyOp != MESH_CFG_MDL_CL_KEY_DEL)
    {
      memcpy(pOffset, pNetKey, MESH_KEY_SIZE_128);
    }

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = clOpId;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_NETKEY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = apiEvt;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets a NetKey List.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_NETKEY_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNetKeyGet(meshAddress_t cfgMdlSrAddr, const uint8_t *pCfgMdlSrDevKey,
                           uint16_t cfgMdlSrNetKeyIndex)
{
  meshCfgMdlClOpReq_t *pReq;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_NETKEY_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_NETKEY_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_NETKEY_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_NETKEY_LIST;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_NETKEY_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Modifies an AppKey.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] pAppKeyBind          Pointer to bind definition for the Application Key.
 *  \param[in] keyOp                Key Operation.
 *  \param[in] pAppKey              Pointer to memory holding the Application Key.
 *
 *  \see meshCfgMdlClEvent_t, MESH_CFG_MDL_APPKEY_ADD_EVENT, MESH_CFG_MDL_APPKEY_UPDT_EVENT,
 *                         MESH_CFG_MDL_APPKEY_DEL_EVENT
 *
 *  \return    None.
 *
 *  \remarks   If the operation is ::MESH_CFG_MDL_CL_KEY_DEL, pAppKey is ignored.
 *
 */
/*************************************************************************************************/
void MeshCfgMdlClAppKeyChg(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                           const uint8_t *pCfgMdlSrDevKey, const meshAppNetKeyBind_t *pAppKeyBind,
                           meshCfgMdlClKeyOp_t keyOp, const uint8_t *pAppKey)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *pOffset;
  wsfMsgHdr_t evt =
  {
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };
  uint16_t msgLen;
  uint8_t apiEvt;
  meshCfgMdlClOpId_t clOpId;

  /* Set event type, message length, operation id and API event. */
  switch (keyOp)
  {
    case MESH_CFG_MDL_CL_KEY_ADD:
      apiEvt = MESH_CFG_MDL_APPKEY_ADD_EVENT;
      msgLen = CFG_MDL_MSG_APPKEY_ADD_NUM_BYTES;
      clOpId = MESH_CFG_MDL_CL_APPKEY_ADD;
      break;
    case MESH_CFG_MDL_CL_KEY_UPDT:
      apiEvt = MESH_CFG_MDL_APPKEY_UPDT_EVENT;
      msgLen = CFG_MDL_MSG_APPKEY_UPDT_NUM_BYTES;
      clOpId = MESH_CFG_MDL_CL_APPKEY_UPDT;
      break;
    case MESH_CFG_MDL_CL_KEY_DEL:
      apiEvt = MESH_CFG_MDL_APPKEY_DEL_EVENT;
      msgLen = CFG_MDL_MSG_APPKEY_DEL_NUM_BYTES;
      clOpId = MESH_CFG_MDL_CL_APPKEY_DEL;
      break;
    default:
      MESH_TRACE_ERR0("CFG CL: Out of bounds key operation type");
      return;
  }

  evt.event = apiEvt;
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate key bind and key. */
  if ((pAppKeyBind == NULL) || (pAppKeyBind->appKeyIndex > MESH_APP_KEY_INDEX_MAX_VAL) ||
      (pAppKeyBind->netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL) ||
      ((keyOp != MESH_CFG_MDL_CL_KEY_DEL) && (pAppKey == NULL)))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex, msgLen);

  if (pReq != NULL)
  {
    pOffset = pReq->pMsgParam;

    /* Pack key binding. */
    pOffset += meshCfgMsgPackTwoKeyIndex(pOffset, pAppKeyBind->netKeyIndex, pAppKeyBind->appKeyIndex);

    /* If operation is not delete, pack the key. */
    if (keyOp != MESH_CFG_MDL_CL_KEY_DEL)
    {
      memcpy(pOffset, pAppKey, MESH_KEY_SIZE_128);
    }

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = clOpId;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_APPKEY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = apiEvt;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets an AppKey List.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] netKeyIndex          NetKey Index for which bound AppKeys indexes are read.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_APPKEY_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClAppKeyGet(meshAddress_t cfgMdlSrAddr, const uint8_t *pCfgMdlSrDevKey,
                           uint16_t cfgMdlSrNetKeyIndex, uint16_t netKeyIndex)
{
  meshCfgMdlClOpReq_t *pReq;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_APPKEY_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate NetKeyIndex. */
  if (netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL)
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_APPKEY_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack NetKeyIndex. */
    (void)meshCfgMsgPackSingleKeyIndex(pReq->pMsgParam, netKeyIndex);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_APPKEY_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_APPKEY_LIST;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_APPKEY_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the current Node Identity State of a subnet.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] netKeyIndex          NetKey Index identifying the subnet.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_NODE_IDENTITY_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNodeIdentityGet(meshAddress_t cfgMdlSrAddr, const uint8_t *pCfgMdlSrDevKey,
                                 uint16_t cfgMdlSrNetKeyIndex, uint16_t netKeyIndex)
{
  meshCfgMdlClOpReq_t *pReq;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_NODE_IDENTITY_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate NetKeyIndex. */
  if (netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL)
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_NODE_IDENTITY_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack NetKeyIndex. */
    (void)meshCfgMsgPackSingleKeyIndex(pReq->pMsgParam, netKeyIndex);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_NODE_IDENTITY_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_NODE_IDENTITY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_NODE_IDENTITY_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the current Node Identity State of a subnet.
 *
 *  \param[in] cfgMdlSrAddr          Primary element containing an instance of Configuration Server
 *                                   model.
 *  \param[in] cfgMdlSrNetKeyIndex   Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey       Pointer to the Device Key of the remote Configuration Server
 *                                   or NULL for local (cfgMdlSrAddr is
 *                                   MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] netKeyIndex           NetKey Index identifying the subnet.
 *  \param[in] nodeIdentityState     New value for the Node Identity state.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNodeIdentitySet(meshAddress_t cfgMdlSrAddr, const uint8_t *pCfgMdlSrDevKey,
                                 uint16_t cfgMdlSrNetKeyIndex, uint16_t netKeyIndex,
                                 meshNodeIdentityStates_t nodeIdentityState)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *pTemp;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate NetKeyIndex and state. */
  if ((netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL) ||
      (nodeIdentityState >= MESH_NODE_IDENTITY_NOT_SUPPORTED))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_NODE_IDENTITY_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    pTemp = pReq->pMsgParam;

    /* Pack NetKeyIndex. */
    pTemp += meshCfgMsgPackSingleKeyIndex(pTemp, netKeyIndex);

    /* Pack state. */
    UINT8_TO_BSTREAM(pTemp, nodeIdentityState);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_NODE_IDENTITY_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_NODE_IDENTITY_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Binds or unbinds a model to an AppKey.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] bind                 TRUE to bind model to AppKey, FALSE to unbind.
 *  \param[in] appKeyIndex          Global identifier of the AppKey to be (un)bound to the model.
 *  \param[in] elemAddr             Address of the element containing the model.
 *  \param[in] sigModelId           SIG model identifier.
 *  \param[in] vendorModelId        Vendor model identifier.
 *  \param[in] isSig                TRUE if SIG model identifier should be used, FALSE for vendor.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_APP_BIND_EVENT, MESH_CFG_MDL_APP_UNBIND_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClAppBind(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                         const uint8_t *pCfgMdlSrDevKey, bool_t bind, uint16_t appKeyIndex,
                         meshAddress_t elemAddr, meshSigModelId_t sigModelId,
                         meshVendorModelId_t vendorModelId, bool_t isSig)
{
  meshCfgMdlClOpReq_t *pReq;
  wsfMsgHdr_t evt =
  {
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };

  evt.event = bind ? MESH_CFG_MDL_APP_BIND_EVENT : MESH_CFG_MDL_APP_UNBIND_EVENT;
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate state parameters. */
  if ((appKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL) || !MESH_IS_ADDR_UNICAST(elemAddr))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     bind ? CFG_MDL_MSG_MODEL_APP_BIND_NUM_BYTES(isSig) :
                                            CFG_MDL_MSG_MODEL_APP_UNBIND_NUM_BYTES(isSig));

  if (pReq != NULL)
  {
    /* Pack state. */
    meshCfgMsgPackModelAppBind(pReq->pMsgParam, elemAddr, appKeyIndex, sigModelId, vendorModelId,
                               isSig);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = bind ? MESH_CFG_MDL_CL_MODEL_APP_BIND : MESH_CFG_MDL_CL_MODEL_APP_UNBIND;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_MODEL_APP_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = evt.event;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets a list of AppKeys bound to a model.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] elemAddr             Address of the element containing the model.
 *  \param[in] sigModelId           SIG model identifier.
 *  \param[in] vendorModelId        Vendor model identifier.
 *  \param[in] isSig                TRUE if SIG model identifier should be used, FALSE for vendor.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_APP_SIG_GET_EVENT, MESH_CFG_MDL_APP_VENDOR_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClAppGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                        const uint8_t *pCfgMdlSrDevKey, meshAddress_t elemAddr,
                        meshSigModelId_t sigModelId, meshVendorModelId_t vendorModelId,
                        bool_t isSig)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *pOffset;
  wsfMsgHdr_t evt =
  {
    .status = MESH_CFG_MDL_CL_INVALID_PARAMS
  };

  evt.event = isSig ? MESH_CFG_MDL_APP_SIG_GET_EVENT : MESH_CFG_MDL_APP_VENDOR_GET_EVENT;
  evt.param = cfgMdlSrAddr;

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate state parameters. */
  if (!MESH_IS_ADDR_UNICAST(elemAddr))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_MODEL_APP_GET_NUM_BYTES(isSig));

  if (pReq != NULL)
  {
    pOffset = pReq->pMsgParam;

    /* Pack element address. */
    UINT16_TO_BSTREAM(pOffset, elemAddr);

    /* Pack model identifier. */
    if (isSig)
    {
      UINT16_TO_BSTREAM(pOffset, sigModelId);
    }
    else
    {
      VEND_MDL_TO_BSTREAM(pOffset, vendorModelId);
    }

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = isSig ? MESH_CFG_MDL_CL_MODEL_APP_SIG_GET :
                                       MESH_CFG_MDL_CL_MODEL_APP_VENDOR_GET;
    pReq->pReqParam->rspOpId = isSig ? MESH_CFG_MDL_SR_MODEL_APP_SIG_LIST :
                                       MESH_CFG_MDL_SR_MODEL_APP_VENDOR_LIST;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = evt.event;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Reset Node state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_NODE_RESET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNodeReset(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                           const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_CL_NODE_RESET,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_NODE_RESET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_NODE_RESET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_NODE_RESET_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_NODE_RESET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Friend state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_FRIEND_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClFriendGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                           const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_FRIEND_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_FRIEND_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_FRIEND_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_FRIEND_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_FRIEND_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Friend state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] friendState          New value for the Friend state.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_FRIEND_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClFriendSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                           const uint8_t *pCfgMdlSrDevKey, meshFriendStates_t friendState)
{
  meshCfgMdlClOpReq_t *pReq;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_FRIEND_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate Friend state values. */
  if (friendState >= MESH_FRIEND_FEATURE_NOT_SUPPORTED)
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_FRIEND_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Set Friend field. */
    pReq->pMsgParam[0] = friendState;

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_FRIEND_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_FRIEND_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_FRIEND_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Key Refresh Phase state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] netKeyIndex          Network Key Index for which the phase is read.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_KEY_REF_PHASE_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClKeyRefPhaseGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                                const uint8_t *pCfgMdlSrDevKey, uint16_t netKeyIndex)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_KEY_REF_PHASE_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate NetKey Index. */
  if (netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL)
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_KEY_REF_PHASE_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack NetKey Index. */
    (void)meshCfgMsgPackSingleKeyIndex(pReq->pMsgParam, netKeyIndex);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_KEY_REF_PHASE_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_KEY_REF_PHASE_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_KEY_REF_PHASE_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Key Refresh Phase state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] netKeyIndex          Network Key Index for which the phase is read.
 *  \param[in] transition           Transition number. (Table 4.18 of the specification)
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClKeyRefPhaseSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                                const uint8_t *pCfgMdlSrDevKey, uint16_t netKeyIndex,
                                meshKeyRefreshTrans_t transition)
{
  meshCfgMdlClOpReq_t *pReq;
  uint8_t *ptr;
  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  if ((transition != MESH_KEY_REFRESH_TRANS02) && (transition != MESH_KEY_REFRESH_TRANS03))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_KEY_REF_PHASE_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    ptr = pReq->pMsgParam;

    /* Pack Key Refresh Phase state. */
    ptr += meshCfgMsgPackSingleKeyIndex(ptr, netKeyIndex);
    /* Pack transition. */
    UINT8_TO_BSTREAM(ptr, transition);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_KEY_REF_PHASE_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_KEY_REF_PHASE_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets a Heartbeat Publication state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_HB_PUB_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClHbPubGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                          const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_HB_PUB_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_HB_PUB_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_HB_PUB_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_HB_PUB_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_HB_PUB_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets Heartbeat Publication states.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] pHbPubState          Pointer to new values for Heartbeat Publication state.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_HB_PUB_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClHbPubSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                          const uint8_t *pCfgMdlSrDevKey, meshHbPub_t *pHbPubState)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_HB_PUB_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate Heartbeat Publication data. */
  if (((pHbPubState->countLog >= CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_START) &&
      (pHbPubState->countLog <= CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_END)) ||
      (pHbPubState->periodLog >= CFG_MDL_HB_PUB_PERIOD_LOG_NOT_ALLOW_START) ||
      (pHbPubState->ttl >= CFG_MDL_HB_PUB_TTL_NOT_ALLOW_START) ||
      (MESH_IS_ADDR_VIRTUAL(pHbPubState->dstAddr)) ||
      (pHbPubState->netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Clear RFU bits. */
  pHbPubState->features &= (MESH_FEAT_RFU_START - 1);

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_HB_PUB_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack Heartbeat Publication state. */
    meshCfgMsgPackHbPub(pReq->pMsgParam, pHbPubState);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_HB_PUB_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_HB_PUB_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_HB_PUB_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets a Heartbeat Subscription state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_HB_SUB_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClHbSubGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                          const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_HB_SUB_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_HB_SUB_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_HB_SUB_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_HB_SUB_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_HB_SUB_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets Heartbeat Subscription states.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] pHbSubState          Pointer to new values for Heartbeat Subscription state.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_HB_SUB_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClHbSubSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                          const uint8_t *pCfgMdlSrDevKey, meshHbSub_t *pHbSubState)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_HB_SUB_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate Heartbeat Subscription data. */
  if ((pHbSubState->periodLog >= CFG_MDL_HB_SUB_PERIOD_LOG_NOT_ALLOW_START) ||
      MESH_IS_ADDR_VIRTUAL(pHbSubState->dstAddr) ||
      MESH_IS_ADDR_VIRTUAL(pHbSubState->srcAddr) ||
      MESH_IS_ADDR_GROUP(pHbSubState->srcAddr))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_HB_SUB_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack Heartbeat Subscription set. */
    meshCfgMsgPackHbSubSet(pReq->pMsgParam, pHbSubState);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_HB_SUB_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_HB_SUB_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_HB_SUB_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the PollTimeout state of a Low Power Node from a Friend node.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] lpnAddr              Primary element address of the Low Power Node.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_LPN_POLLTIMEOUT_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClPollTimeoutGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                                const uint8_t *pCfgMdlSrDevKey, meshAddress_t lpnAddr)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_LPN_POLLTIMEOUT_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate LPN address. */
  if (!MESH_IS_ADDR_UNICAST(lpnAddr))
  {
    /* Invoke callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_LPN_POLLTIMEOUT_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack LPN address. */
    UINT16_TO_BUF(pReq->pMsgParam, lpnAddr);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_LPN_PT_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_LPN_PT_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_LPN_POLLTIMEOUT_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Network Transmit state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNwkTransmitGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                                const uint8_t *pCfgMdlSrDevKey)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_NWK_TRANS_GET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_NWK_TRANS_GET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_NWK_TRANS_GET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_NWK_TRANS_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_NWK_TRANS_GET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Network Transmit state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                                  model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                                  or NULL for local (cfgMdlSrAddr is
 *                                  MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *  \param[in] pNwkTransmit         Pointer to new value for the Network Transmit state.
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNwkTransmitSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                                const uint8_t *pCfgMdlSrDevKey, meshNwkTransState_t *pNwkTransmit)
{
  meshCfgMdlClOpReq_t *pReq;

  meshCfgMdlHdr_t evt =
  {
    .hdr.event = MESH_CFG_MDL_SR_EVENT,
    .hdr.param = MESH_CFG_MDL_NWK_TRANS_SET_EVENT,
    .hdr.status = MESH_CFG_MDL_CL_INVALID_PARAMS,
    .peerAddress = cfgMdlSrAddr
  };

  /* Run default server parameters check and call user callback for invalid. */
  if(!meshCfgMdlClCheckSrParamsAndNotify(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                         (meshCfgMdlClEvt_t *)&evt))
  {
    return;
  }

  /* Validate state consistency. */
  if ((pNwkTransmit == NULL) ||
      (pNwkTransmit->transCount >
      (CFG_MDL_MSG_NWK_TRANS_STATE_CNT_MASK >> CFG_MDL_MSG_NWK_TRANS_STATE_CNT_SHIFT)) ||
      (pNwkTransmit->transIntervalSteps10Ms >
      (CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_MASK >> CFG_MDL_MSG_NWK_TRANS_STATE_INTVL_SHIFT)))
  {
    /* Invoke user callback. */
    meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

    return;
  }

  /* Allocate request. */
  pReq = meshCfgMdlClAllocateRequest(cfgMdlSrAddr, pCfgMdlSrDevKey, cfgMdlSrNetKeyIndex,
                                     CFG_MDL_MSG_NWK_TRANS_SET_NUM_BYTES);

  if (pReq != NULL)
  {
    /* Pack Network Transmit state. */
    meshCfgMsgPackNwkTrans(pReq->pMsgParam, pNwkTransmit);

    /* Configure request and response operation identifiers. */
    pReq->pReqParam->reqOpId = MESH_CFG_MDL_CL_NWK_TRANS_SET;
    pReq->pReqParam->rspOpId = MESH_CFG_MDL_SR_NWK_TRANS_STATUS;

    /* Configure API event in case of timeout. */
    pReq->pReqParam->apiEvt = MESH_CFG_MDL_NWK_TRANS_SET_EVENT;

    /* Send WSF message. */
    WsfMsgSend(meshCb.handlerId, (void *)pReq);
  }
}
