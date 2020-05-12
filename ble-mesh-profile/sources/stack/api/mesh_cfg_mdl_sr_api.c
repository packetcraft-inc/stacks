/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_sr_api.c
 *
 *  \brief  Configuration Server API implementation.
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

#include "wsf_types.h"
#include "wsf_assert.h"
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
#include "mesh_access.h"

#include "mesh_utils.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_sr.h"
#include "mesh_cfg_mdl_sr_main.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Definition of Core model to be registered in the Access Layer */
static meshAccCoreMdl_t cfgMdlSrAccMdl =
{
  .msgRecvCback   = meshCfgMdlSrAccMsgRcvCback,        /*!< Message received callback */
  .pOpcodeArray   = meshCfgMdlClOpcodes,               /*!< Opcodes registered for Rx */
  .opcodeArrayLen = MESH_CFG_MDL_CL_MAX_OP,            /*!< Number of opcodes */
  .elemId         = 0,                                 /*!< Only primary element allowed for
                                                        *   Configuration Server
                                                        */
  .mdlId.isSigModel         = TRUE,                    /*! SIG model */
  .mdlId.modelId.sigModelId = MESH_CFG_MDL_SR_MODEL_ID /*! Configuration Server Model ID. */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Configuration Server control block */
meshCfgMdlSrCb_t meshCfgMdlSrCb =
{
  .cback = meshCfgMdlSrEmptyCback,
  .friendStateChgCback = meshCfgMdlSrEmptyFriendStateChgCback,
  .netKeyDelNotifyCback = meshCfgMdlSrEmptyNetKeyDelNotifyCback,
  .pollTimeoutGetCback = meshCfgMdlSrEmptyPollTimeoutGetCback
};

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Configuration Server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshCfgMdlSrInit(void)
{
  MESH_TRACE_INFO0("MESH CFG SR: init");

  /* Register the Configuration Server in the Access Layer. */
  MeshAccRegisterCoreModel(&cfgMdlSrAccMdl);

  /* Register to default user callback. */
  meshCfgMdlSrCb.cback = meshCfgMdlSrEmptyCback;

  /* Register empty callback for Friend State changed notification. */
  meshCfgMdlSrCb.friendStateChgCback = meshCfgMdlSrEmptyFriendStateChgCback;

  /* Register empty callback for NetKey deleted notification. */
  meshCfgMdlSrCb.netKeyDelNotifyCback = meshCfgMdlSrEmptyNetKeyDelNotifyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Installs the Configuration Server callback.
 *
 *  \param[in] meshCfgMdlSrCback  Upper layer callback to notify that a local state has changed.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlSrRegister(meshCfgMdlSrCback_t meshCfgMdlSrCback)
{
  /* Check if callback is not NULL. */
  if (meshCfgMdlSrCback != NULL)
  {
    /* Enter critical section. */
    WSF_CS_INIT(cs);
    WSF_CS_ENTER(cs);

    /* Store callback. */
    meshCfgMdlSrCb.cback = meshCfgMdlSrCback;

    /* Exit critical section. */
    WSF_CS_EXIT(cs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers friend state changed notification callback.
 *
 *  \param[in] friendStateChgCback   Friendship state changed callback.
 *  \param[in] netKeyDelNotifyCback  NetKey deleted notification callback.
 *  \param[in] pollTimeoutGetCback   Poll Timeout get callback.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshCfgMdlSrRegisterFriendship(meshCfgMdlSrFriendStateChgCback_t friendStateChgCback,
                                    meshCfgMdlSrNetKeyDelNotifyCback_t netKeyDelNotifyCback,
                                    meshCfgMdlSrPollTimeoutGetCback_t pollTimeoutGetCback)
{
  if(friendStateChgCback != NULL)
  {
    meshCfgMdlSrCb.friendStateChgCback = friendStateChgCback;
  }

  if (netKeyDelNotifyCback != NULL)
  {
    meshCfgMdlSrCb.netKeyDelNotifyCback = netKeyDelNotifyCback;
  }

  if(pollTimeoutGetCback != NULL)
  {
    meshCfgMdlSrCb.pollTimeoutGetCback = pollTimeoutGetCback;
  }
}
