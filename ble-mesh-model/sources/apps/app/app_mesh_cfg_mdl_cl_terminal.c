/*************************************************************************************************/
/*!
 *  \file   app_mesh_cfg_mdl_cl_terminal.c
 *
 *  \brief  Common Mesh Config Client Terminal handler.
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
#include <stdio.h>

#include "util/print.h"
#include "util/terminal.h"
#include "util/wstr.h"
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_buf.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"

#include "mmdl_defs.h"
#include "mmdl_types.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_cfg_mdl_cl_pg0_bstream.h"

#include "mesh_ht_cl_api.h"
#include "mesh_ht_sr_api.h"

#include "app_mesh_api.h"
#include "app_mesh_cfg_mdl_cl_terminal.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

typedef struct appMeshCfgMdlClTerminalCb_tag
{
  meshAddress_t srAddr;                   /*!< Primary element containing an instance of
                                           *   Configuration Server model
                                           */
  uint8_t *pSrDevKey;                     /*!< Pointer to Device Key of the remote Configuration
                                           *   Server or NULL for Local Node
                                           */
  uint16_t srNetKeyIndex;                 /*!< Global identifier of the Network on which the request
                                           *   is sent
                                           */
  uint8_t srDevKeyBuf[MESH_KEY_SIZE_128]; /*!< Buffer to store the Device Key */
} appMeshCfgMdlClTerminalCb_t;

/**************************************************************************************************
  Local Functions Prototypes
**************************************************************************************************/

/*! \brief   Handler for Configuration Client AppKey terminal command */
static uint8_t appMeshCfgMdlClTerminalCcAppKeyHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Secure Network Beacon terminal command */
static uint8_t appMeshCfgMdlClTerminalCcBeaconHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Composition Data terminal command */
static uint8_t appMeshCfgMdlClTerminalCcCompDataHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Default TTL terminal command */
static uint8_t appMeshCfgMdlClTerminalCcDefTtlHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Destination Server terminal command */
static uint8_t appMeshCfgMdlClTerminalCcDstHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Friend terminal command */
static uint8_t appMeshCfgMdlClTerminalCcFriendHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Gatt Proxy terminal command */
static uint8_t appMeshCfgMdlClTerminalCcGattProxylHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Heartbeat Publication terminal command */
static uint8_t appMeshCfgMdlClTerminalCcHbPubHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Heartbeat Subscription terminal command */
static uint8_t appMeshCfgMdlClTerminalCcHbSubHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Key Refresh Phase terminal command */
static uint8_t appMeshCfgMdlClTerminalCcKeyRefreshHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Low Power Node PollTimeout terminal command */
static uint8_t appMeshCfgMdlClTerminalCcPollTimeoutHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Model App terminal command */
static uint8_t appMeshCfgMdlClTerminalCcModelAppHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Model Publication terminal command */
static uint8_t appMeshCfgMdlClTerminalCcMdlPubHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Model Publication Virtual terminal command */
static uint8_t appMeshCfgMdlClTerminalCcMdlPubVirtualHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Model Subscription terminal command */
static uint8_t appMeshCfgMdlClTerminalCcModelSubHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Model Subscription Virtual terminal command */
static uint8_t appMeshCfgMdlClTerminalCcMdlSubVirtualHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client NetKey terminal command */
static uint8_t appMeshCfgMdlClTerminalCcNetKeyHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Node Identity terminal command */
static uint8_t appMeshCfgMdlClTerminalCcNodeIdentityHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Node Reset terminal command */
static uint8_t appMeshCfgMdlClTerminalCcNodeRstHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Network Transmit terminal command */
static uint8_t appMeshCfgMdlClTerminalCcNwkTransHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Configuration Client Relay terminal command */
static uint8_t appMeshCfgMdlClTerminalCcRelayHandler(uint32_t argc, char **argv);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! App Config Client Terminal commands table */
static terminalCommand_t appMeshCfgMdlClTerminalTbl[] =
{
    /*! Configuration Client AppKey */
    { NULL, "ccappkey", "ccappkey <add|updt|del|get|aidx|key|nidx>", appMeshCfgMdlClTerminalCcAppKeyHandler },
    /*! Configuration Client Beacon */
    { NULL, "ccbeacon", "ccbeacon <set|get|beacon>", appMeshCfgMdlClTerminalCcBeaconHandler },
    /*! Configuration Composition Data */
    { NULL, "cccompdata", "cccompdata <get|page>", appMeshCfgMdlClTerminalCcCompDataHandler },
    /*! Configuration Client Default TTL */
    { NULL, "ccdefttl", "ccdefttl <set|get|ttl>", appMeshCfgMdlClTerminalCcDefTtlHandler },
    /*! Configuration Client Destination Server */
    { NULL, "ccdst", "ccdst <addr|nidx|devkey>", appMeshCfgMdlClTerminalCcDstHandler },
    /*! Configuration Client Friend */
    { NULL, "ccfrnd", "ccfrnd <set|get|friend>", appMeshCfgMdlClTerminalCcFriendHandler },
    /*! Configuration Client GATT Proxy */
    { NULL, "ccgattproxy", "ccgattproxy <set|get|proxy>", appMeshCfgMdlClTerminalCcGattProxylHandler },
    /*! Configuration Client Heartbeat Publication */
    { NULL, "cchbpub", "cchbpub <set|get|dst|count|period|ttl|feat|nidx>", appMeshCfgMdlClTerminalCcHbPubHandler },
    /*! Configuration Client Heartbeat Subscription */
    { NULL, "cchbsub", "cchbsub <set|get|src|dst|period|>", appMeshCfgMdlClTerminalCcHbSubHandler },
    /*! Configuration Client Key Refresh Phase */
    { NULL, "cckeyrp", "cckeyrp <set|get|nidx|trans>", appMeshCfgMdlClTerminalCcKeyRefreshHandler },
    /*! Configuration Client Key Refresh Phase */
    { NULL, "cclpnpt", "cclpnpt <get|lpnaddr>", appMeshCfgMdlClTerminalCcPollTimeoutHandler },
    /*! Configuration Client Model to AppKey Bind */
    { NULL, "ccmodelapp", "ccmodelapp <bind|unbind|get|vend|elemaddr|aidx|modelid>",
      appMeshCfgMdlClTerminalCcModelAppHandler },
    /*! Configuration Client Model Publication */
    { NULL, "ccmodelpub", "ccmodelpub <set|get|vend|elemaddr|pubaddr|aidx|cred|ttl|persteps|perstepres|count|steps|modelid>",
      appMeshCfgMdlClTerminalCcMdlPubHandler },
    /*! Configuration Client Model Publication Virtual */
    { NULL, "ccmodelpubvirt", "ccmodelpubvirt <set|vend|elemaddr|uuid|aidx|cred|ttl|persteps|perstepres|count|steps|modelid>",
      appMeshCfgMdlClTerminalCcMdlPubVirtualHandler },
    /*! Configuration Client Model Subscription */
    { NULL, "ccmodelsub", "ccmodelsub <add|del|ovr|get|vend|elemaddr|subaddr|modelid>",
      appMeshCfgMdlClTerminalCcModelSubHandler },
    /*! Configuration Client Model Subscription Virtual */
    { NULL, "ccmodelsubvirt", "ccmodelsubvirt <add|del|ovr|vend|elemaddr|uuid|modelid>",
      appMeshCfgMdlClTerminalCcMdlSubVirtualHandler },
    /*! Configuration Client NetKey */
    { NULL, "ccnetkey", "ccnetkey <add|updt|del|get|nidx|key>", appMeshCfgMdlClTerminalCcNetKeyHandler },
    /*! Configuration Client Node Identity */
    { NULL, "ccnodeident", "ccnodeident <set|get|nidx|ident>",
      appMeshCfgMdlClTerminalCcNodeIdentityHandler },
    /*! Configuration Client Node Reset */
    { NULL, "ccnoderst", "ccnoderst", appMeshCfgMdlClTerminalCcNodeRstHandler },
    /*! Configuration Client Network Transmit */
    { NULL, "ccnwktrans", "ccnwktrans <set|get|count|steps>", appMeshCfgMdlClTerminalCcNwkTransHandler },
    /*! Configuration Client Relay */
    { NULL, "ccrelay", "ccrelay <set|get|relay|count|steps>", appMeshCfgMdlClTerminalCcRelayHandler },
};

/*! Strings mapping the Configuration Client Events */
static const char *pAppMeshCfgMdlClTerminalEvt[] =
{
  "ccbeacon_ind get ",
  "ccbeacon_ind set ",
  "cccompdata_ind get ",
  "ccdefttl_ind get ",
  "ccdefttl_ind set ",
  "ccgattproxy_ind get ",
  "ccgattproxy_ind set ",
  "ccrelay_ind get ",
  "ccrelay_ind set ",
  "ccmodelpub_ind get ",
  "ccmodelpub_ind set ",
  "ccmodelpubvirt_ind set ",
  "ccmodelsub_ind add ",
  "ccmodelsubvirt_ind add ",
  "ccmodelsub_ind del ",
  "ccmodelsubvirt_ind del ",
  "ccmodelsub_ind ovr ",
  "ccmodelsubvirt_ind ovr ",
  "ccmodelsub_ind del_all ",
  "ccmodelsub_ind sig_get ",
  "ccmodelsub_ind vendor_get ",
  "ccnetkey_ind add ",
  "ccnetkey_ind updt ",
  "ccnetkey_ind del ",
  "ccnetkey_ind get ",
  "ccappkey_ind add ",
  "ccappkey_ind updt ",
  "ccappkey_ind del ",
  "ccappkey_ind get ",
  "ccnodeident_ind get ",
  "ccnodeident_ind set ",
  "ccmodelapp_ind bind ",
  "ccmodelapp_ind unbind ",
  "ccmodelapp_ind sig_get ",
  "ccmodelapp_ind vendor_get ",
  "ccnoderst_ind ",
  "ccfrnd_ind get ",
  "ccfrnd_ind set ",
  "cckeyrp_ind get ",
  "cckeyrp_ind set ",
  "cchbpub_ind get ",
  "cchbpub_ind set ",
  "cchbsub_ind get ",
  "cchbsub_ind set ",
  "cclpnpt_ind get ",
  "ccnwktrans_ind get ",
  "ccnwktrans_ind set "
};

/*! Strings mapping the Configuration Client Events status */
static const char *pAppMeshCfgMdlClTerminalEvtStatus[] =
{
  "success ",
  "out_of_resources ",
  "invalid_params ",
  "timeout ",
  "unknown_error ",
  "reserved",
  "invalid_addr ",
  "invalid_model ",
  "invalid_appkey_index ",
  "invalid_netkey_index ",
  "insufficient_resources ",
  "key_index_exists ",
  "invalid_pub_params ",
  "not_subscribe_model ",
  "storage_failure ",
  "feature_not_supported ",
  "cannot_update ",
  "cannot_remove ",
  "cannot_bind ",
  "temp_unable_to_change_state ",
  "cannot_set ",
  "unspecified ",
  "invalid_binding "
};

/*! Mesh Configuration Client control block */
static appMeshCfgMdlClTerminalCb_t appMeshCfgMdlClTerminalCb =
{
  .srAddr = MESH_ADDR_TYPE_UNASSIGNED,
  .pSrDevKey = NULL,
  .srNetKeyIndex = 0x0000,
  .srDevKeyBuf = { 0x00 }
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client AppKey terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcAppKeyHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint16_t appKeyIndex = 0xFFFF;
  uint16_t netKeyIndex = 0xFFFF;
  meshAppNetKeyBind_t keyBind;
  uint8_t keyArray[MESH_KEY_SIZE_128];
  uint8_t i;
  bool_t add = FALSE;
  bool_t del = FALSE;
  bool_t updt = FALSE;
  bool_t get = FALSE;

  if (argc < 2)
  {
    TerminalTxStr("ccappkey_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "add") == 0)
    {
      add = TRUE;
    }
    else if (strcmp(argv[1], "del") == 0)
    {
      del = TRUE;
    }
    else if (strcmp(argv[1], "updt") == 0)
    {
      updt = TRUE;
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      get = TRUE;
    }
    else
    {
      TerminalTxPrint("ccappkey_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 5) && ((add == TRUE) || (updt == TRUE))) || ((argc < 4) && (del == TRUE)) ||
        ((argc < 3) && (get == TRUE)))
    {
      TerminalTxStr("ccappkey_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "aidx=") != NULL)
        {
          /* Found AppKey index field. */
          pChar = strchr(argv[i], '=');

          appKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "key=") != NULL)
        {
          /* Found AppKey field. */
          pChar = strchr(argv[i], '=');

          WStrHexToArray(pChar + 1, keyArray, sizeof(keyArray));
        }
        else if (strstr(argv[i], "nidx=") != NULL)
        {
          /* Found NetKey index field. */
          pChar = strchr(argv[i], '=');

          netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccappkey_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  keyBind.appKeyIndex = appKeyIndex;
  keyBind.netKeyIndex = netKeyIndex;

  if (add == TRUE)
  {
    MeshCfgMdlClAppKeyChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                          appMeshCfgMdlClTerminalCb.pSrDevKey, &keyBind, MESH_CFG_MDL_CL_KEY_ADD, keyArray);
  }
  else if (updt == TRUE)
  {
    MeshCfgMdlClAppKeyChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                          appMeshCfgMdlClTerminalCb.pSrDevKey, &keyBind, MESH_CFG_MDL_CL_KEY_UPDT, keyArray);
  }
  else if (del == TRUE)
  {
    MeshCfgMdlClAppKeyChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                          appMeshCfgMdlClTerminalCb.pSrDevKey, &keyBind, MESH_CFG_MDL_CL_KEY_DEL, NULL);
  }
  else if (get == TRUE)
  {
    MeshCfgMdlClAppKeyGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.pSrDevKey,
                          appMeshCfgMdlClTerminalCb.srNetKeyIndex, netKeyIndex);
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Secure Network Beacon terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcBeaconHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t beacon;

  if (argc < 2)
  {
    TerminalTxStr("ccbeacon_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 3)
      {
        TerminalTxStr("ccbeacon_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      if (strstr(argv[2], "beacon=") != NULL)
      {
        /* Found Beacon field. */
        pChar = strchr(argv[2], '=');

        beacon = (uint8_t)strtol(pChar + 1, NULL, 0);

        MeshCfgMdlClBeaconSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                              appMeshCfgMdlClTerminalCb.pSrDevKey, beacon);
      }
      else
      {
        TerminalTxPrint("ccbeacon_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClBeaconGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                            appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("ccbeacon_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Composition Data terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcCompDataHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t page;

  if (argc < 2)
  {
    TerminalTxStr("cccompdata_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      if (argc < 3)
      {
        TerminalTxStr("cccompdata_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      if (strstr(argv[2], "page=") != NULL)
      {
        /* Found Page field. */
        pChar = strchr(argv[2], '=');

        page = (uint8_t)strtol(pChar + 1, NULL, 0);

        MeshCfgMdlClCompDataGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                appMeshCfgMdlClTerminalCb.pSrDevKey, page);
      }
      else
      {
        TerminalTxPrint("cccompdata_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else
    {
      TerminalTxPrint("cccompdata_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Default TTL terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcDefTtlHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t ttl;

  if (argc < 2)
  {
    TerminalTxStr("ccdefttl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 3)
      {
        TerminalTxStr("ccdefttl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      if (strstr(argv[2], "ttl=") != NULL)
      {
        /* Found TTL field. */
        pChar = strchr(argv[2], '=');

        ttl = (uint8_t)strtol(pChar + 1, NULL, 0);

        MeshCfgMdlClDefaultTtlSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                  appMeshCfgMdlClTerminalCb.pSrDevKey, ttl);
      }
      else
      {
        TerminalTxPrint("ccdefttl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClDefaultTtlGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("ccdefttl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Destination Server terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcDstHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshAddress_t addr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t netKeyIndex = 0xFFFF;
  uint8_t keyArray[MESH_KEY_SIZE_128];
  uint8_t i;

  if (argc < 2)
  {
    TerminalTxStr("ccdst_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "addr=") != NULL)
    {
      /* Found destination address field. */
      pChar = strchr(argv[1], '=');

      addr = (meshAddress_t)strtol(pChar + 1, NULL, 0);

      if (MESH_IS_ADDR_UNICAST(addr))
      {
        if (argc < 4)
        {
          TerminalTxStr("ccdst_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

          return TERMINAL_ERROR_EXEC;
        }

        for (i = 2; i < argc; i++)
        {
          if (strstr(argv[i], "nidx=") != NULL)
          {
            /* Found Net Key index field. */
            pChar = strchr(argv[i], '=');

            netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
          }
          else if (strstr(argv[i], "devkey=") != NULL)
          {
            /* Found DevKey field. */
            pChar = strchr(argv[i], '=');

            WStrHexToArray(pChar + 1, keyArray, sizeof(keyArray));
          }
          else
          {
            TerminalTxPrint("ccdst_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
        }

        appMeshCfgMdlClTerminalCb.srAddr = addr;
        memcpy(appMeshCfgMdlClTerminalCb.srDevKeyBuf, keyArray, MESH_KEY_SIZE_128);
        appMeshCfgMdlClTerminalCb.pSrDevKey = appMeshCfgMdlClTerminalCb.srDevKeyBuf;
        appMeshCfgMdlClTerminalCb.srNetKeyIndex = netKeyIndex;
      }
      else if (MESH_IS_ADDR_UNASSIGNED(addr))
      {
        if (argc < 3)
        {
          TerminalTxStr("ccdst_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

          return TERMINAL_ERROR_EXEC;
        }

        if (strstr(argv[2], "nidx=") != NULL)
        {
          /* Found Net Key index field. */
          pChar = strchr(argv[2], '=');

          netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccdst_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

          return TERMINAL_ERROR_EXEC;
        }

        appMeshCfgMdlClTerminalCb.srAddr = MESH_ADDR_TYPE_UNASSIGNED;
        appMeshCfgMdlClTerminalCb.pSrDevKey = NULL;
        appMeshCfgMdlClTerminalCb.srNetKeyIndex = netKeyIndex;
      }
      else
      {
        TerminalTxPrint("ccdst_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[1]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else
    {
      TerminalTxPrint("ccdst_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("ccdst_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Friend terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcFriendHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshFriendStates_t friendState;

  if (argc < 2)
  {
    TerminalTxStr("ccfrnd_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 3)
      {
        TerminalTxStr("ccfrnd_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      if (strstr(argv[2], "friend=") != NULL)
      {
        /* Found friend field. */
        pChar = strchr(argv[2], '=');

        friendState = (meshFriendStates_t)strtol(pChar + 1, NULL, 0);

        MeshCfgMdlClFriendSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                              appMeshCfgMdlClTerminalCb.pSrDevKey, friendState);
      }
      else
      {
        TerminalTxPrint("ccfrnd_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClFriendGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                            appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("ccfrnd_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client GATT Proxy terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcGattProxylHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t gattProxy;

  if (argc < 2)
  {
    TerminalTxStr("ccgattproxy_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 3)
      {
        TerminalTxStr("ccgattproxy_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      if (strstr(argv[2], "proxy=") != NULL)
      {
        /* Found proxy field. */
        pChar = strchr(argv[2], '=');

        gattProxy = (uint8_t)strtol(pChar + 1, NULL, 0);

        MeshCfgMdlClGattProxySet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                 appMeshCfgMdlClTerminalCb.pSrDevKey, gattProxy);
      }
      else
      {
        TerminalTxPrint("ccgattproxy_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClGattProxyGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                               appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("ccgattproxy_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Heartbeat Publication terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcHbPubHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t i;
  meshHbPub_t meshHbPub = { 0x00 };

  if (argc < 2)
  {
    TerminalTxStr("cchbpub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 8)
      {
        TerminalTxStr("cchbpub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "dst=") != NULL)
        {
          /* Found destination address field. */
          pChar = strchr(argv[i], '=');

          meshHbPub.dstAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "count=") != NULL)
        {
          /* Found number of heartbeats field. */
          pChar = strchr(argv[i], '=');

          meshHbPub.countLog = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "period=") != NULL)
        {
          /* Found node identity state field. */
          pChar = strchr(argv[i], '=');

          meshHbPub.periodLog = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ttl=") != NULL)
        {
          /* Found TTL field. */
          pChar = strchr(argv[i], '=');

          meshHbPub.ttl = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "feat=") != NULL)
        {
          /* Found features field. */
          pChar = strchr(argv[i], '=');

          meshHbPub.features = (meshFeatures_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "nidx=") != NULL)
        {
          /* Found NetKey Index field. */
          pChar = strchr(argv[i], '=');

          meshHbPub.netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("cchbpub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClHbPubSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                           appMeshCfgMdlClTerminalCb.pSrDevKey, &meshHbPub);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClHbPubGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                           appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("cchbpub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Heartbeat Subscription terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcHbSubHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t i;
  meshHbSub_t meshHbSub = { 0x00 };

  if (argc < 2)
  {
    TerminalTxStr("cchbsub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 5)
      {
        TerminalTxStr("cchbsub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "src=") != NULL)
        {
          /* Found source address field. */
          pChar = strchr(argv[i], '=');

          meshHbSub.srcAddr= (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "dst=") != NULL)
        {
          /* Found destination address field. */
          pChar = strchr(argv[i], '=');

          meshHbSub.dstAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "period=") != NULL)
        {
          /* Found period field. */
          pChar = strchr(argv[i], '=');

          meshHbSub.periodLog = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("cchbsub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClHbSubSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                           appMeshCfgMdlClTerminalCb.pSrDevKey, &meshHbSub);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClHbSubGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                           appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("cchbsub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Key Refresh Phase terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcKeyRefreshHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t i;
  uint16_t netKeyIdx = 0xFFFF;
  meshKeyRefreshTrans_t transition = 0;

  if (argc < 3)
  {
    TerminalTxStr("cckeyrp_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 4)
      {
        TerminalTxStr("cckeyrp_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "nidx=") != NULL)
        {
          /* Found NetKey index field. */
          pChar = strchr(argv[i], '=');

          netKeyIdx = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          /* Found node identity state field. */
          pChar = strchr(argv[i], '=');

          transition = (meshKeyRefreshTrans_t)strtol(pChar + 1, NULL, 0);
        }
      }

      MeshCfgMdlClKeyRefPhaseSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                 appMeshCfgMdlClTerminalCb.pSrDevKey, netKeyIdx, transition);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      if (strstr(argv[2], "nidx=") != NULL)
      {
        /* Found NetKey index field. */
        pChar = strchr(argv[2], '=');

        netKeyIdx = (uint16_t)strtol(pChar + 1, NULL, 0);
      }

      MeshCfgMdlClKeyRefPhaseGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                 appMeshCfgMdlClTerminalCb.pSrDevKey, netKeyIdx);
    }
    else
    {
      TerminalTxPrint("cckeyrp_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Low Power Node PollTimeout terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcPollTimeoutHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshAddress_t lpnAddr = MESH_ADDR_TYPE_UNASSIGNED;

  if (argc < 3)
  {
    TerminalTxStr("cclpnpt_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      if (strstr(argv[2], "lpnaddr=") != NULL)
      {
        /* Found netkey index field. */
        pChar = strchr(argv[2], '=');

        lpnAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
      }

      MeshCfgMdlClPollTimeoutGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                 appMeshCfgMdlClTerminalCb.pSrDevKey, lpnAddr);
    }
    else
    {
      TerminalTxPrint("cclpnpt_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Model App terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcModelAppHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t modelId = 0;
  uint16_t appKeyIndex = 0xFFFF;
  meshAddress_t addr = MESH_ADDR_TYPE_UNASSIGNED;
  uint8_t i;
  bool_t bind = FALSE;
  bool_t unbind = FALSE;
  bool_t get = FALSE;
  bool_t vend = FALSE;

  if (argc < 2)
  {
    TerminalTxStr("ccmodelapp_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "bind") == 0)
    {
      bind = TRUE;
    }
    else if (strcmp(argv[1], "unbind") == 0)
    {
      unbind = TRUE;
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      get = TRUE;
    }
    else
    {
      TerminalTxPrint("ccmodelapp_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 5) && ((bind == TRUE) || (unbind == TRUE))) || ((argc < 4) && (get == TRUE)))
    {
      TerminalTxStr("ccmodelapp_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strcmp(argv[i], "vend") == 0)
        {
          /* Vendor Model ID used. */
          vend = TRUE;
        }
        else if (strstr(argv[i], "elemaddr=") != NULL)
        {
          /* Found address field. */
          pChar = strchr(argv[i], '=');

          addr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "aidx=") != NULL)
        {
          /* Found Net Key index field. */
          pChar = strchr(argv[i], '=');

          appKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "modelid=") != NULL)
        {
          /* Found Model ID field. */
          pChar = strchr(argv[i], '=');

          modelId = (uint32_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccmodelapp_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  if (bind == TRUE)
  {
    MeshCfgMdlClAppBind(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                        appMeshCfgMdlClTerminalCb.pSrDevKey, TRUE, appKeyIndex, addr,
                        (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                        (vend == FALSE));
  }
  else if (unbind == TRUE)
  {
    MeshCfgMdlClAppBind(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                       appMeshCfgMdlClTerminalCb.pSrDevKey, FALSE, appKeyIndex, addr,
                       (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                       (vend == FALSE));
  }
  else if (get == TRUE)
  {
    MeshCfgMdlClAppGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                       appMeshCfgMdlClTerminalCb.pSrDevKey, addr,
                       (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                       (vend == FALSE));
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Model Publication terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcMdlPubHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t modelId = 0;
  meshModelPublicationParams_t pubParams = { 0x00 };
  meshAddress_t elemAddr = MESH_ADDR_TYPE_UNASSIGNED;
  meshAddress_t pubAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint8_t i;
  bool_t vend = FALSE;

  if (argc < 4)
  {
    TerminalTxStr("ccmodelpub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 12)
      {
        TerminalTxStr("ccmodelpub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      for (i = 2; i < argc; i++)
      {
        if (strcmp(argv[i], "vend") == 0)
        {
          /* Vendor Model ID used. */
          vend = TRUE;
        }
        else if (strstr(argv[i], "elemaddr=") != NULL)
        {
          /* Found element address field. */
          pChar = strchr(argv[i], '=');

          elemAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "pubaddr=") != NULL)
        {
          /* Found publish address field. */
          pChar = strchr(argv[i], '=');

          pubAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "aidx=") != NULL)
        {
          /* Found AppKey index field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishAppKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "cred=") != NULL)
        {
          /* Found credential flag field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishFriendshipCred = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ttl=") != NULL)
        {
          /* Found publish ttl field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishTtl = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "persteps=") != NULL)
        {
          /* Found publish period field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishPeriodNumSteps = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "perstepres=") != NULL)
        {
          /* Found publish period field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishPeriodStepRes = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "count=") != NULL)
        {
          /* Found publish count field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishRetransCount = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "steps=") != NULL)
        {
          /* Found publish ttl field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishRetransSteps50Ms = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "modelid=") != NULL)
        {
          /* Found Model ID field. */
          pChar = strchr(argv[i], '=');

          modelId = (uint32_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccmodelpub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClPubSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                         appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr, pubAddr, NULL, &pubParams,
                         (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                         (vend == FALSE));
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      for (i = 2; i < argc; i++)
      {
        if (strcmp(argv[i], "vend") == 0)
        {
          /* Vendor Model ID used. */
          vend = TRUE;
        }
        else if (strstr(argv[i], "elemaddr=") != NULL)
        {
          /* Found element address field. */
          pChar = strchr(argv[i], '=');

          elemAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "modelid=") != NULL)
        {
          /* Found Model ID field. */
          pChar = strchr(argv[i], '=');

          modelId = (uint32_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccmodelpub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClPubGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                         appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr,
                         (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                         (vend == FALSE));
    }
    else
    {
      TerminalTxPrint("ccmodelpub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Model Publication Virtual terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcMdlPubVirtualHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t modelId = 0;
  meshModelPublicationParams_t pubParams = { 0x00 };
  meshAddress_t elemAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint8_t uuid[MESH_KEY_SIZE_128];
  uint8_t i;
  bool_t vend = FALSE;

  if (argc < 12)
  {
    TerminalTxStr("ccmodelpubvirt_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      for (i = 2; i < argc; i++)
      {
        if (strcmp(argv[i], "vend") == 0)
        {
          /* Vendor Model ID used. */
          vend = TRUE;
        }
        else if (strstr(argv[i], "elemaddr=") != NULL)
        {
          /* Found element address field. */
          pChar = strchr(argv[i], '=');

          elemAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "uuid=") != NULL)
        {
          /* Found uuid field. */
          pChar = strchr(argv[i], '=');

          WStrHexToArray(pChar + 1, uuid, sizeof(uuid));
        }
        else if (strstr(argv[i], "aidx=") != NULL)
        {
          /* Found app key index field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishAppKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "cred=") != NULL)
        {
          /* Found credential flag field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishFriendshipCred = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ttl=") != NULL)
        {
          /* Found publish ttl field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishTtl = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "persteps=") != NULL)
        {
          /* Found publish period field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishPeriodNumSteps = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "perstepres=") != NULL)
        {
          /* Found publish period field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishPeriodStepRes = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "count=") != NULL)
        {
          /* Found publish count field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishRetransCount = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "steps=") != NULL)
        {
          /* Found publish retransmit steps field. */
          pChar = strchr(argv[i], '=');

          pubParams.publishRetransSteps50Ms = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "modelid=") != NULL)
        {
          /* Found Model ID field. */
          pChar = strchr(argv[i], '=');

          modelId = (uint32_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccmodelpubvirt_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClPubSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                         appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr, 0, uuid, &pubParams,
                         (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                         (vend == FALSE));
    }
    else
    {
      TerminalTxPrint("ccmodelpubvirt_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Model Subscription terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcModelSubHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t modelId = 0;
  meshAddress_t subAddr = MESH_ADDR_TYPE_UNASSIGNED;
  meshAddress_t elemAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint8_t i;
  bool_t add = FALSE;
  bool_t del = FALSE;
  bool_t ovr = FALSE;
  bool_t get = FALSE;
  bool_t vend = FALSE;
  bool_t all = FALSE;

  if (argc < 2)
  {
    TerminalTxStr("ccmodelsub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "add") == 0)
    {
      add = TRUE;
    }
    else if (strcmp(argv[1], "del") == 0)
    {
      del = TRUE;
    }
    else if (strcmp(argv[1], "del=all") == 0)
    {
      all = TRUE;
      del = TRUE;
    }
    else if (strcmp(argv[1], "ovr") == 0)
    {
      ovr = TRUE;
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      get = TRUE;
    }
    else
    {
      TerminalTxPrint("ccmodelsub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 5) && ((add == TRUE) || ((del == TRUE) && (all == FALSE)) || (ovr == TRUE))) ||
        ((argc < 4) && ((get == TRUE) || ((del == TRUE) && (all == TRUE)))))
    {
      TerminalTxStr("ccmodelsub_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strcmp(argv[i], "vend") == 0)
        {
          /* Vendor Model ID used. */
          vend = TRUE;
        }
        else if (strstr(argv[i], "elemaddr=") != NULL)
        {
          /* Found element address field. */
          pChar = strchr(argv[i], '=');

          elemAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "subaddr=") != NULL)
        {
          /* Found subscription address. */
          pChar = strchr(argv[i], '=');

          subAddr = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "modelid=") != NULL)
        {
          /* Found Model ID field. */
          pChar = strchr(argv[i], '=');

          modelId = (uint32_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccmodelsub_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  if (add == TRUE)
  {
    MeshCfgMdlClSubscrListChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                              appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr, MESH_CFG_MDL_CL_SUBSCR_ADDR_ADD,
                              subAddr, NULL, (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                              (vend == FALSE));
  }
  else if (del == TRUE)
  {
    MeshCfgMdlClSubscrListChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                              appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr,
                              ((all == FALSE) ? MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL :
                                                MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL_ALL),
                              subAddr, NULL, (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                              (vend == FALSE));
  }
  else if (ovr == TRUE)
  {
    MeshCfgMdlClSubscrListChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                              appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr, MESH_CFG_MDL_CL_SUBSCR_ADDR_OVR,
                              subAddr, NULL, (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                              (vend == FALSE));
  }
  else if (get == TRUE)
  {
    MeshCfgMdlClSubscrListGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                              appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr,
                              (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                              (vend == FALSE));
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Model Subscription Virtual terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcMdlSubVirtualHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint32_t modelId = 0;
  meshAddress_t elemAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint8_t uuid[MESH_KEY_SIZE_128];
  meshCfgMdlClSubscrAddrOp_t opValue = MESH_CFG_MDL_CL_KEY_UNDEFINED;
  uint8_t i;
  bool_t vend = FALSE;

  if (argc < 5)
  {
    TerminalTxStr("ccmodelsubvirt_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "add") == 0)
    {
      opValue = MESH_CFG_MDL_CL_SUBSCR_ADDR_ADD;
    }
    else if (strcmp(argv[1], "del") == 0)
    {
      opValue = MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL;
    }
    else if (strcmp(argv[1], "ovr") == 0)
    {
      opValue = MESH_CFG_MDL_CL_SUBSCR_ADDR_OVR;
    }
    else if (strcmp(argv[1], "del=all") == 0)
    {
      opValue = MESH_CFG_MDL_CL_SUBSCR_ADDR_DEL_ALL;
    }

    if (opValue != MESH_CFG_MDL_CL_KEY_UNDEFINED)
    {
      for (i = 2; i < argc; i++)
      {
        if (strcmp(argv[i], "vend") == 0)
        {
          /* Vendor Model ID used. */
          vend = TRUE;
        }
        else if (strstr(argv[i], "elemaddr=") != NULL)
        {
          /* Found element address field. */
          pChar = strchr(argv[i], '=');

          elemAddr = (meshAddress_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "uuid=") != NULL)
        {
          /* Found uuid field. */
          pChar = strchr(argv[i], '=');

          WStrHexToArray(pChar + 1, uuid, sizeof(uuid));
        }
        else if (strstr(argv[i], "modelid=") != NULL)
        {
          /* Found Model ID field. */
          pChar = strchr(argv[i], '=');

          modelId = (uint32_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccmodelsubvirt_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE,
                          argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClSubscrListChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                appMeshCfgMdlClTerminalCb.pSrDevKey, elemAddr, opValue, 0, uuid,
                                (meshSigModelId_t)modelId, (meshVendorModelId_t)modelId,
                                (vend == FALSE));
    }
    else
    {
      TerminalTxPrint("ccmodelsubvirt_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client NetKey terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcNetKeyHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint16_t netKeyIndex = 0xFFFF;
  uint8_t keyArray[MESH_KEY_SIZE_128];
  uint8_t i;
  bool_t add = FALSE;
  bool_t del = FALSE;
  bool_t updt = FALSE;
  bool_t get = FALSE;

  if (argc < 2)
  {
    TerminalTxStr("ccnetkey_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "add") == 0)
    {
      add = TRUE;
    }
    else if (strcmp(argv[1], "del") == 0)
    {
      del = TRUE;
    }
    else if (strcmp(argv[1], "updt") == 0)
    {
      updt = TRUE;
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      get = TRUE;
    }
    else
    {
      TerminalTxPrint("ccnetkey_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && ((add == TRUE) || (updt == TRUE))) || ((argc < 3) && (del == TRUE)))
    {
      TerminalTxStr("ccnetkey_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "nidx=") != NULL)
        {
          /* Found Net Key index field. */
          pChar = strchr(argv[i], '=');

          netKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "key=") != NULL)
        {
          /* Found NetKey field. */
          pChar = strchr(argv[i], '=');

          WStrHexToArray(pChar + 1, keyArray, sizeof(keyArray));
        }
        else
        {
          TerminalTxPrint("ccnetkey_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }
  }

  if (add == TRUE)
  {
    MeshCfgMdlClNetKeyChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                          appMeshCfgMdlClTerminalCb.pSrDevKey, netKeyIndex, MESH_CFG_MDL_CL_KEY_ADD, keyArray);
  }
  else if (updt == TRUE)
  {
    MeshCfgMdlClNetKeyChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                          appMeshCfgMdlClTerminalCb.pSrDevKey, netKeyIndex, MESH_CFG_MDL_CL_KEY_UPDT, keyArray);
  }
  else if (del == TRUE)
  {
    MeshCfgMdlClNetKeyChg(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                          appMeshCfgMdlClTerminalCb.pSrDevKey, netKeyIndex, MESH_CFG_MDL_CL_KEY_DEL, NULL);
  }
  else if (get == TRUE)
  {
    MeshCfgMdlClNetKeyGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.pSrDevKey,
                          appMeshCfgMdlClTerminalCb.srNetKeyIndex);
  }

  return TERMINAL_ERROR_OK;
}


/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Node Identity terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcNodeIdentityHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint16_t netKeyIdx = 0xFFFF;
  meshNodeIdentityStates_t identState = 0;
  uint8_t i;

  if (argc < 3)
  {
    TerminalTxStr("ccnodeident_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 4)
      {
        TerminalTxStr("ccnodeident_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "nidx=") != NULL)
        {
          /* Found NetKey index field. */
          pChar = strchr(argv[i], '=');

          netKeyIdx = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ident=") != NULL)
        {
          /* Found node identity state field. */
          pChar = strchr(argv[i], '=');

          identState = (meshNodeIdentityStates_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccnodeident_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClNodeIdentitySet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.pSrDevKey,
                                  appMeshCfgMdlClTerminalCb.srNetKeyIndex, netKeyIdx, identState);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      if (strstr(argv[2], "nidx=") != NULL)
      {
        /* Found NetKey index field. */
        pChar = strchr(argv[2], '=');

        netKeyIdx = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else
      {
        TerminalTxPrint("ccnodeident_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }

      MeshCfgMdlClNodeIdentityGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.pSrDevKey,
                                  appMeshCfgMdlClTerminalCb.srNetKeyIndex, netKeyIdx);
    }
    else
    {
      TerminalTxPrint("ccnodeident_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Node Reset terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcNodeRstHandler(uint32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  MeshCfgMdlClNodeReset(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                        appMeshCfgMdlClTerminalCb.pSrDevKey);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Network Transmit terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcNwkTransHandler(uint32_t argc, char **argv)
{
  char *pChar;
  meshNwkTransState_t transState = { 0x00 };
  uint8_t i;

  if (argc < 2)
  {
    TerminalTxStr("ccnwktrans_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 4)
      {
        TerminalTxStr("ccnwktrans_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "count=") != NULL)
        {
          /* Found Network Transmit Count field. */
          pChar = strchr(argv[i], '=');

          transState.transCount = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "steps=") != NULL)
        {
          /* Found Network Transmit Interval Steps field. */
          pChar = strchr(argv[i], '=');

          transState.transIntervalSteps10Ms = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccnwktrans_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClNwkTransmitSet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                 appMeshCfgMdlClTerminalCb.pSrDevKey, &transState);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClNwkTransmitGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                                 appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("ccnwktrans_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Configuration Client Relay terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshCfgMdlClTerminalCcRelayHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t i;
  uint8_t relay = 0;
  meshRelayRetransState_t retrans = { 0 };

  if (argc < 2)
  {
    TerminalTxStr("ccrelay_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 5)
      {
        TerminalTxStr("ccrelay_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      for (i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "relay=") != NULL)
        {
          /* Found relay field. */
          pChar = strchr(argv[i], '=');

          relay = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "count=") != NULL)
        {
          /* Found retransmit count field. */
          pChar = strchr(argv[i], '=');

          retrans.retransCount = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "steps=") != NULL)
        {
          /* Found retransmit interval steps field. */
          pChar = strchr(argv[i], '=');

          retrans.retransIntervalSteps10Ms = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("ccrelay_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }

      MeshCfgMdlClRelaySet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                           appMeshCfgMdlClTerminalCb.pSrDevKey, relay, &retrans);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      MeshCfgMdlClRelayGet(appMeshCfgMdlClTerminalCb.srAddr, appMeshCfgMdlClTerminalCb.srNetKeyIndex,
                           appMeshCfgMdlClTerminalCb.pSrDevKey);
    }
    else
    {
      TerminalTxPrint("ccrelay_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Prints formatted Composition Data Page 0 in terminal.
 *
 *  \param[in] pPg0Data    Raw data for Composition Data Page 0.
 *  \param[in] pg0DataLen  Data length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void appMeshCfgMdlClTerminalCompDataPg0Print(uint8_t *pPg0Data, uint16_t pg0DataLen)
{
  uint16_t cid, pid, vid, crpl, locDescr;
  uint8_t numS, numV;
  meshFeatures_t feat;
  meshSigModelId_t sigModelId;
  meshVendorModelId_t vendModelId;

  /* Extract header. */
  BSTREAM_TO_CFG_CL_COMP_PG0_HDR(pPg0Data, pg0DataLen, cid, pid, vid, crpl, feat);

  /* Print header. */
  TerminalTxPrint(" cid=0x%x pid=0x%x vid=0x%x crpl=0x%x feat=0x%x\r\n", cid, pid, vid, crpl, feat);

  uint8_t i = 0;
  while (pg0DataLen > 0)
  {
    /* First get element header. */
    BSTREAM_TO_CFG_CL_COMP_PG0_ELEM_HDR(pPg0Data, pg0DataLen, locDescr, numS, numV);

    TerminalTxPrint("\r\ncompdata_ind elemid=0x%x locdescr=0x%x numsig=0x%x numvend=0x%x", i, locDescr, numS, numV);

    if(numS != 0)
    {
      TerminalTxPrint("\r\ncompdata_ind elemid=0x%x sigmdl=", i);
    }
    /* Then get all SIG models for this element. */
    while ((pg0DataLen != 0) && numS != 0)
    {
      /* Get SIG model ID. */
      BSTREAM_TO_CFG_CL_COMP_PG0_SIG_MODEL_ID(pPg0Data, pg0DataLen, sigModelId);

      TerminalTxPrint("0x%x ", sigModelId);
      numS--;
    }
    if(numV != 0)
    {
      TerminalTxPrint("\r\ncompdata_ind elemid=0x%x vendmdl=", i);
    }
    /* Finally get all Vendor models for this element. */
    while ((pg0DataLen != 0) && numV != 0)
    {
      /* Get Vendor model ID. */
      BSTREAM_TO_CFG_CL_COMP_PG0_VENDOR_MODEL_ID(pPg0Data, pg0DataLen, vendModelId);

      TerminalTxPrint("0x%x ", vendModelId);
      numV--;
    }
    TerminalTxStr(TERMINAL_STRING_NEW_LINE);

    i++;
  }
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Mesh Config Client terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void appMeshCfgMdlClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(appMeshCfgMdlClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&appMeshCfgMdlClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process messages from the Config Client.
 *
 *  \param  pMsg  Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void appMeshCfgMdlClTerminalProcMsg(meshCfgMdlClEvt_t* pMsg)
{
  char *pBuf;
  uint8_t i;

  WSF_ASSERT(pMsg->hdr.param < MESH_CFG_MDL_MAX_EVENT);

  /* Print first indication part. */
  TerminalTxStr(pAppMeshCfgMdlClTerminalEvt[pMsg->hdr.param]);

  /* Print status. */
  TerminalTxStr(pAppMeshCfgMdlClTerminalEvtStatus[pMsg->hdr.status]);

  if ((pMsg->hdr.status == MESH_CFG_MDL_CL_SUCCESS) ||
      (pMsg->hdr.status > MESH_CFG_MDL_CL_REMOTE_ERROR_BASE))
  {
    switch (pMsg->hdr.param)
    {
      case MESH_CFG_MDL_APPKEY_ADD_EVENT:
      case MESH_CFG_MDL_APPKEY_UPDT_EVENT:
      case MESH_CFG_MDL_APPKEY_DEL_EVENT:
        TerminalTxPrint("peer_addr=0x%x nidx=0x%x aidx=0x%x",
                        ((meshCfgMdlAppKeyChgEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlAppKeyChgEvt_t *)pMsg)->bind.netKeyIndex,
                        ((meshCfgMdlAppKeyChgEvt_t *)pMsg)->bind.appKeyIndex);
        break;
      case MESH_CFG_MDL_APPKEY_GET_EVENT:
        TerminalTxPrint("peer_addr=0x%x nidx=0x%x ",
                       ((meshCfgMdlAppKeyListEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                       ((meshCfgMdlAppKeyListEvt_t *)pMsg)->appKeyList.netKeyIndex);

        for (i = 0; i < ((meshCfgMdlAppKeyListEvt_t *)pMsg)->appKeyList.appKeyCount; i++)
        {
           TerminalTxPrint("aidx=0x%x ",
                           ((meshCfgMdlAppKeyListEvt_t *)pMsg)->appKeyList.pAppKeyIndexes[i]);
        }
        break;
      case MESH_CFG_MDL_BEACON_GET_EVENT:
      case MESH_CFG_MDL_BEACON_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x beacon=0x%x",
                        ((meshCfgMdlBeaconStateEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlBeaconStateEvt_t *)pMsg)->state);
        break;
      case MESH_CFG_MDL_COMP_PAGE_GET_EVENT:
        TerminalTxPrint("peer_addr=0x%x page=%d",
                        ((meshCfgMdlCompDataEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlCompDataEvt_t *)pMsg)->data.pageNumber);

        if(((meshCfgMdlCompDataEvt_t *)pMsg)->data.pageNumber == 0)
        {
          /* Formatted print for page 0. */
          appMeshCfgMdlClTerminalCompDataPg0Print(((meshCfgMdlCompDataEvt_t *)pMsg)->data.pPage,
                                                  ((meshCfgMdlCompDataEvt_t *)pMsg)->data.pageSize);
        }
        else
        {
          /* Allocate memory for Terminal Print. */
          if ((pBuf = (char *)WsfBufAlloc(2 * ((meshCfgMdlCompDataEvt_t *)pMsg)->data.pageSize + 1)) != NULL)
          {
            for (i = 0; i < ((meshCfgMdlCompDataEvt_t *)pMsg)->data.pageSize; i++)
            {
              sprintf(&pBuf[2 * i], "%02x", ((meshCfgMdlCompDataEvt_t *)pMsg)->data.pPage[i]);
            }

            TerminalTxPrint(" content=%s", pBuf);

            WsfBufFree(pBuf);
          }
        }
        break;
      case MESH_CFG_MDL_DEFAULT_TTL_GET_EVENT:
      case MESH_CFG_MDL_DEFAULT_TTL_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x ttl=0x%x",
                        ((meshCfgMdlDefaultTtlStateEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlDefaultTtlStateEvt_t *)pMsg)->ttl);
        break;
      case MESH_CFG_MDL_FRIEND_GET_EVENT:
      case MESH_CFG_MDL_FRIEND_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x friend=0x%x",
                         ((meshCfgMdlFriendEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                         ((meshCfgMdlFriendEvt_t *)pMsg)->friendState);

        break;
      case MESH_CFG_MDL_GATT_PROXY_GET_EVENT:
      case MESH_CFG_MDL_GATT_PROXY_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x proxy=0x%x",
                        ((meshCfgMdlGattProxyEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlGattProxyEvt_t *)pMsg)->gattProxy);
        break;
      case MESH_CFG_MDL_HB_PUB_GET_EVENT:
      case MESH_CFG_MDL_HB_PUB_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x dst=0x%x count=0x%x period=0x%x ttl=0x%x feat=0x%x nidx=0x%x",
                        ((meshCfgMdlHbPubEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlHbPubEvt_t *)pMsg)->hbPub.dstAddr,
                        ((meshCfgMdlHbPubEvt_t *)pMsg)->hbPub.countLog,
                        ((meshCfgMdlHbPubEvt_t *)pMsg)->hbPub.periodLog,
                        ((meshCfgMdlHbPubEvt_t *)pMsg)->hbPub.ttl,
                        ((meshCfgMdlHbPubEvt_t *)pMsg)->hbPub.features,
                        ((meshCfgMdlHbPubEvt_t *)pMsg)->hbPub.netKeyIndex);
        break;
      case MESH_CFG_MDL_HB_SUB_GET_EVENT:
      case MESH_CFG_MDL_HB_SUB_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x src=0x%x dst=0x%x period=0x%x count=0x%x min_hops=0x%x max_hops=0x%x",
                        ((meshCfgMdlHbSubEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlHbSubEvt_t *)pMsg)->hbSub.srcAddr,
                        ((meshCfgMdlHbSubEvt_t *)pMsg)->hbSub.periodLog,
                        ((meshCfgMdlHbSubEvt_t *)pMsg)->hbSub.countLog,
                        ((meshCfgMdlHbSubEvt_t *)pMsg)->hbSub.minHops,
                        ((meshCfgMdlHbSubEvt_t *)pMsg)->hbSub.maxHops);
        break;
      case MESH_CFG_MDL_KEY_REF_PHASE_GET_EVENT:
      case MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x nidx=0x%x phase=0x%x",
                        ((meshCfgMdlKeyRefPhaseEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlKeyRefPhaseEvt_t *)pMsg)->netKeyIndex,
                        ((meshCfgMdlKeyRefPhaseEvt_t *)pMsg)->keyRefState);
        break;
      case MESH_CFG_MDL_LPN_POLLTIMEOUT_GET_EVENT:
        TerminalTxPrint("peer_addr=0x%x lpnaddr=0x%x timeout=%d",
                        ((meshCfgMdlLpnPollTimeoutEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlLpnPollTimeoutEvt_t *)pMsg)->lpnAddr,
                        ((meshCfgMdlLpnPollTimeoutEvt_t *)pMsg)->pollTimeout100Ms);
        break;
      case MESH_CFG_MDL_APP_BIND_EVENT:
      case MESH_CFG_MDL_APP_UNBIND_EVENT:
        TerminalTxPrint("peer_addr=0x%x elemaddr=0x%x aidx=0x%x modelid=0x%x",
                        ((meshCfgMdlModelAppBindEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlModelAppBindEvt_t *)pMsg)->elemAddr,
                        ((meshCfgMdlModelAppBindEvt_t *)pMsg)->appKeyIndex,
                        (((meshCfgMdlModelAppBindEvt_t *)pMsg)->isSig) ?
                         ((meshCfgMdlModelAppBindEvt_t *)pMsg)->modelId.sigModelId :
                         ((meshCfgMdlModelAppBindEvt_t *)pMsg)->modelId.vendorModelId);
        break;
      case MESH_CFG_MDL_APP_SIG_GET_EVENT:
      case MESH_CFG_MDL_APP_VENDOR_GET_EVENT:
        TerminalTxPrint("peer_addr=0x%x elemaddr=0x%x modelid=0x%x ",
          ((meshCfgMdlModelAppListEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
          ((meshCfgMdlModelAppListEvt_t *)pMsg)->modelAppList.elemAddr,
          (((meshCfgMdlModelAppListEvt_t *)pMsg)->modelAppList.isSig) ?
           ((meshCfgMdlModelAppListEvt_t *)pMsg)->modelAppList.modelId.sigModelId :
           ((meshCfgMdlModelAppListEvt_t *)pMsg)->modelAppList.modelId.vendorModelId);

        for (i = 0; i < ((meshCfgMdlModelAppListEvt_t *)pMsg)->modelAppList.appKeyCount; i++)
        {
          TerminalTxPrint("aidx=0x%x ",
                          ((meshCfgMdlModelAppListEvt_t *)pMsg)->modelAppList.pAppKeyIndexes[i]);
        }
        break;
      case MESH_CFG_MDL_PUB_GET_EVENT:
      case MESH_CFG_MDL_PUB_SET_EVENT:
      case MESH_CFG_MDL_PUB_VIRT_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x elemaddr=0x%x pubaddr=0x%x aidx=0x%x cred=0x%x" \
                        "ttl=0x%x persteps=0x%x perstepres=0x%x count=0x%x steps=0x%x modelid=0x%x",
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->elemAddr,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubAddr,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubParams.publishAppKeyIndex,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubParams.publishFriendshipCred,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubParams.publishTtl,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubParams.publishPeriodNumSteps,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubParams.publishPeriodStepRes,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubParams.publishRetransCount,
                        ((meshCfgMdlModelPubEvt_t *)pMsg)->pubParams.publishRetransSteps50Ms,
                        (((meshCfgMdlModelPubEvt_t *)pMsg)->isSig) ?
                         ((meshCfgMdlModelPubEvt_t *)pMsg)->modelId.sigModelId :
                         ((meshCfgMdlModelPubEvt_t *)pMsg)->modelId.vendorModelId);
        break;
      case MESH_CFG_MDL_SUBSCR_ADD_EVENT:
      case MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT:
      case MESH_CFG_MDL_SUBSCR_DEL_EVENT:
      case MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT:
      case MESH_CFG_MDL_SUBSCR_OVR_EVENT:
      case MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT:
      case MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT:
        TerminalTxPrint("peer_addr=0x%x elemaddr=0x%x subaddr=0x%x modelid=0x%x",
                        ((meshCfgMdlModelSubscrChgEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlModelSubscrChgEvt_t *)pMsg)->elemAddr,
                        ((meshCfgMdlModelSubscrChgEvt_t *)pMsg)->subscrAddr,
                        (((meshCfgMdlModelSubscrChgEvt_t *)pMsg)->isSig) ?
                         ((meshCfgMdlModelSubscrChgEvt_t *)pMsg)->modelId.sigModelId :
                         ((meshCfgMdlModelSubscrChgEvt_t *)pMsg)->modelId.vendorModelId);
        break;
      case MESH_CFG_MDL_SUBSCR_SIG_GET_EVENT:
      case MESH_CFG_MDL_SUBSCR_VENDOR_GET_EVENT:
        TerminalTxPrint("peer_addr=0x%x elemaddr=0x%x modelid=0x%x ",
                        ((meshCfgMdlModelSubscrListEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlModelSubscrListEvt_t *)pMsg)->elemAddr,
                        (((meshCfgMdlModelSubscrListEvt_t *)pMsg)->isSig) ?
                         ((meshCfgMdlModelSubscrListEvt_t *)pMsg)->modelId.sigModelId :
                         ((meshCfgMdlModelSubscrListEvt_t *)pMsg)->modelId.vendorModelId);

        for (i = 0; i < ((meshCfgMdlModelSubscrListEvt_t *)pMsg)->subscrListSize; i++)
        {
          TerminalTxPrint("subaddr=0x%x ", ((meshCfgMdlModelSubscrListEvt_t *)pMsg)->pSubscrList[i]);
        }
        break;
      case MESH_CFG_MDL_NETKEY_ADD_EVENT:
      case MESH_CFG_MDL_NETKEY_UPDT_EVENT:
      case MESH_CFG_MDL_NETKEY_DEL_EVENT:
        TerminalTxPrint("peer_addr=0x%x nidx=0x%x",
                        ((meshCfgMdlNetKeyChgEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlNetKeyChgEvt_t *)pMsg)->netKeyIndex);
        break;
      case MESH_CFG_MDL_NETKEY_GET_EVENT:
        TerminalTxPrint("peer_addr=0x%x ", ((meshCfgMdlNetKeyListEvt_t *)pMsg)->cfgMdlHdr.peerAddress);

        for (i = 0; i < ((meshCfgMdlNetKeyListEvt_t *)pMsg)->netKeyList.netKeyCount; i++)
        {
           TerminalTxPrint("nidx=0x%x ",
                           ((meshCfgMdlNetKeyListEvt_t *)pMsg)->netKeyList.pNetKeyIndexes[i]);
        }
        break;
      case MESH_CFG_MDL_NODE_IDENTITY_GET_EVENT:
      case MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x nidx=0x%x ident=0x%x",
                        ((meshCfgMdlNodeIdentityEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlNodeIdentityEvt_t *)pMsg)->netKeyIndex,
                        ((meshCfgMdlNodeIdentityEvt_t *)pMsg)->state);
        break;
      case MESH_CFG_MDL_NODE_RESET_EVENT:
        break;
      case MESH_CFG_MDL_NWK_TRANS_GET_EVENT:
      case MESH_CFG_MDL_NWK_TRANS_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x count=0x%x steps=0x%x",
                        ((meshCfgMdlNwkTransStateEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlNwkTransStateEvt_t *)pMsg)->nwkTransState.transCount,
                        ((meshCfgMdlNwkTransStateEvt_t *)pMsg)->nwkTransState.transIntervalSteps10Ms);
        break;
      case MESH_CFG_MDL_RELAY_GET_EVENT:
      case MESH_CFG_MDL_RELAY_SET_EVENT:
        TerminalTxPrint("peer_addr=0x%x relay=0x%x count=0x%x steps=0x%x",
                        ((meshCfgMdlRelayCompositeStateEvt_t *)pMsg)->cfgMdlHdr.peerAddress,
                        ((meshCfgMdlRelayCompositeStateEvt_t *)pMsg)->relayState,
                        ((meshCfgMdlRelayCompositeStateEvt_t *)pMsg)->relayRetrans.retransCount,
                        ((meshCfgMdlRelayCompositeStateEvt_t *)pMsg)->relayRetrans.retransIntervalSteps10Ms);
        break;
      default:
        break;
    }
  }

  TerminalTxStr(TERMINAL_STRING_NEW_LINE);
}
