/*************************************************************************************************/
/*!
 *  \file   testapp_main.c
 *
 *  \brief  TestApp application.
 *
 *  Copyright (c) 2010-2019 Arm Ltd.
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
#include "wsf_nvm.h"
#include "wsf_buf.h"
#include "dm_api.h"
#include "util/bstream.h"
#include "util/terminal.h"

#include "att_api.h"
#include "app_api.h"
#include "app_cfg.h"
#include "svc_mprxs.h"
#include "svc_mprvs.h"
#include "mprxc/mprxc_api.h"
#include "mprvc/mprvc_api.h"
#include "mprxs/mprxs_api.h"
#include "mprvs/mprvs_api.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_replay_protection.h"
#include "mesh_local_config.h"

#include "adv_bearer.h"
#include "gatt_bearer_cl.h"
#include "gatt_bearer_sr.h"

#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"
#include "mesh_prv_cl_api.h"
#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl_cl_api.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_vendor_test_cl_api.h"
#include "mesh_ht_sr_api.h"
#include "mesh_ht_cl_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_gen_powerlevel_cl_api.h"
#include "mmdl_gen_powerlevel_sr_api.h"
#include "mmdl_gen_powerlevelsetup_sr_api.h"
#include "mmdl_gen_default_trans_cl_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_gen_battery_cl_api.h"
#include "mmdl_gen_battery_sr_api.h"
#include "mmdl_time_cl_api.h"
#include "mmdl_time_sr_api.h"
#include "mmdl_timesetup_sr_api.h"
#include "mmdl_scene_cl_api.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_light_hsl_cl_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_scheduler_cl_api.h"
#include "mmdl_scheduler_sr_api.h"
#include "mmdl_bindings_api.h"

#include "app_mesh_api.h"
#include "app_mesh_terminal.h"
#include "app_mmdl_terminal.h"
#include "app_mesh_cfg_mdl_cl_terminal.h"
#include "app_bearer.h"

#include "testapp_api.h"
#include "testapp_config.h"
#include "testapp_terminal.h"

#include "testapp_version.h"

#if defined(NRF52840_XXAA)
#include "nrf.h"
#else
static void NVIC_SystemReset(void)
{
  /* Stub */
}
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Health Server company ID registered in the instance. */
#define TESTAPP_HT_SR_COMPANY_ID              0xFFFF
/*! Health Server test ID for the associated to the test company ID */
#define TESTAPP_HT_SR_TEST_ID                 0x00

/*! Number of handles to be discovered */
#define DISC_HANDLES_NUM                      3

#if defined(_WINDOWS)
#define TESTAPP_NEWLINE
#define TESTAPP_PRINT0(msg)                   APP_TRACE_INFO0(msg)
#define TESTAPP_PRINT1(msg, var1)             APP_TRACE_INFO1(msg, var1)
#define TESTAPP_PRINT2(msg, var1, var2)       APP_TRACE_INFO2(msg, var1, var2)
#define TESTAPP_PRINT3(msg, var1, var2, var3) APP_TRACE_INFO3(msg, var1, var2, var3)
#define TESTAPP_PRINT4(msg, var1, var2, var3, var4) \
                                              APP_TRACE_INFO4(msg, var1, var2, var3, var4)
#define TESTAPP_PRINT5(msg, var1, var2, var3, var4, var5) \
                                              APP_TRACE_INFO5(msg, var1, var2, var3, var4, var5)
#define TESTAPP_PRINT6(msg, var1, var2, var3, var4, var5, var6) \
                                              APP_TRACE_INFO6(msg, var1, var2, var3, var4, var5, var6)
#define TESTAPP_PRINT7(msg, var1, var2, var3, var4, var5, var6, var7) \
                                              APP_TRACE_INFO7(msg, var1, var2, var3, var4, var5, var6, var7)
#define TESTAPP_PRINT8(msg, var1, var2, var3, var4, var5, var6, var7, var8) \
                                              APP_TRACE_INFO8(msg, var1, var2, var3, var4, var5, var6, var7, var8)
#define TESTAPP_PRINT9(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9) \
                                              APP_TRACE_INFO9(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9)
#define TESTAPP_PRINT12(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12) \
                                              APP_TRACE_INFO12(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12)
#else
#define TESTAPP_NEWLINE                       TERMINAL_STRING_NEW_LINE
#define TESTAPP_PRINT0(msg)                   TerminalTxStr(msg)
#define TESTAPP_PRINT1(msg, var1)             TerminalTxPrint(msg, var1)
#define TESTAPP_PRINT2(msg, var1, var2)       TerminalTxPrint(msg, var1, var2)
#define TESTAPP_PRINT3(msg, var1, var2, var3) TerminalTxPrint(msg, var1, var2, var3)
#define TESTAPP_PRINT4(msg, var1, var2, var3, var4) \
                                              TerminalTxPrint(msg, var1, var2, var3, var4)
#define TESTAPP_PRINT5(msg, var1, var2, var3, var4, var5) \
                                              TerminalTxPrint(msg, var1, var2, var3, var4, var5)
#define TESTAPP_PRINT6(msg, var1, var2, var3, var4, var5, var6) \
                                              TerminalTxPrint(msg, var1, var2, var3, var4, var5, var6)
#define TESTAPP_PRINT7(msg, var1, var2, var3, var4, var5, var6, var7) \
                                              TerminalTxPrint(msg, var1, var2, var3, var4, var5, var6, var7)
#define TESTAPP_PRINT8(msg, var1, var2, var3, var4, var5, var6, var7, var8) \
                                              TerminalTxPrint(msg, var1, var2, var3, var4, var5, var6, var7, var8)
#define TESTAPP_PRINT9(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9) \
                                              TerminalTxPrint(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9)
#define TESTAPP_PRINT12(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12) \
                                              TerminalTxPrint(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12)
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Enumeration of client characteristic configuration descriptors */
enum
{
  TESTAPP_DOUT_CCC_IDX,      /*! Mesh Proxy/Provisioning service, Data Out */
  TESTAPP_NUM_CCC_IDX
};

/*! TestApp control block structure */
typedef struct testAppCb_tag
{
  dmCback_t     discCback;                  /*!< GATT discovery callback */
  wsfTimer_t    nodeIdentityTmr;            /*!< WSF Timer for Node Identity timeout */
  uint16_t      hdlList[DISC_HANDLES_NUM];  /*!< Handles discovered by the GATT client */
  uint16_t      netKeyIndexAdv;             /*!< Net Key Index used for GATT advertising */
  bool_t        nodeIdentityRunning;        /*!< TRUE if Node Identity is started
                                             *   FALSE otherwise */
  bool_t        proxyFeatEnabled;           /*!< TRUE if GATT Proxy Server is enabled
                                             *   FALSE otherwise */
  bool_t        prvSrStarted;               /*!< TRUE if Provisioning Server is started
                                             *   FALSE otherwise */
  bool_t        prvClStarted;               /*!< TRUE if Provisioning Client is started
                                             *   FALSE otherwise */
  bool_t        proxySrStarted;             /*!< TRUE if GATT Proxy Server is started
                                             *   FALSE otherwise */
  bool_t        proxyClStarted;             /*!< TRUE if GATT Proxy Client is started
                                             *   FALSE otherwise */
  bool_t        brGattSrStarted;            /*!< TRUE if GATT Server is started
                                             *   FALSE otherwise */
  bool_t        brGattClStarted;            /*!< TRUE if GATT Client is started
                                             *   FALSE otherwise */
} testAppCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief  ATT Client Data */

/*! Configurable parameters for service and characteristic discovery */
static const appDiscCfg_t testAppDiscCfg =
{
  FALSE                   /*! TRUE to wait for a secure connection before initiating discovery */
};

/*! \brief  ATT Client Configuration Data after service discovery */

/*! Default value for CCC notifications */
const uint8_t dataOutCccNtfVal[2] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_NOTIFY)};

/*! List of characteristics to configure after service discovery */
static const attcDiscCfg_t discCfgList[] =
{
  /*! Write: Data Out ccc descriptor */
  {dataOutCccNtfVal, sizeof(dataOutCccNtfVal), (MPRXC_MPRXS_DOUT_CCC_HDL_IDX)}
};

/*! Characteristic configuration list length */
#define MESH_SVC_DISC_CFG_LIST_LEN   (sizeof(discCfgList) / sizeof(attcDiscCfg_t))

/*! \brief  Client Characteristic Configuration Descriptors */

/*! Client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t testAppPrvCccSet[TESTAPP_NUM_CCC_IDX] =
{
  /*! TESTAPP_MPRVS_CPM_CCC_IDX */
  {
    MPRVS_DOUT_CH_CCC_HDL,       /*!< CCCD handle */
    ATT_CLIENT_CFG_NOTIFY,       /*!< Value range */
    DM_SEC_LEVEL_NONE            /*!< Security level */
  },
};

/*! Client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t testAppPrxCccSet[TESTAPP_NUM_CCC_IDX] =
{
  /*! TESTAPP_MPRXS_CPM_CCC_IDX */
  {
    MPRXS_DOUT_CH_CCC_HDL,       /*!< CCCD handle */
    ATT_CLIENT_CFG_NOTIFY,       /*!< Value range */
    DM_SEC_LEVEL_NONE            /*!< Security level */
  },
};

/*! WSF handler ID */
static wsfHandlerId_t testAppHandlerId;

/*! WSF Timer for Node Reset timeout */
static wsfTimer_t testAppNodeRstTmr;

/*! Buffer used to print the received message */
static char printBuf[1024];

/*! TestApp control block */
static testAppCb_t testAppCb;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
/*! Mesh Stack Test mode control block */
meshTestCb_t  meshTestCb;
#endif

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Application Discovery Process message callback.
 *
 *  \param[in] pDmEvt  DM callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppDiscProcDmMsgEmpty(dmEvt_t *pDmEvt)
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
static void testAppDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t *pMsg;
  uint16_t len;

  len = DmSizeOfEvt(pDmEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pDmEvt, len);
    WsfMsgSend(testAppHandlerId, pMsg);
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
static void testAppMeshCfgMdlClCback(meshCfgMdlClEvt_t *pEvt)
{
  meshCfgMdlClEvt_t *pMsg;
  uint16_t len;

  len = MeshCfgSizeOfEvt((wsfMsgHdr_t *) pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    if (MeshCfgMsgDeepCopy((wsfMsgHdr_t *) pMsg, (wsfMsgHdr_t *) pEvt))
    {
      WsfMsgSend(testAppHandlerId, pMsg);
    }
    else
    {
      WsfMsgFree(pMsg);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Notification callback triggered by Configuration Server.
 *
 *  \param[in] pEvt  Pointer to the event structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppMeshCfgMdlSrCback(const meshCfgMdlSrEvt_t *pEvt)
{
  meshCfgMdlSrEvt_t *pMsg;
  uint16_t len;

  len = MeshCfgSizeOfEvt((wsfMsgHdr_t *) pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    if (MeshCfgMsgDeepCopy((wsfMsgHdr_t *) pMsg, (wsfMsgHdr_t *) pEvt))
    {
      WsfMsgSend(testAppHandlerId, pMsg);
    }
    else
    {
      WsfMsgFree(pMsg);
    }
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
static void testAppMeshCback(meshEvt_t *pEvt)
{
  meshEvt_t *pMsg;
  uint16_t len;

  len = MeshSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(testAppHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application Mesh Provisioning Server callback.
 *
 *  \param[in] pEvt  Mesh Provisioning Server callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppMeshPrvSrCback(meshPrvSrEvt_t *pEvt)
{
  meshPrvSrEvt_t *pMsg;
  uint16_t len;

  len = MeshPrvSrSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(testAppHandlerId, pMsg);
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
static void testAppMeshPrvClCback(meshPrvClEvt_t *pEvt)
{
  meshPrvClEvt_t *pMsg;
  uint16_t len;

  len = MeshPrvClSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(testAppHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application Mesh Stack Test callback.
 *
 *  \param[in] pEvt  Mesh Stack Test callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
static void testAppMeshTestCback(meshTestEvt_t *pEvt)
{
  uint16_t i;

  switch (pEvt->hdr.param)
  {
  case MESH_TEST_PB_LINK_CLOSED_IND:
    TESTAPP_PRINT0("prvbr_ind link_closed" TESTAPP_NEWLINE);
    break;

  case MESH_TEST_PB_INVALID_OPCODE_IND:
    TESTAPP_PRINT1("prvbr_ind invalid_opcode opcode=0x%x" TESTAPP_NEWLINE,
                   (((meshTestPbInvalidOpcodeInd_t *)pEvt)->opcode));
    break;

  case MESH_TEST_NWK_PDU_RCVD_IND:
    for (i = 0; i < ((meshTestNwkPduRcvdInd_t *)pEvt)->pduLen; i++)
    {
      sprintf(&printBuf[2 * i], "%02x", (((meshTestNwkPduRcvdInd_t *)pEvt)->pLtrPdu[i]));
    }

    TESTAPP_PRINT7("nwk_ind nid=0x%x src=0x%x dst=0x%x ttl=0x%x ctl=0x%x pdulen=%d pdu=0x%s"
                   TESTAPP_NEWLINE,
                   ((meshTestNwkPduRcvdInd_t *)pEvt)->nid,
                   ((meshTestNwkPduRcvdInd_t *)pEvt)->src,
                   ((meshTestNwkPduRcvdInd_t *)pEvt)->dst,
                   ((meshTestNwkPduRcvdInd_t *)pEvt)->ttl,
                   ((meshTestNwkPduRcvdInd_t *)pEvt)->ctl,
                   ((meshTestNwkPduRcvdInd_t *)pEvt)->pduLen,
                   printBuf);
    break;

  case MESH_TEST_SAR_RX_TIMEOUT_IND:
    TESTAPP_PRINT1("sar_ind rx_timeout srcaddr=0x%x" TESTAPP_NEWLINE,
                   (((meshTestSarRxTimeoutInd_t *)pEvt)->srcAddr));
    break;

  case MESH_TEST_UTR_ACC_PDU_RCVD_IND:
    for (i = 0; i < ((meshTestUtrAccPduRcvdInd_t *)pEvt)->pduLen; i++)
    {
      sprintf(&printBuf[2 * i], "%02x", (((meshTestUtrAccPduRcvdInd_t *)pEvt)->pAccPdu[i]));
    }

    TESTAPP_PRINT7("utr_ind acc src=0x%x dst=0x%x ttl=0x%x aidx=0x%x nidx=0x%x pdulen=%d pdu=0x%s"
                   TESTAPP_NEWLINE,
                   ((meshTestUtrAccPduRcvdInd_t *)pEvt)->src,
                   ((meshTestUtrAccPduRcvdInd_t *)pEvt)->dst,
                   ((meshTestUtrAccPduRcvdInd_t *)pEvt)->ttl,
                   ((meshTestUtrAccPduRcvdInd_t *)pEvt)->devKeyUse ?
                    0xFFFF : ((meshTestUtrAccPduRcvdInd_t *)pEvt)->appKeyIndex,
                   ((meshTestUtrAccPduRcvdInd_t *)pEvt)->netKeyIndex,
                   ((meshTestUtrAccPduRcvdInd_t *)pEvt)->pduLen,
                   printBuf);
    break;

  case MESH_TEST_UTR_CTL_PDU_RCVD_IND:
    for (i = 0; i < ((meshTestUtrCtlPduRcvdInd_t *)pEvt)->pduLen; i++)
    {
      sprintf(&printBuf[2 * i], "%02x", (((meshTestUtrCtlPduRcvdInd_t *)pEvt)->pUtrCtlPdu[i]));
    }

    TESTAPP_PRINT7("utr_ind ctl src=0x%x dst=0x%x ttl=0x%x nidx=0x%x opcode=0x%x pdulen=%d pdu=0x%s"
                   TESTAPP_NEWLINE,
                   ((meshTestUtrCtlPduRcvdInd_t *)pEvt)->src,
                   ((meshTestUtrCtlPduRcvdInd_t *)pEvt)->dst,
                   ((meshTestUtrCtlPduRcvdInd_t *)pEvt)->ttl,
                   ((meshTestUtrCtlPduRcvdInd_t *)pEvt)->netKeyIndex,
                   ((meshTestUtrCtlPduRcvdInd_t *)pEvt)->opcode,
                   ((meshTestUtrCtlPduRcvdInd_t *)pEvt)->pduLen,
                   printBuf);
    break;

  case MESH_TEST_PROXY_PDU_RCVD_IND:
    for (i = 0; i < ((meshTestProxyCfgPduRcvdInd_t *)pEvt)->pduLen; i++)
    {
      sprintf(&printBuf[2 * i], "%02x", (((meshTestProxyCfgPduRcvdInd_t *)pEvt)->pPdu[i]));
    }

    TESTAPP_PRINT3("proxy_ind pduType=0x%x pdulen=%d pdu=0x%s" TESTAPP_NEWLINE,
                   ((meshTestProxyCfgPduRcvdInd_t *)pEvt)->pduType,
                   ((meshTestProxyCfgPduRcvdInd_t *)pEvt)->pduLen,
                   printBuf);
    break;

  case MESH_TEST_SEC_NWK_BEACON_RCVD_IND:
    for (i = 0; i < MESH_NWK_ID_NUM_BYTES; i++)
    {
      sprintf(&printBuf[2 * i], "%02x", (((meshTestSecNwkBeaconRcvdInd_t *)pEvt)->networkId[i]));
    }

    TESTAPP_PRINT4("snb_ind IVU=0x%x KR=0x%x IVI=0x%04x NETID=0x%s" TESTAPP_NEWLINE,
                   ((meshTestSecNwkBeaconRcvdInd_t *)pEvt)->ivUpdate,
                   ((meshTestSecNwkBeaconRcvdInd_t *)pEvt)->keyRefresh,
                   ((meshTestSecNwkBeaconRcvdInd_t *)pEvt)->ivi,
                   printBuf);
    break;

  case MESH_TEST_MPRVS_WRITE_INVALID_RCVD_IND:
    for (i = 0; i < ((meshTestMprvsWriteInvalidRcvdInd_t *)pEvt)->len; i++)
    {
      sprintf(&printBuf[2 * i], "%02x", (((meshTestMprvsWriteInvalidRcvdInd_t *)pEvt)->pValue[i]));
    }

    TerminalTxPrint("mps_ind invalid_data hdl=0x%x len=%d val=0x%s" TERMINAL_STRING_NEW_LINE,
                    ((meshTestMprvsWriteInvalidRcvdInd_t *)pEvt)->handle,
                    ((meshTestMprvsWriteInvalidRcvdInd_t *)pEvt)->len,
                    printBuf);
    break;

  default:
    break;
  }
}
#endif

/*************************************************************************************************/
/*!
 *  \brief     Application ATTS client characteristic configuration callback.
 *
 *  \param[in] pDmEvt  DM callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppCccCback(attsCccEvt_t *pEvt)
{
  attsCccEvt_t  *pMsg;
  appDbHdl_t    dbHdl;

  /* If CCC not set from initialization and there's a device record. */
  if ((pEvt->handle != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl((dmConnId_t)pEvt->hdr.param)) != APP_DB_HDL_NONE))
  {
    /* Store value in device database. */
    AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
  }

  if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
    WsfMsgSend(testAppHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Core messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcMeshCoreMsg(meshEvt_t *pMsg)
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

        APP_TRACE_INFO0("TESTAPP: ADV Interface added");
      }
      else
      {
        APP_TRACE_ERR1("TESTAPP: ADV Interface add error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_REMOVE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        /* Unregister advertising interface from bearer. */
        AdvBearerDeregisterIf();

        /* Disable ADV bearer scheduling. */
        AppBearerDisableSlot(BR_ADV_SLOT);

        APP_TRACE_INFO0("TESTAPP: ADV Interface removed");
      }
      else
      {
        APP_TRACE_ERR1("TESTAPP: ADV Interface remove error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_CLOSE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        APP_TRACE_INFO0("TESTAPP: ADV Interface closed");
      }
      else
      {
        APP_TRACE_ERR1("TESTAPP: ADV Interface close error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_GATT_CONN_ADD_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
         APP_TRACE_INFO0("TESTAPP: GATT Interface added");
         TESTAPP_PRINT1("gatt_ind added connid=%d" TESTAPP_NEWLINE, pMsg->gattConn.connId);

         if (!testAppCb.proxyClStarted && !testAppCb.proxySrStarted)
         {
           /* Begin provisioning. */
           if (testAppCb.brGattSrStarted)
           {
             MeshPrvSrEnterPbGattProvisioningMode(pMsg->gattConn.connId);
           }
           else if (testAppCb.brGattClStarted)
           {
             MeshPrvClStartPbGattProvisioning(pMsg->gattConn.connId, &testAppPrvClSessionInfo);
           }
         }
         else if (testAppCb.nodeIdentityRunning)
         {
           /* Stop Node Identity timer. */
           WsfTimerStop(&testAppCb.nodeIdentityTmr);

           /* Stop Node Identity ADV. */
           testAppCb.nodeIdentityRunning = FALSE;
         }
      }
      else
      {
        APP_TRACE_ERR1("TESTAPP: GATT Interface add error, %d", pMsg->hdr.status);
      }
      break;
    case MESH_CORE_GATT_CONN_CLOSE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        TESTAPP_PRINT1("gatt_ind close connid=%d" TESTAPP_NEWLINE, pMsg->gattConn.connId);
        /* Disconnect from peer. */
        AppConnClose(pMsg->gattConn.connId);
      }
    break;

    case MESH_CORE_GATT_CONN_REMOVE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        TESTAPP_PRINT1("gatt_ind removed connid=%d" TESTAPP_NEWLINE, pMsg->gattConn.connId);

        if (testAppCb.brGattSrStarted && testAppCb.prvSrStarted && MeshIsProvisioned())
        {
          /* We are provisioned. Remove the Mesh Provisioning Service. */
          SvcMprvsRemoveGroup();

          testAppCb.prvSrStarted = FALSE;

          /* Register the Mesh Proxy Service. */
          SvcMprxsRegister(MprxsWriteCback);

          /* Add the Mesh Proxy Service. */
          SvcMprxsAddGroup();

          /* Register Mesh Proxy Service CCC*/
          AttsCccRegister(TESTAPP_NUM_CCC_IDX, (attsCccSet_t *) testAppPrxCccSet, testAppCccCback);

          /* Configure GATT server for Mesh Proxy. */
          MprxsSetCccIdx(TESTAPP_DOUT_CCC_IDX);

          /* Register GATT Bearer callback */
          MeshRegisterGattProxyPduSendCback(MprxsSendDataOut);

          /* Start advertising with node identity on the primary subnet. */
          MeshProxySrGetServiceData(testAppCb.netKeyIndexAdv, MESH_PROXY_NODE_IDENTITY_TYPE);

          testAppCb.proxySrStarted = TRUE;
          testAppCb.nodeIdentityRunning = TRUE;
        }
      }
      else
      {
        APP_TRACE_ERR1("TESTAPP: GATT Interface close/remove error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ATTENTION_CHG_EVENT:
      if (pMsg->attention.attentionOn)
      {
        TESTAPP_PRINT1("mesh_ind attention=on elemid=%d" TESTAPP_NEWLINE,
                       pMsg->attention.elementId);
      }
      else
      {
        TESTAPP_PRINT1("mesh_ind attention=off elemid=%d" TESTAPP_NEWLINE,
                       pMsg->attention.elementId);
      }
      break;

    case MESH_CORE_NODE_STARTED_EVENT:
      if(pMsg->nodeStarted.hdr.status == MESH_SUCCESS)
      {
        TESTAPP_PRINT2(TESTAPP_NEWLINE "mesh_ind node_started elemaddr=0x%x elemcnt=%d"
                       TESTAPP_NEWLINE, pMsg->nodeStarted.address, pMsg->nodeStarted.elemCnt);

        /* Bind the interface. */
        MeshAddAdvIf(TESTAPP_ADV_IF_ID);

        /* OnPowerUp procedure must called after states and binding restoration. To ensure
         * models publish state changes the node must be started and an interface must exist.
         */
        MmdlGenPowOnOffOnPowerUp();
      }
      else
      {
        TESTAPP_PRINT0(TESTAPP_NEWLINE "mesh_ind node_started failed"
                       TESTAPP_NEWLINE);
      }
      break;

    case MESH_CORE_PROXY_SERVICE_DATA_EVENT:

      if (pMsg->serviceData.serviceDataLen != 0)
      {
        /* Set ADV data for a proxy server */
        GattBearerSrSetPrxSvcData(pMsg->serviceData.serviceData, pMsg->serviceData.serviceDataLen);
      }
      break;

    case MESH_CORE_PROXY_FILTER_STATUS_EVENT:
      TESTAPP_PRINT2("mesh_ind proxy_filter type=%d, list_size=%d" TESTAPP_NEWLINE,
                     pMsg->filterStatus.filterType, pMsg->filterStatus.listSize);
      break;

    case MESH_CORE_IV_UPDATED_EVENT:
      TESTAPP_PRINT1(TESTAPP_NEWLINE "mesh_ind ividx=0x%x" TESTAPP_NEWLINE,
                     pMsg->ivUpdt.ivIndex);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Configuration Client messages from the event handler.
 *
 *  \param[in] pEvt  Configuration Server event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcMeshCfgMdlSrMsg(meshCfgMdlSrEvt_t *pEvt)
{
  switch(pEvt->hdr.param)
  {
    case MESH_CFG_MDL_GATT_PROXY_SET_EVENT:
      /* Check if GATT Proxy Server was initialized. */
      if (testAppCb.brGattSrStarted)
      {
        /* Stop Node Identity timer. */
        WsfTimerStop(&testAppCb.nodeIdentityTmr);

        if (pEvt->gattProxy.gattProxy == MESH_GATT_PROXY_FEATURE_ENABLED)
        {
          if (!testAppCb.proxySrStarted)
          {
            /* Register the Mesh Proxy Service. */
            SvcMprxsRegister(MprxsWriteCback);

            /* Add the Mesh Proxy Service. */
            SvcMprxsAddGroup();

            /* Register Mesh Proxy Service CCC*/
            AttsCccRegister(TESTAPP_NUM_CCC_IDX, (attsCccSet_t *) testAppPrxCccSet, testAppCccCback);

            /* Configure GATT server for Mesh Proxy. */
            MprxsSetCccIdx(TESTAPP_DOUT_CCC_IDX);

            /* Register GATT Bearer callback */
            MeshRegisterGattProxyPduSendCback(MprxsSendDataOut);

            /* Using GATT for Proxy. */
            testAppCb.proxySrStarted = TRUE;
          }

          MeshProxySrGetNextServiceData(MESH_PROXY_NWK_ID_TYPE);

          testAppCb.netKeyIndexAdv = 0xFFFF;
          testAppCb.proxyFeatEnabled = TRUE;
          testAppCb.nodeIdentityRunning = FALSE;

          /* Enable bearer slot */
          AppBearerEnableSlot(BR_GATT_SLOT);
        }
        else if (pEvt->gattProxy.gattProxy == MESH_GATT_PROXY_FEATURE_DISABLED)
        {
          testAppCb.proxyFeatEnabled = FALSE;

          /* Disable bearer slot */
          AppBearerDisableSlot(BR_GATT_SLOT);
        }
      }
      break;
    case MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT:

      /* Check if GATT Proxy Server was initialized. */
      if (testAppCb.brGattSrStarted)
      {
        if (pEvt->nodeIdentity.state == MESH_NODE_IDENTITY_RUNNING)
        {
          /* Get Service Data for the specified netkey index */
          MeshProxySrGetServiceData(pEvt->nodeIdentity.netKeyIndex, MESH_PROXY_NODE_IDENTITY_TYPE);
          testAppCb.netKeyIndexAdv = pEvt->nodeIdentity.netKeyIndex;
          testAppCb.nodeIdentityRunning = TRUE;

          /* Start Node Identity timer. */
          WsfTimerStartMs(&testAppCb.nodeIdentityTmr, APP_MESH_NODE_IDENTITY_TIMEOUT_MS);

          /* Enable bearer slot */
          AppBearerEnableSlot(BR_GATT_SLOT);
        }
        else if (pEvt->nodeIdentity.state == MESH_NODE_IDENTITY_STOPPED)
        {
          /* Stop Node Identity timer. */
          WsfTimerStop(&testAppCb.nodeIdentityTmr);

          /* Node Identity stopped */
          MeshProxySrGetNextServiceData(MESH_PROXY_NWK_ID_TYPE);
          testAppCb.netKeyIndexAdv = 0xFFFF;
          testAppCb.nodeIdentityRunning = FALSE;

          /* Check if Proxy is started */
          if (!testAppCb.proxyFeatEnabled)
          {
            /* Disable bearer slot */
            AppBearerDisableSlot(BR_GATT_SLOT);
          }
        }
      }
      break;
    case MESH_CFG_MDL_NODE_RESET_EVENT:
      /* Start Node Reset timer. */
      WsfTimerStartMs(&testAppNodeRstTmr, APP_MESH_NODE_RST_TIMEOUT_MS);
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Configuration Client messages from the event handler.
 *
 *  \param[in] pEvt  Pointer to the event structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcMeshCfgMdlClMsg(meshCfgMdlClEvt_t* pEvt)
{
  appMeshCfgMdlClTerminalProcMsg(pEvt);
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Provisioning Server messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcMeshPrvSrMsg(const meshPrvSrEvt_t *pMsg)
{
  meshPrvData_t prvData;
  char alphanumericOob[1 + MESH_PRV_INOUT_OOB_MAX_SIZE];

  switch (pMsg->hdr.param)
  {
    case MESH_PRV_SR_LINK_OPENED_EVENT:
      TESTAPP_PRINT0("prvsr_ind link_opened" TESTAPP_NEWLINE);
      break;

    case MESH_PRV_SR_OUTPUT_OOB_EVENT:
      if (pMsg->outputOob.outputOobSize == 0)
      {
        /* Output is numeric */
        TESTAPP_PRINT1("prvsr_ind output_oob num=%d" TESTAPP_NEWLINE,
                       pMsg->outputOob.outputOobData.numericOob);
      }
      else if (pMsg->outputOob.outputOobSize <= MESH_PRV_INOUT_OOB_MAX_SIZE)
      {
        /* Output is alphanumeric */
        memcpy(alphanumericOob, pMsg->outputOob.outputOobData.alphanumericOob,
               pMsg->outputOob.outputOobSize);
        alphanumericOob[pMsg->outputOob.outputOobSize] = 0;

        TESTAPP_PRINT1("prvsr_ind output_oob alpha=%s" TESTAPP_NEWLINE,
                       alphanumericOob);
      }
      break;

    case MESH_PRV_SR_OUTPUT_CONFIRMED_EVENT:
      TESTAPP_PRINT0("prvsr_ind output_confirmed" TESTAPP_NEWLINE);
      break;

    case MESH_PRV_SR_INPUT_OOB_EVENT:
      TESTAPP_PRINT1("prvsr_ind input_oob type=%s" TESTAPP_NEWLINE,
                     ((pMsg->inputOob.inputOobAction == MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM) ?
                      "alpha" : "num"));
      break;

    case MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT:
      /* Stop PB-ADV provisioning loop. */
      pMeshPrvSrCfg->pbAdvRestart = FALSE;

      /* Store Provisioning NetKey index. */
      testAppCb.netKeyIndexAdv = ((meshPrvSrEvtPrvComplete_t *)pMsg)->netKeyIndex;

      prvData.pDevKey = ((meshPrvSrEvtPrvComplete_t *)pMsg)->devKey;
      prvData.pNetKey = ((meshPrvSrEvtPrvComplete_t *)pMsg)->netKey;
      prvData.primaryElementAddr = ((meshPrvSrEvtPrvComplete_t *)pMsg)->address;
      prvData.ivIndex = ((meshPrvSrEvtPrvComplete_t *)pMsg)->ivIndex;
      prvData.netKeyIndex = ((meshPrvSrEvtPrvComplete_t *)pMsg)->netKeyIndex;
      prvData.flags = ((meshPrvSrEvtPrvComplete_t *)pMsg)->flags;

      /* Load provisioning data. */
      MeshLoadPrvData(&prvData);

      /* Start Node. */
      MeshStartNode();

      TESTAPP_PRINT1("prvsr_ind prv_complete elemaddr=0x%x" TESTAPP_NEWLINE,
                     prvData.primaryElementAddr);
      break;

    case MESH_PRV_SR_PROVISIONING_FAILED_EVENT:
      TESTAPP_PRINT1("prvsr_ind prv_failed reason=0x%x" TESTAPP_NEWLINE,
                     pMsg->prvFailed.reason);

      /* Re-enter provisioning mode. */
      if (pMeshPrvSrCfg->pbAdvRestart)
      {
        MeshPrvSrEnterPbAdvProvisioningMode(pMeshPrvSrCfg->pbAdvIfId, pMeshPrvSrCfg->pbAdvInterval);
        TESTAPP_PRINT0("prvsr_ind prv_restarted" TESTAPP_NEWLINE);
      }
      break;

    default:
      break;
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
static void testAppProcMeshPrvClMsg(const meshPrvClEvt_t *pMsg)
{
  char alphanumericOob[1 + MESH_PRV_INOUT_OOB_MAX_SIZE];
  char devKeyStr[2 * MESH_KEY_SIZE_128 + 1];
  uint8_t i;

  switch (pMsg->hdr.param)
  {
    case MESH_PRV_CL_LINK_OPENED_EVENT:
      TESTAPP_PRINT0("prvcl_ind link_opened" TESTAPP_NEWLINE);
      break;

    case MESH_PRV_CL_RECV_CAPABILITIES_EVENT:
      TESTAPP_PRINT8("prvcl_ind capabilities num_elem=%d algo=0x%x oobpk=0x%x static_oob=0x%x "
                     "output_oob_size=0x%x output_oob_act=0x%x input_oob_size=0x%x "
                     "input_oob_action=0x%x" TESTAPP_NEWLINE,
                     pMsg->recvCapab.capabilities.numOfElements,
                     pMsg->recvCapab.capabilities.algorithms,
                     pMsg->recvCapab.capabilities.publicKeyType,
                     pMsg->recvCapab.capabilities.staticOobType,
                     pMsg->recvCapab.capabilities.outputOobSize,
                     pMsg->recvCapab.capabilities.outputOobAction,
                     pMsg->recvCapab.capabilities.inputOobSize,
                     pMsg->recvCapab.capabilities.inputOobAction);
      break;

    case MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT:
      TESTAPP_PRINT1("prvcl_ind enter_output_oob type=%s" TESTAPP_NEWLINE,
                     ((pMsg->enterOutputOob.outputOobAction ==
                      MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM) ? "alpha" : "num"));
      break;

    case MESH_PRV_CL_DISPLAY_INPUT_OOB_EVENT:
      if (pMsg->inputOob.inputOobSize == 0)
      {
        /* Input OOB is numeric */
        TESTAPP_PRINT1("prvcl_ind display_input_oob num=%d" TESTAPP_NEWLINE,
                       pMsg->inputOob.inputOobData.numericOob);
      }
      else if (pMsg->inputOob.inputOobSize <= MESH_PRV_INOUT_OOB_MAX_SIZE)
      {
        /* Input OOB is alphanumeric */
        memcpy(alphanumericOob, pMsg->inputOob.inputOobData.alphanumericOob,
               pMsg->inputOob.inputOobSize);
        alphanumericOob[pMsg->inputOob.inputOobSize] = 0;

        TESTAPP_PRINT1("prvcl_ind display_input_oob alpha=%s" TESTAPP_NEWLINE,
                       alphanumericOob);
      }
      break;

    case MESH_PRV_CL_PROVISIONING_COMPLETE_EVENT:

      for (i = 0; i < MESH_PRV_DEVICE_UUID_SIZE; i++)
      {
        sprintf(&devKeyStr[2 * i], "%02x", pMsg->prvComplete.devKey[i]);
      }

      TESTAPP_PRINT3("prvcl_ind prv_complete elemaddr=0x%x elemcnt=%d devkey=0x%s" TESTAPP_NEWLINE,
                     pMsg->prvComplete.address, pMsg->prvComplete.numOfElements, devKeyStr);
      break;

    case MESH_PRV_CL_PROVISIONING_FAILED_EVENT:
      TESTAPP_PRINT1("prvcl_ind prv_failed reason=0x%x" TESTAPP_NEWLINE,
                     pMsg->prvFailed.reason);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcMeshMsg(wsfMsgHdr_t *pMsg)
{
  switch (pMsg->event)
  {
    case MESH_CORE_EVENT:
      testAppProcMeshCoreMsg((meshEvt_t *) pMsg);
      break;

    case MESH_CFG_MDL_CL_EVENT:
      testAppProcMeshCfgMdlClMsg((meshCfgMdlClEvt_t *) pMsg);
      break;

    case MESH_CFG_MDL_SR_EVENT:
      testAppProcMeshCfgMdlSrMsg((meshCfgMdlSrEvt_t *) pMsg);
      break;

    case MESH_PRV_CL_EVENT:
      testAppProcMeshPrvClMsg((meshPrvClEvt_t *) pMsg);
      break;

    case MESH_PRV_SR_EVENT:
      testAppProcMeshPrvSrMsg((meshPrvSrEvt_t *) pMsg);
      break;

    default:
      break;
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
static void testAppDiscCback(dmConnId_t connId, uint8_t status)
{
  switch(status)
  {
    case APP_DISC_INIT:
      /* Set handle list when initialization requested. */
      AppDiscSetHdlList(connId, DISC_HANDLES_NUM, testAppCb.hdlList);
      break;

    case APP_DISC_READ_DATABASE_HASH:
      /* Placeholder for Disocvering peer Database hash */
      /* Fallthrough */
    case APP_DISC_START:
      /* Discover service. */
      if (testAppCb.proxyClStarted)
      {
        MprxcMprxsDiscover(connId, testAppCb.hdlList);
      }
      else if (testAppCb.prvClStarted)
      {
        MprvcMprvsDiscover(connId, testAppCb.hdlList);
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

        TESTAPP_PRINT2("svc disc_ind start_hdl=0x%X end_hdl=0x%X" TESTAPP_NEWLINE,
                       startHandle, endHandle);

        /* Discovery complete. */
        AppDiscComplete(connId, APP_DISC_CMPL);

        if (testAppCb.proxyClStarted)
        {
          TESTAPP_PRINT3("disc_ind mesh_prx data_in_hdl=0x%x data_out_hdl=0x%x data_out_cccd_hdl=0x%x"
                         TESTAPP_NEWLINE, testAppCb.hdlList[0], testAppCb.hdlList[1],
                         testAppCb.hdlList[2]);
        }
        else if (testAppCb.prvClStarted)
        {
          TESTAPP_PRINT3("disc_ind mesh_prv data_in_hdl=0x%x data_out_hdl=0x%x data_out_cccd_hdl=0x%x"
                         TESTAPP_NEWLINE, testAppCb.hdlList[0], testAppCb.hdlList[1],
                         testAppCb.hdlList[2]);
        }

        /* Start configuration. */
        AppDiscConfigure(connId, APP_DISC_CFG_START, MESH_SVC_DISC_CFG_LIST_LEN,
                         (attcDiscCfg_t *) discCfgList, MESH_SVC_DISC_CFG_LIST_LEN, testAppCb.hdlList);
      }
      break;

    case APP_DISC_CFG_START:
      /* Start configuration. */
      AppDiscConfigure(connId, APP_DISC_CFG_START, MESH_SVC_DISC_CFG_LIST_LEN,
                       (attcDiscCfg_t *) discCfgList, MESH_SVC_DISC_CFG_LIST_LEN, testAppCb.hdlList);
      break;

    case APP_DISC_CFG_CMPL:
      AppDiscComplete(connId, status);

      if (testAppCb.proxyClStarted)
      {
        MprxcSetHandles(connId, testAppCb.hdlList[0], testAppCb.hdlList[1]);
      }
      else if (testAppCb.prvClStarted)
      {
        MprvcSetHandles(connId, testAppCb.hdlList[0], testAppCb.hdlList[1]);
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
static void testAppAttCback(attEvt_t *pEvt)
{
  attEvt_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attEvt_t));
    pMsg->pValue = (uint8_t *) (pMsg + 1);
    memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
    WsfMsgSend(testAppHandlerId, pMsg);
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
static void testAppBearerCback(uint8_t slot)
{
  meshProxyIdType_t idType;

  /* Switch ADV Data only on Proxy */
  if ((slot == BR_GATT_SLOT) && MeshIsProvisioned())
  {
    idType = (testAppCb.nodeIdentityRunning) ? MESH_PROXY_NODE_IDENTITY_TYPE : MESH_PROXY_NWK_ID_TYPE;

    if (testAppCb.netKeyIndexAdv == 0xFFFF)
    {
      /* No specific netKey is used for advertising. Cycle through all.*/
      MeshProxySrGetNextServiceData(idType);
    }
    else
    {
      /* Advertise only on the specified netKey .*/
      MeshProxySrGetServiceData(testAppCb.netKeyIndexAdv, idType);
    }
  }
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
static bool_t testAppCheckServiceUuid(dmEvt_t *pMsg)
{
  uint8_t *pData = NULL;
  bool_t serviceFound = FALSE;

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

  /* if found and length checks out ok */
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
 *  \brief     Handle a scan report.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppScanReport(dmEvt_t *pMsg)
{
  uint8_t *pData = NULL;
  uint8_t *pAddr = NULL;
  bool_t dataMatches = FALSE;
  uint8_t *pUuid = NULL;
  meshPrvOobInfoSource_t oob;
  uint8_t i, serviceDataLen;
  uint8_t addrType = 0;

  /* Service is not found. Do not continue processing. */
  if (!testAppCheckServiceUuid(pMsg))
  {
    return;
  }

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

  if ((pData != NULL) && pData[DM_AD_LEN_IDX] >= (ATT_16_UUID_LEN + 1))
  {
    serviceDataLen = pData[DM_AD_LEN_IDX] - ATT_16_UUID_LEN - 1;
    pData += DM_AD_DATA_IDX;

    /* Match service UUID in service data. */
    if (!BYTES_UINT16_CMP(pData, pGattBearerClCfg->serviceUuid))
    {
      return;
    }
    else
    {
      if (pGattBearerClCfg->serviceUuid == ATT_UUID_MESH_PRV_SERVICE &&
          (serviceDataLen == (MESH_PRV_DEVICE_UUID_SIZE + sizeof(meshPrvOobInfoSource_t))))
      {
        /* Connect to anyone*/
        dataMatches = TRUE;

        pData += ATT_16_UUID_LEN;
        pUuid = pData;
        pData += MESH_PRV_DEVICE_UUID_SIZE;
        BYTES_TO_UINT16(oob, pData);

        if (dataMatches)
        {
          for (i = 0; i < MESH_PRV_DEVICE_UUID_SIZE; i++)
          {
            sprintf(&printBuf[2 * i], "%02x", pUuid[i]);
          }

          TESTAPP_PRINT3("adv_ind addr=0x%s uuid=0x%s oob=0x%x" TESTAPP_NEWLINE,
                         Bda2Str(pAddr), printBuf, oob);
        }
      }
      else if(pGattBearerClCfg->serviceUuid == ATT_UUID_MESH_PROXY_SERVICE)
      {
        /* Connect to anyone*/
        dataMatches = TRUE;
      }
    }
  }

  /* Found match in scan report */
  if (dataMatches)
  {
    /* Initiate connection */
    GattBearerClConnect(addrType, pAddr);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Set up the node if provisioned, otherwise start provisioning procedure.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void testAppSetup(void)
{
  static bool_t setupComplete = FALSE;

  /* This function is called once. */
  if (setupComplete)
  {
    return;
  }

  /* Check if device is provisioned. */
  if (MeshIsProvisioned())
  {
    /* Start Node. */
    MeshStartNode();
  }
  else
  {
    TESTAPP_PRINT0(TESTAPP_NEWLINE "mesh_ind device_unprovisioned" TESTAPP_NEWLINE);
  }

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
static void testAppProcMsg(dmEvt_t *pMsg)
{
  switch(pMsg->hdr.event)
  {
    case DM_RESET_CMPL_IND:
      testAppSetup();
      break;

    case DM_EXT_SCAN_REPORT_IND:
    case DM_SCAN_REPORT_IND:
      if ((testAppCb.prvClStarted) || (testAppCb.proxyClStarted))
      {
        testAppScanReport(pMsg);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Vendor Test Client receive callback.
 *
 *  \param[in] pEvt  Event passed to the callback.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppMmdlVendorClEventCback(const mmdlVendorTestClEvent_t *pEvt)
{
  uint16_t i;

  switch(pEvt->hdr.event)
  {
    case MMDL_VENDOR_TEST_CL_STATUS_EVENT:
      for (i = 0; i < ((mmdlVendorTestClStatusEvent_t *)pEvt)->messageParamsLen; i++)
      {
        sprintf(&printBuf[2 * i], "%02x",
        (((mmdlVendorTestClStatusEvent_t *)pEvt)->pMsgParams[i]));
      }

      TESTAPP_PRINT4("accmsg_ind addr=0x%x ttl=0x%x pdulen=%d pdu=0x%s" TESTAPP_NEWLINE,
                     ((mmdlVendorTestClStatusEvent_t *)pEvt)->serverAddr,
                     ((mmdlVendorTestClStatusEvent_t *)pEvt)->ttl,
                     ((mmdlVendorTestClStatusEvent_t *)pEvt)->messageParamsLen,
                     printBuf);
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
static void testAppProcessMmdlGenOnOffEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_ONOFF_CL_STATUS_EVENT:
      if (((mmdlGenOnOffClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT4("goo_ind status elemid=%d state=0x%X target=0x%X remtime=0x%X" TESTAPP_NEWLINE,
                       ((mmdlGenOnOffClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlGenOnOffClStatusEvent_t *)pEvt)->state,
                       ((mmdlGenOnOffClStatusEvent_t *)pEvt)->targetState,
                       ((mmdlGenOnOffClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT2("goo_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                       ((mmdlGenOnOffClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlGenOnOffClStatusEvent_t *)pEvt)->state);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic Power On Off event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlGenPowerOnOffEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_POWER_ONOFF_CL_STATUS_EVENT:
      TESTAPP_PRINT2("gpoo_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                     ((mmdlGenPowOnOffClStatusEvent_t *)pEvt)->elementId,
                     ((mmdlGenPowOnOffClStatusEvent_t *)pEvt)->state);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic Level event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlGenLevelEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_LEVEL_CL_STATUS_EVENT:
      if (((mmdlGenLevelClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT4("glv_ind status elemid=%d state=0x%X target=0x%X remtime=0x%X" TESTAPP_NEWLINE,
                       ((mmdlGenLevelClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlGenLevelClStatusEvent_t *)pEvt)->state,
                       ((mmdlGenLevelClStatusEvent_t *)pEvt)->targetState,
                       ((mmdlGenLevelClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT2("glv_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                       ((mmdlGenLevelClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlGenLevelClStatusEvent_t *)pEvt)->state);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Light Lightness event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlLightLightnessEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_LIGHT_LIGHTNESS_CL_STATUS_EVENT:
      if (((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.remainingTime > 0)
      {
        TESTAPP_PRINT4("llact_ind status elemid=%d state=0x%X target=0x%X remtime=0x%X" TESTAPP_NEWLINE,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.presentLightness,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.targetLightness,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.remainingTime);
      }
      else
      {
        TESTAPP_PRINT2("llact_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.presentLightness);
      }
      break;

    case MMDL_LIGHT_LIGHTNESS_LINEAR_CL_STATUS_EVENT:
      if (((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.remainingTime > 0)
      {
        TESTAPP_PRINT4("lllin_ind status elemid=%d state=0x%X target=0x%X remtime=0x%X" TESTAPP_NEWLINE,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.presentLightness,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.targetLightness,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.remainingTime);
      }
      else
      {
        TESTAPP_PRINT2("lllin_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                       ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.presentLightness);
      }
      break;

    case MMDL_LIGHT_LIGHTNESS_LAST_CL_STATUS_EVENT:
      TESTAPP_PRINT2("lllast_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.lastStatusEvent.lightness);
      break;

    case MMDL_LIGHT_LIGHTNESS_DEFAULT_CL_STATUS_EVENT:
      TESTAPP_PRINT2("lldef_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.defaultStatusEvent.lightness);
      break;

    case MMDL_LIGHT_LIGHTNESS_RANGE_CL_STATUS_EVENT:
      TESTAPP_PRINT4("llrange_ind status elemid=%d status=0x%X min=0x%X max=0x%X" TESTAPP_NEWLINE,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.rangeStatusEvent.statusCode,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.rangeStatusEvent.rangeMin,
                     ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.rangeStatusEvent.rangeMax);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Time event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlTimeEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_TIME_CL_STATUS_EVENT:
      sprintf(printBuf, "0x%X%08lX", (uint8_t)(((mmdlTimeClStatusEvent_t *)pEvt)->state.taiSeconds >> 32),
             (uint32_t)((mmdlTimeClStatusEvent_t *)pEvt)->state.taiSeconds);
      TESTAPP_PRINT7("tim_ind status elemid=%d taiseconds=%s subsecond=0x%X uncertainty=0x%X timeauth=0x%X "
                      "delta=0x%X zoneoffset=0x%X" TESTAPP_NEWLINE,
                      ((mmdlTimeClStatusEvent_t *)pEvt)->elementId,
                      printBuf,
                      ((mmdlTimeClStatusEvent_t *)pEvt)->state.subSecond,
                      ((mmdlTimeClStatusEvent_t *)pEvt)->state.uncertainty,
                      ((mmdlTimeClStatusEvent_t *)pEvt)->state.timeAuthority,
                      ((mmdlTimeClStatusEvent_t *)pEvt)->state.taiUtcDelta,
                      ((mmdlTimeClStatusEvent_t *)pEvt)->state.timeZoneOffset);
    break;

    case MMDL_TIMEZONE_CL_STATUS_EVENT:
      sprintf(printBuf, "0x%X%08lX", (uint8_t)(((mmdlTimeClZoneStatusEvent_t *)pEvt)->taiZoneChange >> 32),
              (uint32_t)((mmdlTimeClZoneStatusEvent_t *)pEvt)->taiZoneChange);
      TESTAPP_PRINT4("timzone_ind status elemid=%d offsetcur=0x%X offsetnew=0x%X taichg=%s" TESTAPP_NEWLINE,
                     ((mmdlTimeClZoneStatusEvent_t *)pEvt)->elementId,
                     ((mmdlTimeClZoneStatusEvent_t *)pEvt)->offsetCurrent,
                     ((mmdlTimeClZoneStatusEvent_t *)pEvt)->offsetNew,
                     printBuf);
      break;

    case MMDL_TIMEDELTA_CL_STATUS_EVENT:
      sprintf(printBuf, "0x%X%08lX", (uint8_t)(((mmdlTimeClDeltaStatusEvent_t *)pEvt)->deltaChange >> 32),
              (uint32_t)((mmdlTimeClDeltaStatusEvent_t *)pEvt)->deltaChange);
      TESTAPP_PRINT4("timdelta_ind status elemid=%d deltacur=0x%X deltanew=0x%X deltachg=%s"
                     TESTAPP_NEWLINE,
                     ((mmdlTimeClDeltaStatusEvent_t *)pEvt)->elementId,
                     ((mmdlTimeClDeltaStatusEvent_t *)pEvt)->deltaCurrent,
                     ((mmdlTimeClDeltaStatusEvent_t *)pEvt)->deltaNew,
                     printBuf);
      break;

    case MMDL_TIMEROLE_CL_STATUS_EVENT:
      TESTAPP_PRINT2("timrole_ind status elemid=%d role=0x%X " TESTAPP_NEWLINE,
                     ((mmdlTimeClRoleStatusEvent_t *)pEvt)->elementId,
                     ((mmdlTimeClRoleStatusEvent_t *)pEvt)->timeRole);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic Power Level event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlGenPowerLevelEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_POWER_LEVEL_CL_STATUS_EVENT:
      if (((mmdlGenLevelClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT4("gpl_ind status elemid=%d state=0x%X target=0x%X remtime=0x%X" TESTAPP_NEWLINE,
                       ((mmdlGenPowerLevelClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlGenPowerLevelClStatusEvent_t *)pEvt)->state,
                       ((mmdlGenPowerLevelClStatusEvent_t *)pEvt)->targetState,
                       ((mmdlGenPowerLevelClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT2("gpl_ind status elemid=%d state=0x%X" TESTAPP_NEWLINE,
                       ((mmdlGenPowerLevelClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlGenPowerLevelClStatusEvent_t *)pEvt)->state);
      }
      break;

    case MMDL_GEN_POWER_LAST_CL_STATUS_EVENT:
      TESTAPP_PRINT2("gpl_ind last elemid=%d laststate=0x%X" TESTAPP_NEWLINE,
                     ((mmdlGenPowerLastClStatusEvent_t *)pEvt)->elementId,
                     ((mmdlGenPowerLastClStatusEvent_t *)pEvt)->lastState);
      break;

    case MMDL_GEN_POWER_DEFAULT_CL_STATUS_EVENT:
      TESTAPP_PRINT2("gpl_ind default elemid=%d defaultstate=0x%X" TESTAPP_NEWLINE,
                     ((mmdlGenPowerDefaultClStatusEvent_t *)pEvt)->elementId,
                     ((mmdlGenPowerDefaultClStatusEvent_t *)pEvt)->state);
      break;

    case MMDL_GEN_POWER_RANGE_CL_STATUS_EVENT:
      TESTAPP_PRINT4("gpl_ind range elemid=%d status=0x%X min=0x%X max=0x%X" TESTAPP_NEWLINE,
                     ((mmdlGenPowerRangeClStatusEvent_t *)pEvt)->elementId,
                     ((mmdlGenPowerRangeClStatusEvent_t *)pEvt)->statusCode,
                     ((mmdlGenPowerRangeClStatusEvent_t *)pEvt)->powerMin,
                     ((mmdlGenPowerRangeClStatusEvent_t *)pEvt)->powerMax);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Scene event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlSceneEventCback(const wsfMsgHdr_t *pEvt)
{
  uint8_t sceneIdx, scenesCount;

  switch(pEvt->param)
  {
    case MMDL_SCENE_CL_STATUS_EVENT:
      if (((mmdlSceneClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT5("sce_ind status elemid=%d code=%d scene=0x%X target=0x%X remtime=0x%X" TESTAPP_NEWLINE,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->status,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->currentScene,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->targetScene,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT3("sce_ind status elemid=%d code=%d scene=0x%X" TESTAPP_NEWLINE,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->status,
                       ((mmdlSceneClStatusEvent_t *)pEvt)->currentScene);
      }
      break;

    case MMDL_SCENE_CL_REG_STATUS_EVENT:
      TESTAPP_PRINT3("sce_ind regstatus elemid=%d code=%d scene=0x%X",
                     ((mmdlSceneClRegStatusEvent_t *)pEvt)->elementId,
                     ((mmdlSceneClRegStatusEvent_t *)pEvt)->status,
                     ((mmdlSceneClRegStatusEvent_t *)pEvt)->currentScene);

      /* Get scene count */
      scenesCount = ((mmdlSceneClRegStatusEvent_t *)pEvt)->scenesCount;
      if (scenesCount > 0)
      {
        TESTAPP_PRINT1(" scenescnt=%d scenes=", scenesCount);
        for (sceneIdx = 0; sceneIdx < scenesCount - 1; sceneIdx++)
        {
          TESTAPP_PRINT1(" 0x%X,", ((mmdlSceneClRegStatusEvent_t *)pEvt)->scenes[sceneIdx]);
        }
        TESTAPP_PRINT1(" 0x%X" TESTAPP_NEWLINE,
                       ((mmdlSceneClRegStatusEvent_t *)pEvt)->scenes[scenesCount-1]);
      }
      else
      {
        TESTAPP_PRINT0(" scenescnt=0" TESTAPP_NEWLINE);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Scheduler event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlSchedulerEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_SCHEDULER_CL_STATUS_EVENT:
      TESTAPP_PRINT2("sch_ind status elemid=%d schedulesbf=0x%X" TESTAPP_NEWLINE,
                     ((mmdlSchedulerClStatusEvent_t *)pEvt)->elementId,
                     ((mmdlSchedulerClStatusEvent_t *)pEvt)->schedulesBf);
      break;

    case MMDL_SCHEDULER_CL_ACTION_STATUS_EVENT:
      TESTAPP_PRINT12("sch_ind actstatus elemid=%d index=0x%X y=0x%X m=0x%X d=0x%X h=0x%X min=0x%X"
                      " sec=0x%X dof=0x%X act=0x%X tran=0x%X scenenum=0x%X" TESTAPP_NEWLINE,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->elementId,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->index,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.year,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.months,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.day,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.hour,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.minute,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.second,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.daysOfWeek,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.action,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.transTime,
                      ((mmdlSchedulerClActionStatusEvent_t *)pEvt)->scheduleRegister.sceneNumber);
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
static void testAppProcessMmdlLightHslEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_LIGHT_HSL_CL_STATUS_EVENT:
      if (((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT5("hsl_ind status elemid=%d lightness=0x%X hue=0x%X sat=0x%X remtime=0x%X"
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT4("hsl_ind status elemid=%d lightness=0x%X hue=0x%X sat=0x%X "
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation);
      }
      break;

    case MMDL_LIGHT_HSL_CL_TARGET_STATUS_EVENT:
      if (((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT5("hsl_ind targetstatus elemid=%d lightness=0x%X hue=0x%X sat=0x%X remtime=0x%X"
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT4("hsl_ind targetstatus elemid=%d lightness=0x%X hue=0x%X sat=0x%X "
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                       ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation);
      }
      break;

    case MMDL_LIGHT_HSL_CL_HUE_STATUS_EVENT:
      if (((mmdlLightHslClHueStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT4("hsl_ind huestatus elemid=%d present=0x%X target=0x%X remtime=0x%X"
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClHueStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClHueStatusEvent_t *)pEvt)->presentHue,
                       ((mmdlLightHslClHueStatusEvent_t *)pEvt)->targetHue,
                       ((mmdlLightHslClHueStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT2("hsl_ind huestatus elemid=%d present=0x%X"
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClHueStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClHueStatusEvent_t *)pEvt)->presentHue);
      }
      break;

    case MMDL_LIGHT_HSL_CL_SAT_STATUS_EVENT:
      if (((mmdlLightHslClSatStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        TESTAPP_PRINT4("hsl_ind satstatus elemid=%d present=0x%X target=0x%X remtime=0x%X"
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClSatStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClSatStatusEvent_t *)pEvt)->presentSat,
                       ((mmdlLightHslClSatStatusEvent_t *)pEvt)->targetSat,
                       ((mmdlLightHslClSatStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        TESTAPP_PRINT2("hsl_ind satstatus elemid=%d present=0x%X"
                       TESTAPP_NEWLINE,
                       ((mmdlLightHslClSatStatusEvent_t *)pEvt)->elementId,
                       ((mmdlLightHslClSatStatusEvent_t *)pEvt)->presentSat);
      }
      break;

    case MMDL_LIGHT_HSL_CL_DEF_STATUS_EVENT:
      TESTAPP_PRINT4("hsl_ind default elemid=%d lightness=0x%X hue=0x%X sat=0x%X"
                     TESTAPP_NEWLINE,
                     ((mmdlLightHslClDefStatusEvent_t *)pEvt)->elementId,
                     ((mmdlLightHslClDefStatusEvent_t *)pEvt)->lightness,
                     ((mmdlLightHslClDefStatusEvent_t *)pEvt)->hue,
                     ((mmdlLightHslClDefStatusEvent_t *)pEvt)->saturation);
      break;

    case MMDL_LIGHT_HSL_CL_RANGE_STATUS_EVENT:
      TESTAPP_PRINT6("hsl_ind range elemid=%d status=0x%X minhue=0x%X maxhue=0x%X minsat=0x%X maxsat=0x%X"
                     TESTAPP_NEWLINE,
                     ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->elementId,
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
 *  \brief     Process Mesh Model Generic Default Transition event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlGenDefaultTransEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_DEFAULT_TRANS_CL_STATUS_EVENT:
      TerminalTxPrint("gdtt_ind status elemid=%d state=0x%X" TERMINAL_STRING_NEW_LINE,
                      ((mmdlGenDefaultTransClStatusEvent_t *)pEvt)->elementId,
                      ((mmdlGenDefaultTransClStatusEvent_t *)pEvt)->state);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic Battery event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppProcessMmdlGenBatteryEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_BATTERY_CL_STATUS_EVENT:
      TerminalTxPrint("gbat_ind status elemid=%d level=0x%X discharge=0x%X charge=0x%X flags=0x%X" TERMINAL_STRING_NEW_LINE,
                      ((mmdlGenBatteryClStatusEvent_t *)pEvt)->elementId,
                      ((mmdlGenBatteryClStatusEvent_t *)pEvt)->state,
                      ((mmdlGenBatteryClStatusEvent_t *)pEvt)->timeToDischarge,
                      ((mmdlGenBatteryClStatusEvent_t *)pEvt)->timeToCharge,
                      ((mmdlGenBatteryClStatusEvent_t *)pEvt)->flags);
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
static void testAppMeshHtSrEventCback(const wsfMsgHdr_t *pHtSrEvt)
{
  meshHtSrEvt_t *pEvt = (meshHtSrEvt_t*)pHtSrEvt;
  bool_t success = FALSE;

  switch(pEvt->hdr.param)
  {
    case MESH_HT_SR_TEST_START_EVENT:
      if(pEvt->testStartEvt.testId == TESTAPP_HT_SR_TEST_ID)
      {
        /* Default behavior is to log 0 faults and just update test id. */
        MeshHtSrAddFault(pEvt->testStartEvt.elemId, pEvt->testStartEvt.companyId,
                         pEvt->testStartEvt.testId, MESH_HT_MODEL_FAULT_NO_FAULT);

        /* Check if response is needed. */
        if(pEvt->testStartEvt.notifTestEnd)
        {
          /* Signal test end. */
          MeshHtSrSignalTestEnd(pEvt->testStartEvt.elemId, pEvt->testStartEvt.companyId,
                                pEvt->testStartEvt.htClAddr, pEvt->testStartEvt.appKeyIndex,
                                pEvt->testStartEvt.useTtlZero, pEvt->testStartEvt.unicastReq);
        }

        success = TRUE;
      }

      TESTAPP_PRINT9("htsrtest_ind %s elemid=0x%x htcladdr=0x%x cid=0x%x testid=0x%x aidx=0x%x %s %s %s"
                     TESTAPP_NEWLINE,
                     success ? "success" : "unsupported_test_id",
                     ((meshHtSrTestStartEvt_t *)pEvt)->elemId,
                     ((meshHtSrTestStartEvt_t *)pEvt)->htClAddr,
                     ((meshHtSrTestStartEvt_t *)pEvt)->companyId,
                     ((meshHtSrTestStartEvt_t *)pEvt)->testId,
                     ((meshHtSrTestStartEvt_t *)pEvt)->appKeyIndex,
                     ((meshHtSrTestStartEvt_t *)pEvt)->useTtlZero ? "ttlzero" : "",
                     ((meshHtSrTestStartEvt_t *)pEvt)->unicastReq ? "unicast" : "",
                     ((meshHtSrTestStartEvt_t *)pEvt)->notifTestEnd ? "testend" : "");
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Health Client event callback.
 *
 *  \param[in] pEvt  Health Client event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppMeshHtClEventCback(const wsfMsgHdr_t *pHtClEvt)
{
  meshHtClEvt_t *pEvt = (meshHtClEvt_t*)pHtClEvt;
  uint16_t i;

  switch(pEvt->hdr.param)
  {
    case MESH_HT_CL_CURRENT_STATUS_EVENT:
    case MESH_HT_CL_FAULT_STATUS_EVENT:
      for (i = 0; i < ((meshHtClFaultStatusEvt_t *)pEvt)->healthStatus.faultIdArrayLen; i++)
      {
        sprintf(&printBuf[2 * i], "%02x",
        (((meshHtClFaultStatusEvt_t *)pEvt)->healthStatus.pFaultIdArray[i]));
      }

      TESTAPP_PRINT5("htclfault_ind elemid=0x%x htsraddr=0x%x testid=0x%x cid=0x%x fault=%s"
                     TESTAPP_NEWLINE,
                     ((meshHtClFaultStatusEvt_t *)pEvt)->elemId,
                     ((meshHtClFaultStatusEvt_t *)pEvt)->htSrElemAddr,
                     ((meshHtClFaultStatusEvt_t *)pEvt)->healthStatus.testId,
                     ((meshHtClFaultStatusEvt_t *)pEvt)->healthStatus.companyId,
                     printBuf);
      break;

    case MESH_HT_CL_PERIOD_STATUS_EVENT:
      TESTAPP_PRINT3("htclperiod_ind elemid=0x%x htsraddr=0x%x period=0x%x"
                     TESTAPP_NEWLINE,
                     ((meshHtClPeriodStatusEvt_t *)pEvt)->elemId,
                     ((meshHtClPeriodStatusEvt_t *)pEvt)->htSrElemAddr,
                     ((meshHtClPeriodStatusEvt_t *)pEvt)->periodDivisor);
      break;

    case MESH_HT_CL_ATTENTION_STATUS_EVENT:
      TESTAPP_PRINT3("htclattention_ind elemid=0x%x htsraddr=0x%x period=0x%x"
                     TESTAPP_NEWLINE,
                     ((meshHtClAttentionStatusEvt_t *)pEvt)->elemId,
                     ((meshHtClAttentionStatusEvt_t *)pEvt)->htSrElemAddr,
                     ((meshHtClAttentionStatusEvt_t *)pEvt)->attTimerState);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh model event callback.
 *
 *  \param[in] pEvt Mesh model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void testAppMmdlEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->event)
  {
    case MESH_HT_SR_EVENT:
      testAppMeshHtSrEventCback(pEvt);
      break;

    case MESH_HT_CL_EVENT:
      testAppMeshHtClEventCback(pEvt);
      break;

    case MMDL_GEN_ONOFF_CL_EVENT:
      testAppProcessMmdlGenOnOffEventCback(pEvt);
      break;

    case MMDL_GEN_LEVEL_CL_EVENT:
      testAppProcessMmdlGenLevelEventCback(pEvt);
      break;

    case MMDL_GEN_POWER_ONOFF_CL_EVENT:
      testAppProcessMmdlGenPowerOnOffEventCback(pEvt);
      break;

    case MMDL_GEN_POWER_LEVEL_CL_EVENT:
      testAppProcessMmdlGenPowerLevelEventCback(pEvt);
      break;

    case MMDL_LIGHT_LIGHTNESS_CL_EVENT:
      testAppProcessMmdlLightLightnessEventCback(pEvt);
      break;

    case MMDL_LIGHT_HSL_CL_EVENT:
      testAppProcessMmdlLightHslEventCback(pEvt);
      break;

    case MMDL_TIME_CL_EVENT:
      testAppProcessMmdlTimeEventCback(pEvt);
      break;

    case MMDL_SCENE_CL_EVENT:
      testAppProcessMmdlSceneEventCback(pEvt);
      break;

    case MMDL_SCHEDULER_CL_EVENT:
      testAppProcessMmdlSchedulerEventCback(pEvt);
      break;

    case MMDL_GEN_DEFAULT_TRANS_CL_EVENT:
      testAppProcessMmdlGenDefaultTransEventCback(pEvt);
      break;

    case MMDL_GEN_BATTERY_CL_EVENT:
      testAppProcessMmdlGenBatteryEventCback(pEvt);
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
 *  \brief     Application handler init function called during system initialization.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \retval    None.
 */
/*************************************************************************************************/
void TestAppHandlerInit(wsfHandlerId_t handlerId)
{
  APP_TRACE_INFO0("TESTAPP: Test Application Initialize");

  /* Set handler ID. */
  testAppHandlerId = handlerId;

  /* Set Node Reset timeout timer. */
  testAppNodeRstTmr.handlerId = handlerId;
  testAppNodeRstTmr.isStarted = FALSE;
  testAppNodeRstTmr.msg.event = APP_MESH_NODE_RST_TIMEOUT_EVT;

  /* Register empty disconnect cback. */
  testAppCb.discCback = testAppDiscProcDmMsgEmpty;
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize Mesh configuration for the application.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void TestAppConfigInit(void)
{
  /* Initialize configuration. */
  pMeshConfig = &testAppMeshConfig;
}

/*************************************************************************************************/
/*!
 *  \brief     The WSF event handler for the Test App.
 *
 *  \param[in] event  Event mask.
 *  \param[in] pMsg   Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void TestAppHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  (void)event;

  if (pMsg != NULL)
  {
    APP_TRACE_INFO1("TESTAPP: App got evt %d", pMsg->event);

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
          if (testAppCb.prvSrStarted || testAppCb.proxyFeatEnabled || testAppCb.nodeIdentityRunning)
          {
            /* Enable GATT bearer after connection closed */
            AppBearerEnableSlot(BR_GATT_SLOT);
          }
        }
      }

      /* Process discovery-related messages. */
      testAppCb.discCback((dmEvt_t *) pMsg);
    }
    else if ((pMsg->event >= MESH_CBACK_START) && (pMsg->event <= MESH_CBACK_END))
    {
      /* Process Mesh Stack message */
      testAppProcMeshMsg(pMsg);
    }
    else
    {
      /* Application events. */
      if (pMsg->event == APP_BR_TIMEOUT_EVT)
      {
        AppBearerSchedulerTimeout();
      }

      if (pMsg->event == APP_MESH_NODE_IDENTITY_TIMEOUT_EVT)
      {
        /* Node Identity stopped */
        MeshProxySrGetNextServiceData(MESH_PROXY_NWK_ID_TYPE);

        testAppCb.netKeyIndexAdv = 0xFFFF;
        testAppCb.nodeIdentityRunning = FALSE;

        /* Check if Proxy is started */
        if (!testAppCb.proxyFeatEnabled)
        {
          /* Disable bearer slot */
          AppBearerDisableSlot(BR_GATT_SLOT);
        }

        TESTAPP_PRINT0("nodeident_ind timeout");
      }

      if (pMsg->event == APP_MESH_NODE_RST_TIMEOUT_EVT)
      {
        /* Clear NVM. */
        TestAppConfigErase();
        MeshLocalCfgEraseNvm();
        MeshRpNvmErase();

        /* Reset system. */
        NVIC_SystemReset();
      }
    }

    if (testAppCb.proxySrStarted)
    {
      MprxsProcMsg(pMsg);
    }
    else if (testAppCb.proxyClStarted)
    {
      MprxcProcMsg(pMsg);
    }
    else if (testAppCb.prvSrStarted)
    {
      MprvsProcMsg(pMsg);
    }
    else if (testAppCb.prvClStarted)
    {
      MprvcProcMsg(pMsg);
    }

    testAppProcMsg((dmEvt_t *)pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppStart(void)
{
  /* Initialize the LE Stack. */
  DmConnRegister(DM_CLIENT_ID_APP, testAppDmCback);

  /* Register for stack callbacks. */
  DmRegister(testAppDmCback);
  AttRegister(testAppAttCback);

  /* Reset the device. */
  DmDevReset();

  /* Set application version. */
  AppMeshSetVersion(TESTAPP_VERSION);

  /* Register callback. */
  MeshRegister(testAppMeshCback);

  /* Initialize GATT Proxy */
  MeshGattProxyInit();

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  /* Initialize the Mesh Test module. */
  MeshTestInit();

  /* Register callback */
  MeshTestRegister(testAppMeshTestCback);
#endif

  /* Initialize Configuration Server. */
  MeshCfgMdlSrInit();

  /* Register callback. */
  MeshCfgMdlSrRegister(testAppMeshCfgMdlSrCback);

  /* Register Mesh Configuration Client callback. */
  MeshCfgMdlClRegister(testAppMeshCfgMdlClCback, TESTAPP_CFG_CL_TIMEOUT);

  /* Initialize Health Server. */
  MeshHtSrInit();

  /* Register callback. */
  MeshHtSrRegister(testAppMmdlEventCback);

  /* Configure company ID to an unused one. */
  MeshHtSrSetCompanyId(0, 0, TESTAPP_HT_SR_COMPANY_ID);

  /* Add 0 faults to update recent test ID. */
  MeshHtSrAddFault(0, 0xFFFF, TESTAPP_HT_SR_TEST_ID, MESH_HT_MODEL_FAULT_NO_FAULT);

  /* Initialize Health Client. */
  MeshHtClInit();

  /* Register callback. */
  MeshHtClRegister(testAppMmdlEventCback);

  /* Initialize application bearer scheduler. */
  AppBearerInit(testAppHandlerId);

  /* Register callback for application bearer events */
  AppBearerRegister(testAppBearerCback);

  /* Initialize the Advertising Bearer. */
  AdvBearerInit((advBearerCfg_t *)&testAppAdvBearerCfg);

  /* Register ADV Bearer callback. */
  MeshRegisterAdvIfPduSendCback(AdvBearerSendPacket);

  TestAppConfig();

  /* Initialize the models */
  MmdlGenOnOffSrInit();
  MmdlGenLevelSrInit();
  MmdlGenPowOnOffSrInit();
  MmdlGenPowOnOffSetupSrInit();
  MmdlGenPowerLevelSrInit();
  MmdlGenDefaultTransSrInit();
  MmdlGenBatterySrInit();
  MmdlLightLightnessSrInit();
  MmdlLightLightnessSetupSrInit();
  MmdlTimeSrInit();
  MmdlTimeSetupSrInit();
  MmdlSceneSrInit();
  MmdlLightHslSrInit();
  MmdlLightHslHueSrInit();
  MmdlLightHslSatSrInit();
  MmdlSchedulerSrInit();

  /* Install Generic model callbacks. */
  MmdlGenPowOnOffSrRegister(testAppMmdlEventCback);
  MmdlGenPowOnOffSetupSrRegister(testAppMmdlEventCback);
  MmdlGenOnOffSrRegister(testAppMmdlEventCback);
  MmdlGenOnOffClRegister(testAppMmdlEventCback);
  MmdlGenPowOnOffClRegister(testAppMmdlEventCback);
  MmdlGenLevelSrRegister(testAppMmdlEventCback);
  MmdlGenLevelClRegister(testAppMmdlEventCback);
  MmdlGenPowerLevelClRegister(testAppMmdlEventCback);
  MmdlGenPowerLevelSrRegister(testAppMmdlEventCback);
  MmdlGenDefaultTransSrRegister(testAppMmdlEventCback);
  MmdlGenDefaultTransClRegister(testAppMmdlEventCback);
  MmdlGenBatteryClRegister(testAppMmdlEventCback);
  MmdlGenBatterySrRegister(testAppMmdlEventCback);
  MmdlTimeClRegister(testAppMmdlEventCback);
  MmdlTimeSrRegister(testAppMmdlEventCback);
  MmdlTimeSetupSrRegister(testAppMmdlEventCback);
  MmdlSceneClRegister(testAppMmdlEventCback);
  MmdlSchedulerClRegister(testAppMmdlEventCback);

  /* Install Lighting model callbacks. */
  MmdlLightLightnessClRegister(testAppMmdlEventCback);
  MmdlLightLightnessSrRegister(testAppMmdlEventCback);
  MmdlLightLightnessSetupSrRegister(testAppMmdlEventCback);
  MmdlLightHslClRegister(testAppMmdlEventCback);

  /* Add bindings */
  MmdlGenPowerLevelSrBind2GenLevel(ELEM_GEN, ELEM_GEN);
  MmdlGenPowerLevelSrBind2GenOnOff(ELEM_GEN, ELEM_GEN);
  MmdlLightHslHueSrBind2GenLevel(ELEM_HUE, ELEM_HUE);
  MmdlLightHslSatSrBind2GenLevel(ELEM_SAT, ELEM_SAT);
  MmdlLightLightnessSrBind2GenLevel(ELEM_LIGHT, ELEM_LIGHT);
  MmdlLightLightnessSrBind2OnOff(ELEM_LIGHT, ELEM_LIGHT);
  MmdlLightHslSrBind2LtLtnessAct(ELEM_LIGHT, ELEM_LIGHT);

  /* Link Main, Hue and Sat elements */
  MmdlLightHslSrLinkElements(ELEM_LIGHT, ELEM_HUE, ELEM_SAT);

  /* Add OnPowerUp bindings */
  MmdlGenOnOffSrBind2OnPowerUp(ELEM_GEN, ELEM_GEN);
  MmdlGenPowerLevelSrBind2OnPowerUp(ELEM_GEN, ELEM_GEN);
  MmdlGenOnOffSrBind2OnPowerUp(ELEM_LIGHT, ELEM_LIGHT);
  MmdlLightLightnessSrBind2OnPowerUp(ELEM_LIGHT, ELEM_LIGHT);
  MmdlLightHslSrBind2OnPowerUp(ELEM_LIGHT, ELEM_LIGHT);

  /* Add Scheduler Bindings */
  MmdlSchedulerSrBind2GenOnOff(ELEM_GEN, ELEM_GEN);
  MmdlSchedulerSrBind2SceneReg(ELEM_GEN, ELEM_GEN);

  /* Install model callback. */
  MmdlVendorTestClRegister(testAppMmdlVendorClEventCback);

  /* Set provisioning configuration pointer. */
  pMeshPrvSrCfg = &testAppMeshPrvSrCfg;

  /* Initialize common Mesh Application functionality. */
  AppMeshNodeInit();
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppInitPrvSr(void)
{
  /* Initialize Provisioning Server. */
  MeshPrvSrInit(&testAppPrvSrUpdInfo);

  /* Register Provisioning Server callback. */
  MeshPrvSrRegister(testAppMeshPrvSrCback);
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Client module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppInitPrvCl(void)
{
  /* Initialize Provisioning Client. */
  MeshPrvClInit();

  /* Register Provisioning Server callback. */
  MeshPrvClRegister(testAppMeshPrvClCback);
}

/*************************************************************************************************/
/*!
 *  \brief  Start the Proxy Server feature.
 *
 *  \return None.
 */
/*************************************************************************************************/
void TestAppStartGattSr(void)
{
  if (!testAppCb.brGattClStarted)
  {
    if (!testAppCb.brGattSrStarted)
    {
      /* Register server callback*/
      AttConnRegister(AppServerConnCback);

      /* Initialize GATT Bearer Server */
      GattBearerSrInit((gattBearerSrCfg_t *)&testAppGattBearerSrCfg);

      /* Initialize Proxy Server */
      MeshProxySrInit();

      /* Schedule GATT bearer. */
      AppBearerScheduleSlot(BR_GATT_SLOT, GattBearerSrStart, GattBearerSrStop, GattBearerSrProcDmMsg,
                            5000);

      /* Set timer parameters. */
      testAppCb.nodeIdentityTmr.isStarted = FALSE;
      testAppCb.nodeIdentityTmr.handlerId = testAppHandlerId;
      testAppCb.nodeIdentityTmr.msg.event = APP_MESH_NODE_IDENTITY_TIMEOUT_EVT;

      testAppCb.brGattSrStarted = TRUE;
    }

    /* Check if Provisioned. */
    if (!MeshIsProvisioned())
    {
      if (!testAppCb.prvSrStarted)
      {
        /* Register the Mesh Prov Service. */
        SvcMprvsRegister(MprvsWriteCback);

        /* Add the Mesh Provisioning Service. */
        SvcMprvsAddGroup();

        /* Register Mesh Provisioning Service CCC*/
        AttsCccRegister(TESTAPP_NUM_CCC_IDX, (attsCccSet_t *) testAppPrvCccSet, testAppCccCback);

        /* Configure GATT server for Mesh Provisioning. */
        MprvsSetCccIdx(TESTAPP_DOUT_CCC_IDX);

        /* Register GATT Bearer callback */
        MeshRegisterGattProxyPduSendCback(MprvsSendDataOut);

        /* Set ADV data for an unprovisioned node. */
        GattBearerSrSetPrvSvcData(testAppPrvSrDevUuid, testAppPrvSrUpdInfo.oobInfoSrc);

        /* Enable bearer slot */
        AppBearerEnableSlot(BR_GATT_SLOT);

        /* Using GATT for Provisioning. */
        testAppCb.prvSrStarted = TRUE;
      }
    }
    else
    {
      if (!testAppCb.proxySrStarted)
      {
        /* Register the Mesh Proxy Service. */
        SvcMprxsRegister(MprxsWriteCback);

        /* Add the Mesh Proxy Service. */
        SvcMprxsAddGroup();

        /* Register Mesh Proxy Service CCC*/
        AttsCccRegister(TESTAPP_NUM_CCC_IDX, (attsCccSet_t *) testAppPrxCccSet, testAppCccCback);

        /* Configure GATT server for Mesh Proxy. */
        MprxsSetCccIdx(TESTAPP_DOUT_CCC_IDX);

        /* Register GATT Bearer callback */
        MeshRegisterGattProxyPduSendCback(MprxsSendDataOut);

        /* Using GATT for Proxy. */
        testAppCb.proxySrStarted = TRUE;

        if (MeshIsGattProxyEnabled())
        {
          testAppCb.netKeyIndexAdv = 0xFFFF;
          testAppCb.proxyFeatEnabled = TRUE;

          /* Enable bearer slot */
          AppBearerEnableSlot(BR_GATT_SLOT);
        }
      }
      else
      {
        /* Get Service Data for the specified netkey index*/
        MeshProxySrGetNextServiceData(MESH_PROXY_NODE_IDENTITY_TYPE);
        testAppCb.netKeyIndexAdv = 0xFFFF;
        testAppCb.nodeIdentityRunning = TRUE;

        /* Enable bearer slot */
        AppBearerEnableSlot(BR_GATT_SLOT);

        /* Start Node Identity timer. */
        WsfTimerStartMs(&testAppCb.nodeIdentityTmr, APP_MESH_NODE_IDENTITY_TIMEOUT_MS);
      }
    }
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
void TestAppStartGattCl(bool_t enableProv, uint16_t newAddress)
{
  if (!testAppCb.brGattSrStarted)
  {
    if (!testAppCb.brGattClStarted)
    {
      AppDiscInit();

      /* Set configuration pointer. */
      pAppDiscCfg = (appDiscCfg_t *) &testAppDiscCfg;
      pGattBearerClConnCfg = (hciConnSpec_t *) &testAppConnCfg;

      /* Remove Advertising Interface. */
      MeshRemoveAdvIf(TESTAPP_ADV_IF_ID);

      /* Register for app framework discovery callbacks. */
      AppDiscRegister(testAppDiscCback);
      testAppCb.discCback = AppDiscProcDmMsg;

      /* Initialize the GATT Bearer as Client. */
      GattBearerClInit();

      testAppCb.brGattClStarted = TRUE;
    }

    if (enableProv)
    {
      /* Using GATT for Provisioning. */
      testAppCb.prvClStarted = TRUE;
      testAppCb.proxyClStarted = FALSE;

      testAppPrvClSessionInfo.pData->address = newAddress;
      pGattBearerClCfg = (gattBearerClCfg_t *)&testAppPrvClCfg;

      /* Register GATT Bearer callback. */
      MeshRegisterGattProxyPduSendCback(MprvcSendDataIn);
    }
    else
    {
      /* Using GATT for Proxy. */
      testAppCb.proxyClStarted = TRUE;
      testAppCb.prvClStarted = FALSE;

      pGattBearerClCfg = (gattBearerClCfg_t *)&testAppProxyClCfg;

      /* Initialize Proxy Client */
      MeshProxyClInit();

      /* Register GATT Bearer callback. */
      MeshRegisterGattProxyPduSendCback(MprxcSendDataIn);
    }

    /* Schedule and enable GATT bearer. */
    AppBearerScheduleSlot(BR_GATT_SLOT, GattBearerClStart, GattBearerClStop, GattBearerClProcDmMsg,
                          5000);

    AppBearerEnableSlot(BR_GATT_SLOT);
  }
}
