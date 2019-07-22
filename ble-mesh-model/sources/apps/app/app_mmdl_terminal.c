/*************************************************************************************************/
/*!
 *  \file   app_mmdl_terminal.c
 *
 *  \brief  Common Mesh models application Terminal handler.
 *
 *  Copyright (c) 2010-2018 Arm Ltd.
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

#include "util/print.h"
#include "util/terminal.h"
#include "util/wstr.h"
#include "wsf_types.h"
#include "wsf_assert.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mmdl_defs.h"
#include "mmdl_types.h"

#include "mesh_ht_mdl_api.h"
#include "mesh_ht_cl_api.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_gen_battery_cl_api.h"
#include "mmdl_gen_default_trans_cl_api.h"
#include "mmdl_time_cl_api.h"
#include "mmdl_gen_powerlevel_cl_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"
#include "mmdl_scene_cl_api.h"
#include "mmdl_scheduler_cl_api.h"

#include "app_mesh_api.h"
#include "app_mmdl_terminal.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Minimum number of parameters in a model command */
#define MMDL_TERMINAL_MIN_PARAM       6

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Model commands */
enum mmdlCmd
{
  MMDL_REG_GET  = 0x00,     /*!< Register Get command. */
  MMDL_GET,                 /*!< Get command. */
  MMDL_SET,                 /*!< Set command. */
  MMDL_SET_NO_ACK,          /*!< Set Unacknowledged command. */
  MMDL_STORE,               /*!< Store command. */
  MMDL_STORE_NO_ACK,        /*!< Store Unacknowledged command. */
  MMDL_RECALL,              /*!< Recall command. */
  MMDL_RECALL_NO_ACK,       /*!< Recall Unacknowledged command. */
  MMDL_DELETE,              /*!< Delete command. */
  MMDL_DELETE_NO_ACK,       /*!< Delete Unacknowledged command. */
  MMDL_ACT_GET,             /*!< Action Get command. */
  MMDL_ACT_SET,             /*!< Action Set command. */
  MMDL_ACT_SET_NO_ACK,      /*!< Action Set Unacknowledged command */
};

/*! Mesh Model commands type. See ::mmdlCmd */
typedef uint8_t mmdlCmd_t;

/*! Generic request structure */
typedef struct mmdlReq_tag
{
  mmdlCmd_t         cmd;          /*!< Command */
  meshElementId_t   elementId;    /*!< Element ID */
  meshAddress_t     serverAddr;   /*!< Server Address */
  uint16_t          appKeyIndex;  /*!< AppKey Index */
  uint8_t           ttl;          /*!< Time-to-leave */
} mmdlReq_t;

/**************************************************************************************************
  Local Functions Prototypes
**************************************************************************************************/

/*! \brief   Handler for Generic On Off Client Model terminal commands */
static uint8_t appMmdlTerminalGooClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Generic Power On Off Client Model terminal commands */
static uint8_t appMmdlTerminalGpooClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Generic Default Transition Client Model terminal commands */
static uint8_t appMmdlTerminalGdttClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Generic Battery Client Model terminal commands */
static uint8_t appMmdlTerminalGbatClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Generic Level Client Model terminal commands */
static uint8_t appMmdlTerminalGlvClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Light Lightness Client Model terminal commands */
static uint8_t appMmdlTerminalLlClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Time Client Model terminal commands */
static uint8_t appMmdlTerminalTimClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Scene Client Model terminal commands */
static uint8_t appMmdlTerminalSceClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Scheduler Client Model terminal commands */
static uint8_t appMmdlTerminalSchClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Power Level Client Model terminal commands */
static uint8_t appMmdlTerminalGplClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Power Level Last state terminal commands */
static uint8_t appMmdlTerminalGpLastClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Power Level Default state  commands */
static uint8_t appMmdlTerminalGpDefClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Power Level Range state terminal commands */
static uint8_t appMmdlTerminalGpRangeClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Light HSL Client Model terminal commands */
static uint8_t appMmdlTerminalLhslClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Light HSL Hue Client Model terminal commands */
static uint8_t appMmdlTerminalLhslHueClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Light HSL Saturation Client Model terminal commands */
static uint8_t appMmdlTerminalLhslSatClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Light HSL Default state terminal commands */
static uint8_t appMmdlTerminalLhslDefClHandler(uint32_t argc, char **argv);

/*! \brief   Handler for Light HSL Range state terminal commands */
static uint8_t appMmdlTerminalLhslRangeClHandler(uint32_t argc, char **argv);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Generic OnOff Client Terminal commands table */
static terminalCommand_t gooClTerminalTbl[] =
{
  { NULL, "goo", "goo <set|get|setnack|elemid|sraddr|aidx|ttl|onoff|tid|trans|delay>",
    appMmdlTerminalGooClHandler },
};

/*! Generic Power OnOff Client Terminal commands table */
static terminalCommand_t gpooClTerminalTbl[] =
{
  { NULL, "gpoo", "gpoo <set|get|setnack|elemid|sraddr|aidx|ttl|onpowerup>",
    appMmdlTerminalGpooClHandler },
};

/*! Generic Default Transition Client Terminal commands table */
static terminalCommand_t gdttClTerminalTbl[] =
{
  { NULL, "gdtt", "gdtt <set|get|setnack|elemid|sraddr|aidx|ttl|ttime>",
    appMmdlTerminalGdttClHandler },
};

/*! Generic Battery Client Terminal commands table */
static terminalCommand_t gbatClTerminalTbl[] =
{
  { NULL, "gbat", "gbat <get|elemid|sraddr|aidx|ttl>",
    appMmdlTerminalGbatClHandler },
};

/*! Generic Level Client Terminal commands table */
static terminalCommand_t glvClTerminalTbl[] =
{
  { NULL, "glv", "glv <set|get|setnack|elemid|sraddr|aidx|ttl|level|tid|trans|delay>",
    appMmdlTerminalGlvClHandler },
  { NULL, "gdelta", "gdelta <set|setnack|elemid|sraddr|aidx|ttl|delta|tid|trans|delay>",
    appMmdlTerminalGlvClHandler },
  { NULL, "gmov", "gmov <set|setnack|elemid|sraddr|aidx|ttl|level|tid|trans|delay>",
    appMmdlTerminalGlvClHandler },
};

/*! Light Lightness Client Terminal commands table */
static terminalCommand_t llClTerminalTbl[] =
{
  { NULL, "llact", "llact <set|get|setnack|elemid|sraddr|aidx|ttl|lightness|tid|trans|delay>",
    appMmdlTerminalLlClHandler },
  { NULL, "lllin", "lllin <set|get|setnack|elemid|sraddr|aidx|ttl|lightness|tid|trans|delay>",
    appMmdlTerminalLlClHandler },
  { NULL, "lllast", "lllast <get|elemid|sraddr|aidx|ttl>",
    appMmdlTerminalLlClHandler },
  { NULL, "lldef", "lldef <set|get|setnack|elemid|sraddr|aidx|ttl|lightness>",
    appMmdlTerminalLlClHandler },
  { NULL, "llrange", "llrange <set|get|setnack|elemid|sraddr|aidx|ttl|min|max>",
    appMmdlTerminalLlClHandler },
};

/*! Time Terminal commands table */
static terminalCommand_t timClTerminalTbl[] =
{
  { NULL, "tim", "tim <set|get|elemid|sraddr|aidx|ttl|tais|subs|uncer|tauth|delta|zoffset>",
    appMmdlTerminalTimClHandler },
  { NULL, "tzone", "tzone <set|get|elemid|sraddr|aidx|ttl|new|chg>",
    appMmdlTerminalTimClHandler },
  { NULL, "tdelta", "tdelta <set|get|elemid|sraddr|aidx|ttl|new|chg>",
    appMmdlTerminalTimClHandler },
  { NULL, "trole", "trole <set|get|elemid|sraddr|aidx|ttl|role>",
    appMmdlTerminalTimClHandler },
};

/*! Scene Client Terminal commands table */
static terminalCommand_t sceClTerminalTbl[] =
{
  { NULL, "sce", "sce <get|store|storenack|recall|recallnack|delete|deletenack|regget|elemid|sraddr|aidx|"
                      "ttl|scenenum|tid|trans|delay>",
    appMmdlTerminalSceClHandler },
};

/*! Scheduler Client Terminal commands table */
terminalCommand_t schClTerminalTbl[] =
{
  { NULL, "sch", "sch <get|actget|actset|actsetnack|elemid|sraddr|aidx|"
                      "ttl|index|y|m|d|h|min|sec|dof|act|trans|scenenum>",
    appMmdlTerminalSchClHandler },
};

/*! Generic Power Level Client Terminal commands table */
static terminalCommand_t gplClTerminalTbl[] =
{
  { NULL, "gpl", "gpl <set|get|setnack|elemid|sraddr|aidx|ttl|power|tid|trans|delay>",
    appMmdlTerminalGplClHandler },
  { NULL, "gplast", "gplast <get|elemid|sraddr|aidx|ttl>",
    appMmdlTerminalGpLastClHandler },
  { NULL, "gpdef", "gpdef <set|get|setnack|elemid|sraddr|aidx|ttl|power>",
    appMmdlTerminalGpDefClHandler },
  { NULL, "gprange", "gprange <set|get|setnack|elemid|sraddr|aidx|ttl|min|max>",
    appMmdlTerminalGpRangeClHandler },
};

/*! Light HSL Client Terminal commands table */
terminalCommand_t lightHslClTerminalTbl[] =
{
  { NULL, "lhsl", "lhsl <set|get|setnack|elemid|sraddr|aidx|ttl|ltness|hue|sat|tid|trans|delay>",
    appMmdlTerminalLhslClHandler },
  { NULL, "lhsltarget", "lhsltarget <get|setnack|elemid|sraddr|aidx|ttl>",
    appMmdlTerminalLhslClHandler },
  { NULL, "lhslhue", "lhslhue <set|get|setnack|elemid|sraddr|aidx|ttl|hue|tid|trans|delay>",
    appMmdlTerminalLhslHueClHandler },
  { NULL, "lhslsat", "lhslsat <set|get|setnack|elemid|sraddr|aidx|ttl|sat|tid|trans|delay>",
    appMmdlTerminalLhslSatClHandler },
  { NULL, "lhsldef", "lhsldef <set|get|setnack|elemid|sraddr|aidx|ttl|ltness|hue|sat>",
    appMmdlTerminalLhslDefClHandler },
  { NULL, "lhslrange", "lhslrange <set|get|setnack|elemid|sraddr|aidx|ttl|minhue|maxhue|minsat|"
                       "maxsat>",
    appMmdlTerminalLhslRangeClHandler },
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief         Helper function for processing common request parameters.
 *
 *  \param[in]     argc  The number of arguments passed to the command.
 *  \param[in]     argv  The array of arguments; the 0th argument is the command.
 *  \param[in/out] pReq  Generic request structure. See ::mmdlReq_t.
 *
 *  \return        Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGetReqParams(uint32_t argc, char **argv, mmdlReq_t *pReq)
{
  char *pChar;
  uint8_t i;

  (void)argc;

  /* Extract command */
  if (strcmp(argv[1], "get") == 0)
  {
    pReq->cmd = MMDL_GET;
  }
  else if (strcmp(argv[1], "set") == 0)
  {
    pReq->cmd = MMDL_SET;
  }
  else if (strcmp(argv[1], "setnack") == 0)
  {
    pReq->cmd = MMDL_SET_NO_ACK;
  }
  else if (strcmp(argv[1], "store") == 0)
  {
    pReq->cmd = MMDL_STORE;
  }
  else if (strcmp(argv[1], "storenack") == 0)
  {
    pReq->cmd = MMDL_STORE_NO_ACK;
  }
  else if (strcmp(argv[1], "recall") == 0)
  {
    pReq->cmd = MMDL_RECALL;
  }
  else if (strcmp(argv[1], "recallnack") == 0)
  {
    pReq->cmd = MMDL_RECALL_NO_ACK;
  }
  else if (strcmp(argv[1], "delete") == 0)
  {
    pReq->cmd = MMDL_DELETE;
  }
  else if (strcmp(argv[1], "deletenack") == 0)
  {
    pReq->cmd = MMDL_DELETE_NO_ACK;
  }
  else if (strcmp(argv[1], "regget") == 0)
  {
    pReq->cmd = MMDL_REG_GET;
  }
  else if (strcmp(argv[1], "actget") == 0)
  {
    pReq->cmd = MMDL_ACT_GET;
  }
  else if (strcmp(argv[1], "actset") == 0)
  {
    pReq->cmd = MMDL_ACT_SET;
  }
  else if (strcmp(argv[1], "actsetnack") == 0)
  {
    pReq->cmd = MMDL_ACT_SET_NO_ACK;
  }
  else
  {
    return 1;
  }

  for (i = 2; i < MMDL_TERMINAL_MIN_PARAM; i++)
  {
    if (strstr(argv[i], "elemid=") != NULL)
    {
      /* Found Element ID field. */
      pChar = strchr(argv[i], '=');

      pReq->elementId = (uint16_t)strtol(pChar + 1, NULL, 0);
    }
    else if (strstr(argv[i], "sraddr=") != NULL)
    {
      /* Found Health Server address field. */
      pChar = strchr(argv[i], '=');

      pReq->serverAddr = (uint16_t)strtol(pChar + 1, NULL, 0);
    }
    else if (strstr(argv[i], "aidx=") != NULL)
    {
      /* Found AppKey index field. */
      pChar = strchr(argv[i], '=');

      pReq->appKeyIndex = (uint16_t)strtol(pChar + 1, NULL, 0);
    }
    else if (strstr(argv[i], "ttl=") != NULL)
    {
      /* Found TTL field. */
      pChar = strchr(argv[i], '=');

      pReq->ttl = (uint8_t)strtol(pChar + 1, NULL, 0);
    }
    else
    {
      return i;
    }
  }

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic On Off Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGooClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlGenOnOffSetParam_t  setParam;
  uint8_t argIdx, i;

  /* By default no transition time is sent. */
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxStr("goocl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters */
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("goocl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 2) || (argc == MMDL_TERMINAL_MIN_PARAM + 4)))
    {
      TerminalTxStr("goocl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "onoff=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tid=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("goocl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlGenOnOffClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlGenOnOffClSet(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      case MMDL_SET_NO_ACK:
        MmdlGenOnOffClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("goocl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Power On Off Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGpooClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlGenPowOnOffSetParam_t  setParam;
  uint8_t argIdx, i;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxStr("gpoocl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters */
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("gpoocl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) && (argc != MMDL_TERMINAL_MIN_PARAM + 1))
    {
      TerminalTxStr("gpoocl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "onpowerup=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("gpoocl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlGenPowOnOffClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlGenPowOnOffClSet(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      case MMDL_SET_NO_ACK:
        MmdlGenPowOnOffClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("gpoocl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Default Transition Time Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGdttClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlGenDefaultTransSetParam_t  setParam;
  uint8_t argIdx, i;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxStr("gdttcl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters */
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("gdttcl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
       !((argc == MMDL_TERMINAL_MIN_PARAM + 1)  || (argc == MMDL_TERMINAL_MIN_PARAM)))
    {
      TerminalTxStr("gdttcl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "ttime=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("gdttcl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlGenDefaultTransClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlGenDefaultTransClSet(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      case MMDL_SET_NO_ACK:
        MmdlGenDefaultTransClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("gdttcl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Battery Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGbatClHandler(uint32_t argc, char **argv)
{
  mmdlReq_t req;
  uint8_t argIdx;


  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxStr("gbatcl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters */
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("gbatcl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM)))
    {
      TerminalTxStr("gbatcl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

      return TERMINAL_ERROR_EXEC;
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlGenBatteryClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("gbatcl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Level Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGlvClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlGenLevelSetParam_t  setParam;
  mmdlGenDeltaSetParam_t  setDeltaParam;
  uint8_t argIdx, i;

  /* By default no transition time is sent. */
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters */
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 2) || (argc == MMDL_TERMINAL_MIN_PARAM + 4)))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "level=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state  = (int16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delta=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setDeltaParam.delta = (int32_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tid=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
          setDeltaParam.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
          setDeltaParam.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
          setDeltaParam.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    /* Extract command */
    if (strcmp(argv[0], "glv") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlGenLevelClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlGenLevelClSet(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
          break;

        case MMDL_SET_NO_ACK:
          MmdlGenLevelClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "gmov") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_SET:
          MmdlGenMoveClSet(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
          break;

        case MMDL_SET_NO_ACK:
          MmdlGenMoveClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "gdelta") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_SET:
          MmdlGenDeltaClSet(req.elementId, req.serverAddr, req.ttl, &setDeltaParam, req.appKeyIndex);
          break;

        case MMDL_SET_NO_ACK:
          MmdlGenDeltaClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setDeltaParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
  }

  TerminalTxStr("glvcl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Time Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalTimClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlTimeSetParam_t  setParam;
  mmdlTimeZoneSetParam_t setZoneParam;
  mmdlTimeDeltaSetParam_t setDeltaParam;
  mmdlTimeRoleSetParam_t setRoleParam;

  uint8_t argIdx, i;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }


    if ((req.cmd > MMDL_GET) &&
      !(((argc == MMDL_TERMINAL_MIN_PARAM + 6) && (strstr(argv[0], "tim") != NULL))
        || ((argc == MMDL_TERMINAL_MIN_PARAM + 2) && (strstr(argv[0], "tzone") != NULL))
        || ((argc == MMDL_TERMINAL_MIN_PARAM + 2) && (strstr(argv[0], "tdelta") != NULL))
        || ((argc == MMDL_TERMINAL_MIN_PARAM + 1) && (strstr(argv[0], "trole") != NULL))))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "tais=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state.taiSeconds = (uint64_t)strtoull(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "subs=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state.subSecond = (int8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "uncer=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state.uncertainty = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tauth=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state.timeAuthority = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delta=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state.taiUtcDelta = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "zoffset=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state.timeZoneOffset = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "new=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setZoneParam.state.offsetNew = (uint8_t)strtol(pChar + 1, NULL, 0);
          setDeltaParam.state.deltaNew = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "chg=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setZoneParam.state.taiZoneChange = (uint64_t)strtoull(pChar + 1, NULL, 0);
          setDeltaParam.state.deltaChange = (uint64_t)strtoull(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "role=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setRoleParam.state.timeRole = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
      }
    }

    /* Extract command */
    if (strcmp(argv[0], "tim") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlTimeClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlTimeClSet(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "tzone") == 0)
    {
      switch (req.cmd)
      {
        case MMDL_GET:
          MmdlTimeClZoneGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlTimeClZoneSet(req.elementId, req.serverAddr, req.ttl, &setZoneParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "tdelta") == 0)
    {
      switch (req.cmd)
      {
        case MMDL_GET:
          MmdlTimeClDeltaGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlTimeClDeltaSet(req.elementId, req.serverAddr, req.ttl, &setDeltaParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "trole") == 0)
    {
      switch (req.cmd)
      {
        case MMDL_GET:
          MmdlTimeClRoleGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlTimeClRoleSet(req.elementId, req.serverAddr, req.ttl, &setRoleParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
  }
  TerminalTxStr("tim_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Scene Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalSceClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  uint8_t argIdx, i;
  mmdlSceneRecallParam_t param = {0, 0, MMDL_GEN_TR_UNKNOWN, 0};
  bool_t argCheck;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    /* Check number of parameters */
    switch(req.cmd)
    {
      case MMDL_REG_GET:
      case MMDL_GET:
        argCheck = (argc == MMDL_TERMINAL_MIN_PARAM);
        break;

      case MMDL_DELETE:
      case MMDL_DELETE_NO_ACK:
      case MMDL_STORE:
      case MMDL_STORE_NO_ACK:
        argCheck = (argc == MMDL_TERMINAL_MIN_PARAM + 1);
        break;

      case MMDL_RECALL:
      case MMDL_RECALL_NO_ACK:
        argCheck = (argc == MMDL_TERMINAL_MIN_PARAM + 2) || (argc == MMDL_TERMINAL_MIN_PARAM + 4);
        break;

      default:
        argCheck = FALSE;
        break;
    }

    if (!argCheck)
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);
      return TERMINAL_ERROR_EXEC;
    }

    for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
    {
      if (strstr(argv[i], "scenenum=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.sceneNum  = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "tid=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "trans=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "delay=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else
      {
        TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[i]);
        return TERMINAL_ERROR_EXEC;
      }
    }

    /* Execute command */
    switch(req.cmd)
    {
      case MMDL_REG_GET:
        MmdlSceneClRegisterGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_GET:
        MmdlSceneClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_STORE:
        MmdlSceneClStore(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, param.sceneNum);
        break;

      case MMDL_STORE_NO_ACK:
        MmdlSceneClStoreNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, param.sceneNum);
        break;

      case MMDL_RECALL:
        MmdlSceneClRecall(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex,  &param);
        break;

      case MMDL_RECALL_NO_ACK:
        MmdlSceneClRecallNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &param);
        break;

      case MMDL_DELETE:
        MmdlSceneClDelete(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, param.sceneNum);
        break;

      case MMDL_DELETE_NO_ACK:
        MmdlSceneClDeleteNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, param.sceneNum);
        break;

     default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Scheduler Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalSchClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  uint8_t argIdx, i, entryIdx = 0xFF;
  mmdlSchedulerRegisterEntry_t param;
  bool_t argCheck;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    memset(&param, 0, sizeof(mmdlSchedulerRegisterEntry_t));

    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    /* Check number of parameters */
    switch(req.cmd)
    {
      case MMDL_GET:
        argCheck = (argc == MMDL_TERMINAL_MIN_PARAM);
        break;

      case MMDL_ACT_GET:
        argCheck = (argc == MMDL_TERMINAL_MIN_PARAM + 1);
        break;

      case MMDL_ACT_SET:
      case MMDL_ACT_SET_NO_ACK:
        argCheck = (argc == MMDL_TERMINAL_MIN_PARAM + 11);
        break;

      default:
        argCheck = FALSE;
        break;
    }

    if (!argCheck)
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);
      return TERMINAL_ERROR_EXEC;
    }

    for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
    {
      if (strstr(argv[i], "index=") != NULL)
      {
         pChar = strchr(argv[i], '=');
         entryIdx = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "scenenum=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.sceneNumber  = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "y=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.year  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "m=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.months  = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "d=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.day  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "h=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.hour  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "min=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.minute  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "sec=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.second  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "dof=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.daysOfWeek  = (uint16_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "act=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.action  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else if (strstr(argv[i], "trans=") != NULL)
      {
        pChar = strchr(argv[i], '=');
        param.transTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
      }
      else
      {
        TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[i]);
        return TERMINAL_ERROR_EXEC;
      }
    }

    /* Execute command */
    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlSchedulerClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_ACT_GET:
        if(entryIdx == 0xFF)
        {
          TerminalTxPrint("%s_cnf missing %s" TERMINAL_STRING_NEW_LINE, argv[0], "index");
          return TERMINAL_ERROR_EXEC;
        }
        MmdlSchedulerClActionGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, entryIdx);
        break;

      case MMDL_ACT_SET_NO_ACK:
        if(entryIdx == 0xFF)
        {
          TerminalTxPrint("%s_cnf missing %s" TERMINAL_STRING_NEW_LINE, argv[0], "index");
          return TERMINAL_ERROR_EXEC;
        }
        MmdlSchedulerClActionSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, entryIdx,
                                      (const mmdlSchedulerRegisterEntry_t *)&param);
        break;

      case MMDL_ACT_SET:
        if(entryIdx == 0xFF)
        {
          TerminalTxPrint("%s_cnf missing %s" TERMINAL_STRING_NEW_LINE, argv[0], "index");
          return TERMINAL_ERROR_EXEC;
        }
        MmdlSchedulerClActionSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, entryIdx,
                                 (const mmdlSchedulerRegisterEntry_t *)&param);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Power Level Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGplClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlGenPowerLevelSetParam_t  setParam;
  uint8_t argIdx, i;

  /* By default no transition time is sent. */
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 2) || (argc == MMDL_TERMINAL_MIN_PARAM + 4)))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "power=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.state  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tid=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlGenPowerLevelClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlGenPowerLevelClSet(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      case MMDL_SET_NO_ACK:
        MmdlGenPowerLevelClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setParam, req.appKeyIndex);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Power Level Last state terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGpLastClHandler(uint32_t argc, char **argv)
{
  mmdlReq_t req;
  uint8_t argIdx;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd == MMDL_GET) && (argc == MMDL_TERMINAL_MIN_PARAM))
    {
      MmdlGenPowerLastClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
    }
    else
    {
      TerminalTxPrint("%s_cnf too_many_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);
      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Power Level Default state terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGpDefClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlGenPowerLevelState_t powerLevel = 0;
  uint8_t argIdx, i;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) && !(argc == MMDL_TERMINAL_MIN_PARAM + 1))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "power=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          powerLevel  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlGenPowerDefaultClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlGenPowerDefaultClSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, powerLevel);
        break;

      case MMDL_SET_NO_ACK:
        MmdlGenPowerDefaultClSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, powerLevel);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Generic Power Level Range state terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalGpRangeClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlGenPowerRangeSetParam_t setParam = {0, 0};
  uint8_t argIdx, i;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 2) || (argc == MMDL_TERMINAL_MIN_PARAM + 4)))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "min=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.powerMin  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "max=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.powerMax  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlGenPowerRangeClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlGenPowerRangeClSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      case MMDL_SET_NO_ACK:
        MmdlGenPowerRangeClSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Light Lightness Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalLlClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlLightLightnessSetParam_t        setActParam;
  mmdlLightLightnessLinearSetParam_t  setLinParam;
  mmdlLightLightnessDefaultSetParam_t setDefParam;
  mmdlLightLightnessRangeSetParam_t   setRangeParam;
  uint8_t argIdx, i;

  /* By default no transition time is sent. */
  setActParam.transitionTime = MMDL_GEN_TR_UNKNOWN;
  setLinParam.transitionTime = MMDL_GEN_TR_UNKNOWN;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters */
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 1) || (argc == MMDL_TERMINAL_MIN_PARAM + 2) ||
          (argc == MMDL_TERMINAL_MIN_PARAM + 4)))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "lightness=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setActParam.lightness = setLinParam.lightness = setDefParam.lightness =
              (int16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "min=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setRangeParam.rangeMin = (int16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "max=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setRangeParam.rangeMax = (int16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tid=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setActParam.tid  = setLinParam.tid =
              (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setActParam.transitionTime  = setLinParam.transitionTime =
              (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setActParam.delay  = setLinParam.delay =
              (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    /* Extract command */
    if (strcmp(argv[0], "llact") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlLightLightnessClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlLightLightnessClSet(req.elementId, req.serverAddr, req.ttl, &setActParam, req.appKeyIndex);
          break;

        case MMDL_SET_NO_ACK:
          MmdlLightLightnessClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setActParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "lllin") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlLightLightnessLinearClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlLightLightnessLinearClSet(req.elementId, req.serverAddr, req.ttl, &setLinParam, req.appKeyIndex);
          break;

        case MMDL_SET_NO_ACK:
          MmdlLightLightnessLinearClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setLinParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }

    else if (strcmp(argv[0], "lllast") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlLightLightnessLastClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "lldef") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlLightLightnessDefaultClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlLightLightnessDefaultClSet(req.elementId, req.serverAddr, req.ttl, &setDefParam, req.appKeyIndex);
          break;

        case MMDL_SET_NO_ACK:
          MmdlLightLightnessDefaultClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setDefParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
    else if (strcmp(argv[0], "llrange") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlLightLightnessRangeClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlLightLightnessRangeClSet(req.elementId, req.serverAddr, req.ttl, &setRangeParam, req.appKeyIndex);
          break;

        case MMDL_SET_NO_ACK:
          MmdlLightLightnessRangeClSetNoAck(req.elementId, req.serverAddr, req.ttl, &setRangeParam, req.appKeyIndex);
          break;

        default:
          break;
      }
    }
  }

  TerminalTxStr("llcl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Light HSL Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalLhslClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlLightHslSetParam_t  setParam;
  uint8_t argIdx, i;

  /* By default no transition time is sent. */
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 4) || (argc == MMDL_TERMINAL_MIN_PARAM + 6)))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "ltness=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.lightness  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "hue=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.hue  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "sat=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.saturation  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tid=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    if (strcmp(argv[0], "lhsl") == 0)
    {
      switch(req.cmd)
      {
        case MMDL_GET:
          MmdlLightHslClGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
          break;

        case MMDL_SET:
          MmdlLightHslClSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
          break;

        case MMDL_SET_NO_ACK:
          MmdlLightHslClSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
          break;

        default:
          break;
      }
    }
    else if (req.cmd == MMDL_GET)
    {
      MmdlLightHslClTargetGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Light HSL Hue Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalLhslHueClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlLightHslHueSetParam_t  setParam;
  uint8_t argIdx, i;

  /* By default no transition time is sent. */
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 2) || (argc == MMDL_TERMINAL_MIN_PARAM + 4)))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "hue=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.hue  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tid=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlLightHslClHueGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlLightHslClHueSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      case MMDL_SET_NO_ACK:
        MmdlLightHslClHueSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Light HSL Saturation Client Model terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalLhslSatClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlLightHslSatSetParam_t  setParam;
  uint8_t argIdx, i;

  /* By default no transition time is sent. */
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) &&
        !((argc == MMDL_TERMINAL_MIN_PARAM + 2) || (argc == MMDL_TERMINAL_MIN_PARAM + 4)))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "sat=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.saturation  = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "tid=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.tid  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "trans=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.transitionTime  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "delay=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.delay  = (uint8_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlLightHslClSatGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlLightHslClSatSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      case MMDL_SET_NO_ACK:
        MmdlLightHslClSatSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Light HSL Default state terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalLhslDefClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlLightHslParam_t setParam = {0, 0, 0};
  uint8_t argIdx, i;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) && (argc != MMDL_TERMINAL_MIN_PARAM + 3))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "hue=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.hue = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "sat=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.saturation = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "ltness=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.lightness = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlLightHslClDefGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlLightHslClDefSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      case MMDL_SET_NO_ACK:
        MmdlLightHslClDefSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Light HSL Range state terminal commands.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMmdlTerminalLhslRangeClHandler(uint32_t argc, char **argv)
{
  char *pChar;
  mmdlReq_t req;
  mmdlLightHslRangeSetParam_t setParam = {0, 0, 0, 0};
  uint8_t argIdx, i;

  if (argc < MMDL_TERMINAL_MIN_PARAM)
  {
    TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    /* Extract request parameters*/
    if ((argIdx = appMmdlTerminalGetReqParams(argc, argv, &req)) != 0)
    {
      TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[argIdx]);
      return TERMINAL_ERROR_EXEC;
    }

    if ((req.cmd > MMDL_GET) && (argc != MMDL_TERMINAL_MIN_PARAM + 4))
    {
      TerminalTxPrint("%s_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE, argv[0]);

      return TERMINAL_ERROR_EXEC;
    }
    else
    {
      for (i = MMDL_TERMINAL_MIN_PARAM; i < argc; i++)
      {
        if (strstr(argv[i], "minhue=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.minHue = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "maxhue=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.maxHue = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "minsat=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.minSaturation = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else if (strstr(argv[i], "maxsat=") != NULL)
        {
          pChar = strchr(argv[i], '=');
          setParam.maxSaturation = (uint16_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("%s_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[0], argv[1]);
          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(req.cmd)
    {
      case MMDL_GET:
        MmdlLightHslClRangeGet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex);
        break;

      case MMDL_SET:
        MmdlLightHslClRangeSet(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      case MMDL_SET_NO_ACK:
        MmdlLightHslClRangeSetNoAck(req.elementId, req.serverAddr, req.ttl, req.appKeyIndex, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxPrint("%s_cnf success" TERMINAL_STRING_NEW_LINE, argv[0]);

  return TERMINAL_ERROR_OK;
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic On Off Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGooClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(gooClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&gooClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Power On Off Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGpooClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(gpooClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&gpooClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Level Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGlvClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(glvClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&glvClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Default Transition Time Client Model Application
 *          common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGdttClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(gdttClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&gdttClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Battery Client Model Application common
 *          terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGbatClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(gbatClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&gbatClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Time Client Model Application common
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlTimClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(timClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&timClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Scene Client Model Application common
 *          terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlSceneClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(sceClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&sceClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Scheduler Client Model Application common
 *          terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlSchedulerClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(schClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&schClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Generic Power Level Client Model Application common
 *          terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlGenPowerLevelClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(gplClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&gplClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Light Lightness Client Model Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlLlClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(llClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&llClTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Registers the Light HSL Client Model Application common
 *          terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMmdlLightHslClTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(lightHslClTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&lightHslClTerminalTbl[i]);
  }
}
