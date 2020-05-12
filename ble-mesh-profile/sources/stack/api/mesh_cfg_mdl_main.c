/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Configuration Model commmon implementation.
 *
 *  Copyright (c) 2018-2019 Arm Ltd. All Rights Reserved.
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

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Provisioning Client callback event length table */
static const uint16_t meshCfgEvtCbackLen[MESH_CFG_MDL_MAX_EVENT] =
{
  sizeof(meshCfgMdlBeaconStateEvt_t),         /*!< MESH_CFG_MDL_BEACON_GET_EVENT */
  sizeof(meshCfgMdlBeaconStateEvt_t),         /*!< MESH_CFG_MDL_BEACON_SET_EVENT */
  sizeof(meshCfgMdlCompDataEvt_t),            /*!< MESH_CFG_MDL_COMP_PAGE_GET_EVENT */
  sizeof(meshCfgMdlDefaultTtlStateEvt_t),     /*!< MESH_CFG_MDL_DEFAULT_TTL_GET_EVENT */
  sizeof(meshCfgMdlDefaultTtlStateEvt_t),     /*!< MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT */
  sizeof(meshCfgMdlGattProxyEvt_t),           /*!< MESH_CFG_MDL_GATT_PROXY_GET_EVENT */
  sizeof(meshCfgMdlGattProxyEvt_t),           /*!< MESH_CFG_MDL_GATT_PROXY_SET_EVENT */
  sizeof(meshCfgMdlRelayCompositeStateEvt_t), /*!< MESH_CFG_MDL_RELAY_GET_EVENT */
  sizeof(meshCfgMdlRelayCompositeStateEvt_t), /*!< MESH_CFG_MDL_RELAY_SET_EVENT */
  sizeof(meshCfgMdlModelPubEvt_t),            /*!< MESH_CFG_MDL_PUB_GET_EVENT */
  sizeof(meshCfgMdlModelPubEvt_t),            /*!< MESH_CFG_MDL_PUB_SET_EVENT */
  sizeof(meshCfgMdlModelPubEvt_t),            /*!< MESH_CFG_MDL_PUB_VIRT_SET_EVENT */
  sizeof(meshCfgMdlModelSubscrChgEvt_t),      /*!< MESH_CFG_MDL_SUBSCR_ADD_EVENT */
  sizeof(meshCfgMdlModelSubscrChgEvt_t),      /*!< MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT */
  sizeof(meshCfgMdlModelSubscrChgEvt_t),      /*!< MESH_CFG_MDL_SUBSCR_DEL_EVENT */
  sizeof(meshCfgMdlModelSubscrChgEvt_t),      /*!< MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT */
  sizeof(meshCfgMdlModelSubscrChgEvt_t),      /*!< MESH_CFG_MDL_SUBSCR_OVR_EVENT */
  sizeof(meshCfgMdlModelSubscrChgEvt_t),      /*!< MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT */
  sizeof(meshCfgMdlModelSubscrChgEvt_t),      /*!< MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT */
  sizeof(meshCfgMdlModelSubscrListEvt_t),     /*!< MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT */
  sizeof(meshCfgMdlModelSubscrListEvt_t),     /*!< MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT */
  sizeof(meshCfgMdlNetKeyChgEvt_t),           /*!< MESH_CFG_MDL_NETKEY_ADD_EVENT */
  sizeof(meshCfgMdlNetKeyChgEvt_t),           /*!< MESH_CFG_MDL_NETKEY_UPDT_EVENT */
  sizeof(meshCfgMdlNetKeyChgEvt_t),           /*!< MESH_CFG_MDL_NETKEY_DEL_EVENT */
  sizeof(meshCfgMdlNetKeyListEvt_t),          /*!< MESH_CFG_MDL_NETKEY_GET_EVENT */
  sizeof(meshCfgMdlAppKeyChgEvt_t),           /*!< MESH_CFG_MDL_APPKEY_ADD_EVENT */
  sizeof(meshCfgMdlAppKeyChgEvt_t),           /*!< MESH_CFG_MDL_APPKEY_UPDT_EVENT */
  sizeof(meshCfgMdlAppKeyChgEvt_t),           /*!< MESH_CFG_MDL_APPKEY_DEL_EVENT */
  sizeof(meshCfgMdlAppKeyListEvt_t),          /*!< MESH_CFG_MDL_APPKEY_GET_EVENT */
  sizeof(meshCfgMdlNodeIdentityEvt_t),        /*!< MESH_CFG_MDL_NODE_IDENTITY_GET_EVENT */
  sizeof(meshCfgMdlNodeIdentityEvt_t),        /*!< MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT */
  sizeof(meshCfgMdlModelAppBindEvt_t),        /*!< MESH_CFG_MDL_APP_BIND_EVENT */
  sizeof(meshCfgMdlModelAppBindEvt_t),        /*!< MESH_CFG_MDL_APP_UNBIND_EVENT */
  sizeof(meshCfgMdlModelAppListEvt_t),        /*!< MESH_CFG_MDL_APP_SIG_GET_EVENT */
  sizeof(meshCfgMdlModelAppListEvt_t),        /*!< MESH_CFG_MDL_APP_VENDOR_GET_EVENT */
  sizeof(meshCfgMdlNodeResetStateEvt_t),      /*!< MESH_CFG_MDL_NODE_RESET_EVENT */
  sizeof(meshCfgMdlFriendEvt_t),              /*!< MESH_CFG_MDL_FRIEND_GET_EVENT */
  sizeof(meshCfgMdlFriendEvt_t),              /*!< MESH_CFG_MDL_FRIEND_SET_EVENT */
  sizeof(meshCfgMdlKeyRefPhaseEvt_t),         /*!< MESH_CFG_MDL_KEY_REF_PHASE_GET_EVENT */
  sizeof(meshCfgMdlKeyRefPhaseEvt_t),         /*!< MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT */
  sizeof(meshCfgMdlHbPubEvt_t),               /*!< MESH_CFG_MDL_HB_PUB_GET_EVENT */
  sizeof(meshCfgMdlHbPubEvt_t),               /*!< MESH_CFG_MDL_HB_PUB_SET_EVENT */
  sizeof(meshCfgMdlHbSubEvt_t),               /*!< MESH_CFG_MDL_HB_SUB_GET_EVENT */
  sizeof(meshCfgMdlHbSubEvt_t),               /*!< MESH_CFG_MDL_HB_SUB_SET_EVENT */
  sizeof(meshCfgMdlLpnPollTimeoutEvt_t),      /*!< MESH_CFG_MDL_LPN_POLLTIMEOUT_GET_EVENT */
  sizeof(meshCfgMdlNwkTransStateEvt_t),       /*!< MESH_CFG_MDL_NWK_TRANS_GET_EVENT */
  sizeof(meshCfgMdlNwkTransStateEvt_t),       /*!< MESH_CFG_MDL_NWK_TRANS_SET_EVENT */
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

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
uint16_t MeshCfgSizeOfEvt(wsfMsgHdr_t *pMeshCfgEvt)
{
  /* If a valid event ID */
  if (((pMeshCfgEvt->event == MESH_CFG_MDL_CL_EVENT) ||
       (pMeshCfgEvt->event == MESH_CFG_MDL_SR_EVENT)) &&
      (pMeshCfgEvt->param < MESH_CFG_MDL_MAX_EVENT))
  {
    uint16_t len;

    len = meshCfgEvtCbackLen[pMeshCfgEvt->param];

    if (pMeshCfgEvt->status == MESH_CFG_MDL_CL_SUCCESS)
    {

      switch (pMeshCfgEvt->param)
      {
        case MESH_CFG_MDL_COMP_PAGE_GET_EVENT:
          len += ((meshCfgMdlCompDataEvt_t *) pMeshCfgEvt)->data.pageSize * sizeof(uint8_t);
          break;

        case MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT:
        case MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT:
          len += ((meshCfgMdlModelSubscrListEvt_t *) pMeshCfgEvt)->subscrListSize * sizeof(meshAddress_t);
          break;

        case MESH_CFG_MDL_NETKEY_GET_EVENT:
          len += ((meshCfgMdlNetKeyListEvt_t *) pMeshCfgEvt)->netKeyList.netKeyCount * sizeof(uint16_t);
          break;

        case MESH_CFG_MDL_APPKEY_GET_EVENT:
          len += ((meshCfgMdlAppKeyListEvt_t *) pMeshCfgEvt)->appKeyList.appKeyCount * sizeof(uint16_t);
          break;

        case MESH_CFG_MDL_APP_SIG_GET_EVENT:
        case MESH_CFG_MDL_APP_VENDOR_GET_EVENT:
          len += ((meshCfgMdlModelAppListEvt_t *) pMeshCfgEvt)->modelAppList.appKeyCount * sizeof(uint16_t);
          break;

        default:
          break;
      }
    }

    return len;
  }

  return 0;
}

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
bool_t MeshCfgMsgDeepCopy(wsfMsgHdr_t *pMeshCfgEvtOut, const wsfMsgHdr_t *pMeshCfgEvtIn)
{
  if (((pMeshCfgEvtIn->event == MESH_CFG_MDL_CL_EVENT) ||
       (pMeshCfgEvtIn->event == MESH_CFG_MDL_SR_EVENT)) &&
      (pMeshCfgEvtIn->param < MESH_CFG_MDL_MAX_EVENT))
  {
    /* Copy over basic structure */
    memcpy(pMeshCfgEvtOut, pMeshCfgEvtIn, meshCfgEvtCbackLen[pMeshCfgEvtIn->param]);

    if (pMeshCfgEvtIn->status == MESH_CFG_MDL_CL_SUCCESS)
    {

      /* Perform event specific copying. */
      switch (pMeshCfgEvtIn->param)
      {
        case MESH_CFG_MDL_COMP_PAGE_GET_EVENT:
          ((meshCfgMdlCompDataEvt_t *) pMeshCfgEvtOut)->data.pPage = (uint8_t *) ((meshCfgMdlCompDataEvt_t *) pMeshCfgEvtOut + 1);

          memcpy(((meshCfgMdlCompDataEvt_t *) pMeshCfgEvtOut)->data.pPage,
                 ((meshCfgMdlCompDataEvt_t *) pMeshCfgEvtIn)->data.pPage,
                 ((meshCfgMdlCompDataEvt_t *) pMeshCfgEvtIn)->data.pageSize);
          break;

        case MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT:
        case MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT:
          ((meshCfgMdlModelSubscrListEvt_t *) pMeshCfgEvtOut)->pSubscrList = (uint16_t *) ((meshCfgMdlModelSubscrListEvt_t *) pMeshCfgEvtOut + 1);

          memcpy(((meshCfgMdlModelSubscrListEvt_t *) pMeshCfgEvtOut)->pSubscrList,
                 ((meshCfgMdlModelSubscrListEvt_t *) pMeshCfgEvtIn)->pSubscrList,
                 ((meshCfgMdlModelSubscrListEvt_t *) pMeshCfgEvtIn)->subscrListSize * sizeof(meshAddress_t));

          break;

        case MESH_CFG_MDL_NETKEY_GET_EVENT:
          ((meshCfgMdlNetKeyListEvt_t *) pMeshCfgEvtOut)->netKeyList.pNetKeyIndexes = (uint16_t *) ((meshCfgMdlNetKeyListEvt_t *) pMeshCfgEvtOut + 1);

          memcpy(((meshCfgMdlNetKeyListEvt_t *) pMeshCfgEvtOut)->netKeyList.pNetKeyIndexes,
                 ((meshCfgMdlNetKeyListEvt_t *) pMeshCfgEvtIn)->netKeyList.pNetKeyIndexes,
                 ((meshCfgMdlNetKeyListEvt_t *) pMeshCfgEvtIn)->netKeyList.netKeyCount * sizeof(uint16_t));
          break;

        case MESH_CFG_MDL_APPKEY_GET_EVENT:
          ((meshCfgMdlAppKeyListEvt_t *) pMeshCfgEvtOut)->appKeyList.pAppKeyIndexes = (uint16_t *) ((meshCfgMdlAppKeyListEvt_t *) pMeshCfgEvtOut + 1);

          memcpy(((meshCfgMdlAppKeyListEvt_t *) pMeshCfgEvtOut)->appKeyList.pAppKeyIndexes,
                 ((meshCfgMdlAppKeyListEvt_t *) pMeshCfgEvtIn)->appKeyList.pAppKeyIndexes,
                 ((meshCfgMdlAppKeyListEvt_t *) pMeshCfgEvtIn)->appKeyList.appKeyCount * sizeof(uint16_t));
          break;

        case MESH_CFG_MDL_APP_SIG_GET_EVENT:
        case MESH_CFG_MDL_APP_VENDOR_GET_EVENT:
          ((meshCfgMdlModelAppListEvt_t *) pMeshCfgEvtOut)->modelAppList.pAppKeyIndexes = (uint16_t *) ((meshCfgMdlModelAppListEvt_t *) pMeshCfgEvtOut + 1);

          memcpy(((meshCfgMdlModelAppListEvt_t *) pMeshCfgEvtOut)->modelAppList.pAppKeyIndexes,
                 ((meshCfgMdlModelAppListEvt_t *) pMeshCfgEvtIn)->modelAppList.pAppKeyIndexes,
                 ((meshCfgMdlModelAppListEvt_t *) pMeshCfgEvtIn)->modelAppList.appKeyCount * sizeof(uint16_t));
          break;

        default:
          break;
      }
    }

    return TRUE;
  }

  return FALSE;
}
