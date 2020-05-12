/*************************************************************************************************/
/*!
 *  \file   provisioner_main.c
 *
 *  \brief  Provisioner application.
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
 */
/*************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include "wsf_types.h"
#include "wsf_timer.h"
#include "wsf_trace.h"
#include "wsf_msg.h"
#include "wsf_buf.h"
#include "wsf_assert.h"
#include "dm_api.h"
#include "util/bstream.h"
#include "util/wstr.h"

#include "att_api.h"
#include "app_api.h"
#include "app_cfg.h"
#include "mprxc/mprxc_api.h"
#include "mprvc/mprvc_api.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"

#include "mmdl_types.h"

#include "adv_bearer.h"
#include "gatt_bearer_cl.h"

#include "mesh_prv.h"
#include "mesh_prv_cl_api.h"
#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl_cl_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "app_mesh_api.h"
#include "app_bearer.h"

#include "provisioner_api.h"
#include "provisioner_config.h"
#include "provisioner_version.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Primary address of the provisioner. */
#ifndef PROVISIONER_PRIMARY_ADDRESS
#define PROVISIONER_PRIMARY_ADDRESS               0x0001
#endif

/*! Health Server company ID registered in the instance. */
#define PROVISIONER_HT_SR_COMPANY_ID              0xFFFF
/*! Health Server test ID for the associated to the test company ID. */
#define PROVISIONER_HT_SR_TEST_ID                 0x00

/*! Number of handles to be discovered. */
#define DISC_HANDLES_NUM                          3

/*! Starting element address of provisioned nodes. */
#define PROVISIONER_NODE_ADDR_START               0x0100

/*! Master group address. */
#define PROVISIONER_MASTER_ADDR                   (0x0000 | MESH_ADDR_TYPE_GROUP_VIRTUAL_MASK)

/*! Starting room address.
 *  \note this initial value will be incremented in the control block before assignment.
 */
#define PROVISIONER_ROOM_ADDR_START               PROVISIONER_MASTER_ADDR

/*! Maximum number of retries. */
#define PROVISIONER_MAX_RETRIES                   10

/*! Time to wait after provisioning before configuration in seconds. */
#define PROVISIONER_TIMER_CC_START_TIMEOUT        2

/*! WSF message event starting value. */
#define PROVISIONER_MSG_START                     0xFE

/*! WSF message event enumeration. */
enum
{
  PROVISIONER_CCSTART_TIMER_EVENT = PROVISIONER_MSG_START, /*!< Configuration Start Timer event. */
};

#define PROVISIONER_NEWLINE
#define PROVISIONER_PRINT0(msg)                   APP_TRACE_INFO0(msg)
#define PROVISIONER_PRINT1(msg, var1)             APP_TRACE_INFO1(msg, var1)
#define PROVISIONER_PRINT2(msg, var1, var2)       APP_TRACE_INFO2(msg, var1, var2)
#define PROVISIONER_PRINT3(msg, var1, var2, var3) APP_TRACE_INFO3(msg, var1, var2, var3)
#define PROVISIONER_PRINT4(msg, var1, var2, var3, var4) \
                                                  APP_TRACE_INFO4(msg, var1, var2, var3, var4)
#define PROVISIONER_PRINT5(msg, var1, var2, var3, var4, var5) \
                                                  APP_TRACE_INFO5(msg, var1, var2, var3, var4, var5)
#define PROVISIONER_PRINT6(msg, var1, var2, var3, var4, var5, var6) \
                                                  APP_TRACE_INFO6(msg, var1, var2, var3, var4, var5, var6)
#define PROVISIONER_PRINT8(msg, var1, var2, var3, var4, var5, var6, var7, var8) \
                                                  APP_TRACE_INFO8(msg, var1, var2, var3, var4, var5, var6, var7, var8)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Enumeration of client characteristic configuration descriptors */
enum
{
  PROVISIONER_DOUT_CCC_IDX,      /*! Mesh Proxy/Provisioning service, Data Out */
  PROVISIONER_NUM_CCC_IDX
};

/*! Configuration Client Action function. */
typedef void (* provisionerActCCFn_t)(void);

/*! Provisioner control block structure */
typedef struct provisionerCb_tag
{
  dmCback_t               discCback;                              /*!< GATT discovery callback */
  uint16_t                hdlList[DISC_HANDLES_NUM];              /*!< Handles discovered by the GATT client */
  uint16_t                netKeyIndexAdv;                         /*!< Net Key Index used for GATT advertising */
  bool_t                  proxyClStarted;                         /*!< TRUE if GATT Proxy Client is started
                                                                   *   FALSE otherwise */
  bool_t                  prvGattClStarted;                       /*!< TRUE if Provisioning Client is started
                                                                   *   FALSE otherwise */
  wsfTimer_t              currNodeCCStartTimer;                   /*!< Timer to delay start of configruation. */
  meshAddress_t           currNodePrimAddr;                       /*!< Current Node primary address. */
  meshAddress_t           currRoomAddress;                        /*!< Current Room address. */
  provisionerPrvDevType_t currNodeDeviceType;                     /*!< Current Node device type. See ::provisionerPrvDeviceTypes. */
  provisionerState_t      currNodeState;                          /*!< Current Node provisioning state. See ::provisionerCfgStates. */
  uint8_t                 currNodeCCCommIdx;                      /*!< Current node configuration client index of common state machine. */
  uint8_t                 currNodeCCRetry;                        /*!< Current node retries until failure. */
  uint16_t                currNodeAppKeyIdx;                      /*!< Current node application key index. */
  uint16_t                currNodeNetKeyIdx;                      /*!< Current node network key index. */
  uint16_t                currNodeGooMmdlAddr;                    /*!< Current node Generic On Off Mesh Model element address. */
  uint8_t                 currDevUuid[MESH_PRV_DEVICE_UUID_SIZE]; /*!< Current Node device UUID. */
  uint8_t                 currNodeDevKey[MESH_KEY_SIZE_128];      /*!< Current node key used to provision with. */
  provisionerActCCFn_t    *currNodeStateMachine;                  /*!< Current node configuration client state machine. */
} provisionerCb_t;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static void provisionerCCSetNwkTrans(void);
static void provisionerCCSetAppKey(void);
static void provisionerCCAppKeyBindGOOMmdlSr(void);
static void provisionerCCSubGOORoom(void);
static void provisionerCCSubGOOMaster(void);
static void provisionerCCAppKeyBindGOOMmdlCl(void);
static void provisionerCCAppKeyBindGOOMmdlSr(void);
static void provisionerCCPubGOORoom(void);
static void provisionerCCPubGOOMaster(void);
static void provisionerCCEnd(void);
static void provisionerCCExecute(void);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief  ATT Client Data */

/*! Configurable parameters for service and characteristic discovery */
static const appDiscCfg_t provisionerDiscCfg =
{
  FALSE                   /*! TRUE to wait for a secure connection before initiating discovery */
};

/*! \brief  ATT Client Configuration Data after service discovery */

/*! Default value for CCC notifications */
const uint8_t provisionerDataOutCccNtfVal[2] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_NOTIFY)};

/*! List of characteristics to configure after service discovery */
static const attcDiscCfg_t discCfgList[] =
{
  /*! Write: Data Out ccc descriptor */
  {provisionerDataOutCccNtfVal, sizeof(provisionerDataOutCccNtfVal), (MPRXC_MPRXS_DOUT_CCC_HDL_IDX)}
};

/*! Characteristic configuration list length */
#define MESH_SVC_DISC_CFG_LIST_LEN   (sizeof(discCfgList) / sizeof(attcDiscCfg_t))

/*! Local device key */
uint8_t provisionerDevKey[] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x00 };

/*! Network key */
uint8_t provisionerNetKey[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

/*! WSF handler ID */
static wsfHandlerId_t provisionerHandlerId;

/*! Configuration Client common functions */
static provisionerActCCFn_t provisionerCommonCCFuncs[] =
{
  provisionerCCSetNwkTrans,
  provisionerCCSetAppKey,
};

/*! Configuration Client Light functions */
static provisionerActCCFn_t provisionerLightCCFuncs[] =
{
  provisionerCCAppKeyBindGOOMmdlSr,
  provisionerCCSubGOORoom,
  provisionerCCSubGOOMaster,
  provisionerCCEnd
};

/*! Configuration Client Room Switch functions */
static provisionerActCCFn_t provisionerRoomSwCCFuncs[] =
{
  provisionerCCAppKeyBindGOOMmdlCl,
  provisionerCCPubGOORoom,
  provisionerCCEnd
};

/*! Configuration Client Master Switch functions */
static provisionerActCCFn_t provisionerMasterSwCCFunc[] =
{
  provisionerCCAppKeyBindGOOMmdlCl,
  provisionerCCPubGOOMaster,
  provisionerCCEnd
};

/* Configuration Client Node State Machines. Indexed by ::provisionerPrvDeviceTypes */
static provisionerActCCFn_t * provisionerCCNodeTypeSM[] =
{
 provisionerMasterSwCCFunc,
 provisionerRoomSwCCFuncs,
 provisionerLightCCFuncs,
};

/*! Provisioner App control block */
static provisionerCb_t provCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client set network transmit state.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCSetNwkTrans(void)
{
  static meshNwkTransState_t transState =
  {
    .transCount = 7,
    .transIntervalSteps10Ms = 0,
  };

  MeshCfgMdlClNwkTransmitSet(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx, provCb.currNodeDevKey,
                             &transState);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client set application key.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCSetAppKey(void)
{
  meshAppNetKeyBind_t keyBind =
  {
    .appKeyIndex = 0,
    .netKeyIndex = provCb.currNodeNetKeyIdx,
  };

  /* Static application key accross all nodes. */
  uint8_t appKey[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                       0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

  provCb.currNodeAppKeyIdx = keyBind.appKeyIndex;

  MeshCfgMdlClAppKeyChg(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx, provCb.currNodeDevKey,
                        &keyBind, MESH_CFG_MDL_CL_KEY_ADD, appKey);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client set application binding to Generic On Off Mesh Server
 *             Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCAppKeyBindGOOMmdlSr(void)
{
  MeshCfgMdlClAppBind(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx, provCb.currNodeDevKey,
                      TRUE, provCb.currNodeAppKeyIdx, provCb.currNodeGooMmdlAddr,
                      MMDL_GEN_ONOFF_SR_MDL_ID, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client set application binding to Generic On Off Mesh Client
 *             Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCAppKeyBindGOOMmdlCl(void)
{
  MeshCfgMdlClAppBind(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx, provCb.currNodeDevKey,
                      TRUE, provCb.currNodeAppKeyIdx, provCb.currNodeGooMmdlAddr,
                      MMDL_GEN_ONOFF_CL_MDL_ID, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client subscribe to Generic On Off Mesh Client Model with room
 *             address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCSubGOORoom(void)
{
  MeshCfgMdlClSubscrListChg(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx,
                            provCb.currNodeDevKey, provCb.currNodeGooMmdlAddr,
                            MESH_CFG_MDL_CL_SUBSCR_ADDR_ADD, provCb.currRoomAddress, NULL,
                            MMDL_GEN_ONOFF_SR_MDL_ID, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client subscribe to Generic On Off Mesh Client Model with master
 *             address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCSubGOOMaster(void)
{
 MeshCfgMdlClSubscrListChg(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx,
                           provCb.currNodeDevKey, provCb.currNodeGooMmdlAddr,
                           MESH_CFG_MDL_CL_SUBSCR_ADDR_ADD, PROVISIONER_MASTER_ADDR, NULL,
                           MMDL_GEN_ONOFF_SR_MDL_ID, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client publish Generic On Off status to room.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCPubGOORoom(void)
{
  meshModelPublicationParams_t pubParams =
  {
    .publishAppKeyIndex = provCb.currNodeAppKeyIdx,
    .publishFriendshipCred = MESH_PUBLISH_MASTER_SECURITY,
    .publishTtl = 0,
    .publishPeriodNumSteps = 1,
    .publishPeriodStepRes = 1,
    .publishRetransCount = 0,
    .publishRetransSteps50Ms = 0,
  };

  MeshCfgMdlClPubSet(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx, provCb.currNodeDevKey,
                     provCb.currNodeGooMmdlAddr, provCb.currRoomAddress, NULL, &pubParams,
                     MMDL_GEN_ONOFF_CL_MDL_ID, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send Configuration Client publish Generic On Off status to all device in network.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCPubGOOMaster(void)
{
  meshModelPublicationParams_t pubParams =
  {
    .publishAppKeyIndex = provCb.currNodeAppKeyIdx,
    .publishFriendshipCred = MESH_PUBLISH_MASTER_SECURITY,
    .publishTtl = 0,
    .publishPeriodNumSteps = 1,
    .publishPeriodStepRes = 1,
    .publishRetransCount = 0,
    .publishRetransSteps50Ms = 0,
  };

  MeshCfgMdlClPubSet(provCb.currNodePrimAddr, provCb.currNodeNetKeyIdx, provCb.currNodeDevKey,
                     provCb.currNodeGooMmdlAddr, PROVISIONER_MASTER_ADDR, NULL, &pubParams,
                     MMDL_GEN_ONOFF_CL_MDL_ID, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Clear current node variables.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerClearCurrNode(void)
{
  /* Reset state machine. */
  provCb.currNodeStateMachine = NULL;
  provCb.currNodeCCCommIdx = 0;
  provCb.currNodeState = PROVISIONER_ST_PRV_START;
  provCb.currNodeDeviceType = PROVISIONER_PRV_NONE;
  provCb.currNodeCCRetry = PROVISIONER_MAX_RETRIES;
  memset(provCb.currNodeDevKey, 0x00, MESH_KEY_SIZE_128);
}

/*************************************************************************************************/
/*!
 *  \brief     Call Provisioner UI.
 *
 *  \param[in] status  Status of provisioning and configuration.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCallUi(uint8_t status)
{
  uint8_t revDevKey[MESH_KEY_SIZE_128];

  /* Add type specific cleanup.  Remove if not used afterall. */
  switch(provCb.currNodeDeviceType)
  {
    case PROVISIONER_PRV_MASTER_SWITCH:
      break;

    case PROVISIONER_PRV_ROOM_SWITCH:
      /* Reuse room address if this switch wasn't successful. */
      provCb.currRoomAddress = (status == MESH_SUCCESS) ? provCb.currRoomAddress :\
                                                          provCb.currRoomAddress - 1;
      break;

    case PROVISIONER_PRV_LIGHT:
      break;

    default:
      break;
  }

  /* Reverse endianness. */
  WStrReverseCpy(revDevKey, provCb.currNodeDevKey, MESH_KEY_SIZE_128);

  /* Clear state machine. */
  provisionerClearCurrNode();

  /* Call UI. */
  ProvisionerMenuHandleEvent(status, provCb.currDevUuid, revDevKey);
}

/*************************************************************************************************/
/*!
 *  \brief     End configuration client state machine.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCEnd(void)
{
  if (provCb.currNodeDeviceType != PROVISIONER_PRV_NONE)
  {
    /* Call UI. */
    provisionerCallUi(MESH_SUCCESS);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Execute state of configuration client state machine.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCExecute(void)
{
  if (provCb.currNodeCCCommIdx < (sizeof(provisionerCommonCCFuncs)/ sizeof(provisionerActCCFn_t)))
  {
    provisionerCommonCCFuncs[provCb.currNodeCCCommIdx]();
  }
  else if (provCb.currNodeStateMachine != NULL)
  {
    (*provCb.currNodeStateMachine)();
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Execute nextstate of configuration client state machine.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerCCExecuteNext(void)
{
  provCb.currNodeCCRetry = PROVISIONER_MAX_RETRIES;

  /* Increment to next state. */
  if (provCb.currNodeCCCommIdx < (sizeof(provisionerCommonCCFuncs)/ sizeof(provisionerActCCFn_t)))
  {
    provCb.currNodeCCCommIdx++;
  }
  else if (provCb.currNodeStateMachine != NULL)
  {
    provCb.currNodeStateMachine++;
  }

  /* Execute state machine. */
  provisionerCCExecute();
}

/*************************************************************************************************/
/*!
 *  \brief     Application Discovery Process message callback.
 *
 *  \param[in] pDmEvt  DM callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerDiscProcDmMsgEmpty(dmEvt_t *pDmEvt)
{
  (void)pDmEvt;
}

/*************************************************************************************************/
/*!
 *  \brief     Application DM callback.
 *
 *  \param[in] pDmEvt  DM callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t *pMsg;
  uint16_t len;
  uint16_t reportLen;

  len = DmSizeOfEvt(pDmEvt);

  switch(pDmEvt->hdr.event)
  {
    case DM_SCAN_REPORT_IND:
      reportLen = pDmEvt->scanReport.len;
      break;

    case DM_EXT_SCAN_REPORT_IND:
      reportLen = pDmEvt->extScanReport.len;
      break;

    default:
      reportLen = 0;
      break;
  }

  if ((pMsg = WsfMsgAlloc(len + reportLen)) != NULL)
  {
    memcpy(pMsg, pDmEvt, len);

    if (pDmEvt->hdr.event == DM_SCAN_REPORT_IND)
    {
      pMsg->scanReport.pData = (uint8_t *) ((uint8_t *) pMsg + len);
      memcpy(pMsg->scanReport.pData, pDmEvt->scanReport.pData, reportLen);
    }
    else if (pDmEvt->hdr.event == DM_EXT_SCAN_REPORT_IND)
    {
      pMsg->extScanReport.pData = (uint8_t *) ((uint8_t *) pMsg + len);
      memcpy(pMsg->extScanReport.pData, pDmEvt->extScanReport.pData, reportLen);
    }

    WsfMsgSend(provisionerHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application Mesh Stack callback.
 *
 *  \param[in] pEvt  Mesh Stack callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerMeshCback(meshEvt_t *pEvt)
{
  meshEvt_t *pMsg;
  uint16_t len;

  len = MeshSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(provisionerHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application Mesh Provisioning Client callback.
 *
 *  \param[in] pEvt  Mesh Provisioning Client callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerMeshPrvClCback(meshPrvClEvt_t *pEvt)
{
  meshPrvClEvt_t *pMsg;
  uint16_t len;

  len = MeshPrvClSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(provisionerHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Notification callback triggered after a Configuration Client modifies a local state.
 *
 *  \param[in] pEvt  Pointer to the event structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerMeshCfgMdlSrCback(const meshCfgMdlSrEvt_t* pEvt)
{
  /* Not used. */
}

/*************************************************************************************************/
/*!
 *  \brief     Notification callback triggered by Configuration Client.
 *
 *  \param[in] pEvt  Pointer to the event structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerMeshCfgMdlClCback(meshCfgMdlClEvt_t* pEvt)
{
  meshCfgMdlClEvt_t *pMsg;
  uint16_t len;

  len = MeshCfgSizeOfEvt((wsfMsgHdr_t *) pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    if (MeshCfgMsgDeepCopy((wsfMsgHdr_t *) pMsg, (wsfMsgHdr_t *) pEvt))
    {
      WsfMsgSend(provisionerHandlerId, pMsg);
    }
    else
    {
      WsfMsgFree(pMsg);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Discovery callback.
 *
 *  \param[in] connId  Connection identifier.
 *  \param[in] status  Service or configuration status.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerDiscCback(dmConnId_t connId, uint8_t status)
{
  switch(status)
  {
    case APP_DISC_INIT:
      /* Set handle list when initialization requested. */
      AppDiscSetHdlList(connId, DISC_HANDLES_NUM, provCb.hdlList);
      break;

    case APP_DISC_READ_DATABASE_HASH:
      /* Placeholder for Disocvering peer Database hash */
      /* Fallthrough */
    case APP_DISC_START:
      /* Discover service. */
      if (provCb.proxyClStarted)
      {
        MprxcMprxsDiscover(connId, provCb.hdlList);
      }
      else if (provCb.prvGattClStarted)
      {
        MprvcMprvsDiscover(connId, provCb.hdlList);
      }
      break;

    case APP_DISC_FAILED:
      /* Close connection if discovery failed. */
      AppConnClose(connId);
      break;

    case APP_DISC_CMPL:
      {
        uint16_t startHandle, endHandle;

        (void) AppDiscGetHandleRange(connId, &startHandle, &endHandle);

        PROVISIONER_PRINT2("svc disc_ind start_hdl=0x%X end_hdl=0x%X" PROVISIONER_NEWLINE,
                           startHandle, endHandle);

        /* Discovery complete. */
        AppDiscComplete(connId, APP_DISC_CMPL);

        if (provCb.proxyClStarted)
        {
          PROVISIONER_PRINT3("disc_ind mesh_prx data_in_hdl=0x%x data_out_hdl=0x%x data_out_cccd_hdl=0x%x"
                             PROVISIONER_NEWLINE, provCb.hdlList[0], provCb.hdlList[1],
                             provCb.hdlList[2]);
        }
        else if (provCb.prvGattClStarted)
        {
          PROVISIONER_PRINT3("disc_ind mesh_prv data_in_hdl=0x%x data_out_hdl=0x%x data_out_cccd_hdl=0x%x"
                             PROVISIONER_NEWLINE, provCb.hdlList[0], provCb.hdlList[1],
                             provCb.hdlList[2]);
        }

        /* Start configuration. */
        AppDiscConfigure(connId, APP_DISC_CFG_START, MESH_SVC_DISC_CFG_LIST_LEN,
                         (attcDiscCfg_t *) discCfgList, MESH_SVC_DISC_CFG_LIST_LEN, provCb.hdlList);
      }
      break;

    case APP_DISC_CFG_START:
      /* Start configuration. */
      AppDiscConfigure(connId, APP_DISC_CFG_START, MESH_SVC_DISC_CFG_LIST_LEN,
                       (attcDiscCfg_t *) discCfgList, MESH_SVC_DISC_CFG_LIST_LEN, provCb.hdlList);
      break;

    case APP_DISC_CFG_CMPL:
      AppDiscComplete(connId, status);
      if (provCb.proxyClStarted)
      {
        MprxcSetHandles(connId, provCb.hdlList[0], provCb.hdlList[1]);
      }
      else if (provCb.prvGattClStarted)
      {
        MprvcSetHandles(connId, provCb.hdlList[0], provCb.hdlList[1]);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application ATT callback.
 *
 *  \param[in] pEvt  ATT callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerAttCback(attEvt_t *pEvt)
{
  attEvt_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attEvt_t));
    pMsg->pValue = (uint8_t *) (pMsg + 1);
    memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
    WsfMsgSend(provisionerHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application bearer callback that signals the scheduled slot that has run.
 *
 *  \param[in] slot  Bearer slot. See ::bearerSlots.
 *
 *  \retval    None.
 */
/*************************************************************************************************/
static void provisionerBearerCback(uint8_t slot)
{
  (void)slot;
}

/*************************************************************************************************/
/*!
 *  \brief     Checks if the Service UUID is advertised
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static bool_t provisionerCheckServiceUuid(dmEvt_t *pMsg)
{
  uint8_t *pData = NULL;
  bool_t serviceFound = FALSE;

  /* Find list of 16 bit UUIDs in Scan Report */
  switch(pMsg->hdr.event)
  {
    case DM_EXT_SCAN_REPORT_IND:
      /* Find Service UUID list; if full list not found search for partial */
      if ((pData = DmFindAdType(DM_ADV_TYPE_16_UUID, pMsg->extScanReport.len,
                                pMsg->extScanReport.pData)) == NULL)
      {
        pData = DmFindAdType(DM_ADV_TYPE_16_UUID_PART, pMsg->extScanReport.len,
                             pMsg->extScanReport.pData);
      }
      break;
    case DM_SCAN_REPORT_IND:
      /* Find Service UUID list; if full list not found search for partial */
      if ((pData = DmFindAdType(DM_ADV_TYPE_16_UUID, pMsg->scanReport.len,
                                pMsg->scanReport.pData)) == NULL)
      {
        pData = DmFindAdType(DM_ADV_TYPE_16_UUID_PART, pMsg->scanReport.len,
                             pMsg->scanReport.pData);
      }
      break;

    default:
      break;
  }

  /* if the Service UUID of the registered GATT Bearer is found and length checks out ok */
  if (pData != NULL && pData[DM_AD_LEN_IDX] >= (ATT_16_UUID_LEN + 1))
  {
    uint8_t len = pData[DM_AD_LEN_IDX] - 1;
    pData += DM_AD_DATA_IDX;

    while ((!serviceFound) && (len >= ATT_16_UUID_LEN))
    {
      /* Connect if desired service is included */
      if (BYTES_UINT16_CMP(pData, pGattBearerClCfg->serviceUuid))
      {
        serviceFound = TRUE;
        break;
      }

      pData += ATT_16_UUID_LEN;
      len -= ATT_16_UUID_LEN;
    }
  }

  return serviceFound;
}

/*************************************************************************************************/
/*!
 *  \brief     Handle a scan report for GATT Bearer.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static bool_t provisionerProcessGattBearerScanReport(dmEvt_t *pMsg)
{
  uint8_t *pData = NULL;
  uint8_t *pAddr = NULL;
  uint8_t *pUuid = NULL;
  uint8_t serviceDataLen;
  uint8_t addrType = 0;
  bool_t devUuidFound = FALSE;

  /* Check for service data AD type*/
  switch(pMsg->hdr.event)
  {
    case DM_EXT_SCAN_REPORT_IND:
      /* Service has been found. Look for service data. */
      pData = DmFindAdType(DM_ADV_TYPE_SERVICE_DATA, pMsg->extScanReport.len, pMsg->extScanReport.pData);
      pAddr = pMsg->extScanReport.addr;
      addrType = pMsg->extScanReport.addrType;
      break;

    case DM_SCAN_REPORT_IND:
      /* Service has been found. Look for service data. */
      pData = DmFindAdType(DM_ADV_TYPE_SERVICE_DATA, pMsg->scanReport.len, pMsg->scanReport.pData);
      pAddr = pMsg->scanReport.addr;
      addrType = pMsg->scanReport.addrType;
      break;

    default:
      break;
  }

  /* If data found and right length */
  if ((pData != NULL) && pData[DM_AD_LEN_IDX] >= (ATT_16_UUID_LEN + 1))
  {
    serviceDataLen = pData[DM_AD_LEN_IDX] - ATT_16_UUID_LEN - 1;
    pData += DM_AD_DATA_IDX;

    /* Match service UUID in service data. */
    if (BYTES_UINT16_CMP(pData, pGattBearerClCfg->serviceUuid))
    {
      /* If GATT bearer is PB-GATT search for device UUID. */
      if (pGattBearerClCfg->serviceUuid == ATT_UUID_MESH_PRV_SERVICE &&
          (serviceDataLen == (MESH_PRV_DEVICE_UUID_SIZE + sizeof(meshPrvOobInfoSource_t))))
      {
        pData += ATT_16_UUID_LEN;
        pUuid = pData;

        /* Copy in this device UUID. */
        memcpy(provCb.currDevUuid, pUuid, MESH_PRV_DEVICE_UUID_SIZE);
        provisionerPrvClSessionInfo.pDeviceUuid = provCb.currDevUuid;

        /* Connect to this device. */
        devUuidFound = TRUE;
      }
      else if(pGattBearerClCfg->serviceUuid == ATT_UUID_MESH_PROXY_SERVICE)
      {
        /* Connect to anyone*/
        devUuidFound = TRUE;
      }
    }
  }

  /* Found match in scan report */
  if (devUuidFound)
  {
    /* Initiate connection */
    GattBearerClConnect(addrType, pAddr);

    return TRUE;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handle a scan report on PB-ADV.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static bool_t provisionerProcessAdvBearerScanReport(dmEvt_t *pMsg)
{
  uint8_t *pData = NULL;

  /* Check for service data AD type*/
  switch(pMsg->hdr.event)
  {
    case DM_EXT_SCAN_REPORT_IND:
      /* Service has been found. Look for service data. */
      pData = DmFindAdType(MESH_AD_TYPE_BEACON, pMsg->extScanReport.len, pMsg->extScanReport.pData);
      break;

    case DM_SCAN_REPORT_IND:
      /* Service has been found. Look for service data. */
      pData = DmFindAdType(MESH_AD_TYPE_BEACON, pMsg->scanReport.len, pMsg->scanReport.pData);
      break;

    default:
      break;
  }

  /* If data found and length is okay */
  if ((pData != NULL) && (pData[DM_AD_LEN_IDX] >= MESH_PRV_DEVICE_UUID_SIZE + 1))
  {
    pData += DM_AD_DATA_IDX;

    /* If Beacon Type if Unprovisioned Device Beacon */
    if (pData[0] == MESH_BEACON_TYPE_UNPROV)
    {
      pData++;

      /* Copy in Device UUID. */
      memcpy(provCb.currDevUuid, pData, MESH_PRV_DEVICE_UUID_SIZE);
      provisionerPrvClSessionInfo.pDeviceUuid = provCb.currDevUuid;

      /* Begin provisioning. */
      MeshPrvClStartPbAdvProvisioning(BR_ADV_SLOT, &provisionerPrvClSessionInfo);

      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Process a scan report.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerScanReport(dmEvt_t *pMsg)
{
  /* GATT Proxy/PRV Service Found. */
  if ((provCb.prvGattClStarted || provCb.proxyClStarted) &&  provisionerCheckServiceUuid(pMsg))
  {
    if (provisionerProcessGattBearerScanReport(pMsg))
    {
      provCb.currNodeState = PROVISIONER_ST_PRV_GATT_INPRG;
    }
  }
  else /* Check if this is an unprovisioned device beacon. */
  {
    if (provisionerProcessAdvBearerScanReport(pMsg))
    {
      provCb.currNodeState = PROVISIONER_ST_PRV_ADV_INPRG;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process messages from the Mesh Core event handler.
 *
 *  \param  pMsg  Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcMeshCoreMsg(meshEvt_t *pMsg)
{
  switch (pMsg->hdr.param)
  {
    case MESH_CORE_ADV_IF_ADD_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        /* Register advertising interface into bearer. */
        AdvBearerRegisterIf(((meshAdvIfEvt_t *)pMsg)->ifId);

        /* Schedule and enable ADV bearer. */
        AppBearerScheduleSlot(BR_ADV_SLOT, AdvBearerStart, AdvBearerStop, AdvBearerProcDmMsg, 5000);
        AppBearerEnableSlot(BR_ADV_SLOT);

        APP_TRACE_INFO0("PROVISIONER: ADV Interface added");
      }
      else
      {
        APP_TRACE_ERR1("PROVISIONER: ADV Interface add error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_REMOVE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        /* Unregister advertising interface from bearer. */
        AdvBearerDeregisterIf();

        /* Disable ADV bearer scheduling. */
        AppBearerDisableSlot(BR_ADV_SLOT);

        APP_TRACE_INFO0("PROVISIONER: ADV Interface removed");
      }
      else
      {
        APP_TRACE_ERR1("PROVISIONER: ADV Interface remove error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_CLOSE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        APP_TRACE_INFO0("PROVISIONER: ADV Interface closed");
      }
      else
      {
        APP_TRACE_ERR1("PROVISIONER: ADV Interface close error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_GATT_CONN_ADD_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
         APP_TRACE_INFO0("PROVISIONER: GATT Interface added");
         PROVISIONER_PRINT1("gatt_ind added connid=%d" PROVISIONER_NEWLINE, pMsg->gattConn.connId);

         if (provCb.prvGattClStarted)
         {
           /* Begin provisioning. */
           MeshPrvClStartPbGattProvisioning(pMsg->gattConn.connId, &provisionerPrvClSessionInfo);
         }
      }
      else
      {
        APP_TRACE_ERR1("PROVISIONER: GATT Interface add error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_GATT_CONN_CLOSE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        PROVISIONER_PRINT1("gatt_ind close connid=%d" PROVISIONER_NEWLINE, pMsg->gattConn.connId);
        /* Disconnect from peer. */
        AppConnClose(pMsg->gattConn.connId);
      }
    break;

    case MESH_CORE_GATT_CONN_REMOVE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        PROVISIONER_PRINT1("gatt_ind removed connid=%d" PROVISIONER_NEWLINE, pMsg->gattConn.connId);
      }
      else
      {
        APP_TRACE_ERR1("PROVISIONER: GATT Interface close/remove error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ATTENTION_CHG_EVENT:
      if (pMsg->attention.attentionOn)
      {
        PROVISIONER_PRINT1("mesh_ind attention=on elemid=%d" PROVISIONER_NEWLINE,
                        pMsg->attention.elementId);
      }
      else
      {
        PROVISIONER_PRINT1("mesh_ind attention=off elemid=%d" PROVISIONER_NEWLINE,
                        pMsg->attention.elementId);
      }
      break;

    case MESH_CORE_NODE_STARTED_EVENT:
      if(pMsg->nodeStarted.hdr.status == MESH_SUCCESS)
      {
        PROVISIONER_PRINT2(PROVISIONER_NEWLINE "mesh_ind node_started elemaddr=0x%x elemcnt=%d"
                           PROVISIONER_NEWLINE, pMsg->nodeStarted.address, pMsg->nodeStarted.elemCnt);
      }
      else
      {
        PROVISIONER_PRINT0(PROVISIONER_NEWLINE "mesh_ind node_started failed"
                           PROVISIONER_NEWLINE);
      }
      break;

    case MESH_CORE_PROXY_SERVICE_DATA_EVENT:

      break;

    case MESH_CORE_PROXY_FILTER_STATUS_EVENT:
      PROVISIONER_PRINT2("mesh_ind proxy_filter type=%d, list_size=%d" PROVISIONER_NEWLINE,
                         pMsg->filterStatus.filterType, pMsg->filterStatus.listSize);
      break;

    case MESH_CORE_IV_UPDATED_EVENT:
      PROVISIONER_PRINT1(PROVISIONER_NEWLINE "mesh_ind ividx=0x%x" PROVISIONER_NEWLINE,
                         pMsg->ivUpdt.ivIndex);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Notification callback triggered by Configuration Client.
 *
 *  \param[in] pEvt  Pointer to the event structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerProcMeshCfgMsg(meshCfgMdlClEvt_t* pEvt)
{
  if (pEvt->hdr.status == MESH_SUCCESS)
  {
    provisionerCCExecuteNext();
  }
  else if (provCb.currNodeCCRetry)
  {
    provCb.currNodeCCRetry--;
    provisionerCCExecute();
  }
  else
  {
    /* Send Failure to UI. */
    provisionerCallUi(pEvt->hdr.status);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Provisioning Client messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerProcMeshPrvClMsg(const meshPrvClEvt_t *pMsg)
{
  char alphanumericOob[1 + MESH_PRV_INOUT_OOB_MAX_SIZE];
  char devKeyStr[2 * MESH_KEY_SIZE_128 + 1];
  uint8_t i;

  switch (pMsg->hdr.param)
  {
    case MESH_PRV_CL_LINK_OPENED_EVENT:
      PROVISIONER_PRINT0("prvcl_ind link_opened" PROVISIONER_NEWLINE);
      break;

    case MESH_PRV_CL_RECV_CAPABILITIES_EVENT:
      {
        /* Use simplest authentication capabilities. */
        meshPrvClSelectAuth_t selectAuth =
        {
          .useOobPublicKey = FALSE,
          .oobAuthMethod = MESH_PRV_CL_NO_OBB_AUTH,
          .oobSize = 0,
        };

        /* Send Capabilities. */
        MeshPrvClSelectAuthentication(&selectAuth);

        PROVISIONER_PRINT8("prvcl_ind capabilities num_elem=%d algo=0x%x oobpk=0x%x static_oob=0x%x "
                           "output_oob_size=0x%x output_oob_act=0x%x input_oob_size=0x%x "
                           "input_oob_action=0x%x" PROVISIONER_NEWLINE,
                           pMsg->recvCapab.capabilities.numOfElements,
                           pMsg->recvCapab.capabilities.algorithms,
                           pMsg->recvCapab.capabilities.publicKeyType,
                           pMsg->recvCapab.capabilities.staticOobType,
                           pMsg->recvCapab.capabilities.outputOobSize,
                           pMsg->recvCapab.capabilities.outputOobAction,
                           pMsg->recvCapab.capabilities.inputOobSize,
                           pMsg->recvCapab.capabilities.inputOobAction);
      }
      break;

    case MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT:
      PROVISIONER_PRINT1("prvcl_ind enter_output_oob type=%s" PROVISIONER_NEWLINE,
                         ((pMsg->enterOutputOob.outputOobAction ==
                          MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM) ? "alpha" : "num"));
      break;

    case MESH_PRV_CL_DISPLAY_INPUT_OOB_EVENT:
      if (pMsg->inputOob.inputOobSize == 0)
      {
        /* Input OOB is numeric */
        PROVISIONER_PRINT1("prvcl_ind display_input_oob num=%d" PROVISIONER_NEWLINE,
                           pMsg->inputOob.inputOobData.numericOob);
      }
      else if (pMsg->inputOob.inputOobSize <= MESH_PRV_INOUT_OOB_MAX_SIZE)
      {
        /* Input OOB is alphanumeric */
        memcpy(alphanumericOob, pMsg->inputOob.inputOobData.alphanumericOob,
               pMsg->inputOob.inputOobSize);
        alphanumericOob[pMsg->inputOob.inputOobSize] = 0;

        PROVISIONER_PRINT1("prvcl_ind display_input_oob alpha=%s" PROVISIONER_NEWLINE,
                           alphanumericOob);
      }
      break;

    case MESH_PRV_CL_PROVISIONING_COMPLETE_EVENT:
      /* Update address for next provisioning. */
      provisionerPrvClSessionInfo.pData->address = pMsg->prvComplete.address + pMsg->prvComplete.numOfElements;

      /* Update for current node configuration. */
      memcpy(provCb.currNodeDevKey, pMsg->prvComplete.devKey, MESH_KEY_SIZE_128);
      provCb.currNodePrimAddr = provCb.currNodeGooMmdlAddr = pMsg->prvComplete.address;
      provCb.currNodeCCCommIdx = 0;
      provCb.currNodeState = (provCb.currNodeState == PROVISIONER_ST_PRV_ADV_INPRG)? \
                               PROVISIONER_ST_CC_ADV_INPRG: PROVISIONER_ST_CC_GATT_INPRG;

      /* Start timer to begin Configuration */
      WsfTimerStartSec(&provCb.currNodeCCStartTimer, PROVISIONER_TIMER_CC_START_TIMEOUT);

      for (i = 0; i < MESH_PRV_DEVICE_UUID_SIZE; i++)
      {
        sprintf(&devKeyStr[2 * i], "%02x", pMsg->prvComplete.devKey[i]);
      }

      PROVISIONER_PRINT3("prvcl_ind prv_complete elemaddr=0x%x elemcnt=%d devkey=0x%s" PROVISIONER_NEWLINE,
                         pMsg->prvComplete.address, pMsg->prvComplete.numOfElements, devKeyStr);
      break;

    case MESH_PRV_CL_PROVISIONING_FAILED_EVENT:
      /* Call UI. */
      provisionerCallUi(pMsg->prvFailed.reason);

      PROVISIONER_PRINT1("prvcl_ind prv_failed reason=0x%x" PROVISIONER_NEWLINE,
                         pMsg->prvFailed.reason);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg  Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcMeshMsg(wsfMsgHdr_t *pMsg)
{
  switch (pMsg->event)
  {
    case MESH_CORE_EVENT:
      provisionerProcMeshCoreMsg((meshEvt_t *) pMsg);
      break;

    case MESH_CFG_MDL_CL_EVENT:
      provisionerProcMeshCfgMsg((meshCfgMdlClEvt_t *) pMsg);
      break;

    case MESH_PRV_CL_EVENT:
      provisionerProcMeshPrvClMsg((meshPrvClEvt_t *) pMsg);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic On Off event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerProcessMmdlGenOnOffEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_ONOFF_CL_STATUS_EVENT:
      PROVISIONER_PRINT2("genonoff_ind status addr=0x%x state=%s" PROVISIONER_NEWLINE,
                         ((mmdlGenOnOffClEvent_t *)pEvt)->statusEvent.serverAddr,
                         (((mmdlGenOnOffClEvent_t *)pEvt)->statusEvent.state == MMDL_GEN_ONOFF_STATE_ON) ?
                          "on" : "off");
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Light HSL event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerProcessMmdlLighHslEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_LIGHT_HSL_CL_STATUS_EVENT:
      if (((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        PROVISIONER_PRINT5("lighthsl_ind status addr=0x%x lightness=0x%X hue=0x%X sat=0x%X remtime=0x%X"
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        PROVISIONER_PRINT4("lighthsl_ind status addr=0x%x lightness=0x%X hue=0x%X sat=0x%X "
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation);
      }
      break;

    case MMDL_LIGHT_HSL_CL_TARGET_STATUS_EVENT:
      if (((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        PROVISIONER_PRINT5("lighthsl_ind targetstatus addr=0x%x lightness=0x%X hue=0x%X sat=0x%X remtime=0x%X"
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        PROVISIONER_PRINT4("lighthsl_ind targetstatus addr=0x%x lightness=0x%X hue=0x%X sat=0x%X "
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                           ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation);
      }
      break;

    case MMDL_LIGHT_HSL_CL_HUE_STATUS_EVENT:
      if (((mmdlLightHslClHueStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        PROVISIONER_PRINT4("lighth_ind status addr=0x%x present=0x%X target=0x%X remtime=0x%X"
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClHueStatusEvent_t *)pEvt)->serverAddr,
                           ((mmdlLightHslClHueStatusEvent_t *)pEvt)->presentHue,
                           ((mmdlLightHslClHueStatusEvent_t *)pEvt)->targetHue,
                           ((mmdlLightHslClHueStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        PROVISIONER_PRINT2("lighth_ind status addr=0x%x present=0x%X "
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClHueStatusEvent_t *)pEvt)->serverAddr,
                           ((mmdlLightHslClHueStatusEvent_t *)pEvt)->presentHue);
      }
      break;

    case MMDL_LIGHT_HSL_CL_SAT_STATUS_EVENT:
      if (((mmdlLightHslClSatStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        PROVISIONER_PRINT4("lights_ind status addr=0x%x present=0x%X target=0x%X remtime=0x%X"
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClSatStatusEvent_t *)pEvt)->elementId,
                           ((mmdlLightHslClSatStatusEvent_t *)pEvt)->presentSat,
                           ((mmdlLightHslClSatStatusEvent_t *)pEvt)->targetSat,
                           ((mmdlLightHslClSatStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        PROVISIONER_PRINT2("lights_ind status addr=0x%x present=0x%X "
                           PROVISIONER_NEWLINE,
                           ((mmdlLightHslClSatStatusEvent_t *)pEvt)->serverAddr,
                           ((mmdlLightHslClSatStatusEvent_t *)pEvt)->presentSat);
      }
      break;

    case MMDL_LIGHT_HSL_CL_DEF_STATUS_EVENT:
      PROVISIONER_PRINT4("lighthsl_ind default addr=0x%x lightness=0x%X hue=0x%X sat=0x%X"
                         PROVISIONER_NEWLINE,
                         ((mmdlLightHslClDefStatusEvent_t *)pEvt)->serverAddr,
                         ((mmdlLightHslClDefStatusEvent_t *)pEvt)->lightness,
                         ((mmdlLightHslClDefStatusEvent_t *)pEvt)->hue,
                         ((mmdlLightHslClDefStatusEvent_t *)pEvt)->saturation);
      break;

    case MMDL_LIGHT_HSL_CL_RANGE_STATUS_EVENT:
      PROVISIONER_PRINT6("lighthsl_ind range addr=0x%x status=0x%X minhue=0x%X maxhue=0x%X \
                          minsat=0x%X maxsat=0x%X" PROVISIONER_NEWLINE,
                         ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->serverAddr,
                         ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->opStatus,
                         ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->minHue,
                         ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->maxHue,
                         ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->minSaturation,
                         ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->maxSaturation);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Health Server event callback.
 *
 *  \param[in] pEvt  Health Server event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerMeshHtSrEventCback(const wsfMsgHdr_t *pEvt)
{
  meshHtSrEvt_t *pHtSrEvt = (meshHtSrEvt_t *)pEvt;
  switch(pHtSrEvt->hdr.param)
  {
    case MESH_HT_SR_TEST_START_EVENT:
      /* Default behavior is to log 0 faults and just update test id. */
      MeshHtSrAddFault(pHtSrEvt->testStartEvt.elemId, pHtSrEvt->testStartEvt.companyId,
                       pHtSrEvt->testStartEvt.testId, MESH_HT_MODEL_FAULT_NO_FAULT);

      /* Check if response is needed. */
      if(pHtSrEvt->testStartEvt.notifTestEnd)
      {
        /* Signal test end. */
        MeshHtSrSignalTestEnd(pHtSrEvt->testStartEvt.elemId, pHtSrEvt->testStartEvt.companyId,
                              pHtSrEvt->testStartEvt.htClAddr, pHtSrEvt->testStartEvt.appKeyIndex,
                              pHtSrEvt->testStartEvt.useTtlZero, pHtSrEvt->testStartEvt.unicastReq);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Model event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerMmdlEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->event)
  {
    case MESH_HT_SR_EVENT:
      provisionerMeshHtSrEventCback(pEvt);
      break;

    case MMDL_GEN_ONOFF_CL_EVENT:
      provisionerProcessMmdlGenOnOffEventCback(pEvt);
      break;

    case MMDL_LIGHT_HSL_CL_EVENT:
      provisionerProcessMmdlLighHslEventCback(pEvt);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Set up the node if provisioned, otherwise start provisioning procedure.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerSetup(void)
{
  static bool_t setupComplete = FALSE;

  /* This function is called once. */
  if (setupComplete)
  {
    return;
  }

  /* Check if device is provisioned. */
  if (!MeshIsProvisioned())
  {
    /* Provisioning data of the Provisioner. */
    meshPrvData_t provisionerPrvData =
    {
      .pDevKey = provisionerDevKey,
      .pNetKey = provisionerNetKey,
      .ivIndex = 0x0000,
      .netKeyIndex = 0x0000,
      .primaryElementAddr = PROVISIONER_PRIMARY_ADDRESS,
      .flags = 0,
    };

    provCb.currNodeNetKeyIdx = provisionerPrvData.netKeyIndex;

    /* Load provisioning data. */
    MeshLoadPrvData(&provisionerPrvData);
  }

  /* Start Node. */
  MeshStartNode();

  /* Add Advertising Bearer. */
  MeshAddAdvIf(BR_ADV_SLOT);

  setupComplete = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Process messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void provisionerProcMsg(dmEvt_t *pMsg)
{
  switch(pMsg->hdr.event)
  {
    case DM_RESET_CMPL_IND:
      provisionerSetup();
      break;

    case DM_EXT_SCAN_REPORT_IND:
    case DM_SCAN_REPORT_IND:
      /* process scan report if searching for device to provision or in proxy configuration mode. */
      if (((provCb.currNodeDeviceType != PROVISIONER_PRV_NONE) &&
           (provCb.currNodeState == PROVISIONER_ST_PRV_START)) ||
          (provCb.proxyClStarted))
      {
        provisionerScanReport(pMsg);
      }
      break;

    case PROVISIONER_CCSTART_TIMER_EVENT:
      /* Begin configuration. */
      provisionerCCExecute();
      break;

    default:
      break;
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerStart(void)
{
  /* Initialize the LE Stack. */
  DmConnRegister(DM_CLIENT_ID_APP, provisionerDmCback);

  /* Register for stack callbacks. */
  DmRegister(provisionerDmCback);
  AttRegister(provisionerAttCback);

  /* Reset the device. */
  DmDevReset();

  /* Set application version. */
  AppMeshSetVersion(PROVISIONER_VERSION);

  /* Register callback. */
  MeshRegister(provisionerMeshCback);

  /* Initialize GATT Proxy */
  MeshGattProxyInit();

  /* Initialize the GATT Bearer as Client. */
  GattBearerClInit();

  /* Initialize Proxy Client */
  MeshProxyClInit();

  /* Initialize Provisioning Client. */
  MeshPrvClInit();

  /* Register Provisioning Server callback. */
  MeshPrvClRegister(provisionerMeshPrvClCback);

  /* Initialize Configuration Server. */
  MeshCfgMdlSrInit();

  /* Register Configuration Server callback. */
  MeshCfgMdlSrRegister(provisionerMeshCfgMdlSrCback);

  /* Register Mesh Configuration Client callback. */
  MeshCfgMdlClRegister(provisionerMeshCfgMdlClCback, PROVISIONER_CFG_CL_TIMEOUT);

  /* Initialize Health Server */
  MeshHtSrInit();

  /* Register callback. */
  MeshHtSrRegister(provisionerMmdlEventCback);

  /* Configure company ID to an unused one. */
  MeshHtSrSetCompanyId(0, 0, PROVISIONER_HT_SR_COMPANY_ID);

  /* Add 0 faults to update recent test ID. */
  MeshHtSrAddFault(0, 0xFFFF, PROVISIONER_HT_SR_TEST_ID, MESH_HT_MODEL_FAULT_NO_FAULT);

  /* Initialize application bearer scheduler. */
  AppBearerInit(provisionerHandlerId);

  /* Register callback for application bearer events */
  AppBearerRegister(provisionerBearerCback);

  /* Initialize the Advertising Bearer. */
  AdvBearerInit((advBearerCfg_t *)&provisionerAdvBearerCfg);

  /* Register ADV Bearer callback. */
  MeshRegisterAdvIfPduSendCback(AdvBearerSendPacket);

  AppDiscInit();

  /* Set configuration pointer. */
  pAppDiscCfg = (appDiscCfg_t *) &provisionerDiscCfg;
  pGattBearerClConnCfg = (hciConnSpec_t *) &provisionerConnCfg;

  /* Register for app framework discovery callbacks. */
  AppDiscRegister(provisionerDiscCback);
  provCb.discCback = AppDiscProcDmMsg;

  /* Initialize Provisioner state. */
  provCb.currRoomAddress = PROVISIONER_ROOM_ADDR_START;
  provCb.currNodePrimAddr = PROVISIONER_NODE_ADDR_START;
  provisionerPrvClSessionInfo.pData->address = PROVISIONER_NODE_ADDR_START;
  provisionerClearCurrNode();

  /* Disable Proxy and GATT Provisioning. */
  provCb.prvGattClStarted = FALSE;
  provCb.proxyClStarted = FALSE;

  /* TODO Proxy and GATT Provisioning */
  /* Schedule and enable GATT bearer. */
  /*AppBearerScheduleSlot(BR_GATT_SLOT, GattBearerClStart, GattBearerClStop, GattBearerClProcDmMsg,
                        5000); */
  /* Enable Proxy Client. */
  /*ProvisionerStartGattCl(TRUE, provisionerPrvClSessionInfo.pData->address); */

  /* Install model client callback. */
  MmdlGenOnOffClRegister(provisionerMmdlEventCback);
  MmdlLightHslClRegister(provisionerMmdlEventCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Application handler init function called during system initialization.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \retval    None.
 */
/*************************************************************************************************/
void ProvisionerHandlerInit(wsfHandlerId_t handlerId)
{
  APP_TRACE_INFO0("PROVISIONER: Provisioner Application Initialize");

  /* Set handler ID. */
  provisionerHandlerId = handlerId;

  /* Register empty disconnect cback. */
  provCb.discCback = provisionerDiscProcDmMsgEmpty;

  /* Initialize timer. */
  provCb.currNodeCCStartTimer.handlerId = handlerId;
  provCb.currNodeCCStartTimer.msg.event = PROVISIONER_CCSTART_TIMER_EVENT;
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize Mesh configuration for the application.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void ProvisionerConfigInit(void)
{
  /* Initialize configuration. */
  pMeshConfig = &provisionerMeshConfig;
}

/*************************************************************************************************/
/*!
 *  \brief     The WSF event handler for the Provisioner App.
 *
 *  \param[in] event  Event mask.
 *  \param[in] pMsg   Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void ProvisionerHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  (void)event;

  if (pMsg != NULL)
  {
    APP_TRACE_INFO1("PROVISIONER: App got evt %d", pMsg->event);

    /* Process ATT messages. */
    if (pMsg->event <= ATT_CBACK_END)
    {
      /* Process discovery-related ATT messages. */
      AppDiscProcAttMsg((attEvt_t *) pMsg);
    }
    else if (pMsg->event <= DM_CBACK_END)
    {
      /* Process advertising and connection-related messages. */
      AppBearerProcDmMsg((dmEvt_t *) pMsg);

      if (pMsg->status == HCI_SUCCESS)
      {
        if(pMsg->event == DM_CONN_OPEN_IND)
        {
          /* Disable GATT bearer slot while in connection */
          AppBearerDisableSlot(BR_GATT_SLOT);
        }
        else if(pMsg->event == DM_CONN_CLOSE_IND)
        {
          if (provCb.proxyClStarted || provCb.prvGattClStarted)
          {
            /* Enable GATT bearer after connection closed */
            AppBearerEnableSlot(BR_GATT_SLOT);
          }
        }
      }

      /* Process discovery-related messages. */
      provCb.discCback((dmEvt_t *) pMsg);
    }
    else if ((pMsg->event >= MESH_CBACK_START) && (pMsg->event <= MESH_CBACK_END))
    {
      /* Process Mesh message. */
      provisionerProcMeshMsg(pMsg);
    }
    else
    {
      /* Application events. */
      if (pMsg->event == APP_BR_TIMEOUT_EVT)
      {
        AppBearerSchedulerTimeout();
      }
    }

    if (provCb.proxyClStarted)
    {
      MprxcProcMsg(pMsg);
    }
    else
    {
      MprvcProcMsg(pMsg);
    }

    provisionerProcMsg((dmEvt_t *)pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Start the GATT Client feature.
 *
 *  \param[in] enableProv  Start GATT client as a Provisioner client.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void ProvisionerStartGattCl(bool_t enableProv, uint16_t newAddress)
{
  if (enableProv)
  {
    /* Using GATT for Provisioning. */
    provCb.prvGattClStarted = TRUE;
    provCb.proxyClStarted = FALSE;

    /* Set address for provisioning client session. */
    provisionerPrvClSessionInfo.pData->address = newAddress;
    pGattBearerClCfg = (gattBearerClCfg_t *)&provisionerPrvClCfg;

    /* Register GATT Bearer callback. */
    MeshRegisterGattProxyPduSendCback(MprvcSendDataIn);
  }
  else
  {
    /* Using GATT for Proxy. */
    provCb.proxyClStarted = TRUE;
    provCb.prvGattClStarted = FALSE;

    pGattBearerClCfg = (gattBearerClCfg_t *)&provisionerProxyClCfg;

    /* Register GATT Bearer callback. */
    MeshRegisterGattProxyPduSendCback(MprxcSendDataIn);
  }

  AppBearerEnableSlot(BR_GATT_SLOT);
}

/*************************************************************************************************/
/*!
 *  \brief  Provisioner application provision device.
 *
 *  \param  deviceType  device type.  See ::provisionerPrvDeviceTypes.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerProvisionDevice(provisionerPrvDevType_t deviceType)
{
  WSF_ASSERT(provCb.currNodeDeviceType == PROVISIONER_PRV_NONE);
  WSF_ASSERT(deviceType != PROVISIONER_PRV_NONE);

  /* Start Scanning or enable provisioning. */
  provCb.currNodeDeviceType = deviceType;

  /* Set configuration client state machine for this device. */
  provCb.currNodeStateMachine = provisionerCCNodeTypeSM[provCb.currNodeDeviceType];

  /* If device type is Room Switch */
  if (provCb.currNodeDeviceType == PROVISIONER_PRV_ROOM_SWITCH)
  {
    /* Provision with unique room address. */
    provCb.currRoomAddress++;

    if (provCb.currRoomAddress == MESH_ADDR_GROUP_PROXY)
    {
      /* There are no more address available! */
      provisionerCallUi(MESH_NO_RESOURCES);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Provisioner cancel ongoing provisioning.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerCancelProvisioning(void)
{
  /* Cancel timer. */
  WsfTimerStop(&provCb.currNodeCCStartTimer);

  /* Cancel provisioning if in progress. */
  if ((provCb.currNodeState == PROVISIONER_ST_PRV_ADV_INPRG) ||
      (provCb.currNodeState == PROVISIONER_ST_PRV_GATT_INPRG))
  {
    MeshPrvClCancel();
  }

  /* Clear state machine. */
  provisionerClearCurrNode();
}
