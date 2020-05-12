/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_api.h
 *
 *  \brief
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

/*! ***********************************************************************************************
 *
 * @addtogroup MESH_CONFIGURATION_MODELS Mesh Configuration Models
 * @{
 *************************************************************************************************/

#ifndef MESH_CFG_MDL_API_H
#define MESH_CFG_MDL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Table 4.108. Status code messages for messages that contain a Status parameter. */
enum meshCfgMdlOtaErrCodes
{
  MESH_CFG_MDL_ERR_INVALID_ADDR             = 0x01, /*!< Invalid Address */
  MESH_CFG_MDL_ERR_INVALID_MODEL            = 0x02, /*!< Invalid Model */
  MESH_CFG_MDL_ERR_INVALID_APPKEY_INDEX     = 0x03, /*!< Invalid AppKey Index */
  MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX     = 0x04, /*!< Invalid NetKey Index */
  MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES   = 0x05, /*!< Insufficient Resources */
  MESH_CFG_MDL_ERR_KEY_INDEX_EXISTS         = 0x06, /*!< Key Index Already Stored */
  MESH_CFG_MDL_ERR_INVALID_PUB_PARAMS       = 0x07, /*!< Invalid Publish Parameters*/
  MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL      = 0x08, /*!< Not a Subscribe Model */
  MESH_CFG_MDL_ERR_STORAGE_FAILURE          = 0x09, /*!< Storage Failure */
  MESH_CFG_MDL_ERR_FEATURE_NOT_SUPPORTED    = 0x0A, /*!< Feature Not Supported */
  MESH_CFG_MDL_ERR_CANNOT_UPDATE            = 0x0B, /*!< Cannot Update */
  MESH_CFG_MDL_ERR_CANNOT_REMOVE            = 0x0C, /*!< Cannot Update */
  MESH_CFG_MDL_ERR_CANNOT_BIND              = 0x0D, /*!< Cannot Bind */
  MESH_CFG_MDL_ERR_TEMP_UNABLE_TO_CHG_STATE = 0x0E, /*!< Temporarily Unable to Change State */
  MESH_CFG_MDL_ERR_CANNOT_SET               = 0x0F, /*!< Cannot Set */
  MESH_CFG_MDL_ERR_UNSPECIFIED              = 0x10, /*!< Unspecified Error */
  MESH_CFG_MDL_ERR_INVALID_BINDING          = 0x11, /*!< Invalid Binding */
  MESH_CFG_MDL_ERR_RFU_START                = 0x12, /*!< RFU */
  MESH_CFG_MDL_ERR_RFU_END                  = 0xFF, /*!< RFU */
};

/*! Enumeration of the configuration model events triggered by the configuration model API.
 *  The list is shared by both the Configuration Client and the Configuration Server.
 *
 *  \remarks Each event in the list is notified at the end of the corresponding operation of the
 *           Configuration Client.
 *  \remarks The Configuration Server can optionally notify events that modify states. This means
 *           that "get" and "list" events are never notified by a Configuration Server.
 */
enum meshCfgMdlEventValues
{
  MESH_CFG_MDL_BEACON_GET_EVENT = 0x00,       /*!< Config Model Beacon Get operation complete */
  MESH_CFG_MDL_BEACON_SET_EVENT,              /*!< Config Model Beacon Set operation complete */
  MESH_CFG_MDL_COMP_PAGE_GET_EVENT,           /*!< Config Model Composition Page Get operation complete */
  MESH_CFG_MDL_DEFAULT_TTL_GET_EVENT,         /*!< Config Model Default TTL Get operation complete */
  MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT,         /*!< Config Model Default TTL Set operation complete */
  MESH_CFG_MDL_GATT_PROXY_GET_EVENT,          /*!< Config Model Gatt Proxy Get operation complete */
  MESH_CFG_MDL_GATT_PROXY_SET_EVENT,          /*!< Config Model Gatt Proxy Set operation complete */
  MESH_CFG_MDL_RELAY_GET_EVENT,               /*!< Config Model Relay Get operation complete */
  MESH_CFG_MDL_RELAY_SET_EVENT,               /*!< Config Model Relay Set operation complete */
  MESH_CFG_MDL_PUB_GET_EVENT,                 /*!< Config Model Model Publication Get complete */
  MESH_CFG_MDL_PUB_SET_EVENT,                 /*!< Config Model Model Publication Set complete */
  MESH_CFG_MDL_PUB_VIRT_SET_EVENT,            /*!< Config Model Model Publication Virtual Set complete */
  MESH_CFG_MDL_SUBSCR_ADD_EVENT,              /*!< Config Model Subscription Add. */
  MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT,         /*!< Config Model Subscription Virtual Address Add. */
  MESH_CFG_MDL_SUBSCR_DEL_EVENT,              /*!< Config Model Subscription Delete. */
  MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT,         /*!< Config Model Subscription Virtual Address Delete. */
  MESH_CFG_MDL_SUBSCR_OVR_EVENT,              /*!< Config Model Subscription Overwrite. */
  MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT,         /*!< Config Model Subscription Virtual Address Overwrite. */
  MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT,          /*!< Config Model Subscription Delete All. */
  MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT,          /*!< Config Model SIG Model Subscription List Get. */
  MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT,       /*!< Config Model Vendor Model Subscription List Get. */
  MESH_CFG_MDL_NETKEY_ADD_EVENT,              /*!< Config Model NetKey Add operation complete */
  MESH_CFG_MDL_NETKEY_UPDT_EVENT,             /*!< Config Model NetKey Update operation complete */
  MESH_CFG_MDL_NETKEY_DEL_EVENT,              /*!< Config Model NetKey Delete operation complete */
  MESH_CFG_MDL_NETKEY_GET_EVENT,              /*!< Config Model NetKey Get operation complete */
  MESH_CFG_MDL_APPKEY_ADD_EVENT,              /*!< Config Model AppKey Add operation complete */
  MESH_CFG_MDL_APPKEY_UPDT_EVENT,             /*!< Config Model AppKey Update operation complete */
  MESH_CFG_MDL_APPKEY_DEL_EVENT,              /*!< Config Model AppKey Delete operation complete */
  MESH_CFG_MDL_APPKEY_GET_EVENT,              /*!< Config Model AppKey Get operation complete */
  MESH_CFG_MDL_NODE_IDENTITY_GET_EVENT,       /*!< Config Model Node Identity Get operation complete */
  MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT,       /*!< Config Model Node Identity Set operation complete */
  MESH_CFG_MDL_APP_BIND_EVENT,                /*!< Config Model App Bind operation complete */
  MESH_CFG_MDL_APP_UNBIND_EVENT,              /*!< Config Model App Unbind operation complete */
  MESH_CFG_MDL_APP_SIG_GET_EVENT,             /*!< Config Model SIG Model App Get operation complete */
  MESH_CFG_MDL_APP_VENDOR_GET_EVENT,          /*!< Config Model Vendor Model App Get operation complete */
  MESH_CFG_MDL_NODE_RESET_EVENT,              /*!< Config Model Node Reset operation complete */
  MESH_CFG_MDL_FRIEND_GET_EVENT,              /*!< Config Model Friend Get operation complete */
  MESH_CFG_MDL_FRIEND_SET_EVENT,              /*!< Config Model Friend Set operation complete */
  MESH_CFG_MDL_KEY_REF_PHASE_GET_EVENT,       /*!< Config Model Key Refresh Phase Get operation complete */
  MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT,       /*!< Config Model Key Refresh Phase Set operation complete */
  MESH_CFG_MDL_HB_PUB_GET_EVENT,              /*!< Config Model Heartbeat Publication Get */
  MESH_CFG_MDL_HB_PUB_SET_EVENT,              /*!< Config Model Heartbeat Publication Set */
  MESH_CFG_MDL_HB_SUB_GET_EVENT,              /*!< Config Model Heartbeat Subscription Get */
  MESH_CFG_MDL_HB_SUB_SET_EVENT,              /*!< Config Model Heartbeat Subscription Set */
  MESH_CFG_MDL_LPN_POLLTIMEOUT_GET_EVENT,     /*!< Config Model Low Power Node Poll Timeout Get */
  MESH_CFG_MDL_NWK_TRANS_GET_EVENT,           /*!< Config Model Network Transmit Get operation complete */
  MESH_CFG_MDL_NWK_TRANS_SET_EVENT,           /*!< Config Model Network Transmit Set operation complete */
  MESH_CFG_MDL_MAX_EVENT                      /*!< Maximum number of Config Model events */
};

/*! Configuration model callback event type (used in wsfMsgHdr_t.event).
 *  See ::meshCfgMdlEventValues
 */
typedef uint8_t meshCfgMdlEventType_t;

/*! \brief Configuration Model header event */
typedef struct meshCfgMdlHdr_tag
{
  wsfMsgHdr_t                      hdr;              /*!< Header structure */
  meshAddress_t                    peerAddress;      /*!< Address of the peer node */
} meshCfgMdlHdr_t;

/*! \brief Beacon state event. */
typedef struct meshCfgMdlBeaconStateEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshBeaconStates_t               state;            /*!< Beacon state */
} meshCfgMdlBeaconStateEvt_t;

/*! \brief Key binding information event */
typedef struct meshCfgMdlCompDataEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshCompData_t                   data;             /*!< Composition data */
} meshCfgMdlCompDataEvt_t;

/*! \brief Default TTL state event. */
typedef struct meshCfgMdlDefaultTtlStateEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  uint8_t                          ttl;              /*!< Default TTL state */
} meshCfgMdlDefaultTtlStateEvt_t;

/*! \brief Key binding information event */
typedef struct meshCfgMdlGattProxyEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshGattProxyStates_t            gattProxy;        /*!< Gatt Proxy state */
} meshCfgMdlGattProxyEvt_t;

/*! \brief Relay composite state event containing Relay state and Relay Retransmit Count state */
typedef struct meshCfgMdlRelayCompositeStateEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshRelayStates_t                relayState;       /*!< Relay feature state */
  meshRelayRetransState_t          relayRetrans;     /*!< Relay retransmit state */
} meshCfgMdlRelayCompositeStateEvt_t;

/*! \brief Model Publication state event for both virtual and non-virtual addresses. */
typedef struct meshCfgMdlModelPubEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshAddress_t                    elemAddr;         /*!< Address of the element containing the model */
  meshAddress_t                    pubAddr;          /*!< Publish address for the model instance */
  meshModelPublicationParams_t     pubParams;        /*!< Publication parameters */
  modelId_t                        modelId;          /*!< Model identifier */
  bool_t                           isSig;            /*!< TRUE if model identifier is SIG, FALSE for
                                                      *   vendor
                                                      */
} meshCfgMdlModelPubEvt_t;

/*! \brief Model Subscription List changed event for both virtual and non-virtual addresses. */
typedef struct meshCfgMdlModelSubscrChgEvt_t
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshAddress_t                    elemAddr;         /*!< Address of the element containing the model */
  meshAddress_t                    subscrAddr;       /*!< Subscription address for the model instance */
  modelId_t                        modelId;          /*!< Model identifier */
  bool_t                           isSig;            /*!< TRUE if model identifier is SIG, FALSE for
                                                      *   vendor
                                                      */
} meshCfgMdlModelSubscrChgEvt_t;

/*! \brief Model Subscription List event. */
typedef struct meshCfgMdlModelSubscrListEvt_t
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshAddress_t                    elemAddr;         /*!< Address of the element containing the model */
  modelId_t                        modelId;          /*!< Model identifier */
  bool_t                           isSig;            /*!< TRUE if model identifier is SIG, FALSE for
                                                      *   vendor
                                                      */
  meshAddress_t                    *pSubscrList;     /*!< Pointer to the model Subscription List. */
  uint8_t                          subscrListSize;   /*!< Subscription List size. */
} meshCfgMdlModelSubscrListEvt_t;

/*! \brief NetKey changed information event */
typedef struct meshCfgMdlNetKeyChgEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  uint16_t                         netKeyIndex;      /*!< NetKey Index */
} meshCfgMdlNetKeyChgEvt_t;

/*! \brief NetKeyList information event */
typedef struct meshCfgMdlNetKeyListEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshNetKeyList_t                 netKeyList;       /*!< NetKey list */
} meshCfgMdlNetKeyListEvt_t;

/*! \brief AppKey changed information event */
typedef struct meshCfgMdlAppKeyChgEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshAppNetKeyBind_t              bind;             /*!< Key binding */
} meshCfgMdlAppKeyChgEvt_t;

/*! \brief AppKey List information event */
typedef struct meshCfgMdlAppKeyListEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshAppKeyList_t                 appKeyList;       /*!< AppKey list */
} meshCfgMdlAppKeyListEvt_t;

/*! \brief Node Identity information event. */
typedef struct meshCfgMdlNodeIdentityEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  uint16_t                         netKeyIndex;      /*!< NetKey Index. */
  meshNodeIdentityStates_t         state;            /*!< Node identity state. */
} meshCfgMdlNodeIdentityEvt_t;

/*! \brief Model to AppKey Bind information event. */
typedef struct meshCfgMdlModelAppBindEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  uint16_t                         appKeyIndex;      /*!< AppKey Index bound to model */
  meshAddress_t                    elemAddr;         /*!< Address of the element containing the model */
  modelId_t                        modelId;          /*!< Model identifier */
  bool_t                           isSig;            /*!< TRUE if model identifier is SIG, FALSE for
                                                      *   vendor
                                                      */
} meshCfgMdlModelAppBindEvt_t;

/*! \brief Model to AppKey List information event. */
typedef struct meshCfgMdlModelAppListEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshModelAppList_t               modelAppList;     /*!< Model AppKey List. */
} meshCfgMdlModelAppListEvt_t;

/*! \brief Node Reset state event */
typedef struct meshCfgMdlNodeResetStateEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
} meshCfgMdlNodeResetStateEvt_t;

/*! \brief Friend state event */
typedef struct meshCfgMdlFriendEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshFriendStates_t               friendState;      /*!< Friend state */
} meshCfgMdlFriendEvt_t;

/*! \brief Key binding information event */
typedef struct meshCfgMdlKeyRefPhaseEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  uint16_t                         netKeyIndex;      /*!< Global identifier of the Network Key */
  meshKeyRefreshStates_t           keyRefState;      /*!< Key Refresh Phase state */
} meshCfgMdlKeyRefPhaseEvt_t;

/*! \brief Heartbeat Publication state event. */
typedef struct meshCfgMdlHbPubEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshHbPub_t                      hbPub;            /*!< Heartbeat Publication state */
} meshCfgMdlHbPubEvt_t;

/*! \brief Heartbeat Subscription state event. */
typedef struct meshCfgMdlHbSubEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshHbSub_t                      hbSub;            /*!< Heartbeat Subscription state */
} meshCfgMdlHbSubEvt_t;

/*! \brief PollTimeout information event. */
typedef struct meshCfgMdlLpnPollTimeoutEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  uint16_t                         lpnAddr;          /*!< Primary element address of the Low Power
                                                      *   node
                                                      */
  uint32_t                         pollTimeout100Ms; /*!< PollTimeout timer value in units of 100 ms
                                                      */
} meshCfgMdlLpnPollTimeoutEvt_t;

/*! \brief Network Transmit state event. */
typedef struct meshCfgMdlNwkTransStateEvt_tag
{
  meshCfgMdlHdr_t                  cfgMdlHdr;        /*!< Header structure */
  meshNwkTransState_t              nwkTransState;    /*!< Network Transmit state */
} meshCfgMdlNwkTransStateEvt_t;

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Configuration Model callback event.
 *
 *  \param[in] pMeshCfgEvt  Mesh Configuration Model callback event.
 *
 *  \return    Size of Mesh Configuration Model callback event.
 */
/*************************************************************************************************/
uint16_t MeshCfgSizeOfEvt(wsfMsgHdr_t *pMeshCfgEvt);

/*************************************************************************************************/
/*!
 *  \brief      Make a deep copy of a Configuration Message.
 *
 *  \param[Out] pMeshCfgEvtOut Local copy of Mesh Configuration Model event.
 *  \param[in]  pMeshCfgEvtIn  Mesh Configuration Model callback event.
 *
 *  \return     TRUE if successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshCfgMsgDeepCopy(wsfMsgHdr_t *pMeshCfgEvtOut, const wsfMsgHdr_t *pMeshCfgEvtIn);

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
