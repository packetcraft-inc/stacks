/*************************************************************************************************/
/*!
 *  \file   provisioner_terminal.c
 *
 *  \brief  Mesh Provisioner Terminal.
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_os.h"
#include "wsf_timer.h"
#include "wsf_trace.h"
#include "util/print.h"
#include "util/terminal.h"
#include "util/wstr.h"

#include "dm_api.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"

#include "adv_bearer.h"
#include "gatt_bearer_cl.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_prv_cl_api.h"

#include "app_mesh_api.h"
#include "provisioner_api.h"
#include "provisioner_config.h"
#include "provisioner_terminal.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Provisioner Terminal Mesh Model commands */
enum provisionerTerminalMmdlCmd
{
  PROVISIONER_TERMINAL_MMDL_GET = 0x00,          /*!< Get command. */
  PROVISIONER_TERMINAL_MMDL_SET,                 /*!< Set command. */
  PROVISIONER_TERMINAL_MMDL_SET_NO_ACK,          /*!< Set Unacknowledged command. */
};

/*! Provisioner Terminal Mesh Model commands type. See ::provisionerTerminalMmdlCmd */
typedef uint8_t provisionerTerminalMmdlCmd_t;

/**************************************************************************************************
  Local Functions Prototypes
**************************************************************************************************/

/*! \brief   Enable GATT Client */
static uint8_t provisionerTerminalGattClHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Generic On Off message */
static uint8_t provisionerTerminalGenOnOffMsgHandler(uint32_t argc, char **argv);

/*! \brief   Add/Remove the Advertising Bearer interface */
static uint8_t provisionerTerminalIfAdvHandler(uint32_t argc, char **argv);

/*! \brief   Manually provision the LE Mesh Stack */
static uint8_t provisionerTerminalLdProvHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Light HSL message */
static uint8_t provisionerTerminalLightHslMsgHandler(uint32_t argc, char **argv);

/*! \brief   Proxy Client commands */
static uint8_t provisionerTerminalProxyClHandler(uint32_t argc, char **argv);

/*! \brief   Select PRV CL authentication */
static uint8_t provisionerTerminalPrvClAuthHandler(uint32_t argc, char **argv);

/*! \brief   Cancel any on-going provisioning procedure */
static uint8_t provisionerTerminalPrvClCancelHandler(uint32_t argc, char **argv);

/*! \brief   Configure PRV CL */
static uint8_t provisionerTerminalPrvClCfgHandler(uint32_t argc, char **argv);

/*! \brief   Enters provisioning OOB data */
static uint8_t provisionerTerminalPrvOobHandler(uint32_t argc, char **argv);

/*! \brief   Start PB-ADV provisioning client */
static uint8_t provisionerTerminalStartPbAdvHandler(uint32_t argc, char **argv);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

char * const pProvisionerLogo[]=
{
  "\f\r\n",
  "\n\n\r\n",
  "#     #                        #####\n\r",
  "##   ## ######  ####  #    #   #    # #####   #####  #         #\n\r",
  "# # # # #      #      #    #   #    # #    # #     #  #       #\n\r",
  "#  #  # #####   ####  ######   #####  #    # #     #   #     #\n\r",
  "#     # #           # #    #   #      #####  #     #    #   #\n\r",
  "#     # #      #    # #    #   #      #  #   #     #     # #\n\r",
  "#     # ######  ####  #    #   #      #   #   #####       #\n\r",
  "\r\n -Press enter for prompt\n\r",
  "\r\n -Type help to display the list of available commands\n\r",
  NULL
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Provisioner Terminal commands table */
static terminalCommand_t provisionerTerminalTbl[] =
{
    /*! Enable GATT Client */
    { NULL, "gattcl", "gattcl <proxy|prv|addr>", provisionerTerminalGattClHandler },
    /*! Transmit Mesh Generic OnOff message. */
    { NULL, "genonoff", "genonoff <get|set|setnack|elemid|state|trans|delay>",
      provisionerTerminalGenOnOffMsgHandler },
    /*! Add/Remove the Advertising Bearer interface. */
    { NULL, "ifadv", "ifadv <add|rm|id>", provisionerTerminalIfAdvHandler },
    /*! Manually provision the LE Mesh Stack. */
    { NULL, "ldprov", "ldprov <addr|devkey|nidx|netkey|ividx>", provisionerTerminalLdProvHandler },
    /*! Transmit Mesh Light HSL message. */
    { NULL, "lighthsl", "lighthsl <get|set|setnack|elemid|h|s|l|trans|delay>",
      provisionerTerminalLightHslMsgHandler },
    /*! Proxy Client command */
    { NULL, "proxycl", "proxycl <ifid|nidx|settype|add|rm>", provisionerTerminalProxyClHandler },
    /*! Select PRV CL authentication */
    { NULL, "prvclauth", "prvclauth <oobpk|method|action|size>", provisionerTerminalPrvClAuthHandler },
    /*! Cancel any on-going provisioning procedure */
    { NULL, "prvclcancel", "prvclcancel", provisionerTerminalPrvClCancelHandler },
    /*! Configure PRV CL */
    { NULL, "prvclcfg", "prvclcfg <devuuid|nidx|netkey|ividx>", provisionerTerminalPrvClCfgHandler },
    /*! Enters provisioning OOB data */
    { NULL, "prvoob", "prvoob <num|alpha>", provisionerTerminalPrvOobHandler },
    /*! Starts PB-ADV provisioning client */
    { NULL, "startpbadv", "startpbadv <ifid|addr>", provisionerTerminalStartPbAdvHandler},
};

/*! Provisioner models transaction ID */
uint8_t provisionerTerminalTid[PROVISIONER_ELEMENT_COUNT] = { 0 };

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static uint8_t provisionerTerminalGattClHandler(uint32_t argc, char **argv)
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
      ProvisionerStartGattCl(FALSE, 0);
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

        ProvisionerStartGattCl(TRUE, addr);
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

static uint8_t provisionerTerminalGenOnOffMsgHandler(uint32_t argc, char **argv)
{
  char                         *pChar;
  meshElementId_t              elementId = 0;
  mmdlGenOnOffSetParam_t       setParam;
  provisionerTerminalMmdlCmd_t cmd = PROVISIONER_TERMINAL_MMDL_GET;
  mmdlGenOnOffState_t          state = 0;
  uint8_t                      transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                      delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("genonoff_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = PROVISIONER_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = PROVISIONER_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = PROVISIONER_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("genonoff_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && (cmd != PROVISIONER_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == PROVISIONER_TERMINAL_MMDL_GET)))
    {
      TerminalTxStr("genonoff_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (uint8_t i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "elemid=") != NULL)
        {
          /* Found element ID field. */
          pChar = strchr(argv[i], '=');

          elementId = (meshElementId_t)strtol(pChar + 1, NULL, 0);

          if (elementId >= PROVISIONER_ELEMENT_COUNT)
          {
            TerminalTxPrint("genonoff_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
        }
        else if (strstr(argv[i], "state=") != NULL)
        {
          /* Found state field. */
          pChar = strchr(argv[i], '=');

          state = (mmdlGenOnOffState_t)strtol(pChar + 1, NULL, 0);

          if (state)
          {
            setParam.state = MMDL_GEN_ONOFF_STATE_ON;
          }
          else
          {
            setParam.state = MMDL_GEN_ONOFF_STATE_OFF;
          }
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          /* Found transition time field. */
          pChar = strchr(argv[i], '=');

          transitionTime = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          /* Found delay field. */
          pChar = strchr(argv[i], '=');

          delay = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("genonoff_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(cmd)
    {
      case PROVISIONER_TERMINAL_MMDL_GET:
        MmdlGenOnOffClGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case PROVISIONER_TERMINAL_MMDL_SET:
        setParam.tid = provisionerTerminalTid[elementId]++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlGenOnOffClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      case PROVISIONER_TERMINAL_MMDL_SET_NO_ACK:
        setParam.tid = provisionerTerminalTid[elementId]++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlGenOnOffClSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("genonoff_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t provisionerTerminalIfAdvHandler(uint32_t argc, char **argv)
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

static uint8_t provisionerTerminalLdProvHandler(uint32_t argc, char **argv)
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

static uint8_t provisionerTerminalLightHslMsgHandler(uint32_t argc, char **argv)
{
  char                          *pChar;
  meshElementId_t               elementId = 0;
  mmdlLightHslSetParam_t        setParam;
  provisionerTerminalMmdlCmd_t  cmd = PROVISIONER_TERMINAL_MMDL_GET;
  uint16_t                      lightness = 0;
  uint16_t                      hue = 0;
  uint16_t                      saturation = 0;
  uint8_t                       transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                       delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("lighthsl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = PROVISIONER_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = PROVISIONER_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = PROVISIONER_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("lighthsl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 6) && (cmd != PROVISIONER_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == PROVISIONER_TERMINAL_MMDL_GET)))
    {
      TerminalTxStr("lighthsl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (uint8_t i = 2; i < argc; i++)
      {
        if (strstr(argv[i], "elemid=") != NULL)
        {
          /* Found element ID field. */
          pChar = strchr(argv[i], '=');

          elementId = (meshElementId_t)strtol(pChar + 1, NULL, 0);

          if (elementId >= PROVISIONER_ELEMENT_COUNT)
          {
            TerminalTxPrint("lighthsl_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
        }
        else if (strstr(argv[i], "h=") != NULL)
        {
          /* Found hue field. */
          pChar = strchr(argv[i], '=');

          hue = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "s=") != NULL)
        {
          /* Found saturation field. */
          pChar = strchr(argv[i], '=');

          saturation = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "l=") != NULL)
        {
          /* Found lightness field. */
          pChar = strchr(argv[i], '=');

          lightness = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          /* Found transition time field. */
          pChar = strchr(argv[i], '=');

          transitionTime = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          /* Found delay field. */
          pChar = strchr(argv[i], '=');

          delay = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("lighthsl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(cmd)
    {
      case PROVISIONER_TERMINAL_MMDL_GET:
        MmdlLightHslClGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case PROVISIONER_TERMINAL_MMDL_SET:
        setParam.hue = hue;
        setParam.saturation = saturation;
        setParam.lightness = lightness;
        setParam.tid = provisionerTerminalTid[elementId]++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightHslClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0, &setParam);
        break;

      case PROVISIONER_TERMINAL_MMDL_SET_NO_ACK:
        setParam.hue = hue;
        setParam.saturation = saturation;
        setParam.lightness = lightness;
        setParam.tid = provisionerTerminalTid[elementId]++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightHslClSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("lighthsl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t provisionerTerminalProxyClHandler(uint32_t argc, char **argv)
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

static uint8_t provisionerTerminalPrvClAuthHandler(uint32_t argc, char **argv)
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

static uint8_t provisionerTerminalPrvClCancelHandler(uint32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  MeshPrvClCancel();

  TerminalTxStr("prvclcancel_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t provisionerTerminalPrvClCfgHandler(uint32_t argc, char **argv)
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
    memcpy(provisionerPrvClSessionInfo.pDeviceUuid, uuid, MESH_PRV_DEVICE_UUID_SIZE);

    /* Set NetKey. */
    if (netKeyFlag == TRUE)
    {
      memcpy(provisionerPrvClSessionInfo.pData->pNetKey, keyArray, MESH_KEY_SIZE_128);
    }

    /* Set NetKey index. */
    if (netKeyIndexFlag == TRUE)
    {
      provisionerPrvClSessionInfo.pData->netKeyIndex = netKeyIndex;
    }

    /* Set IV index. */
    if (ivIdxFlag == TRUE)
    {
      provisionerPrvClSessionInfo.pData->ivIndex = ivIdx;
    }

    TerminalTxStr("prvclcfg_cnf success" TERMINAL_STRING_NEW_LINE);
  }

  return TERMINAL_ERROR_OK;
}

static uint8_t provisionerTerminalPrvOobHandler(uint32_t argc, char **argv)
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

      MeshPrvClEnterOutputOob((meshPrvOutputOobSize_t)0, oobData);
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

      MeshPrvClEnterOutputOob((meshPrvOutputOobSize_t)alphaLen, oobData);
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

static uint8_t provisionerTerminalStartPbAdvHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint16_t addr = MESH_ADDR_TYPE_UNASSIGNED;
  uint8_t id = 0xFF;
  uint8_t i;

  if (argc < 3)
  {
    TerminalTxStr("startpbadv_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    for (i = 1; i < argc; i++)
    {
      if (strstr(argv[i], "ifid=") != NULL)
      {
        pChar = strchr(argv[i], '=');

        id = (uint8_t)strtol(pChar + 1, NULL, 10);
      }
      else if (strstr(argv[i], "addr=") != NULL)
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

    /* Enter provisioning. */
    provisionerPrvClSessionInfo.pData->address = addr;

    MeshPrvClStartPbAdvProvisioning(id, &provisionerPrvClSessionInfo);

    TerminalTxStr("startpbadv_cnf success" TERMINAL_STRING_NEW_LINE);
  }

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
void provisionerTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(provisionerTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&provisionerTerminalTbl[i]);
  }
}
