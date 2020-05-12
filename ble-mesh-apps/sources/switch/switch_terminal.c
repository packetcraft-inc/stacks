/*************************************************************************************************/
/*!
 *  \file   switch_terminal.c
 *
 *  \brief  Switch Terminal.
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
 */
/*************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
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
#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"
#include "mesh_lpn_api.h"

#include "adv_bearer.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "app_mesh_api.h"
#include "switch_api.h"
#include "switch_config.h"
#include "switch_terminal.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Switch Terminal Mesh Model commands */
enum switchTerminalMmdlCmd
{
  SWITCH_TERMINAL_MMDL_GET = 0x00,          /*!< Get command. */
  SWITCH_TERMINAL_MMDL_SET,                 /*!< Set command. */
  SWITCH_TERMINAL_MMDL_SET_NO_ACK,          /*!< Set Unacknowledged command. */
};

/*! Switch Terminal Mesh Model commands type. See ::switchTerminalMmdlCmd */
typedef uint8_t switchTerminalMmdlCmd_t;

/**************************************************************************************************
  Local Functions Prototypes
**************************************************************************************************/

/*! \brief   Emulates a button */
static uint8_t switchTerminalBtnHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Generic On Off message */
static uint8_t switchTerminalGenOnOffMsgHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Generic Level message */
static uint8_t switchTerminalGenLvlMsgHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Generic On Power Up message */
static uint8_t switchTerminalGenOnPowUpMsgHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Light HSL message */
static uint8_t switchTerminalLightHslMsgHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Light HSL Hue message */
static uint8_t switchTerminalLightHMsgHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Light HSL Saturation message */
static uint8_t switchTerminalLightSMsgHandler(uint32_t argc, char **argv);

/*! \brief   Transmit a Light Lightness message */
static uint8_t switchTerminalLightLMsgHandler(uint32_t argc, char **argv);

/*! \brief   LPN functionality */
static uint8_t switchTerminalLpnHandler(uint32_t argc, char **argv);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

char * const pSwitchLogo[]=
{
  "\f\r\n",
  "\n\n\r\n",
  "#     #                         #####\r\n",
  "##   ## ######  ####  #    #   #     # #    # # #####  ####  #    #\r\n",
  "# # # # #      #      #    #   #       #    # #   #   #    # #    #\r\n",
  "#  #  # #####   ####  ######    #####  #    # #   #   #      ######\r\n",
  "#     # #           # #    #         # # ## # #   #   #      #    #\r\n",
  "#     # #      #    # #    #   #     # ##  ## #   #   #    # #    #\r\n",
  "#     # ######  ####  #    #    #####  #    # #   #    ####  #    #\r\n",
  "\r\n -Press enter for prompt\n\r",
  "\r\n -Type help to display the list of available commands\n\r",
  NULL
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Test Terminal commands table */
static terminalCommand_t switchTerminalTbl[] =
{
  /*! Emulates a button */
  { NULL, "btn", "btn <press|release>", switchTerminalBtnHandler },
  /*! Transmit Mesh Generic OnOff message. */
  { NULL, "genonoff", "genonoff <get|set|setnack|elemid|state|trans|delay>",
    switchTerminalGenOnOffMsgHandler },
  /*! Transmit Mesh Generic Level message. */
  { NULL, "genlvl", "genlvl <get|set|setnack|elemid|state|trans|delay>",
    switchTerminalGenLvlMsgHandler },
  /*! Transmit Mesh Generic On Power Up message. */
  { NULL, "genonpowup", "genonpowup <get|set|setnack|elemid|state>",
    switchTerminalGenOnPowUpMsgHandler },
  /*! Transmit Mesh Light HSL message. */
  { NULL, "lighthsl", "lighthsl <get|set|setnack|elemid|h|s|l|trans|delay>",
    switchTerminalLightHslMsgHandler },
  /*! Transmit Mesh Light HSL Hue message. */
  { NULL, "lighth", "lighth <get|set|setnack|elemid|h|trans|delay>",
    switchTerminalLightHMsgHandler },
  /*! Transmit Mesh Light HSL Saturation message. */
  { NULL, "lights", "lights <get|set|setnack|elemid|s|trans|delay>",
    switchTerminalLightSMsgHandler },
  /*! Transmit Mesh Light Lightness message. */
  { NULL, "lightl", "lightl <get|set|setnack|elemid|l|trans|delay>",
    switchTerminalLightLMsgHandler },
    /*! LPN functionality */
  { NULL, "lpn", "lpn <est|term|nidx|rssifact|recvwinfact|minqszlog|sleep|recvdelay|retrycnt>",
    switchTerminalLpnHandler },
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static uint8_t switchTerminalBtnHandler(uint32_t argc, char **argv)
{
  char                   *pChar;
  uint8_t                elementId = 0;
  mmdlGenOnOffSetParam_t setParam;
  uint8_t                btn = 0;

  if (argc < 2)
  {
    TerminalTxStr("btn_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "press") != NULL)
    {
      /* Found button number. */
      pChar = strchr(argv[1], '=');

      if (pChar != NULL)
      {
        btn = (uint8_t)strtol(pChar + 1, NULL, 0);
      }

      /* Check if second button was emulated. */
      if (btn == 2)
      {
        elementId = 1;
      }

      switchElemCb[elementId].state = MMDL_GEN_ONOFF_STATE_ON;
    }
    else if (strstr(argv[1], "release") != NULL)
    {
      /* Found button number. */
      pChar = strchr(argv[1], '=');

      if (pChar != NULL)
      {
        btn = (uint8_t)strtol(pChar + 1, NULL, 0);
      }

      /* Check if second button was emulated. */
      if (btn == 2)
      {
        elementId = 1;
      }

      switchElemCb[elementId].state = MMDL_GEN_ONOFF_STATE_OFF;
    }
    else
    {
      TerminalTxPrint("btn_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  setParam.state = switchElemCb[elementId].state;
  setParam.tid = switchElemCb[elementId].tid++;
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;
  setParam.delay = 0;

  MmdlGenOnOffClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);

  TerminalTxPrint("btn_cnf btn=%d state=on" TERMINAL_STRING_NEW_LINE, (btn ? 2 : 1));

  return TERMINAL_ERROR_OK;
}

static uint8_t switchTerminalGenOnOffMsgHandler(uint32_t argc, char **argv)
{
  char                    *pChar;
  meshElementId_t         elementId = 0;
  mmdlGenOnOffSetParam_t  setParam;
  switchTerminalMmdlCmd_t cmd = SWITCH_TERMINAL_MMDL_GET;
  mmdlGenOnOffState_t     state = 0;
  uint8_t                 transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                 delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("genonoff_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("genonoff_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && (cmd != SWITCH_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == SWITCH_TERMINAL_MMDL_GET)))
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

          if (elementId >= SWITCH_ELEMENT_COUNT)
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
            switchElemCb[elementId].state = MMDL_GEN_ONOFF_STATE_ON;
          }
          else
          {
            switchElemCb[elementId].state = MMDL_GEN_ONOFF_STATE_OFF;
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
      case SWITCH_TERMINAL_MMDL_GET:
        MmdlGenOnOffClGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET:
        setParam.state = switchElemCb[elementId].state;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlGenOnOffClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET_NO_ACK:
        setParam.state = switchElemCb[elementId].state;
        setParam.tid = switchElemCb[elementId].tid++;
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

static uint8_t switchTerminalGenLvlMsgHandler(uint32_t argc, char **argv)
{
  char                    *pChar;
  meshElementId_t         elementId = 0;
  mmdlGenLevelSetParam_t  setParam;
  switchTerminalMmdlCmd_t cmd = SWITCH_TERMINAL_MMDL_GET;
  mmdlGenLevelState_t     state = 0;
  uint8_t                 transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                 delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("genlvl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("genlvl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && (cmd != SWITCH_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == SWITCH_TERMINAL_MMDL_GET)))
    {
      TerminalTxStr("genlvl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

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

          if (elementId >= SWITCH_ELEMENT_COUNT)
          {
            TerminalTxPrint("genlvl_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
        }
        else if (strstr(argv[i], "state=") != NULL)
        {
          /* Found state field. */
          pChar = strchr(argv[i], '=');

          state = (mmdlGenLevelState_t)strtol(pChar + 1, NULL, 0);
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
          TerminalTxPrint("genlvl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(cmd)
    {
      case SWITCH_TERMINAL_MMDL_GET:
        MmdlGenLevelClGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET:
        setParam.state = state;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlGenLevelClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET_NO_ACK:
        setParam.state = state;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlGenLevelClSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("genlvl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t switchTerminalGenOnPowUpMsgHandler(uint32_t argc, char **argv)
{
  char                       *pChar;
  meshElementId_t            elementId = 0;
  mmdlGenPowOnOffSetParam_t  setParam;
  switchTerminalMmdlCmd_t    cmd = SWITCH_TERMINAL_MMDL_GET;
  mmdlGenOnPowerUpState_t    state = MMDL_GEN_ONPOWERUP_STATE_OFF;

  if (argc < 2)
  {
    TerminalTxStr("genonpowup_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("genonpowup_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && (cmd != SWITCH_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == SWITCH_TERMINAL_MMDL_GET)))
    {
      TerminalTxStr("genonpowup_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

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

          if (elementId >= SWITCH_ELEMENT_COUNT)
          {
            TerminalTxPrint("genonpowup_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
        }
        else if (strstr(argv[i], "state=") != NULL)
        {
          /* Found state field. */
          pChar = strchr(argv[i], '=');

          state = (mmdlGenOnPowerUpState_t)strtol(pChar + 1, NULL, 0);
        }
        else
        {
          TerminalTxPrint("genonpowup_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(cmd)
    {
      case SWITCH_TERMINAL_MMDL_GET:
        MmdlGenPowOnOffClGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET:
        setParam.state = state;

        MmdlGenPowOnOffClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET_NO_ACK:
        setParam.state = state;

        MmdlGenPowOnOffClSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("genonpowup_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t switchTerminalLightHslMsgHandler(uint32_t argc, char **argv)
{
  char                     *pChar;
  meshElementId_t          elementId = 0;
  mmdlLightHslSetParam_t   setParam;
  switchTerminalMmdlCmd_t  cmd = SWITCH_TERMINAL_MMDL_GET;
  uint16_t                 lightness = 0;
  uint16_t                 hue = 0;
  uint16_t                 saturation = 0;
  uint8_t                  transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                  delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("lighthsl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("lighthsl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 6) && (cmd != SWITCH_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == SWITCH_TERMINAL_MMDL_GET)))
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

          if (elementId >= SWITCH_ELEMENT_COUNT)
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
      case SWITCH_TERMINAL_MMDL_GET:
        MmdlLightHslClGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET:
        setParam.hue = hue;
        setParam.saturation = saturation;
        setParam.lightness = lightness;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightHslClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0, &setParam);
        break;

      case SWITCH_TERMINAL_MMDL_SET_NO_ACK:
        setParam.hue = hue;
        setParam.saturation = saturation;
        setParam.lightness = lightness;
        setParam.tid = switchElemCb[elementId].tid++;
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

static uint8_t switchTerminalLightHMsgHandler(uint32_t argc, char **argv)
{
  char                       *pChar;
  meshElementId_t            elementId = 0;
  mmdlLightHslHueSetParam_t  setParam;
  switchTerminalMmdlCmd_t    cmd = SWITCH_TERMINAL_MMDL_GET;
  uint16_t                   hue = 0;
  uint8_t                    transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                    delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("lighth_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("lighth_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && (cmd != SWITCH_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == SWITCH_TERMINAL_MMDL_GET)))
    {
      TerminalTxStr("lighth_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

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

          if (elementId >= SWITCH_ELEMENT_COUNT)
          {
            TerminalTxPrint("lighth_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
        }
        else if (strstr(argv[i], "h=") != NULL)
        {
          /* Found hue field. */
          pChar = strchr(argv[i], '=');

          hue = (uint16_t)strtol(pChar + 1, NULL, 0);
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
          TerminalTxPrint("lighth_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(cmd)
    {
      case SWITCH_TERMINAL_MMDL_GET:
        MmdlLightHslClHueGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET:
        setParam.hue = hue;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightHslClHueSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0, &setParam);
        break;

      case SWITCH_TERMINAL_MMDL_SET_NO_ACK:
        setParam.hue = hue;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightHslClHueSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("lighth_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t switchTerminalLightSMsgHandler(uint32_t argc, char **argv)
{
  char                       *pChar;
  meshElementId_t            elementId = 0;
  mmdlLightHslSatSetParam_t  setParam;
  switchTerminalMmdlCmd_t    cmd = SWITCH_TERMINAL_MMDL_GET;
  uint16_t                   saturation = 0;
  uint8_t                    transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                    delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("lights_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("lights_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && (cmd != SWITCH_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == SWITCH_TERMINAL_MMDL_GET)))
    {
      TerminalTxStr("lights_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

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

          if (elementId >= SWITCH_ELEMENT_COUNT)
          {
            TerminalTxPrint("lights_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
        }
        else if (strstr(argv[i], "s=") != NULL)
        {
          /* Found saturation field. */
          pChar = strchr(argv[i], '=');

          saturation = (uint16_t)strtol(pChar + 1, NULL, 0);
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
          TerminalTxPrint("lights_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(cmd)
    {
      case SWITCH_TERMINAL_MMDL_GET:
        MmdlLightHslClSatGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET:
        setParam.saturation = saturation;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightHslClSatSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0, &setParam);
        break;

      case SWITCH_TERMINAL_MMDL_SET_NO_ACK:
        setParam.saturation = saturation;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightHslClSatSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0, &setParam);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("lights_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t switchTerminalLightLMsgHandler(uint32_t argc, char **argv)
{
  char                          *pChar;
  meshElementId_t               elementId = 0;
  mmdlLightLightnessSetParam_t  setParam;
  switchTerminalMmdlCmd_t       cmd = SWITCH_TERMINAL_MMDL_GET;
  uint16_t                      lightness = 0;
  uint8_t                       transitionTime = MMDL_GEN_TR_UNKNOWN;
  uint8_t                       delay = 0;

  if (argc < 2)
  {
    TerminalTxStr("lightl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_GET;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET;
    }
    else if (strcmp(argv[1], "setnack") == 0)
    {
      cmd = SWITCH_TERMINAL_MMDL_SET_NO_ACK;
    }
    else
    {
      TerminalTxPrint("lightl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }

    if (((argc < 4) && (cmd != SWITCH_TERMINAL_MMDL_GET)) ||
        ((argc < 3) && (cmd == SWITCH_TERMINAL_MMDL_GET)))
    {
      TerminalTxStr("lightl_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

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

          if (elementId >= SWITCH_ELEMENT_COUNT)
          {
            TerminalTxPrint("lightl_cnf invalid_value %s" TERMINAL_STRING_NEW_LINE, argv[i]);

            return TERMINAL_ERROR_EXEC;
          }
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
          TerminalTxPrint("lightl_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[i]);

          return TERMINAL_ERROR_EXEC;
        }
      }
    }

    switch(cmd)
    {
      case SWITCH_TERMINAL_MMDL_GET:
        MmdlLightLightnessClGet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET:
        setParam.lightness = lightness;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightLightnessClSet(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      case SWITCH_TERMINAL_MMDL_SET_NO_ACK:
        setParam.lightness = lightness;
        setParam.tid = switchElemCb[elementId].tid++;
        setParam.transitionTime = transitionTime;
        setParam.delay = delay;

        MmdlLightLightnessClSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
        break;

      default:
        break;
    }
  }

  TerminalTxStr("lightl_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t switchTerminalLpnHandler(uint32_t argc, char **argv)
{
  char *pChar;
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

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Switch terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void switchTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(switchTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&switchTerminalTbl[i]);
  }
}
