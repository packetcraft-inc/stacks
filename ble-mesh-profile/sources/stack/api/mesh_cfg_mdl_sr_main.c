/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_sr_main.c
 *
 *  \brief  Configuration Server module implementation.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_handler.h"

#include "mesh_main.h"
#include "mesh_security.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_lower_transport.h"
#include "mesh_network_beacon.h"
#include "mesh_upper_transport.h"
#include "mesh_upper_transport_heartbeat.h"
#include "mesh_access.h"
#include "mesh_proxy_sr.h"
#include "mesh_friend.h"

#include "mesh_utils.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_sr.h"
#include "mesh_cfg_mdl_sr_main.h"

#include "mesh_cfg_mdl_messages.h"

#include <string.h>


/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Configuration Server operation request action table */
const meshCfgMdlSrOpReqAct_t meshCfgMdlSrOpReqActTbl[MESH_CFG_MDL_CL_MAX_OP] =
{
  meshCfgMdlSrHandleBeaconGet,
  meshCfgMdlSrHandleBeaconSet,
  meshCfgMdlSrHandleCompositionDataGet,
  meshCfgMdlSrHandleDefaultTtlGet,
  meshCfgMdlSrHandleDefaultTtlSet,
  meshCfgMdlSrHandleGattProxyGet,
  meshCfgMdlSrHandleGattProxySet,
  meshCfgMdlSrHandleRelayGet,
  meshCfgMdlSrHandleRelaySet,
  meshCfgMdlSrHandleModelPubGet,
  meshCfgMdlSrHandleModelPubSet,
  meshCfgMdlSrHandleModelPubVirtSet,
  meshCfgMdlSrHandleModelSubscrAdd,
  meshCfgMdlSrHandleModelSubscrVirtAdd,
  meshCfgMdlSrHandleModelSubscrDel,
  meshCfgMdlSrHandleModelSubscrVirtDel,
  meshCfgMdlSrHandleModelSubscrOvr,
  meshCfgMdlSrHandleModelSubscrVirtOvr,
  meshCfgMdlSrHandleModelSubscrDelAll,
  meshCfgMdlSrHandleModelSubscrSigGet,
  meshCfgMdlSrHandleModelSubscrVendorGet,
  meshCfgMdlSrHandleNetKeyAdd,
  meshCfgMdlSrHandleNetKeyUpdt,
  meshCfgMdlSrHandleNetKeyDel,
  meshCfgMdlSrHandleNetKeyGet,
  meshCfgMdlSrHandleAppKeyAdd,
  meshCfgMdlSrHandleAppKeyUpdt,
  meshCfgMdlSrHandleAppKeyDel,
  meshCfgMdlSrHandleAppKeyGet,
  meshCfgMdlSrHandleNodeIdentityGet,
  meshCfgMdlSrHandleNodeIdentitySet,
  meshCfgMdlSrHandleModelAppBind,
  meshCfgMdlSrHandleModelAppUnbind,
  meshCfgMdlSrHandleModelAppSigGet,
  meshCfgMdlSrHandleModelAppVendorGet,
  meshCfgMdlSrHandleNodeReset,
  meshCfgMdlSrHandleFriendGet,
  meshCfgMdlSrHandleFriendSet,
  meshCfgMdlSrHandleKeyRefPhaseGet,
  meshCfgMdlSrHandleKeyRefPhaseSet,
  meshCfgMdlSrHandleHbPubGet,
  meshCfgMdlSrHandleHbPubSet,
  meshCfgMdlSrHandleHbSubGet,
  meshCfgMdlSrHandleHbSubSet,
  meshCfgMdlSrHandleLpnPollTimeoutGet,
  meshCfgMdlSrHandleNwkTransGet,
  meshCfgMdlSrHandleNwkTransSet,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Configuration Server response.
 *
 *  \param[in] opId                 Operation identifier of the response.
 *  \param[in] pMsgParam            Pointer to packed response message parameters.
 *  \param[in] msgParamLen          Length of the packed response message parameters.
 *  \param[in] cfgMdlClAddr         Address of the Configuration Client.
 *  \param[in] recvTtl              TTL of the request.
 *  \param[in] cfgMdlClNetKeyIndex  Identifier of the sub-net on which the request was received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrSendRsp(meshCfgMdlSrOpId_t opId, uint8_t *pMsgParam, uint16_t msgParamLen,
                      meshAddress_t cfgMdlClAddr, uint8_t recvTtl, uint16_t cfgMdlClNetKeyIndex)
{
  meshMsgInfo_t msgInfo =
  {
    .appKeyIndex = MESH_APPKEY_INDEX_LOCAL_DEV_KEY, /* Use local device key for the response. */
    .elementId = 0,                                 /* Configuration Server allowed on element 0. */
    .modelId.sigModelId = MESH_CFG_MDL_SR_MODEL_ID  /* Configuration Server model identifier. */
  };

  /* Response address is Configuration Client address. */
  msgInfo.dstAddr = cfgMdlClAddr;
  /* Set opcode. */
  msgInfo.opcode = meshCfgMdlSrOpcodes[opId];
  msgInfo.pDstLabelUuid = NULL;
  /* Set TTL to max or 0 depending on receiving TTL. */
  msgInfo.ttl = recvTtl == 0 ? 0 : MESH_USE_DEFAULT_TTL;

  /* Send message. */
  MeshAccSendMessage((const meshMsgInfo_t *)&msgInfo, pMsgParam, msgParamLen, cfgMdlClNetKeyIndex,
                     MESH_ACC_RSP_MIN_SEND_DELAY_MS, MESH_ACC_RSP_MAX_SEND_DELAY_MS(TRUE));
}

/*************************************************************************************************/
/*!
 *  \brief     Empty callback implementation for notifications.
 *
 *  \param[in] pEvt  Pointer to Configuration Server event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrEmptyCback(const meshCfgMdlSrEvt_t *pEvt)
{
  (void)pEvt;
  MESH_TRACE_ERR0("MESH CFG SR: User callback not registered!");
}

/*************************************************************************************************/
/*!
 *  \brief  Empty callback implementation for Friend State changed notifications.
 *
 *  \return None.
 */
/*************************************************************************************************/
void meshCfgMdlSrEmptyFriendStateChgCback(void)
{

}

/*************************************************************************************************/
/*!
 *  \brief  Empty callback implementation for NetKey deleted notification.
 *
 *  \return None.
 */
/*************************************************************************************************/
void meshCfgMdlSrEmptyNetKeyDelNotifyCback(uint16_t netKeyIndex)
{
  (void)netKeyIndex;
}

/*************************************************************************************************/
/*!
 *  \brief  Empty callback implementation for Poll Timeout get.
 *
 *  \return 0 indicating unsupported Friend feature.
 */
/*************************************************************************************************/
uint32_t meshCfgMdlSrEmptyPollTimeoutGetCback(meshAddress_t lpnAddr)
{
  (void)lpnAddr;
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Callback implementation for receiving Access Layer messages for this core model.
 *
 *  \param[in] opcodeIdx    Index of the opcode in the receive opcodes array registered.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] elemId       Destination element identifier.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrAccMsgRcvCback(uint8_t opcodeIdx, uint8_t *pMsgParam,
                                uint16_t msgParamLen, meshAddress_t src,
                                meshElementId_t elemId, uint8_t ttl,
                                uint16_t netKeyIndex)
{
   /* Check received message opcode matches expected request opcode and valid element id. */
  if ((opcodeIdx < MESH_CFG_MDL_CL_MAX_OP) && (elemId == 0))
  {
    /* Use action function to handle request. */
    meshCfgMdlSrOpReqActTbl[opcodeIdx](pMsgParam, msgParamLen, src, ttl, netKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Beacon Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleBeaconGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  meshBeaconStates_t beaconState;

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_BEACON_GET_NUM_BYTES)
  {
    return;
  }

  /* Read from local config. */
  beaconState = MeshLocalCfgGetBeaconState();

  /* Send Beacons Status. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_BEACON_STATUS, &beaconState, sizeof(beaconState), src, ttl,
                      netKeyIndex);
  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Beacon Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleBeaconSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  meshCfgMdlBeaconStateEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_BEACON_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_BEACON_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Get Beacon state. */
  evt.state = pMsgParam[0];

  /* Validate Beacon state values. */
  if (!MESH_BEACON_STATE_IS_VALID(evt.state))
  {
    return;
  }

  /* Store Beacon state. */
  MeshLocalCfgSetBeaconState(evt.state);

  /* Signal Beacon state changed. */
  MeshNwkBeaconHandleStateChanged();

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_BEACON_STATUS, pMsgParam, msgParamLen, src, ttl, netKeyIndex);

  /* Set event parameters. */
  evt.cfgMdlHdr.peerAddress = src;

  /* Call notification callback. */
  meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Composition Data Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleCompositionDataGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t page;
  uint8_t *rspMsgParam;
  uint16_t rspMsgParamLen;

  /* Validate length. */
  if (msgParamLen < CFG_MDL_MSG_COMP_DATA_GET_NUM_BYTES)
  {
    return;
  }

  page = pMsgParam[0];

  /* Only page 0 supported. */
  if (page != 0)
  {
    page = 0;
  }

  /* Get required memory for Composition Data Status with Page 0. */
  rspMsgParamLen = CFG_MDL_MSG_COMP_DATA_STATE_NUM_BYTES + meshCfgMsgGetPackedCompDataPg0Size();

  /* Allocate memory for it. */
  if ((rspMsgParam = (uint8_t *)WsfBufAlloc(rspMsgParamLen)) == NULL)
  {
    return;
  }

  /* Pack state. */
  meshCfgMsgPackCompData(rspMsgParam, page);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_COMP_DATA_STATUS, rspMsgParam, rspMsgParamLen, src, ttl, netKeyIndex);

  /* Free memory. */
  WsfBufFree(rspMsgParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Default TTL Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleDefaultTtlGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                     uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t defaultTtl = 0;

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_DEFAULT_TTL_GET_NUM_BYTES)
  {
    return;
  }

  /* Read from local config. */
  defaultTtl = MeshLocalCfgGetDefaultTtl();

  /* Send Default TTL Status. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_DEFAULT_TTL_STATUS, &defaultTtl, sizeof(defaultTtl), src, ttl,
                      netKeyIndex);
  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Default TTL Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleDefaultTtlSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                     uint8_t ttl, uint16_t netKeyIndex)
{
  meshCfgMdlDefaultTtlStateEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_DEFAULT_TTL_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Get Default TTL. */
  evt.ttl = pMsgParam[0];

  /* Validate TTL state values. */
  if (!MESH_TTL_IS_VALID(evt.ttl) || (evt.ttl == MESH_TX_TTL_FILTER_VALUE) ||
      (evt.ttl == MESH_USE_DEFAULT_TTL))
  {
    return;
  }

  /* Store Default TTL. */
  MeshLocalCfgSetDefaultTtl(evt.ttl);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_DEFAULT_TTL_STATUS, pMsgParam, msgParamLen, src, ttl, netKeyIndex);

  /* Set event parameters. */
  evt.cfgMdlHdr.peerAddress = src;

  /* Call notification callback. */
  meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Gatt Proxy Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleGattProxyGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                     uint8_t ttl, uint16_t netKeyIndex)
{
  meshGattProxyStates_t gattProxyState;

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_GATT_PROXY_GET_NUM_BYTES)
  {
    return;
  }

  /* Read from local config. */
  gattProxyState = MeshLocalCfgGetGattProxyState();

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_GATT_PROXY_STATUS, &gattProxyState, sizeof(gattProxyState), src,
                      ttl, netKeyIndex);
  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Gatt Proxy Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleGattProxySet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                    uint8_t ttl, uint16_t netKeyIndex)
{
  uint16_t keyIndex;
  uint16_t indexer = 0;
  uint8_t rspMsgParam[CFG_MDL_MSG_GATT_PROXY_STATUS_NUM_BYTES];
  meshCfgMdlGattProxyEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_GATT_PROXY_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  bool_t notifyUpperLayers = FALSE;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_GATT_PROXY_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Get Gatt Proxy state. */
  evt.gattProxy = pMsgParam[0];

  /* Validate Gatt Proxy state values. */
  if (evt.gattProxy > MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED)
  {
    return;
  }

  /* Check if feature supported. */
  if (MeshLocalCfgGetGattProxyState() == MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED)
  {
    evt.gattProxy = MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    /* Check for actual changes to notify upper layers. */
    if(MeshLocalCfgGetGattProxyState() != evt.gattProxy)
    {
      notifyUpperLayers = TRUE;
    }

    /* Store Gatt Proxy state. */
    MeshLocalCfgSetGattProxyState(evt.gattProxy);

    if (notifyUpperLayers)
    {
      /* Inform Hearbeat module that feature is changed. */
      MeshHbFeatureStateChanged(MESH_FEAT_PROXY);
    }

    /* Disable Proxy feature. Disconnect GATT if available. */
    if (evt.gattProxy == MESH_GATT_PROXY_FEATURE_DISABLED)
    {
      /* Disable Proxy on the node. */
      MeshProxySrDisable();

      /* Set Node Identity state to stopped for all subnets. */
      while (MeshLocalCfgGetNextNetKeyIndex(&keyIndex, &indexer) == MESH_SUCCESS)
      {
        MeshLocalCfgSetNodeIdentityState(keyIndex, MESH_NODE_IDENTITY_STOPPED);
      }
    }
  }

  /* Set response. */
  rspMsgParam[0] = evt.gattProxy;

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_GATT_PROXY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  if (notifyUpperLayers)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Relay Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleRelayGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex)
{
  meshRelayStates_t state;
  meshRelayRetransState_t retranState;
  uint8_t rspMsgParam[CFG_MDL_MSG_RELAY_COMP_STATE_NUM_BYTES];

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_RELAY_GET_NUM_BYTES)
  {
    return;
  }

  /* Read Relay State. */
  state = MeshLocalCfgGetRelayState();

  /* Read Relay Retransmit state. */
  retranState.retransCount = MeshLocalCfgGetRelayRetransmitCount();
  retranState.retransIntervalSteps10Ms = MeshLocalCfgGetRelayRetransmitIntvlSteps();

  /* Pack response. */
  meshCfgMsgPackRelay(rspMsgParam, &state, &retranState);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_RELAY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl, netKeyIndex);

  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Relay Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleRelaySet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t rspMsgParam[CFG_MDL_MSG_RELAY_STATUS_NUM_BYTES];
  meshCfgMdlRelayCompositeStateEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_RELAY_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  bool_t notifyUpperLayers = FALSE;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_RELAY_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack states. */
  meshCfgMsgUnpackRelay(pMsgParam, &evt.relayState, &evt.relayRetrans);

  if (evt.relayState > MESH_RELAY_FEATURE_NOT_SUPPORTED)
  {
    return;
  }

  /* Check if feature supported. */
  if (MeshLocalCfgGetRelayState() == MESH_RELAY_FEATURE_NOT_SUPPORTED)
  {
    evt.relayState = MESH_RELAY_FEATURE_NOT_SUPPORTED;
    evt.relayRetrans.retransCount = 0;
    evt.relayRetrans.retransIntervalSteps10Ms = 0;
  }
  else
  {
    /* Check if there is an actual state change. */
    if(MeshLocalCfgGetRelayState() != evt.relayState)
    {
      notifyUpperLayers = TRUE;
    }
    /* Set new states. */
    MeshLocalCfgSetRelayState(evt.relayState);
    MeshLocalCfgSetRelayRetransmitCount(evt.relayRetrans.retransCount);
    MeshLocalCfgSetRelayRetransmitIntvlSteps(evt.relayRetrans.retransIntervalSteps10Ms);

    if (notifyUpperLayers)
    {
      /* Inform Heartbeat module that feature is changed. */
      MeshHbFeatureStateChanged(MESH_FEAT_RELAY);
    }
  }

  /* Pack response. */
  meshCfgMsgPackRelay(rspMsgParam, &evt.relayState, &evt.relayRetrans);

  /* Response contains the same packed states as set message. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_RELAY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  if(notifyUpperLayers)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Node Identity Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNodeIdentityGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                       uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  uint16_t msgNetKeyIndex;
  meshNodeIdentityStates_t state;
  uint8_t rspMsgParam[CFG_MDL_MSG_NODE_IDENTITY_STATUS_NUM_BYTES];

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_NODE_IDENTITY_GET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKey Index.*/
  (void)meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &msgNetKeyIndex);

  /* Verify if NetKey exists by reading Node Identity. */
  state = MeshLocalCfgGetNodeIdentityState(msgNetKeyIndex);

  if (state >= MESH_NODE_IDENTITY_PROHIBITED_START)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
    state = 0;
  }
  else
  {
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }

  ptr = &rspMsgParam[1];

  /* Pack NetKey Index. */
  ptr += meshCfgMsgPackSingleKeyIndex(ptr, msgNetKeyIndex);

  /* Pack state. */
  UINT8_TO_BSTREAM(ptr, state);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NODE_IDENTITY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Node Identity Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNodeIdentitySet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                       uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  meshCfgMdlNodeIdentityEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };
  meshNodeIdentityStates_t localState;
  uint8_t rspMsgParam[CFG_MDL_MSG_NODE_IDENTITY_STATUS_NUM_BYTES];

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_NODE_IDENTITY_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKey Index.*/
  pMsgParam += meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &evt.netKeyIndex);

  /* Unpack state. */
  BSTREAM_TO_UINT8(evt.state, pMsgParam);

  /* Check if received state is prohitibed. */
  if (evt.state >= MESH_NODE_IDENTITY_NOT_SUPPORTED)
  {
    return;
  }

  /* Verify if NetKey exists by reading Node Identity. */
  localState = MeshLocalCfgGetNodeIdentityState(evt.netKeyIndex);

  if (localState >= MESH_NODE_IDENTITY_PROHIBITED_START)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
    evt.state = 0;
  }
  else if (localState == MESH_NODE_IDENTITY_NOT_SUPPORTED)
  {
    evt.state = MESH_NODE_IDENTITY_NOT_SUPPORTED;
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }
  else if (MeshLocalCfgGetGattProxyState() == MESH_GATT_PROXY_FEATURE_DISABLED)
  {
    evt.state = MESH_NODE_IDENTITY_STOPPED;
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }
  else
  {
    /* Set state in local config. */
    MeshLocalCfgSetNodeIdentityState(evt.netKeyIndex, evt.state);

    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }

  ptr = &rspMsgParam[1];

  /* Pack NetKey Index. */
  ptr += meshCfgMsgPackSingleKeyIndex(ptr, evt.netKeyIndex);

  /* Pack state. */
  UINT8_TO_BSTREAM(ptr, evt.state);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NODE_IDENTITY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                   netKeyIndex);

  if ((rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS) && (evt.state != MESH_NODE_IDENTITY_NOT_SUPPORTED))
  {
    /* Invoke callback on state changed. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Node Reset request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNodeReset(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  meshCfgMdlNodeResetStateEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_NODE_RESET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_NODE_RESET_NUM_BYTES)
  {
    return;
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NODE_RESET_STATUS, NULL, CFG_MDL_MSG_NODE_RESET_NUM_BYTES, src, ttl,
                      netKeyIndex);

  /* Set event parameters. */
  evt.cfgMdlHdr.peerAddress = src;

  /* Call notification callback. */
  meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);

  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Friend Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleFriendGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  meshFriendStates_t friendState;

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_FRIEND_GET_NUM_BYTES)
  {
    return;
  }

  /* Read from local config. */
  friendState =  MeshLocalCfgGetFriendState();

  /* Send Friend Status. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_FRIEND_STATUS, &friendState, sizeof(friendState), src, ttl,
                      netKeyIndex);
  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Friend Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleFriendSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t rspMsgParam[CFG_MDL_MSG_FRIEND_STATUS_NUM_BYTES];
  meshCfgMdlFriendEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_FRIEND_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  bool_t notifyUpperLayers = FALSE;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_FRIEND_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Get Friend state. */
  evt.friendState = pMsgParam[0];

  /* Validate Friend state values. */
  if (evt.friendState > MESH_FRIEND_FEATURE_NOT_SUPPORTED)
  {
    return;
  }

  /* Check if feature supported. */
  if (MeshLocalCfgGetFriendState() == MESH_FRIEND_FEATURE_NOT_SUPPORTED)
  {
    evt.friendState = MESH_FRIEND_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    /* Check if there is an actual change. */
    if(MeshLocalCfgGetFriendState() != evt.friendState)
    {
      notifyUpperLayers = TRUE;
    }
    /* Store Friend state. */
    MeshLocalCfgSetFriendState(evt.friendState);

    if (notifyUpperLayers)
    {
      /* Inform Hearbeat module that feature is changed. */
      MeshHbFeatureStateChanged(MESH_FEAT_FRIEND);
    }
  }

  /* Set response. */
  rspMsgParam[0] = evt.friendState;

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_FRIEND_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  if (notifyUpperLayers)
  {
    /* Inform Friendship module that feature is changed. */
    meshCfgMdlSrCb.friendStateChgCback();

    /* Set event parameters. */
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Heartbeat Publication Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleHbPubGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex)
{
  meshHbPub_t hbPubState;
  uint8_t rspMsgParam[CFG_MDL_MSG_HB_PUB_STATUS_NUM_BYTES];

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_HB_PUB_GET_NUM_BYTES)
  {
    return;
  }

  /* Read Heartbeat Publication local state. */

  /* Read destination. */
  hbPubState.dstAddr = MeshLocalCfgGetHbPubDst();

  /* Handle disabled publication (destination is unassigned, 4.4.1.2.15). */
  if (MESH_IS_ADDR_UNASSIGNED(hbPubState.dstAddr))
  {
    hbPubState.countLog = 0;
    hbPubState.periodLog = 0;
    hbPubState.ttl = 0;
  }
  else
  {
    hbPubState.countLog = MeshLocalCfgGetHbPubCountLog();
    hbPubState.periodLog = MeshLocalCfgGetHbPubPeriodLog();
    hbPubState.ttl = MeshLocalCfgGetHbPubTtl();
  }

  hbPubState.features = MeshLocalCfgGetHbPubFeatures();

  if (MeshLocalCfgGetHbPubNetKeyIndex(&hbPubState.netKeyIndex) == MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }
  else
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }

  /* Pack response. */
  meshCfgMsgPackHbPub(&rspMsgParam[1], &hbPubState);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_HB_PUB_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Heartbeat Publication Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleHbPubSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t rspMsgParam[CFG_MDL_MSG_HB_PUB_STATUS_NUM_BYTES];

  meshCfgMdlHbPubEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_HB_PUB_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_HB_PUB_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack states. */
  meshCfgMsgUnpackHbPub(pMsgParam, &evt.hbPub);

  /* Validate destination. */
  if (MESH_IS_ADDR_VIRTUAL(evt.hbPub.dstAddr))
  {
    return;
  }

  /* Read Key Refresh Phase to validate NetKeyIndex. */
  if(MeshLocalCfgGetKeyRefreshPhaseState(evt.hbPub.netKeyIndex)
     >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }
  else
  {
    /* Check destination to see if publication is disabled. */
    if (MESH_IS_ADDR_UNASSIGNED(evt.hbPub.dstAddr))
    {
      /* Reset countLog, periodLog and TTL. */
      evt.hbPub.countLog = 0;
      evt.hbPub.periodLog = 0;
      evt.hbPub.periodLog = 0;
    }

    /* Validate Heartbeat Publication data. */
    if (((evt.hbPub.countLog >= CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_START) &&
         (evt.hbPub.countLog <= CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_END)) ||
        (evt.hbPub.periodLog >= CFG_MDL_HB_PUB_PERIOD_LOG_NOT_ALLOW_START) ||
        (evt.hbPub.ttl >= CFG_MDL_HB_PUB_TTL_NOT_ALLOW_START))
    {
      return;
    }

    /* Clear RFU bits. */
    evt.hbPub.features &= (MESH_FEAT_RFU_START - 1);

    /* Set new states. */
    switch (MeshLocalCfgSetHbPubDst(evt.hbPub.dstAddr))
    {
      case MESH_LOCAL_CFG_ALREADY_EXIST:
      /* Fall-through. */
      case MESH_SUCCESS:
        rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
        MeshLocalCfgSetHbPubCountLog(evt.hbPub.countLog);
        MeshLocalCfgSetHbPubPeriodLog(evt.hbPub.periodLog);
        MeshLocalCfgSetHbPubTtl(evt.hbPub.ttl);
        MeshLocalCfgSetHbPubFeatures(evt.hbPub.features);
        MeshLocalCfgSetHbPubNetKeyIndex(evt.hbPub.netKeyIndex);
        break;
      case MESH_LOCAL_CFG_OUT_OF_MEMORY:
        rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
        break;
      default:
        rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
        break;
    }
  }

  /* Pack response. */
  meshCfgMsgPackHbPub(&rspMsgParam[1], &evt.hbPub);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_HB_PUB_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  /* Inform Heartbeat module and trigger callback on success. */
  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Notify Publication changed. */
    MeshHbPublicationStateChanged();

    /* Set event parameters. */
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Heartbeat Subscription Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleHbSubGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex)
{
  meshHbSub_t hbSubState;
  uint8_t rspMsgParam[CFG_MDL_MSG_HB_SUB_STATUS_NUM_BYTES];

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_HB_SUB_GET_NUM_BYTES)
  {
    return;
  }

  /* Read Heartbeat Subscription local state. */
  hbSubState.srcAddr = MeshLocalCfgGetHbSubSrc();
  hbSubState.dstAddr = MeshLocalCfgGetHbSubDst();

  /* Check if subscription is disabled. (4.4.1.2.16 Heartbeat Subscription state) */
  if(MESH_IS_ADDR_UNASSIGNED(hbSubState.srcAddr) ||
     MESH_IS_ADDR_UNASSIGNED(hbSubState.dstAddr))
  {
    hbSubState.periodLog = 0;
    hbSubState.countLog = 0;
    hbSubState.minHops = 0;
    hbSubState.maxHops = 0;
  }
  else
  {
    /* Read subscription parameters. */
    hbSubState.periodLog = MeshLocalCfgGetHbSubPeriodLog();
    hbSubState.countLog = MeshLocalCfgGetHbSubCountLog();
    hbSubState.minHops = MeshLocalCfgGetHbSubMinHops();
    hbSubState.maxHops = MeshLocalCfgGetHbSubMaxHops();
  }

  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Pack response. */
  meshCfgMsgPackHbSubState(&rspMsgParam[1], &hbSubState);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_HB_SUB_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Heartbeat Publication Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleHbSubSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t rspMsgParam[CFG_MDL_MSG_HB_SUB_STATUS_NUM_BYTES];
  meshAddress_t oldSrc, oldDst, elem0Addr;
  meshLocalCfgRetVal_t retVal;

  meshCfgMdlHbSubEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_HB_SUB_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_HB_SUB_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack states. */
  meshCfgMsgUnpackHbSubSet(pMsgParam, &evt.hbSub);

  /* Validate Heartbeat Subscription data. */
  if ((evt.hbSub.periodLog >= CFG_MDL_HB_SUB_PERIOD_LOG_NOT_ALLOW_START) ||
      MESH_IS_ADDR_VIRTUAL(evt.hbSub.srcAddr) ||
      MESH_IS_ADDR_GROUP(evt.hbSub.srcAddr) ||
      MESH_IS_ADDR_VIRTUAL(evt.hbSub.dstAddr))
  {
    return;
  }

  /* Check if unicast destination is primary element address. */
  if(MESH_IS_ADDR_UNICAST(evt.hbSub.dstAddr))
  {
    MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

    if(elem0Addr != evt.hbSub.dstAddr)
    {
      return;
    }
  }

  /* Check if conditions are met to disable subscription. */
  if(MESH_IS_ADDR_UNASSIGNED(evt.hbSub.srcAddr) ||
     MESH_IS_ADDR_UNASSIGNED(evt.hbSub.dstAddr))
  {
    evt.hbSub.srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
    evt.hbSub.dstAddr = MESH_ADDR_TYPE_UNASSIGNED;
    evt.hbSub.periodLog = 0;

    /* There should be no error in writing unassigned address. */
    (void)MeshLocalCfgSetHbSubSrc(evt.hbSub.srcAddr);
    (void)MeshLocalCfgSetHbSubDst(evt.hbSub.dstAddr);
    MeshLocalCfgSetHbSubPeriodLog(evt.hbSub.periodLog);

    /* Get states for Count Log, Min & Max Hops */
    evt.hbSub.countLog = MeshLocalCfgGetHbSubCountLog();
    evt.hbSub.minHops = MeshLocalCfgGetHbSubMinHops();
    evt.hbSub.maxHops = MeshLocalCfgGetHbSubMaxHops();

    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }
  else
  {
    /* Read old. */
    oldSrc = MeshLocalCfgGetHbSubSrc();
    /* Read old. */
    oldDst = MeshLocalCfgGetHbSubDst();

    /* Set new states. */
    retVal =  MeshLocalCfgSetHbSubSrc(evt.hbSub.srcAddr);

    if((retVal != MESH_SUCCESS) && (retVal != MESH_LOCAL_CFG_ALREADY_EXIST))
    {
      rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
    }
    else
    {
      retVal = MeshLocalCfgSetHbSubDst(evt.hbSub.dstAddr);
      if((retVal != MESH_SUCCESS) && (retVal != MESH_LOCAL_CFG_ALREADY_EXIST))
      {
        /* Restore source and set error code. */
        (void)MeshLocalCfgSetHbSubSrc(oldSrc);
        rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
      }
      else
      {
        /* Set Period Log. */
        MeshLocalCfgSetHbSubPeriodLog(evt.hbSub.periodLog);

        /* Decide if subscription information must also be reset. */
        if((evt.hbSub.periodLog != 0) || (oldSrc != evt.hbSub.srcAddr) || (oldDst != evt.hbSub.dstAddr))
        {
          /* Set CountLog, Min & Max Hops to their initial values. */
          MeshLocalCfgSetHbSubCountLog(0);
          MeshLocalCfgSetHbSubMinHops(CFG_MDL_HB_SUB_MIN_HOPS_NOT_ALLOW_START - 1);
          MeshLocalCfgSetHbSubMaxHops(0);
          /* Set values for Count Log, Min & Max Hops */
          evt.hbSub.countLog = 0;
          evt.hbSub.minHops = CFG_MDL_HB_SUB_MIN_HOPS_NOT_ALLOW_START - 1;
          evt.hbSub.maxHops = 0;
        }
        else
        {
          /* Get states for Count Log, Min & Max Hops */
          evt.hbSub.countLog = MeshLocalCfgGetHbSubCountLog();
          evt.hbSub.minHops = MeshLocalCfgGetHbSubMinHops();
          evt.hbSub.maxHops = MeshLocalCfgGetHbSubMaxHops();
        }

        /* Set status to success. */
        rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
      }
    }
  }

  /* Pack state to build status message. */
  meshCfgMsgPackHbSubState(&rspMsgParam[1], &evt.hbSub);

  /* Response contains the same packed states as set message. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_HB_SUB_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl, netKeyIndex);

  /* On success, notify Hearbeat module and invoke callback. */
  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Notify Subscription changed. */
    MeshHbSubscriptionStateChanged();

    /* Set event parameters. */
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Low Power Node PollTimeout Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleLpnPollTimeoutGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex)
{
  meshAddress_t lpnAddr;
  uint32_t timeout = 0;
  uint8_t rspMsgParam[CFG_MDL_MSG_LPN_POLLTIMEOUT_STATUS_NUM_BYTES];

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_LPN_POLLTIMEOUT_GET_NUM_BYTES)
  {
    return;
  }

  /* Unpack address. */
  BSTREAM_TO_UINT16(lpnAddr, pMsgParam);

  /* Validate address. */
  if (!MESH_IS_ADDR_UNICAST(lpnAddr))
  {
    return;
  }

  if (MeshLocalCfgGetFriendState() != MESH_FRIEND_FEATURE_ENABLED)
  {
    timeout = 0;
  }
  else
  {
    /* Read Poll Timeout. */
    timeout = meshCfgMdlSrCb.pollTimeoutGetCback(lpnAddr);
  }

  /* Pack state. */
  meshCfgMsgPackLpnPollTimeout(rspMsgParam, lpnAddr, timeout);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_LPN_PT_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Network Transmit Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNwkTransGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                   uint8_t ttl, uint16_t netKeyIndex)
{
  meshNwkTransState_t state;
  uint8_t rspMsgParam[CFG_MDL_MSG_NWK_TRANS_STATE_NUM_BYTES] = { 0 };

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_NWK_TRANS_GET_NUM_BYTES)
  {
    return;
  }

  /* Read Network Transmit state. */
  state.transCount = MeshLocalCfgGetNwkTransmitCount();
  state.transIntervalSteps10Ms = MeshLocalCfgGetNwkTransmitIntvlSteps();

  /* Pack state. */
  meshCfgMsgPackNwkTrans(rspMsgParam, &state);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NWK_TRANS_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  (void)pMsgParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Network Transmit Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNwkTransSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                   uint8_t ttl, uint16_t netKeyIndex)
{
  meshCfgMdlNwkTransStateEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_NWK_TRANS_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_NWK_TRANS_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack state. */
  meshCfgMsgUnpackNwkTrans(pMsgParam, &evt.nwkTransState);

  /* Store new values. */
  MeshLocalCfgSetNwkTransmitCount(evt.nwkTransState.transCount);
  MeshLocalCfgSetNwkTransmitIntvlSteps(evt.nwkTransState.transIntervalSteps10Ms);

  /* Response contains state exactly as set message. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NWK_TRANS_STATUS, pMsgParam, msgParamLen, src, ttl, netKeyIndex);

  /* Set event parameters. */
  evt.cfgMdlHdr.peerAddress = src;

  /* Call notification callback. */
  meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
}
