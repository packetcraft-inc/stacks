/*************************************************************************************************/
/*!
 *  \file   app_mesh_terminal.c
 *
 *  \brief  Common Mesh application Terminal handler.
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "util/print.h"
#include "util/terminal.h"
#include "util/wstr.h"
#include "wsf_types.h"
#include "wsf_assert.h"

#include "dm_api.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"
#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"
#include "mesh_prv_cl_api.h"
#include "mesh_lpn_api.h"
#include "mesh_friend_api.h"

#include "mmdl_defs.h"
#include "mmdl_types.h"

#include "mesh_ht_mdl_api.h"
#include "mesh_ht_cl_api.h"
#include "mesh_ht_sr_api.h"

#include "app_mesh_api.h"
#include "app_mesh_terminal.h"
#include "app_bearer.h"

#include "cfg_mesh_stack.h"

#if defined(NRF52840_XXAA)
#include "nrf.h"
#else
#error "Unsupported Hardware"
#endif

/**************************************************************************************************
  Local Functions Prototypes
**************************************************************************************************/

/*! \brief  Handler for LE BD_ADDR terminal command. */
static uint8_t appMeshTerminalBdAddrHandler(uint32_t argc, char **argv);

/*! \brief  Handler for Device UUID terminal command. */
static uint8_t appMeshTerminalDevUuidHandler(uint32_t argc, char **argv);

/*! \brief  Handler for LE Filter Policy terminal command. */
static uint8_t appMeshTerminalFilterPolicyHandler(uint32_t argc, char **argv);

/*! \brief  Handler for LE Random Address terminal command. */
static uint8_t appMeshTerminalRandAddrHandler(uint32_t argc, char **argv);

/*! \brief  Handler for Reset the board or LE Mesh Stack terminal command. */
static uint8_t appMeshTerminalResetHandler(uint32_t argc, char **argv);

/*! \brief  Handler for Get App Version terminal command. */
static uint8_t appMeshTerminalVersionHandler(uint32_t argc, char **argv);

/*! \brief  Handler for LE White List terminal command. */
static uint8_t appMeshTerminalWhiteListHandler(uint32_t argc, char **argv);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! App Common Terminal commands table */
static terminalCommand_t appMeshTerminalTbl[] =
{
    /*! LE BD_ADDR */
    { NULL, "bdaddr", "bdaddr <set|get>", appMeshTerminalBdAddrHandler },
    /*! Device UUID */
    { NULL, "devuuid", "devuuid <get>", appMeshTerminalDevUuidHandler },
    /*! LE Filter Policy */
    { NULL, "filterpolicy", "filterpolicy <set>", appMeshTerminalFilterPolicyHandler },
    /*! LE Random Address */
    { NULL, "randaddr", "randaddr <set>", appMeshTerminalRandAddrHandler },
    /*! Reset the board or LE Mesh Stack */
    { NULL, "reset", "reset <board|factory>", appMeshTerminalResetHandler },
    /*! Get App Version */
    { NULL, "version", "version", appMeshTerminalVersionHandler },
    /*! LE White List */
    { NULL, "wlist", "wlist <add|rm|clr|type>", appMeshTerminalWhiteListHandler },
};

/*! App Common Terminal BD_ADDR */
static bdAddr_t appMeshTerminalBdAddr;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handler for LE BD_ADDR terminal command.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshTerminalBdAddrHandler(uint32_t argc, char **argv)
{
  char *pChar;

  if (argc < 2)
  {
    TerminalTxStr("bdaddr_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 3)
      {
        TerminalTxStr("bdaddr_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

        return TERMINAL_ERROR_EXEC;
      }

      if (strstr(argv[2], "addr=") != NULL)
      {
        /* Found Random Address field. */
        pChar = strchr(argv[2], '=');

        /* Store in static variable */
        WStrHexToArray(pChar, appMeshTerminalBdAddr, sizeof(appMeshTerminalBdAddr));

        /* Set BD ADDR. */
        HciVendorSpecificCmd(HCI_OPCODE(HCI_OGF_VENDOR_SPEC, 0x3F0),
                             sizeof(appMeshTerminalBdAddr),
                             (uint8_t *)appMeshTerminalBdAddr);
      }
      else
      {
        TerminalTxPrint("bdaddr_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[2]);

        return TERMINAL_ERROR_EXEC;
      }
    }
    else if (strcmp(argv[1], "get") == 0)
    {
      if (BdaIsZeros(appMeshTerminalBdAddr))
      {
        /* Store BD_ADDR locally */
        BdaCpy(appMeshTerminalBdAddr, HciGetBdAddr());
      }

      /* Obtain from static variable */
      TerminalTxPrint("bdaddr_cnf addr=%x:%x:%x:%x:%x:%x" TERMINAL_STRING_NEW_LINE,
                      appMeshTerminalBdAddr[5], appMeshTerminalBdAddr[4],
                      appMeshTerminalBdAddr[3], appMeshTerminalBdAddr[2],
                      appMeshTerminalBdAddr[1], appMeshTerminalBdAddr[0]);

      return TERMINAL_ERROR_OK;
    }
    else
    {
      TerminalTxPrint("bdaddr_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("bdaddr_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Device UUID terminal command.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshTerminalDevUuidHandler(uint32_t argc, char **argv)
{
  char printBuf[2 * MESH_PRV_DEVICE_UUID_SIZE + 1];

  if (argc < 2)
  {
    TerminalTxStr("devuuid_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "get") == 0)
    {
      for (uint8_t i = 0; i < MESH_PRV_DEVICE_UUID_SIZE; i++)
      {
        sprintf(&printBuf[2 * i], "%02x", pMeshPrvSrCfg->devUuid[i]);
      }

      TerminalTxPrint("devuuid_cnf success uuid=0x%s" TERMINAL_STRING_NEW_LINE, printBuf);
    }
    else
    {
      TerminalTxPrint("devuuid_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for LE Filter Policy terminal command.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshTerminalFilterPolicyHandler(uint32_t argc, char **argv)
{
  char *pChar;
  uint8_t filterPolicy;

  if (argc < 2)
  {
    TerminalTxStr("filterpolicy_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "set=") != NULL)
    {
      /* Found Filter Policy field. */
      pChar = strchr(argv[1], '=');

      filterPolicy = (uint8_t)strtol(pChar + 1, NULL, 0);

      DmDevSetFilterPolicy(DM_FILT_POLICY_MODE_SCAN, filterPolicy);

      /* Toggle bearer. */
      AppBearerDisableSlot(BR_ADV_SLOT);
      AppBearerEnableSlot(BR_ADV_SLOT);
    }
    else
    {
      TerminalTxPrint("filterpolicy_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("filterpolicy_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for LE Random Address terminal command.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshTerminalRandAddrHandler(uint32_t argc, char **argv)
{
  char *pChar;
  bdAddr_t addr;

  if (argc < 2)
  {
    TerminalTxStr("randaddr_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "set=") != NULL)
    {
      /* Found Random Address field. */
      pChar = strchr(argv[1], '=');

      WStrHexToArray(pChar + 1, addr, sizeof(addr));
      WStrReverse(addr, sizeof(bdAddr_t));

      /* Set Random Address. */
      DmDevSetRandAddr(addr);
      DmAdvSetAddrType(HCI_ADDR_TYPE_RANDOM);

      /* Toggle bearer. */
      AppBearerDisableSlot(BR_ADV_SLOT);
      AppBearerEnableSlot(BR_ADV_SLOT);
    }
    else
    {
      TerminalTxPrint("randaddr_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("randaddr_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Reset the board or LE Mesh Stack terminal command.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshTerminalResetHandler(uint32_t argc, char **argv)
{
  if (argc < 2)
  {
    TerminalTxStr("reset_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strcmp(argv[1], "board") == 0)
    {
      NVIC_SystemReset();
    }
    else if (strcmp(argv[1], "factory") == 0)
    {
      /* Clear NVM. */
      AppMeshClearNvm();

      /* Reset board. */
      NVIC_SystemReset();
    }
    else
    {
      TerminalTxPrint("reset_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("reset_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Get App Version terminal command.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshTerminalVersionHandler(uint32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  const char *pVersion;

  MeshGetVersionNumber(&pVersion);

  TerminalTxPrint("version_cnf %s %s" TERMINAL_STRING_NEW_LINE,
                  pVersion, AppMeshGetVersion());

  return TERMINAL_ERROR_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for LE White List terminal command.
 *
 *  \param[in] argc  The number of arguments passed to the command.
 *  \param[in] argv  The array of arguments; the 0th argument is the command.
 *
 *  \return    Error code.
 */
/*************************************************************************************************/
static uint8_t appMeshTerminalWhiteListHandler(uint32_t argc, char **argv)
{
  char *pChar;
  bdAddr_t addr;
  uint8_t addrType = DM_ADDR_RANDOM;

  if (argc < 2)
  {
    TerminalTxStr("wlist_cnf too_few_arguments" TERMINAL_STRING_NEW_LINE);

    return TERMINAL_ERROR_EXEC;
  }
  else
  {
    if (strstr(argv[1], "add=") != NULL)
    {
      /* Found Address field. */
      pChar = strchr(argv[1], '=');

      WStrHexToArray(pChar + 1, addr, sizeof(addr));
      WStrReverse(addr, sizeof(bdAddr_t));

      if ((argc == 3) && strstr(argv[2], "type=") != NULL)
      {
        /* Found proxy field. */
        pChar = strchr(argv[2], '=');

        addrType = (uint8_t)strtol(pChar + 1, NULL, 0);
      }

      /* Add address to white list. */
      DmDevWhiteListAdd(addrType, addr);

      /* Toggle bearer. */
      AppBearerDisableSlot(BR_ADV_SLOT);
      AppBearerEnableSlot(BR_ADV_SLOT);
    }
    else if (strstr(argv[1], "rm=") != NULL)
    {
      /* Found Address field. */
      pChar = strchr(argv[1], '=');

      WStrHexToArray(pChar + 1, addr, sizeof(addr));
      WStrReverse(addr, sizeof(bdAddr_t));

      if ((argc == 3) && strstr(argv[2], "type=") != NULL)
      {
        /* Found proxy field. */
        pChar = strchr(argv[2], '=');

        addrType = (uint8_t)strtol(pChar + 1, NULL, 0);
      }

      /* Remove address from white list. */
      DmDevWhiteListRemove(addrType, addr);

      /* Toggle bearer. */
      AppBearerDisableSlot(BR_ADV_SLOT);
      AppBearerEnableSlot(BR_ADV_SLOT);
    }
    else if (strcmp(argv[1], "clr") == 0)
    {
      DmDevWhiteListClear();

      /* Toggle bearer. */
      AppBearerDisableSlot(BR_ADV_SLOT);
      AppBearerEnableSlot(BR_ADV_SLOT);
    }
    else
    {
      TerminalTxPrint("wlist_cnf invalid_argument %s" TERMINAL_STRING_NEW_LINE, argv[1]);

      return TERMINAL_ERROR_EXEC;
    }
  }

  TerminalTxStr("wlist_cnf success" TERMINAL_STRING_NEW_LINE);

  return TERMINAL_ERROR_OK;
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers the Mesh Application common terminal commands.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshTerminalInit(void)
{
  uint8_t i;

  for (i = 0; i < sizeof(appMeshTerminalTbl)/sizeof(terminalCommand_t); i++)
  {
    TerminalRegisterCommand((terminalCommand_t *)&appMeshTerminalTbl[i]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Prints menu messages.
 *
 *  \param[in] pMenu  Pointer to constant strings array.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppMeshPrintMenu(char * const pMenu[])
{
  uint8_t i = 0;

  while(pMenu[i])
  {
    TerminalTxStr(pMenu[i]);
    i++;
  }
}
