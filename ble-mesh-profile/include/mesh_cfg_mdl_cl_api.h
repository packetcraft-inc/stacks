/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_cl_api.h
 *
 *  \brief  Configuration Client API.
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
 * @addtogroup MESH_CONFIGURATION_MODELS
 * @{
 *************************************************************************************************/
#ifndef MESH_CFG_MDL_CL_API_H
#define MESH_CFG_MDL_CL_API_H

#include "mesh_cfg_mdl_api.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Mesh Configuration Client API address used for modifying local states */
#define MESH_CFG_MDL_CL_LOCAL_NODE_SR         MESH_ADDR_TYPE_UNASSIGNED

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Configuration Client event status. Applicable to all event types */
enum meshCfgMdlClEventStatusValues
{
  MESH_CFG_MDL_CL_SUCCESS           = 0x00,  /*!< Operation completed without errors */
  MESH_CFG_MDL_CL_OUT_OF_RESOURCES  = 0x01,  /*!< Client has no resources to perform the procedure */
  MESH_CFG_MDL_CL_INVALID_PARAMS    = 0x02,  /*!< Parameters passed to the API are not valid */
  MESH_CFG_MDL_CL_TIMEOUT           = 0x03,  /*!< No response received from the Configuration Server */
  MESH_CFG_MDL_CL_UNKOWN_ERROR      = 0x04,  /*!< Unknown error */
  MESH_CFG_MDL_CL_REMOTE_ERROR_BASE = 0x05   /*!< Start of procedure-specific codes for errors received
                                              *   from Server. Error codes above this value are obtained
                                              *   by subtracting base from the error code and comparing
                                              *   to ::meshCfgMdlOtaErrCodes.
                                              */
};

/*! \brief Operations that can be performed over the Subscription List of a model */
enum meshCfgMdlClSubscrAddrOpValues
{
  MESH_CFG_MDL_CL_SUBSCR_ADDR_ADD = 0, /*!< Subscription Address Add. Generates
                                        *   ::MESH_CFG_MDL_SUBSCR_ADD_EVENT or
                                        *   ::MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT event
                                        */
  MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL,     /*!< Subscription Address Delete. Generates
                                        *   ::MESH_CFG_MDL_SUBSCR_DEL_EVENT or
                                        *   ::MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT event
                                        */
  MESH_CFG_MDL_CL_SUBSCR_ADDR_OVR,     /*!< Subscription Address Overwrite. Generates
                                        *   ::MESH_CFG_MDL_SUBSCR_OVR_EVENT or
                                        *   ::MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT event
                                        */
  MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL_ALL, /*!< Subscription Address Delete All. Generates
                                        *   ::MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT event
                                        */
};

/*! \brief Configuration Client Subscription Address operations. See ::meshCfgMdlClSubscrAddrOpValues */
typedef uint8_t meshCfgMdlClSubscrAddrOp_t;

/*! \brief Operations that can be performed over an AppKey or a NetKey */
enum meshCfgMdlClKeyOpValues
{
  MESH_CFG_MDL_CL_KEY_ADD = 0,  /*!< AppKey or NetKey Add */
  MESH_CFG_MDL_CL_KEY_UPDT,     /*!< AppKey or NetKey Update */
  MESH_CFG_MDL_CL_KEY_DEL,      /*!< AppKey or NetKey Delete */
  MESH_CFG_MDL_CL_KEY_UNDEFINED /*!< Undefined Op Value */
};

/*! \brief Configuration Client AppKey or NetKey operations. See ::meshCfgMdlClKeyOpValues */
typedef uint8_t meshCfgMdlClKeyOp_t;

/*! Configuration Client event
 *
 *  \attention If the operation status (hdr.status) is not success or an OTA error code,
 *             then only the hdr and cfgMdlHdr structure of any event contains valid information.
 */
typedef union meshCfgMdlClEvt_tag
{
  wsfMsgHdr_t                        hdr;             /*!< Header structure. This is used for all
                                                       *   events in case the status field is
                                                       *   ::MESH_CFG_MDL_CL_OUT_OF_RESOURCES or
                                                       *   ::MESH_CFG_MDL_CL_INVALID_PARAMS or
                                                       *   ::MESH_CFG_MDL_CL_TIMEOUT or
                                                       *   ::MESH_CFG_MDL_CL_UNKOWN_ERROR.
                                                       *   For these error codes, the param field
                                                       *   contains the server address
                                                       */
  meshCfgMdlHdr_t                    cfgMdlHdr;       /*!< Header structure extension for
                                                       *   Configuration Model events.
                                                       */
  meshCfgMdlBeaconStateEvt_t         beacon;          /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_BEACON_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_BEACON_SET_EVENT or
                                                       */
  meshCfgMdlCompDataEvt_t            compData;        /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_COMP_PAGE_GET_EVENT
                                                       */
  meshCfgMdlDefaultTtlStateEvt_t     defaultTtl;      /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_DEFAULT_TTL_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT or
                                                       */
  meshCfgMdlGattProxyEvt_t           gattProxy;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_GATT_PROXY_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_GATT_PROXY_SET_EVENT
                                                       */
  meshCfgMdlRelayCompositeStateEvt_t relayComposite;  /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_RELAY_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_RELAY_SET_EVENT
                                                       */
  meshCfgMdlModelPubEvt_t            modelPub;        /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_PUB_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_PUB_SET_EVENT or
                                                       *   ::MESH_CFG_MDL_PUB_VIRT_SET_EVENT
                                                       */
  meshCfgMdlModelSubscrChgEvt_t      subscrChg;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_SUBSCR_ADD_EVENT or
                                                       *   ::MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT or
                                                       *   ::MESH_CFG_MDL_SUBSCR_DEL_EVENT or
                                                       *   ::MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT or
                                                       *   ::MESH_CFG_MDL_SUBSCR_OVR_EVENT or
                                                       *   ::MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT or
                                                       *   ::MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT
                                                       */
  meshCfgMdlModelSubscrListEvt_t     subscrList;      /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT
                                                       */
  meshCfgMdlNetKeyChgEvt_t           netKeyChg;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NETKEY_ADD_EVENT or
                                                       *   ::MESH_CFG_MDL_NETKEY_DEL_EVENT or
                                                       *   ::MESH_CFG_MDL_NETKEY_UPDT_EVENT
                                                       */
  meshCfgMdlNetKeyListEvt_t          netKeyList;      /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NETKEY_GET_EVENT
                                                       */
  meshCfgMdlAppKeyChgEvt_t           appKeyChg;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_APPKEY_ADD_EVENT or
                                                       *   ::MESH_CFG_MDL_APPKEY_DEL_EVENT or
                                                       *   ::MESH_CFG_MDL_APPKEY_UPDT_EVENT
                                                       */
  meshCfgMdlAppKeyListEvt_t          appKeyList;      /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_APPKEY_GET_EVENT
                                                       */
  meshCfgMdlNodeIdentityEvt_t        nodeIdentity;    /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NODE_IDENTITY_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT
                                                       */
  meshCfgMdlModelAppBindEvt_t        modelAppBind;    /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_APP_BIND_EVENT or
                                                       *   ::MESH_CFG_MDL_APP_UNBIND_EVENT
                                                       */
  meshCfgMdlModelAppListEvt_t        modelAppList;    /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_APP_SIG_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_APP_VENDOR_GET_EVENT
                                                       */
  meshCfgMdlNodeResetStateEvt_t      nodeReset;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NODE_RESET_EVENT
                                                       */
  meshCfgMdlFriendEvt_t              friendState;     /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_FRIEND_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_FRIEND_SET_EVENT
                                                       */
  meshCfgMdlKeyRefPhaseEvt_t         keyRefPhase;     /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_KEY_REF_PHASE_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT
                                                       */
  meshCfgMdlHbPubEvt_t               hbPub;           /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_HB_PUB_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_HB_PUB_SET_EVENT
                                                       */
  meshCfgMdlHbSubEvt_t               hbSub;           /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_HB_SUB_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_HB_SUB_SET_EVENT
                                                       */
  meshCfgMdlLpnPollTimeoutEvt_t      pollTimeout;     /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_LPN_POLLTIMEOUT_GET_EVENT
                                                       */
  meshCfgMdlNwkTransStateEvt_t       nwkTrans;        /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NWK_TRANS_GET_EVENT or
                                                       *   ::MESH_CFG_MDL_NWK_TRANS_SET_EVENT
                                                       */
} meshCfgMdlClEvt_t;

/*************************************************************************************************/
/*!
 *  \brief     Callback for informing the upper layer that a requested operation has completed.
 *
 *  \param[in] pEvt  Pointer to Configuration Client event.
 *
 *  \return    None.
 *
 *  \see meshCfgMdlClEvt_t
 */
/*************************************************************************************************/
typedef void (*meshCfgMdlClCback_t)(meshCfgMdlClEvt_t* pEvt);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Gets memory required for configuration.
 *
 *  \return Configuration memory required or ::MESH_MEM_REQ_INVALID_CFG on error.
 */
/*************************************************************************************************/
uint32_t MeshCfgMdlClGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Configuration Client.
 *
 *  \param[in] pFreeMem     Pointer to free memory.
 *  \param[in] freeMemSize  Size of pFreeMem.
 *
 *  \return    Amount of free memory consumed.
 */
/*************************************************************************************************/
uint32_t MeshCfgMdlClInit(uint8_t *pFreeMem, uint32_t freeMemSize);

/*************************************************************************************************/
/*!
 *  \brief     Installs the Configuration Client callback.
 *
 *  \param[in] meshCfgMdlClCback  Upper layer callback to notify operation results.
 *  \param[in] timeoutSeconds  Timeout for configuration operations, in seconds.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClRegister(meshCfgMdlClCback_t meshCfgMdlClCback, uint16_t timeoutSeconds);

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
                           const uint8_t *pCfgMdlSrDevKey);

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
                           const uint8_t *pCfgMdlSrDevKey, meshBeaconStates_t beaconState);

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
                             const uint8_t *pCfgMdlSrDevKey, uint8_t pageNumber);

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
                               const uint8_t *pCfgMdlSrDevKey);

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
                               const uint8_t *pCfgMdlSrDevKey, uint8_t ttl);

/*************************************************************************************************/
/*!
 *  \brief     Gets the Gatt Proxy state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server
 *                               model.
 *  \param[in] cfgMdlSrNetKeyIndex  Global identifier of the Network on which the request is sent.
 *  \param[in] pCfgMdlSrDevKey      Pointer to the Device Key of the remote Configuration Server
 *                               or NULL for local (cfgMdlSrAddr is MESH_CFG_MDL_CL_LOCAL_NODE_SR).
 *
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_GATT_PROXY_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClGattProxyGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                              const uint8_t *pCfgMdlSrDevKey);

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
                              const uint8_t *pCfgMdlSrDevKey, meshGattProxyStates_t gattProxyState);

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
                          const uint8_t *pCfgMdlSrDevKey);

/*************************************************************************************************/
/*!
 *  \brief     Sets Relay and Relay Retransmit states.
 *
 *  \param[in] cfgMdlSrAddr           Primary element containing an instance of Configuration
 *                                    Server model.
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
                          meshRelayRetransState_t *pRelayRetransState);

/*************************************************************************************************/
/*!
 *  \brief     Gets the publish address and parameters of an outgoing message that originates from
 *             a model instance.
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
                        bool_t isSig);

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
                        meshSigModelId_t sigModelId, meshVendorModelId_t vendorModelId, bool_t isSig);

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
                               meshVendorModelId_t vendorModelId, bool_t isSig);

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
                               meshVendorModelId_t vendorModelId, bool_t isSig);

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
                           const uint8_t *pCfgMdlSrDevKey, uint16_t netKeyIndex,
                           meshCfgMdlClKeyOp_t keyOp, const uint8_t *pNetKey);

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
                           uint16_t cfgMdlSrNetKeyIndex);

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
                           meshCfgMdlClKeyOp_t keyOp, const uint8_t *pAppKey);

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
                           uint16_t cfgMdlSrNetKeyIndex, uint16_t netKeyIndex);

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
                                 uint16_t cfgMdlSrNetKeyIndex, uint16_t netKeyIndex);

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
                                 meshNodeIdentityStates_t nodeIdentityState);

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
                         meshVendorModelId_t vendorModelId, bool_t isSig);

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
                        bool_t isSig);

/*************************************************************************************************/
/*!
 *  \brief     Reset Node state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server model.
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
                           const uint8_t *pCfgMdlSrDevKey);

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
                           const uint8_t *pCfgMdlSrDevKey);

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
                           const uint8_t *pCfgMdlSrDevKey, meshFriendStates_t friendState);

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
                                const uint8_t *pCfgMdlSrDevKey, uint16_t netKeyIndex);

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
                                meshKeyRefreshTrans_t transition);

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
                          const uint8_t *pCfgMdlSrDevKey);

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
                          const uint8_t *pCfgMdlSrDevKey, meshHbPub_t *pHbPubState);

/*************************************************************************************************/
/*!
 *  \brief     Gets a Heartbeat Subscription state.
 *
 *  \param[in] cfgMdlSrAddr         Primary element containing an instance of Configuration Server model.
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
                          const uint8_t *pCfgMdlSrDevKey);

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
                          const uint8_t *pCfgMdlSrDevKey, meshHbSub_t *pHbSubState);

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
                                const uint8_t *pCfgMdlSrDevKey, meshAddress_t lpnAddr);

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
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_NWK_TRANS_GET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNwkTransmitGet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                                const uint8_t *pCfgMdlSrDevKey);

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
 *  \see meshCfgMdlClEvt_t, MESH_CFG_MDL_NWK_TRANS_SET_EVENT
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlClNwkTransmitSet(meshAddress_t cfgMdlSrAddr, uint16_t cfgMdlSrNetKeyIndex,
                                const uint8_t *pCfgMdlSrDevKey, meshNwkTransState_t *pNwkTransmit);

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
