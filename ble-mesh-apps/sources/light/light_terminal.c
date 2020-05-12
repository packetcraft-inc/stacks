/*************************************************************************************************/
/*!
 *  \file   light_terminal.c
 *
 *  \brief  Light Terminal.
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
#include "wsf_os.h"
#include "wsf_trace.h"
#include "util/print.h"
#include "util/terminal.h"
#include "util/wstr.h"

#include "dm_api.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_gen_onoff_sr_api.h"

#include "app_mesh_api.h"
#include "light_api.h"
#include "light_terminal.h"

/**************************************************************************************************
  Local Functions Prototypes
**************************************************************************************************/

/*! \brief   GATT Proxy Node Identity user interaction command */
static uint8_t lightTerminalGattSrHandler(uint32_t argc, char **argv);

/*! \brief   Light command */
static uint8_t lightTerminalLightHandler(uint32_t argc, char **argv);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

char * const pLightLogo[]=
{
  "\f\r\n",
  "\n\n\r\n",
  "#     #                        #\r\n",
  "##   ## ######  ####  #    #   #       #  ####  #    # #####\r\n",
  "# # # # #      #      #    #   #       # #    # #    #   #\r\n",
  "#  #  # #####   ####  ######   #       # #      ######   #\r\n",
  "#     # #           # #    #   #       # #  ### #    #   #\r\n",
  "#     # #      #    # #    #   #       # #    # #    #   #\r\n",
  "#     # ######  ####  #    #   ####### #  ####  #    #   #\r\n",
  "\r\n -Press enter for prompt\n\r",
  "\r\n -Type help to display the list of available commands\n\r",
  NULL
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! WSF handler ID */
static wsfHandlerId_t lightTerminalHandlerId;

/*! Test Terminal commands table */
static terminalCommand_t lightTerminalTbl[] =
{
  /*! GATT Proxy Node Identity user interaction command */
  { NULL, "gattsr", "gattsr", lightTerminalGattSrHandler },
  /*! Light command */
  { NULL, "light", "light <on|off>", lightTerminalLightHandler },
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static uint8_t lightTerminalGattSrHandler(uint32_t argc, char **argv)
{
  wsfMsgHdr_t *pMsg;

  (void)argc;
  (void)argv;

  if (!MeshIsProvisioned())
  {
    TerminalTxPrint("gattsr_cnf invalid_state device_unprovisioned" TERMINAL_STRING_NEW_LINE);
    return TERMINAL_ERROR_EXEC;
  }

  if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
  {
    pMsg->event = APP_MESH_NODE_IDENTITY_USER_INTERACTION_EVT;

    /* Send Message. */
    WsfMsgSend(lightTerminalHandlerId, pMsg);
  }

  TerminalTxStr("gattsr_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

static uint8_t lightTerminalLightHandler(uint32_t argc, char **argv)
{
  char    *pChar;
  uint8_t led = 0;
  uint8_t cmd = 0xFF;

  if (argc < 2)
  {
    TerminalTxStr("light_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "on") != NULL)
    {
      pChar = strchr(argv[1], '=');

      if (pChar != NULL)
      {
        led = (uint8_t)strtol(pChar + 1, NULL, 0);
      }

      /* Value not found or invalid. Set default value. */
      if ((led == 0) || (led > 2))
      {
        led = 1;
      }

      cmd = 1;
    }
    else if (strstr(argv[1], "off") != NULL)
    {
      pChar = strchr(argv[1], '=');

      if (pChar != NULL)
      {
        led = (uint8_t)strtol(pChar + 1, NULL, 0);
      }

      /* Value not found or invalid. Set default value. */
      if ((led == 0) || (led > 2))
      {
        led = 1;
      }

      cmd = 0;
    }
    else
    {
      TerminalTxPrint("light_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  MmdlGenOnOffSrSetState(led - 1, cmd ? MMDL_GEN_ONOFF_STATE_ON : MMDL_GEN_ONOFF_STATE_OFF);

  TerminalTxStr("light_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Registers the Light terminal commands.
 *
 *  \param[in] handlerId  Handler ID for TerminalHandler().
 *
 *  \return    None.
 */
/*************************************************************************************************/
void lightTerminalInit(wsfHandlerId_t handlerId)
{
  uint8_t i;

  for (i = 0; i < sizeof(lightTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&lightTerminalTbl[i]);
  }

  lightTerminalHandlerId = handlerId;
}
