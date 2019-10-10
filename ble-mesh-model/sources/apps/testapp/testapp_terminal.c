/*************************************************************************************************/
/*!
 *  \file   testapp_terminal.c
 *
 *  \brief  Mesh Test Terminal.
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_heap.h"
#include "wsf_os.h"
#include "wsf_trace.h"
#include "util/print.h"
#include "util/terminal.h"
#include "util/wstr.h"

#include "dm_api.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"
#include "mesh_lpn_api.h"
#include "mesh_friend_api.h"

#include "mmdl_defs.h"
#include "mmdl_types.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_prv_cl_api.h"
#include "mesh_prv_sr_api.h"

#include "adv_bearer.h"
#include "gatt_bearer_sr.h"
#include "gatt_bearer_cl.h"

#include "mesh_ht_mdl_api.h"
#include "mesh_ht_cl_api.h"
#include "mesh_ht_sr_api.h"

#include "app_mesh_api.h"
#include "testapp_api.h"
#include "testapp_config.h"
#include "testapp_terminal.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

#include "pal_sys.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! TestApp control block structure */
typedef struct testAppTerminalCb_tag
{
  bool_t  prvClInitialized;       /*!< Flag that signals if Provisioning Client is initialized */
  bool_t  prvSrInitialized;       /*!< Flag that signals if Provisioning Server is initialized */
  bool_t  prvIsServer;            /*!< TRUE if running provisioning server,
                                   *   FALSE if running provisioning client
                                   */
} testAppTerminalCb_t;

/**************************************************************************************************
  Local Functions Prototypes
**************************************************************************************************/

/*! \brief   Transmit LE Mesh Access message */
static uint8_t testAppTerminalAccMsgHandler(uint32_t argc, char **argv);

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
/*! \brief   Transmit LE Mesh Control message */
static uint8_t testAppTerminalCtlMsgHandler(uint32_t argc, char **argv);
#endif

/*! \brief   Enter PB-ADV provisioning mode */
static uint8_t testAppTerminalEnterPbAdvHandler(uint32_t argc, char **argv);

/*! \brief   Friend functionality */
static uint8_t testAppTerminalFriendHandler(uint32_t argc, char **argv);

/*! \brief   Enable GATT Client */
static uint8_t testAppTerminalGattClHandler(uint32_t argc, char **argv);

/*! \brief   Enable GATT Server */
static uint8_t testAppTerminalGattSrHandler(uint32_t argc, char **argv);

/*! \brief   Health Client Attention */
static uint8_t testAppTerminalHtClAttentionHandler(uint32_t argc, char **argv);

/*! \brief   Health Client Fault */
static uint8_t testAppTerminalHtClFaultHandler(uint32_t argc, char **argv);

/*! \brief   Health Client Period */
static uint8_t testAppTerminalHtClPeriodHandler(uint32_t argc, char **argv);

/*! \brief   Health Server Fault */
static uint8_t testAppTerminalHtSrFaultHandler(uint32_t argc, char **argv);

/*! \brief   Add/Remove the Advertising Bearer interface */
static uint8_t testAppTerminalIfAdvHandler(uint32_t argc, char **argv);

/*! \brief   Manually provision the LE Mesh Stack */
static uint8_t testAppTerminalLdProvHandler(uint32_t argc, char **argv);

/*! \brief   LPN functionality */
static uint8_t testAppTerminalLpnHandler(uint32_t argc, char **argv);

/*! \brief   Proxy Client commands */
static uint8_t testAppTerminalProxyClHandler(uint32_t argc, char **argv);

/*! \brief   Select PRV CL authentication */
static uint8_t testAppTerminalPrvClAuthHandler(uint32_t argc, char **argv);

/*! \brief   Cancel any on-going provisioning procedure */
static uint8_t testAppTerminalPrvClCancelHandler(uint32_t argc, char **argv);

/*! \brief   Configure PRV CL */
static uint8_t testAppTerminalPrvClCfgHandler(uint32_t argc, char **argv);

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
/*! \brief   Close PB-ADV Link */
static uint8_t testAppTerminalPrvClose(uint32_t argc, char **argv);
#endif

/*! \brief   Enters provisioning OOB data */
static uint8_t testAppTerminalPrvOobHandler(uint32_t argc, char **argv);

/*! \brief   Start PB-ADV provisioning client */
static uint8_t testAppTerminalStartPbAdvHandler(uint32_t argc, char **argv);

/*! \brief   Set IV update test mode and/or settings relative to testing IV update */
static uint8_t testAppTerminalTestIvHandler(uint32_t argc, char **argv);

/*! \brief   Set NetKey params */
static uint8_t testAppTerminalTestNetKeyHandler(uint32_t argc, char **argv);

/*! \brief   Set Replay Protection params */
static uint8_t testAppTerminalTestRpHandler(uint32_t argc, char **argv);

/*! \brief   Dump internal stack debug messages to the console */
static uint8_t testAppTerminalTlogHandler(uint32_t argc, char **argv);

/*! \brief   Send Secure Network Beacon */
static uint8_t testAppTerminalSendSnbHandler(uint32_t argc, char **argv);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

char * const pTestAppLogo[]=
{
  "\f\r\n",
  "\n\n\r\n",
  "#     #                        #######                        #\n\r",
  "##   ## ######  ####  #    #      #    ######  ####  #####   # #   #####  #####\n\r",
  "# # # # #      #      #    #      #    #      #        #    #   #  #    # #    #\n\r",
  "#  #  # #####   ####  ######      #    #####   ####    #   #     # #    # #    #\n\r",
  "#     # #           # #    #      #    #           #   #   ####### #####  #####\n\r",
  "#     # #      #    # #    #      #    #      #    #   #   #     # #      #\n\r",
  "#     # ######  ####  #    #      #    ######  ####    #   #     # #      #\n\r",
  "\r\n -Press enter for prompt\n\r",
  "\r\n -Type help to display the list of available commands\n\r",
  NULL
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Test Terminal control block */
static testAppTerminalCb_t termCb;

/*! Test Terminal commands table */
static terminalCommand_t testAppTerminalTbl[] =
{
    /*! Transmit LE Mesh Access message. */
    { NULL, "accmsg", "accmsg <addr|elemid|uuid|modelid|vend|opcode|ttl|aidx|pattern|pdu|pdulen>",
      testAppTerminalAccMsgHandler },
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
    /*! Transmit LE Mesh Control message. */
    { NULL, "ctlmsg", "ctlmsg <addr|nidx|opcode|ttl|ack|pdu|pdulen>", testAppTerminalCtlMsgHandler },
#endif
    /*! Enter PB-ADV provisioning mode. */
    { NULL, "enterpbadv", "enterpbadv <bcnms|restart>", testAppTerminalEnterPbAdvHandler },
    /*! Friend functionality */
    { NULL, "frnd", "frnd <init|recvwin>", testAppTerminalFriendHandler },
    /*! Enable GATT Client */
    { NULL, "gattcl", "gattcl <proxy|prv|addr>", testAppTerminalGattClHandler },
    /*! Enable GATT Server */
    { NULL, "gattsr", "gattsr", testAppTerminalGattSrHandler },
    /*! Health Client Attention */
    { NULL, "htclattention", "htclattention <set|setnack|get|elemid|htsraddr|aidx|ttl|attention>",
      testAppTerminalHtClAttentionHandler },
    /*! Health Client Fault */
    { NULL, "htclfault", "htclfault <get|clr|clrnack|test|testnack|elemid|htsraddr|aidx|ttl|cid|testid>",
      testAppTerminalHtClFaultHandler },
      /*! Health Client Period */
    { NULL, "htclperiod", "htclperiod <set|setnack|get|elemid|htsraddr|aidx|ttl|period>",
      testAppTerminalHtClPeriodHandler },
    /*! Health Server Fault */
    { NULL, "htsrfault", "htsrfault <add|rm|clr|elemid|cid|testid|faultid>",
      testAppTerminalHtSrFaultHandler },
    /*! Add/Remove the Advertising Bearer interface. */
    { NULL, "ifadv", "ifadv <add|rm|id>", testAppTerminalIfAdvHandler },
    /*! Manually provision the LE Mesh Stack. */
    { NULL, "ldprov", "ldprov <addr|devkey|nidx|netkey|ividx>", testAppTerminalLdProvHandler },
    /*! LPN functionality */
    { NULL, "lpn", "lpn <init|est|term|nidx|rssifact|recvwinfact|minqszlog|sleep|recvdelay|retrycnt>",
      testAppTerminalLpnHandler },
    /*! Proxy Client command */
    { NULL, "proxycl", "proxycl <ifid|nidx|settype|add|rm>", testAppTerminalProxyClHandler },
    /*! Select PRV CL authentication */
    { NULL, "prvclauth", "prvclauth <oobpk|method|action|size>", testAppTerminalPrvClAuthHandler },
    /*! Cancel any on-going provisioning procedure */
    { NULL, "prvclcancel", "prvclcancel", testAppTerminalPrvClCancelHandler },
    /*! Configure PRV CL */
    { NULL, "prvclcfg", "prvclcfg <devuuid|nidx|netkey|ividx>", testAppTerminalPrvClCfgHandler },
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
    /*! Close PB-ADV Link */
    { NULL, "prvclose", "prvclose", testAppTerminalPrvClose },
#endif
    /*! Enters provisioning OOB data */
    { NULL, "prvoob", "prvoob <num|alpha>", testAppTerminalPrvOobHandler },
    /*! Starts PB-ADV provisioning client */
    { NULL, "startpbadv", "startpbadv <addr>", testAppTerminalStartPbAdvHandler},
    /*! Set IV update test mode and/or settings relative to testing IV update. */
    { NULL, "testiv", "testiv <on|off|state>", testAppTerminalTestIvHandler },
    /*! Set NetKey params. */
    { NULL, "testnetkey", "testnetkey <listsize>", testAppTerminalTestNetKeyHandler },
    /*! Set Replay Protection params. */
    { NULL, "testrp", "testrp <clear|get|listsize>", testAppTerminalTestRpHandler },
    /*! Send Secure Network Beacon */
    { NULL, "testsnb", "testsnb <nidx>", testAppTerminalSendSnbHandler},
    /*! Dump internal stack debug messages to the console */
    { NULL, "tlog", "tlog <prvbr|nwk|sar|utr|all|off>", testAppTerminalTlogHandler },
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static void meshLpnEvtNotifyCback(const meshLpnEvt_t *pEvent)
{
  switch(pEvent->hdr.param)
  {
    case MESH_LPN_FRIENDSHIP_ESTABLISHED_EVENT:
      TerminalTxPrint("lpn_ind est nidx=0x%x" TERMINAL_STRING_NEW_LINE,
                      ((meshLpnFriendshipEstablishedEvt_t *)pEvent)->netKeyIndex);
      break;

    case MESH_LPN_FRIENDSHIP_TERMINATED_EVENT:
      TerminalTxPrint("lpn_ind term nidx=0x%x" TERMINAL_STRING_NEW_LINE,
                      ((meshLpnFriendshipTerminatedEvt_t *)pEvent)->netKeyIndex);
      break;

    default:
      break;
  }
}

static uint8_t testAppTerminalAccMsgHandler(uint32_t argc, char **argv)
{
  uint8_t *pLabelUuid = NULL;
  uint8_t *pMsg = NULL;
  meshMsgInfo_t msgInfo = { 0x00 };
  uint32_t opcode;
  uint32_t modelId;
  uint16_t msgLen = 0;
  uint8_t pattern = 0x00;
  char *pChar;
  char *pPdu = NULL;
  uint8_t i;
  bool_t vend = FALSE;

  if (argc < 9)
  {
    TerminalTxStr("accmsg_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* TTL argument is optional. Set it to default. */
    msgInfo.ttl = MESH_USE_DEFAULT_TTL;

    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "addr=") != NULL)
      {
        /* Found destination address field. */
        pChar = strchr(argv[i], '=');

        msgInfo.dstAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "elemid=") != NULL)
      {
        /* Found element ID field. */
        pChar = strchr(argv[i], '=');

        msgInfo.elementId = (meshElementId_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "uuid=") != NULL)
      {
        pChar = strstr(argv[i], "=");

        if ((pLabelUuid = WsfBufAlloc(MESH_LABEL_UUID_SIZE)) != NULL)
        {
          WStrHexToArray(pChar + 1, pLabelUuid, MESH_LABEL_UUID_SIZE);
        }
        else
        {
          TerminalTxStr("accmsg_cnf out_of_memory" TERMINAL_STRING_NEW_LINE);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else if (strcmp(argv[i], "vend") == 0)
      {
        /* Vendor Model ID used. */
        vend = TRUE;
      }
      else if (strstr(argv[i], "modelid=") != NULL)
      {
        /* Found Model ID field. */
        pChar = strchr(argv[i], '=');

        modelId = (uint32_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "opcode=") != NULL)
      {
        /* Found Opcode field. */
        pChar = strchr(argv[i], '=');

        opcode = (uint32_t)strtol(pChar + 1, NULL, 0);

        if ((opcode & 0xFF0000) != 0)
        {
          msgInfo.opcode.opcodeBytes[0] = (uint8_t)((opcode >> 16) & 0xFF);
          msgInfo.opcode.opcodeBytes[1] = (uint8_t)((opcode >> 8) & 0xFF);
          msgInfo.opcode.opcodeBytes[2] = (uint8_t)(opcode & 0xFF);
        }
        else if ((opcode & 0xFF00) != 0)
        {
          msgInfo.opcode.opcodeBytes[0] = (uint8_t)((opcode >> 8) & 0xFF);
          msgInfo.opcode.opcodeBytes[1] = (uint8_t)(opcode & 0xFF);
        }
        else
        {
          msgInfo.opcode.opcodeBytes[0] = (uint8_t)(opcode & 0xFF);
        }
      }
      else if (strstr(argv[i], "ttl=") != NULL)
      {
        /* Found TTL field. */
        pChar = strchr(argv[i], '=');

        msgInfo.ttl = (uint8_t)strtol(pChar + 1, NULL, 0);

        /* Check if TTL value exceeds maximum TTL value. */
        if (msgInfo.ttl > MESH_TTL_MASK)
        {
          /* Force it to default. */
          msgInfo.ttl = MESH_USE_DEFAULT_TTL;
        }
      }
      else if (strstr(argv[i], "aidx=") != NULL)
      {
        /* Found App Key Index field. */
        pChar = strchr(argv[i], '=');

        msgInfo.appKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "pattern=") != NULL)
      {
        /* Found Pattern field. */
        pChar = strchr(argv[i], '=');

        pattern = (uint8_t)strtol(pChar + 1, NULL, 16);
      }
      else if (strstr(argv[i], "pdu=") != NULL)
      {
        /* Found PDU field. */
        pChar = strchr(argv[i], '=');

        pPdu = pChar + 1;
      }
      else if (strstr(argv[i], "pdulen=") != NULL)
      {
        /* Found PDU length field. */
        pChar = strchr(argv[i], '=');

        msgLen = (uint16_t)strtol(pChar + 1, NULL, 0);

        if (msgLen > (380 - MESH_OPCODE_SIZE(msgInfo.opcode)))
        {
          TerminalTxPrint("accmsg_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else
      {
        TerminalTxPrint("accmsg_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }

    if ((msgLen != 0) && ((pMsg = WsfBufAlloc(msgLen)) != NULL))
    {
      memset(pMsg, 0x00, msgLen);

      /* Check if pdu argument was present. */
      if (pPdu != NULL)
      {
        WStrHexToArray(pPdu, pMsg, msgLen);
      }
      else
      {
        memset(pMsg, pattern, msgLen);
      }
    }
    else
    {
      TerminalTxStr("accmsg_cnf out_of_memory" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
  }

  if (vend == TRUE)
  {
    msgInfo.modelId.vendorModelId = (meshVendorModelId_t)modelId;
  }
  else
  {
    msgInfo.modelId.sigModelId = (meshSigModelId_t)modelId;
  }

  if (MESH_IS_ADDR_VIRTUAL(msgInfo.dstAddr))
  {
    if (pLabelUuid == NULL)
    {
      TerminalTxStr("accmsg_cnf invalid_value uuid=" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }

    msgInfo.pDstLabelUuid = pLabelUuid;
  }

  /* Send Mesh Message. */
  MeshSendMessage(&msgInfo, pMsg, msgLen, 0, 0);

  if (pMsg != NULL)
  {
    WsfBufFree(pMsg);
  }

  if (pLabelUuid != NULL)
  {
    WsfBufFree(pLabelUuid);
  }

  TerminalTxStr("accmsg_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))

static uint8_t testAppTerminalCtlMsgHandler(uint32_t argc, char **argv)
{
  meshAddress_t dstAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t  netKeyIndex = 0xFFFF;
  uint8_t ttl, *pMsg, opcode = 0;
  bool_t  ackRequired = FALSE;
  uint16_t  pduLen = 0;
  char *pChar, *pPdu = NULL;
  uint8_t i;

  if (argc < 7)
  {
    TerminalTxStr("ctlmsg_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* TTL argument is optional. Set it to default. */
    ttl = MESH_USE_DEFAULT_TTL;

    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "addr=") != NULL)
      {
        /* Found destination address field. */
        pChar = strchr(argv[i], '=');

        dstAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "opcode=") != NULL)
      {
        /* Found Opcode field. */
        pChar = strchr(argv[i], '=');

        opcode = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "ttl=") != NULL)
      {
        /* Found TTL field. */
        pChar = strchr(argv[i], '=');

        ttl = (uint8_t)strtol(pChar + 1, NULL, 0);

        /* Check if TTL value exceeds maximum TTL value. */
        if (ttl > MESH_TTL_MASK)
        {
          /* Force it to default. */
          ttl = MESH_USE_DEFAULT_TTL;
        }
      }
      else if (strstr(argv[i], "nidx=") != NULL)
      {
        /* Found App Key Index field. */
        pChar = strchr(argv[i], '=');

        netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "ack=") != NULL)
      {
        /* Found App Key Index field. */
        pChar = strchr(argv[i], '=');

        ackRequired = (bool_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "pdu=") != NULL)
      {
        /* Found PDU field. */
        pChar = strchr(argv[i], '=');

        pPdu = pChar + 1;
      }
      else if (strstr(argv[i], "pdulen=") != NULL)
      {
        /* Found PDU length field. */
        pChar = strchr(argv[i], '=');

        pduLen = (uint16_t)strtol(pChar + 1, NULL, 0);

        if (pduLen > (256 - 1))
        {
          TerminalTxPrint("ctlmsg_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else
      {
        TerminalTxPrint("ctlmsg_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }

    if ((pduLen != 0) && (pPdu != NULL) &&((pMsg = WsfBufAlloc(pduLen)) != NULL))
    {
      memset(pMsg, 0x00, pduLen);
      WStrHexToArray(pPdu, pMsg, pduLen);
    }
    else
    {
      TerminalTxStr("accmsg_cnf out_of_memory" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
  }

  /* Send Ctl Message. */
  MeshTestSendCtlMsg(dstAddr, netKeyIndex, opcode, ttl, ackRequired, pMsg, pduLen);

  if (pMsg != NULL)
  {
    WsfBufFree(pMsg);
  }

  TerminalTxStr("ctlmsg_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

#endif

static uint8_t testAppTerminalEnterPbAdvHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t interval = 0;
  uint8_t i;

  if (MeshIsProvisioned())
  {
    TerminalTxStr("enterpbadv_cnf invalid_state already_provisioned" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }

  if (argc < 2)
  {
    TerminalTxStr("enterpbadv_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    pMeshPrvSrCfg->pbAdvRestart = FALSE;

    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "bcnms=") != NULL)
      {
        /* Found Beacon Interval field. */
        pChar = strchr(argv[i], '=');

        interval = (uint32_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strcmp(argv[i], "restart") == 0)
      {
        /* Found "restart" flag. */
        pMeshPrvSrCfg->pbAdvRestart = TRUE;
      }
      else
      {
        TerminalTxPrint("enterpbadv_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }

    /* Ensure Provisioning Server is initialized. */
    if (!termCb.prvSrInitialized)
    {
      if (termCb.prvClInitialized)
      {
        TerminalTxPrint("enterpbadv_cnf invalid_state prvcl_initialized" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }
      else
      {
        termCb.prvSrInitialized = TRUE;

        TestAppInitPrvSr();

        /* Bind the interface. */
        MeshAddAdvIf(TESTAPP_ADV_IF_ID);
      }
    }

    /* Save provisioning mode. */
    pMeshPrvSrCfg->pbAdvIfId = TESTAPP_ADV_IF_ID;
    pMeshPrvSrCfg->pbAdvInterval = interval;
    termCb.prvIsServer = TRUE;

    /* Enter provisioning. */
    MeshPrvSrEnterPbAdvProvisioningMode(TESTAPP_ADV_IF_ID, interval);

    TerminalTxStr("enterpbadv_cnf success" TERMINAL_STRING_NEW_LINE);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalFriendHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t memUsed;
  uint8_t recvWin;

  if (argc < 3)
  {
    TerminalTxStr("frnd_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "init") == 0)
    {
      if (strstr(argv[2], "recvwin=") != NULL)
      {
        /* Found receive window field. */
        pChar = strchr(argv[2], '=');

        recvWin = (uint8_t)strtol(pChar + 1, NULL, 0);

        /* Initialize Mesh Friend. */
        memUsed = MeshFriendMemInit(WsfHeapGetFreeStartAddress(), WsfHeapCountAvailable());
        MeshFriendInit(recvWin);

        if (memUsed == 0)
        {
          TerminalTxStr("frnd_cnf invalid_config");

          return TERMINAL_ERROR_EXEC;
        }

        WsfHeapAlloc(memUsed);
      }
      else
      {
        TerminalTxPrint("frnd_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else
    {
      TerminalTxPrint("frnd_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("frnd_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalGattClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint16_t addr = MESH_ADDR_TYPE_UNASSIGNED;

  if (argc < 2)
  {
    TerminalTxStr("gattcl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "proxy") == 0)
    {
      TestAppStartGattCl(FALSE, 0);
    }
    else if (strcmp(argv[1], "prv") == 0)
    {
      if (strstr(argv[2], "addr="))
      {
        /* Found addr field. */
        pChar = strchr(argv[2], '=');

        addr = (uint16_t)strtol(pChar + 1, NULL, 0);

        if (!MESH_IS_ADDR_UNICAST(addr))
        {
          TerminalTxPrint("gattcl_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[2]);

          return TERMINAL_ERROR_EXEC;
        }

        /* Ensure Provisioning Client is initialized. */
        if (!termCb.prvClInitialized)
        {
          if (termCb.prvSrInitialized)
          {
            TerminalTxPrint("gattcl_cnf invalid_state prvsr_initialized" TERMINAL_STRING_NEW_LINE);

            return TERMINAL_ERROR_EXEC;
          }
          else
          {
            termCb.prvIsServer = FALSE;
            termCb.prvClInitialized = TRUE;
            TestAppInitPrvCl();
          }
        }

        TestAppStartGattCl(TRUE, addr);
      }
      else
      {
        TerminalTxPrint("gattcl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else
    {
      TerminalTxPrint("gattcl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    TerminalTxStr("gattcl_cnf success" TERMINAL_STRING_NEW_LINE);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalGattSrHandler(uint32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  if (!MeshIsProvisioned())
  {
    /* Ensure Provisioning Server is initialized. */
    if (!termCb.prvSrInitialized)
    {
      if (termCb.prvClInitialized)
      {
        TerminalTxPrint("gattsr_cnf invalid_state prvcl_initialized" TERMINAL_STRING_NEW_LINE);
        return TERMINAL_ERROR_EXEC;
      }
      else
      {
        termCb.prvSrInitialized = TRUE;
        TestAppInitPrvSr();
      }
    }

    /* Using GATT for Provisioning. */
    termCb.prvIsServer = TRUE;
  }

  TestAppStartGattSr();

  TerminalTxStr("gattsr_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalHtClAttentionHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshElementId_t elemId = 0xFF;
  meshAddress_t srAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t aidx = 0xFFFF;
  meshHtAttTimer_t attention = 0;
  uint8_t ttl = 0;
  uint8_t i;
  bool_t set = FALSE;
  bool_t get = FALSE;
  bool_t ack = TRUE;

  if (argc < 2)
  {
    TerminalTxStr("htclattention_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      get = TRUE;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      set = TRUE;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      set = TRUE;
      ack = FALSE;
    }
    else
    {
      TerminalTxPrint("htclattention_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 6) && (set == TRUE)) || ((argc < 5) && (get == TRUE)))
    {
      TerminalTxStr("htclattention_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "elemid=") != NULL)
        {
          /* Found Element ID field. */
          pChar = strchr(argv[i], '=');

          elemId = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "htsraddr=") != NULL)
        {
          /* Found Health Server address field. */
          pChar = strchr(argv[i], '=');

          srAddr = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "aidx=") != NULL)
        {
          /* Found AppKey index field. */
          pChar = strchr(argv[i], '=');

          aidx = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ttl=") != NULL)
        {
          /* Found TTL field. */
          pChar = strchr(argv[i], '=');

          ttl = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "attention=") != NULL)
        {
          /* Found attention field. */
          pChar = strchr(argv[i], '=');

          attention = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("htclattention_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  if (set == TRUE)
  {
    MeshHtClAttentionSet(elemId, srAddr, aidx, ttl, attention, ack);
  }
  else if (get == TRUE)
  {
    MeshHtClAttentionGet(elemId, srAddr, aidx, ttl);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalHtClFaultHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshElementId_t elemId = 0xFF;
  meshAddress_t srAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t aidx = 0xFFFF;
  uint16_t companyId = 0xFFFF;
  meshHtMdlTestId_t testId = 0;
  uint8_t ttl = 0;
  uint8_t i;
  bool_t get = FALSE;
  bool_t clr = FALSE;
  bool_t test = FALSE;
  bool_t ack = TRUE;

  if (argc < 2)
  {
    TerminalTxStr("htclfault_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      get = TRUE;
    }
    else if (strcmp(argv[1], "clr") == 0)
    {
      clr = TRUE;
    }
    else if (strcmp(argv[1], "clrnack") == 0)
    {
      clr = TRUE;
      ack = FALSE;
    }
    else if (strcmp(argv[1], "test") == 0)
    {
      test = TRUE;
    }
    else if (strcmp(argv[1], "testnack") == 0)
    {
      test = TRUE;
      ack = FALSE;
    }
    else
    {
      TerminalTxPrint("htclfault_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 6) && ((get == TRUE) || (clr == TRUE))) || ((argc < 7) && (test == TRUE)))
    {
      TerminalTxStr("htclfault_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "elemid=") != NULL)
        {
          /* Found Element ID field. */
          pChar = strchr(argv[i], '=');

          elemId = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "htsraddr=") != NULL)
        {
          /* Found Health Server address field. */
          pChar = strchr(argv[i], '=');

          srAddr = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "aidx=") != NULL)
        {
          /* Found AppKey index field. */
          pChar = strchr(argv[i], '=');

          aidx = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ttl=") != NULL)
        {
          /* Found TTL field. */
          pChar = strchr(argv[i], '=');

          ttl = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "cid=") != NULL)
        {
          /* Found company ID field. */
          pChar = strchr(argv[i], '=');

          companyId = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "testid=") != NULL)
        {
          /* Found company ID field. */
          pChar = strchr(argv[i], '=');

          testId = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("htclfault_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  if (get == TRUE)
  {
    MeshHtClFaultGet(elemId, srAddr, aidx, ttl, companyId);
  }
  else if (clr == TRUE)
  {
    MeshHtClFaultClear(elemId, srAddr, aidx, ttl, companyId, ack);
  }
  else if (test == TRUE)
  {
    MeshHtClFaultTest(elemId, srAddr, aidx, ttl, testId, companyId, ack);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalHtClPeriodHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshElementId_t elemId = 0xFF;
  meshAddress_t srAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t aidx = 0xFFFF;
  meshHtPeriod_t period = 0;
  uint8_t ttl = 0;
  uint8_t i;
  bool_t set = FALSE;
  bool_t get = FALSE;
  bool_t ack = TRUE;

  if (argc < 2)
  {
    TerminalTxStr("htclperiod_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      get = TRUE;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      set = TRUE;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      set = TRUE;
      ack = FALSE;
    }
    else
    {
      TerminalTxPrint("htclperiod_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 6) && (set == TRUE)) || ((argc < 5) && (get == TRUE)))
    {
      TerminalTxStr("htclperiod_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "elemid=") != NULL)
        {
          /* Found Element ID field. */
          pChar = strchr(argv[i], '=');

          elemId = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "htsraddr=") != NULL)
        {
          /* Found Health Server address field. */
          pChar = strchr(argv[i], '=');

          srAddr = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "aidx=") != NULL)
        {
          /* Found AppKey index field. */
          pChar = strchr(argv[i], '=');

          aidx = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ttl=") != NULL)
        {
          /* Found TTL field. */
          pChar = strchr(argv[i], '=');

          ttl = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "period=") != NULL)
        {
          /* Found attention field. */
          pChar = strchr(argv[i], '=');

          period = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("htclperiod_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  if (set == TRUE)
  {
    MeshHtClPeriodSet(elemId, srAddr, aidx, ttl, period, ack);
  }
  else if (get == TRUE)
  {
    MeshHtClPeriodGet(elemId, srAddr, aidx, ttl);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalHtSrFaultHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshElementId_t elemId = 0xFF;
  uint16_t companyId = 0xFFFF;
  meshHtMdlTestId_t recentTestId = 0;
  meshHtFaultId_t faultId = 0;
  uint8_t i;
  bool_t add = FALSE;
  bool_t rm = FALSE;
  bool_t clr = FALSE;

  if (argc < 2)
  {
    TerminalTxStr("htsrfault_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "add") == 0)
    {
      add = TRUE;
    }
    else if (strcmp(argv[1], "rm") == 0)
    {
      rm = TRUE;
    }
    else if (strcmp(argv[1], "clr") == 0)
    {
      clr = TRUE;
    }
    else
    {
      TerminalTxPrint("htsrfault_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 6) && ((add == TRUE) || (rm == TRUE))) || ((argc < 5) && (clr == TRUE)))
    {
      TerminalTxStr("htsrfault_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "elemid=") != NULL)
        {
          /* Found Element ID field. */
          pChar = strchr(argv[i], '=');

          elemId = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "cid=") != NULL)
        {
          /* Found Company ID field. */
          pChar = strchr(argv[i], '=');

          companyId = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "testid=") != NULL)
        {
          /* Found recent test ID field. */
          pChar = strchr(argv[i], '=');

          recentTestId = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "faultid=") != NULL)
        {
          /* Found fault ID field. */
          pChar = strchr(argv[i], '=');

          faultId = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("htsrfault_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  if (add == TRUE)
  {
    MeshHtSrAddFault(elemId, companyId, recentTestId, faultId);
  }
  else if (rm == TRUE)
  {
    MeshHtSrRemoveFault(elemId, companyId, recentTestId, faultId);
  }
  else if (clr == TRUE)
  {
    MeshHtSrClearFaults(elemId, companyId, recentTestId);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalIfAdvHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t id;
  bool_t add;

  if (argc < 3)
  {
    TerminalTxStr("ifadv_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "add") == 0)
    {
     add = TRUE;
    }
    else if (strcmp(argv[1], "rm") == 0)
    {
      add = FALSE;
    }
    else
    {
      TerminalTxPrint("ifadv_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (strstr(argv[2], "id=") != NULL)
    {
      /* Found ID field. */
      pChar = strchr(argv[2], '=');

      id = (uint8_t)strtol(pChar + 1, NULL, 0);

      if (MESH_ADV_IF_ID_IS_VALID(id))
      {
        if (add == TRUE)
        {
          MeshAddAdvIf(id);
        }
        else
        {
          MeshRemoveAdvIf(id);
        }
      }
      else
      {
        TerminalTxPrint("ifadv_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else
    {
      TerminalTxPrint("ifadv_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("ifadv_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalLdProvHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshPrvData_t prvData;
  uint32_t ivIdx = 0;
  meshAddress_t addr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t netKeyIndex = 0xFFFF;
  uint8_t devKey[MESH_KEY_SIZE_128];
  uint8_t netKey[MESH_KEY_SIZE_128];
  uint8_t i;

  if (MeshIsProvisioned())
  {
    TerminalTxStr("ldprov_cnf invalid_state already_provisioned" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }

  if (argc < 6)
  {
    TerminalTxStr("ldprov_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "addr=") != NULL)
      {
        /* Found destination address field. */
        pChar = strchr(argv[i], '=');

        addr = (meshAddress_t)strtol(pChar + 1, NULL, 0);

        if (MESH_IS_ADDR_UNICAST(addr) != TRUE)
        {
          TerminalTxPrint("ldprov_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else if (strstr(argv[i], "devkey=") != NULL)
      {
        /* Found DevKey field. */
        pChar = strchr(argv[i], '=');

        WStrHexToArray(pChar + 1, devKey, sizeof(devKey));
      }
      else if (strstr(argv[i], "nidx=") != NULL)
      {
        /* Found Net Key index field. */
        pChar = strchr(argv[i], '=');

        netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "netkey=") != NULL)
      {
        /* Found NetKey field. */
        pChar = strchr(argv[i], '=');

        WStrHexToArray(pChar + 1, netKey, sizeof(netKey));
      }
      else if (strstr(argv[i], "ividx=") != NULL)
      {
        /* Found ivIndex field. */
        pChar = strchr(argv[i], '=');

        ivIdx = (uint32_t)strtol(pChar + 1, NULL, 0);
      }
      else
      {
        TerminalTxPrint("ldprov_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }
  }

  /* Set Provisioning Data. */
  prvData.pDevKey = devKey;
  prvData.pNetKey = netKey;
  prvData.primaryElementAddr = addr;
  prvData.ivIndex = ivIdx;
  prvData.netKeyIndex = netKeyIndex;
  prvData.flags = 0x00;

  /* Load provisioning data. */
  MeshLoadPrvData(&prvData);

  /* Start node. */
  MeshStartNode();

  TerminalTxStr("ldprov_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalLpnHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t memUsed;
  uint32_t sleepDurationMs = 0;
  uint16_t netKeyIndex = 0xFFFF;
  meshFriendshipCriteria_t criteria;
  uint8_t recvDelayMs = 0;
  uint8_t retryCount = 0;
  uint8_t i;
  bool_t est = FALSE;
  bool_t term = FALSE;

  criteria.minQueueSizeLog = 0;
  criteria.recvWinFactor = 0;
  criteria.rssiFactor = 0;

  if (argc < 2)
  {
    TerminalTxStr("lpn_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else if (strcmp(argv[1], "init") == 0)
  {
    /* Initialize Mesh LPN. */
    memUsed = MeshLpnMemInit(WsfHeapGetFreeStartAddress(), WsfHeapCountAvailable());
    MeshLpnInit();

    if (memUsed == 0)
    {
      TerminalTxStr("lpn_cnf invalid_config");

      return TERMINAL_ERROR_EXEC;
    }

    WsfHeapAlloc(memUsed);

    /* Register LPN event notification callback. */
    MeshLpnRegister(meshLpnEvtNotifyCback);
  }
  else
  {
    if (strcmp(argv[1], "est") == 0)
    {
      est = TRUE;
    }
    else if (strcmp(argv[1], "term") == 0)
    {
      term = TRUE;
    }
    else
    {
      TerminalTxPrint("lpn_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 9) && (est == TRUE)) || ((argc < 3) && (term == TRUE)))
    {
      TerminalTxStr("lpn_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "nidx=") != NULL)
        {
          /* Found NetKey index field. */
          pChar = strchr(argv[i], '=');

          netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "rssifact=") != NULL)
        {
          /* Found RSSI factor field. */
          pChar = strchr(argv[i], '=');

          criteria.rssiFactor = (meshFriendshipRssiFactor_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "recvwinfact=") != NULL)
        {
          /* Found Receive Window Factor field. */
          pChar = strchr(argv[i], '=');

          criteria.recvWinFactor = (meshFriendshipRecvWinFactor_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "minqszlog=") != NULL)
        {
          /* Found MinQueueSize Log field. */
          pChar = strchr(argv[i], '=');

          criteria.minQueueSizeLog = (meshFriendshipMinQueueSizeLog_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "sleep=") != NULL)
        {
          /* Found Sleep Duration field. */
          pChar = strchr(argv[i], '=');

          sleepDurationMs = (uint32_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "recvdelay=") != NULL)
        {
          /* Found Receive Delay field. */
          pChar = strchr(argv[i], '=');

          recvDelayMs = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "retrycnt=") != NULL)
        {
          /* Found Retry Count field. */
          pChar = strchr(argv[i], '=');

          retryCount = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("lpn_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    if (est == TRUE)
    {
      if (MeshLpnEstablishFriendship(netKeyIndex, &criteria, sleepDurationMs, recvDelayMs, retryCount) ==
          FALSE)
      {
        TerminalTxPrint("lpn_cnf est_failed nidx=0x%x" TERMINAL_STRING_NEW_LINE, netKeyIndex);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else if (term == TRUE)
    {
      MeshLpnTerminateFriendship(netKeyIndex);
    }
  }

  TerminalTxStr("lpn_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalProxyClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshGattProxyConnId_t connId = 0xFF;
  uint16_t netKeyIndex = 0xFFFF;
  uint8_t i;
  meshProxyFilterType_t filType;
  meshAddress_t address;

  if (argc < 4)
  {
    TerminalTxStr("proxycl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    for (i = 1; i < argc - 1; i++)
    {
      if (strstr(argv[i], "ifid=") != NULL)
      {
        /* Found ID field. */
        pChar = strchr(argv[i], '=');

        connId = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "nidx=") != NULL)
      {
        /* Found NetKey index field. */
        pChar = strchr(argv[i], '=');

        netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else
      {
        TerminalTxPrint("proxycl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }

    if (strstr(argv[i], "filtype=") != NULL)
    {
      /* Found Filter Type field. */
      pChar = strchr(argv[i], '=');

      filType = (uint8_t)strtol(pChar + 1, NULL, 0);

      /* Set filter type */
      MeshProxyClSetFilterType(connId, netKeyIndex, filType);
    }
    else if (strstr(argv[i], "add=") != NULL)
    {
      /* Found address field. */
      pChar = strchr(argv[i], '=');

      address = (uint16_t)strtol(pChar + 1, NULL, 0);

      /* Add address to filter */
      MeshProxyClAddToFilter(connId, netKeyIndex, &address, 1);
    }
    else if (strstr(argv[i], "rm=") != NULL)
    {
      /* Found address field. */
      pChar = strchr(argv[i], '=');

      address = (uint16_t)strtol(pChar + 1, NULL, 0);

      /* Remove address to filter */
      MeshProxyClRemoveFromFilter(connId, netKeyIndex, &address, 1);
    }
    else
    {
      TerminalTxPrint("proxycl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("proxycl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalPrvClAuthHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshPrvClSelectAuth_t selectAuth;
  uint8_t oobpk = 0;
  uint8_t method = 0;
  uint8_t action = 0;
  uint8_t size = 0;
  uint8_t i;

  if (argc < 5)
  {
    TerminalTxStr("prvclauth_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "oobpk=") != NULL)
      {
        pChar = strchr(argv[i], '=');

        oobpk = (uint8_t)strtol(pChar + 1, NULL, 10);

        if (oobpk > 1)
        {
          TerminalTxPrint("prvclauth_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else if (strstr(argv[i], "method=") != NULL)
      {
        pChar = strchr(argv[i], '=');

        method = (uint8_t)strtol(pChar + 1, NULL, 10);

        if (method > 3)
        {
          TerminalTxPrint("prvclauth_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else if (strstr(argv[i], "action=") != NULL)
      {
        pChar = strchr(argv[i], '=');

        action = (uint8_t)strtol(pChar + 1, NULL, 10);

        if ((method == 2 && action > 4) || (method == 3 && action > 3))
        {
          TerminalTxPrint("prvclauth_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else if (strstr(argv[i], "size=") != NULL)
      {
        pChar = strchr(argv[i], '=');

        size = (uint8_t)strtol(pChar + 1, NULL, 10);

        if (size > 8)
        {
          TerminalTxPrint("prvclauth_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else
      {
        TerminalTxPrint("prvclauth_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }

    /* Select authentication. */
    selectAuth.useOobPublicKey = (oobpk == 1);
    selectAuth.oobAuthMethod = method;

    if (method == 2)
    {
      selectAuth.oobAction.outputOobAction = (1 << action);
    }
    else
    {
      selectAuth.oobAction.inputOobAction = (1 << action);
    }

    selectAuth.oobSize = size;

    MeshPrvClSelectAuthentication(&selectAuth);

    TerminalTxStr("prvclauth_cnf success" TERMINAL_STRING_NEW_LINE);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalPrvClCancelHandler(uint32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  MeshPrvClCancel();

  TerminalTxStr("prvclcancel_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalPrvClCfgHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t uuid[MESH_PRV_DEVICE_UUID_SIZE] = { 0x00 };
  uint16_t netKeyIndex = 0xFFFF;
  uint8_t keyArray[MESH_KEY_SIZE_128] = { 0x00 };
  uint32_t ivIdx;
  bool_t uuidFlag = FALSE;
  bool_t netKeyIndexFlag = FALSE;
  bool_t netKeyFlag = FALSE;
  bool_t ivIdxFlag = FALSE;
  uint8_t i;

  if (argc < 2)
  {
    TerminalTxStr("prvclcfg_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "devuuid=") != NULL)
      {
        pChar = strchr(argv[i], '=');

        WStrHexToArray(pChar + 1, uuid, sizeof(uuid));

        uuidFlag = TRUE;
      }
      else if (strstr(argv[i], "nidx=") != NULL)
      {
        /* Found Net Key Index field. */
        pChar = strchr(argv[i], '=');

        netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);

        netKeyIndexFlag = TRUE;
      }
      else if (strstr(argv[i], "netkey=") != NULL)
      {
        /* Found NetKey field. */
        pChar = strchr(argv[i], '=');

        WStrHexToArray(pChar + 1, keyArray, sizeof(keyArray));

        netKeyFlag = TRUE;
      }
      else if (strstr(argv[i], "ividx=") != NULL)
      {
        /* Found ivIndex field. */
        pChar = strchr(argv[i], '=');

        ivIdx = (uint32_t)strtol(pChar + 1, NULL, 0);

        ivIdxFlag = TRUE;
      }
      else
      {
        TerminalTxPrint("prvclcfg_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }

    /* Device UUID is mandatory. */
    if (uuidFlag == FALSE)
    {
      TerminalTxStr("prvclcfg_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }

    /* Set Device UUID. */
    memcpy(testAppPrvClSessionInfo.pDeviceUuid, uuid, MESH_PRV_DEVICE_UUID_SIZE);

    /* Set NetKey. */
    if (netKeyFlag == TRUE)
    {
      memcpy(testAppPrvClSessionInfo.pData->pNetKey, keyArray, MESH_KEY_SIZE_128);
    }

    /* Set NetKey index. */
    if (netKeyIndexFlag == TRUE)
    {
      testAppPrvClSessionInfo.pData->netKeyIndex = netKeyIndex;
    }

    /* Set IV index. */
    if (ivIdxFlag == TRUE)
    {
      testAppPrvClSessionInfo.pData->ivIndex = ivIdx;
    }

    TerminalTxStr("prvclcfg_cnf success" TERMINAL_STRING_NEW_LINE);
  }

  return TERMINAL_ERROR_OK;
}

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))

static uint8_t testAppTerminalPrvClose(uint32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  MeshTestPrvBrTriggerLinkClose();

  TerminalTxStr("prvclose_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

#endif

static uint8_t testAppTerminalPrvOobHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshPrvInOutOobData_t oobData = { 0x00 };
  uint8_t alphaLen = 0;

  if (argc < 2)
  {
    TerminalTxStr("prvoob_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "num=") != NULL)
    {
      /* Found Numeric OOB field. */
      pChar = strchr(argv[1], '=');

      oobData.numericOob = (uint32_t)strtol(pChar + 1, NULL, 0);

      if (termCb.prvIsServer)
      {
        MeshPrvSrInputComplete((meshPrvInputOobSize_t)0, oobData);
      }
      else
      {
        MeshPrvClEnterOutputOob((meshPrvOutputOobSize_t)0, oobData);
      }
    }
    else if (strstr(argv[1], "alpha=") != NULL)
    {
      /* Found Alphanumeric OOB field. */
      pChar = strchr(argv[1], '=');

      pChar++;
      alphaLen = strlen(pChar);
      if (alphaLen > MESH_PRV_INOUT_OOB_MAX_SIZE)
      {
        TerminalTxPrint("prvoob_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);
        return TERMINAL_ERROR_EXEC;
      }

      memcpy(oobData.alphanumericOob, pChar, alphaLen);

      if (termCb.prvIsServer)
      {
        MeshPrvSrInputComplete((meshPrvInputOobSize_t)alphaLen, oobData);
      }
      else
      {
        MeshPrvClEnterOutputOob((meshPrvOutputOobSize_t)alphaLen, oobData);
      }
    }
    else
    {
      TerminalTxPrint("prvoob_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("prvoob_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalStartPbAdvHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint16_t addr = MESH_ADDR_TYPE_UNASSIGNED;
  uint8_t i;

  if (argc < 2)
  {
    TerminalTxStr("startpbadv_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "addr=") != NULL)
      {
        pChar = strchr(argv[i], '=');

        addr = (uint16_t)strtol(pChar + 1, NULL, 0);

        if (!MESH_IS_ADDR_UNICAST(addr))
        {
          TerminalTxPrint("startpbadv_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
      else
      {
        TerminalTxPrint("startpbadv_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

        return TERMINAL_ERROR_EXEC;
      }
    }

    /* Ensure Provisioning Client is initialized. */
    if (!termCb.prvClInitialized)
    {
      if (termCb.prvSrInitialized)
      {
        TerminalTxPrint("startpbadv_cnf invalid_state prvsr_initialized" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }
      else
      {
        termCb.prvClInitialized = TRUE;

        TestAppInitPrvCl();

        /* Bind the interface. */
        MeshAddAdvIf(TESTAPP_ADV_IF_ID);
      }
    }

    /* Save provisioning mode. */
    pMeshPrvSrCfg->pbAdvIfId = TESTAPP_ADV_IF_ID;
    termCb.prvIsServer = FALSE;

    /* Enter provisioning. */
    testAppPrvClSessionInfo.pData->address = addr;

    MeshPrvClStartPbAdvProvisioning(TESTAPP_ADV_IF_ID, &testAppPrvClSessionInfo);

    TerminalTxStr("startpbadv_cnf success" TERMINAL_STRING_NEW_LINE);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalTestIvHandler(uint32_t argc, char **argv)
{
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  char *pChar;
  uint32_t ivIndex;
  bool_t ivUpdate;
  bool_t enabled = FALSE, transExists = FALSE, update = FALSE;

  if (argc < 2)
  {
    TerminalTxStr("testiv_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Found ON command. */
    if (strcmp(argv[1], "on") == 0)
    {
      enabled = TRUE;
    }
    /* Found OFF command. */
    else if (strcmp(argv[1], "off") == 0)
    {
      enabled = FALSE;
    }
    else
    {
      TerminalTxPrint("testiv_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
    /* Search for state transition parameter. */
    if(argc > 2)
    {
      if (strstr(argv[2], "state=") != NULL)
      {

        pChar = strchr(argv[2], '=');

        if(strcmp(pChar + 1, "normal") == 0)
        {
          transExists = TRUE;
          update = FALSE;
        }
        else if(strcmp(pChar + 1, "update") == 0)
        {
          transExists = TRUE;
          update = TRUE;
        }
        else
        {
          TerminalTxPrint("testiv_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

          return TERMINAL_ERROR_EXEC;
        }
       }
    }

    /* Set mode. */
    MeshTestIvConfigTestMode(enabled, transExists, update, &ivIndex, &ivUpdate);

    TerminalTxPrint("testiv_cnf success iv=%d ivUpdate=%d " TERMINAL_STRING_NEW_LINE,
                    ivIndex, ivUpdate);

    return TERMINAL_ERROR_OK;
  }

#else
  (void)argc;
  (void)argv;

  TerminalTxStr("testiv_cnf not_supported" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
#endif
}

static uint8_t testAppTerminalTestNetKeyHandler(uint32_t argc, char **argv)
{
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  char *pChar;
  uint16_t listSize;

  if (argc < 2)
  {
    TerminalTxStr("testnetkey_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "listsize=") != NULL)
    {
      /* Found NetKey list size field. */
      pChar = strchr(argv[1], '=');

      listSize = (uint16_t)strtol(pChar + 1, NULL, 0);

      listSize = MeshTestAlterNetKeyListSize(listSize);

      TerminalTxPrint("testnetkey_cnf success origsize=%d newsize=%d" TERMINAL_STRING_NEW_LINE,
                      pMeshConfig->pMemoryConfig->netKeyListSize, listSize);
    }
    else
    {
      TerminalTxPrint("testnetkey_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

#else
  (void)argc;
  (void)argv;

  TerminalTxStr("testnetkey_cnf not_supported" TERMINAL_STRING_NEW_LINE);
#endif

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalTestRpHandler(uint32_t argc, char **argv)
{
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  if (argc < 2)
  {
    TerminalTxStr("testrp_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "clear") == 0)
    {
      MeshTestRpClearList();
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      if (strcmp(argv[2], "listsize") == 0)
      {
        TerminalTxPrint("testrp_cnf success listsize=%d" TERMINAL_STRING_NEW_LINE,
                        pMeshConfig->pMemoryConfig->rpListSize);

        return TERMINAL_ERROR_OK;
      }
      else
      {
        TerminalTxPrint("testrp_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else
    {
      TerminalTxPrint("testrp_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("testrp_cnf success" TERMINAL_STRING_NEW_LINE);
#else
  (void)argc;
  (void)argv;

  TerminalTxStr("testrp_cnf not_supported" TERMINAL_STRING_NEW_LINE);
#endif

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalTlogHandler(uint32_t argc, char **argv)
{
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  uint16_t mask = 0x00;
  uint8_t i;

  if (argc < 2)
  {
    TerminalTxStr("tlog_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "off") == 0)
    {
      mask = 0x00;
    }
    else
    {
      for (i = 1; i < argc; i++)
      {
        if (strcmp(argv[i], "all") == 0)
        {
          mask = 0xFF;

          MeshTestSetListenMask(mask);

          TerminalTxStr("tlog_cnf success" TERMINAL_STRING_NEW_LINE);

          return TERMINAL_ERROR_OK;
        }
        else if (strcmp(argv[i], "prvbr") == 0)
        {
          mask |= MESH_TEST_PRVBR_LISTEN;
        }
        else if (strcmp(argv[i], "nwk") == 0)
        {
          mask |= MESH_TEST_NWK_LISTEN;
        }
        else if (strcmp(argv[i], "sar") == 0)
        {
          mask |= MESH_TEST_SAR_LISTEN;
        }
        else if (strcmp(argv[i], "utr") == 0)
        {
          mask |= MESH_TEST_UTR_LISTEN;
        }
        else if (strcmp(argv[i], "proxy") == 0)
        {
          mask |= MESH_TEST_PROXY_LISTEN;
        }
        else
        {
          TerminalTxPrint("tlog_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  MeshTestSetListenMask(mask);

  TerminalTxStr("tlog_cnf success" TERMINAL_STRING_NEW_LINE);
#else
  (void)argc;
  (void)argv;

  TerminalTxStr("tlog_cnf not_supported" TERMINAL_STRING_NEW_LINE);
#endif

  return TERMINAL_ERROR_OK;
}

static uint8_t testAppTerminalSendSnbHandler(uint32_t argc, char **argv)
{
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  uint16_t  netKeyIndex = 0xFFFF;
  char *pChar;

  if (argc < 2)
  {
    TerminalTxStr("testsnb_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "nidx=") != NULL)
    {
      /* Found Net Key Index field. */
      pChar = strchr(argv[1], '=');

      netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);

      /* Trigger the send of a Secure Network Beacon for this sub-net */
      MeshTestSendNwkBeacon(netKeyIndex);
    }
    else
    {
      TerminalTxPrint("testsnb_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("testsnb_cnf success" TERMINAL_STRING_NEW_LINE);
#else
  (void)argc;
  (void)argv;

  TerminalTxStr("testsnb_cnf not_supported" TERMINAL_STRING_NEW_LINE);
#endif

  return TERMINAL_ERROR_OK;
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Mesh Test terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void testAppTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(testAppTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&testAppTerminalTbl[i]);
  }
}
