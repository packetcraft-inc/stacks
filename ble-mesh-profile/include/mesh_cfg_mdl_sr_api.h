/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_sr_api.h
 *
 *  \brief  Configuration Server API.
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

#ifndef MESH_CFG_MDL_SR_API_H
#define MESH_CFG_MDL_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mesh_cfg_mdl_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Configuration Server event */
typedef union meshCfgMdlSrEvt_tag
{
  wsfMsgHdr_t                        hdr;             /*!< Header structure */
  meshCfgMdlHdr_t                    cfgMdlHdr;       /*!< Header structure extension for
                                                       *   Configuration Model events.
                                                       */
  meshCfgMdlBeaconStateEvt_t         beacon;          /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_BEACON_SET_EVENT
                                                       */
  meshCfgMdlDefaultTtlStateEvt_t     defaultTtl;      /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT
                                                       */
  meshCfgMdlGattProxyEvt_t           gattProxy;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_GATT_PROXY_SET_EVENT
                                                       */
  meshCfgMdlRelayCompositeStateEvt_t relayComposite;  /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_RELAY_SET_EVENT
                                                       */
  meshCfgMdlModelPubEvt_t            modelPub;        /*!< Valid if event is
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
  meshCfgMdlNetKeyChgEvt_t           netKeyChg;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NETKEY_ADD_EVENT or
                                                       *   ::MESH_CFG_MDL_NETKEY_DEL_EVENT or
                                                       *   ::MESH_CFG_MDL_NETKEY_UPDT_EVENT
                                                       */
  meshCfgMdlAppKeyChgEvt_t           appKeyChg;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_APPKEY_ADD_EVENT or
                                                       *   ::MESH_CFG_MDL_APPKEY_DEL_EVENT or
                                                       *   ::MESH_CFG_MDL_APPKEY_UPDT_EVENT
                                                       */
  meshCfgMdlNodeIdentityEvt_t        nodeIdentity;    /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT
                                                       */
  meshCfgMdlModelAppBindEvt_t        modelAppBind;    /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_APP_BIND_EVENT or
                                                       *   ::MESH_CFG_MDL_APP_UNBIND_EVENT
                                                       */
  meshCfgMdlNodeResetStateEvt_t      nodeReset;       /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NODE_RESET_EVENT
                                                       */
  meshCfgMdlFriendEvt_t              friendState;     /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_FRIEND_SET_EVENT
                                                       */
  meshCfgMdlKeyRefPhaseEvt_t         keyRefPhase;     /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT
                                                       */
  meshCfgMdlHbPubEvt_t               hbPub;           /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_HB_PUB_SET_EVENT
                                                       */
  meshCfgMdlHbSubEvt_t               hbSub;           /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_HB_SUB_SET_EVENT
                                                       */
  meshCfgMdlNwkTransStateEvt_t       nwkTrans;        /*!< Valid if event is
                                                       *   ::MESH_CFG_MDL_NWK_TRANS_SET_EVENT
                                                       */
} meshCfgMdlSrEvt_t;

/*! \brief Configuration Server event status. Applicable to all event types. */
enum meshCfgMdlSrEventStatusValues
{
  MESH_CFG_MDL_SR_SUCCESS       = 0x00,  /*!< A state change has completed without errors */
};

/*************************************************************************************************/
/*!
 *  \brief     Notification callback triggered after a Configuration Client modifies a local state.
 *
 *  \param[in] pEvt  Pointer to the event structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshCfgMdlSrCback_t) (const meshCfgMdlSrEvt_t* pEvt);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Configuration Server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshCfgMdlSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Installs the Configuration Server callback.
 *
 *  \param[in] meshCfgMdlSrCback  Upper layer callback to notify that a local state has changed.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlSrRegister(meshCfgMdlSrCback_t meshCfgMdlSrCback);

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
