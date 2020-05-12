/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Provisioning menu implementation.
 *
 *  Copyright (c) 2018-2019 Arm Ltd. All Rights Reserved.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_timer.h"
#include "wsf_os.h"
#include "ui_api.h"
#include "hci_defs.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"
#include "mesh_prv_cl_api.h"

#include "provisioner_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Dialog message string length */
#define PROVISIONER_DLG_MSG_LEN               128

/*! Format buffer argument length */
#define PROVISIONER_FORMAT_BUF_ARG_LEN        16

/*! Main menu selections. */
#define PROVISIONER_SEL_ADD_RM_SW             1     /*! Add room switch */
#define PROVISIONER_SEL_ADD_MSTR_SW           2     /*! Add master switch */

/*! Add light dialog selections. */
#define PROVISIONER_SEL_RETRY_ADD_ANOTHER     1     /*! Retry or Add another */
#define PROVISIONER_SEL_CANCEL_DONE           2     /*! Cancel or Done */

/*! UI states. */
#define PROVISIONER_STATE_IDLE                0     /*! Idle state */
#define PROVISIONER_STATE_ADDING_RM_SW        1     /*! Adding room switch state */
#define PROVISIONER_STATE_ADDING_MSTR_SW      2     /*! Adding master switch state */
#define PROVISIONER_STATE_ADDING_LIGHT        3     /*! Adding light state */

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

/*! Menu and dialog process selection function prototypes. */
static void provisionerProcSelMain(const void *pMenu, uint8_t selection);
static void provisionerProcSelGoToMainMenu(const void *pDialog, uint8_t selection);
static void provisionerProcSelGoToAddLight(const void *pDialog, uint8_t selection);
static void provisionerProcSelGoToAddRmSwitch(const void *pDialog, uint8_t selection);
static void provisionerProcSelGoToAddMstrSwitch(const void *pDialog, uint8_t selection);
static void provisionerProcSelLightAdded(const void *pDialog, uint8_t selection);
static void provisionerProcSelRetryAddLight(const void *pDialog, uint8_t selection);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! UI state. */
uint8_t provisionerUiState = PROVISIONER_STATE_IDLE;

/*! Dialog message string for UUIDs, keys and errors. */
char provisionerDlgMsgStr[PROVISIONER_DLG_MSG_LEN];

/*! Splash Screen */
static const UiSplashScreen_t provisionerSplash =
{
  "Mesh Provisioner, r19.02",
  "(c)2018-2019 Arm, Ltd.",
  "\0",
  1500
};

/*! List of menu selections */
static constStr provisionerMainSelectList[] =
{
  "Add Room",
  "Add Master Switch",
};

/*! Main menu */
static const UiMenu_t provisionerMain =
{
  {NULL, NULL},
  "Main Menu",
  sizeof(provisionerMainSelectList) / sizeof(constStr),
  0,
  provisionerProcSelMain,
  provisionerMainSelectList,
  0
};

/*! Single cancel selection */
static constStr provisionerSelectionCancel[] =
{
  "Cancel",
};

/*! Single next selection */
static constStr provisionerSelectionNext[] =
{
  "Next",
};

/*! Single retry selection */
static constStr provisionerSelectionRetry[] =
{
  "Retry",
  "Cancel",
};

/*! Single done selection */
static constStr provisionerSelectionDone[] =
{
  "Done",
};

/*! Two option Add Another Light or Done selection */
static constStr provisionerSelectionAddAnother[] =
{
  "Add Another Light",
  "Done"
};

/*! Add room switch dialog */
static const UiDialog_t provisionerDlgAddRmSwitch =
{
  {NULL, NULL},
  "Add Room Switch",
  "Power on switch...",
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelGoToMainMenu,
  sizeof(provisionerSelectionCancel) / sizeof(constStr),
  provisionerSelectionCancel,
};

/*! Add master switch dialog */
static const UiDialog_t provisionerDlgAddMstrSwitch =
{
  {NULL, NULL},
  "Add Master Switch",
  "Power on switch...",
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelGoToMainMenu,
  sizeof(provisionerSelectionCancel) / sizeof(constStr),
  provisionerSelectionCancel,
};

/*! Add light dialog */
static const UiDialog_t provisionerDlgAddLight =
{
  {NULL, NULL},
  "Add Light",
  "Power on light...",
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelGoToMainMenu,
  sizeof(provisionerSelectionCancel) / sizeof(constStr),
  provisionerSelectionCancel,
};

/*! Room switch added dialog */
static const UiDialog_t provisionerDlgRmSwAdded =
{
  {NULL, NULL},
  "Room Switch Added",
  provisionerDlgMsgStr,
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelGoToAddLight,
  sizeof(provisionerSelectionNext) / sizeof(constStr),
  provisionerSelectionNext,
};

/*! Room switch error dialog */
static const UiDialog_t provisionerDlgRmSwError =
{
  {NULL, NULL},
  "Room Switch Error",
  provisionerDlgMsgStr,
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelGoToAddRmSwitch,
  sizeof(provisionerSelectionRetry) / sizeof(constStr),
  provisionerSelectionRetry,
};

/*! Master switch added dialog */
static const UiDialog_t provisionerDlgMstrSwAdded =
{
  {NULL, NULL},
  "Master Switch Added",
  provisionerDlgMsgStr,
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelGoToMainMenu,
  sizeof(provisionerSelectionDone) / sizeof(constStr),
  provisionerSelectionDone,
};

/*! Master switch error dialog */
static const UiDialog_t provisionerDlgMstrSwError =
{
  {NULL, NULL},
  "Master Switch Error",
  provisionerDlgMsgStr,
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelGoToAddMstrSwitch,
  sizeof(provisionerSelectionRetry) / sizeof(constStr),
  provisionerSelectionRetry,
};

/*! Light added dialog */
static const UiDialog_t provisionerDlgLightAdded =
{
  {NULL, NULL},
  "Light Added",
  provisionerDlgMsgStr,
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelLightAdded,
  sizeof(provisionerSelectionAddAnother) / sizeof(constStr),
  provisionerSelectionAddAnother,
};

/*! Light error dialog */
static const UiDialog_t provisionerDlgLightError =
{
  {NULL, NULL},
  "Light Error",
  provisionerDlgMsgStr,
  UI_DLG_TYPE_INPUT_SELECT,
  NULL,
  0,
  0,
  provisionerProcSelRetryAddLight,
  sizeof(provisionerSelectionRetry) / sizeof(constStr),
  provisionerSelectionRetry,
};

/*************************************************************************************************/
/*!
 *  \brief  Convert an integer buffer to a string.
 *
 *  \param  pBuf    Pointer to the buffer.
 *
 *  \return Pointer to string.
 */
/*************************************************************************************************/
static char *bin2Str(const uint8_t *pBuf)
{
  static const char hex[] = "0123456789ABCDEF";
  static char       str[(PROVISIONER_FORMAT_BUF_ARG_LEN * 2) + 1];
  char              *pStr = str;

  /* Address is little endian so copy in reverse. */
  pBuf += PROVISIONER_FORMAT_BUF_ARG_LEN;

  while (pStr < &str[PROVISIONER_FORMAT_BUF_ARG_LEN * 2])
  {
    *pStr++ = hex[*--pBuf >> 4];
    *pStr++ = hex[*pBuf & 0x0F];
  }

  /* NULL terminate string. */
  *pStr = 0;

  return str;
}

/*************************************************************************************************/
/*!
 *  \brief  Format a 128-bit buffer.
 *
 *  \param  pStr        String buffer
 *  \param  pBuf        Pointer to buffer
 *
 *  \return Pointer to new head of buffer.
 */
/*************************************************************************************************/
static char *format128BitBuf(char *pStr, uint8_t *pBuf)
{
  unsigned int i, pos=0;
  char *pIdStr = bin2Str(pBuf);

  /* Copy buffer and add colons. */
  for (i = 0; i < PROVISIONER_FORMAT_BUF_ARG_LEN; i++)
  {
    pStr[pos++] = pIdStr[i*2];
    pStr[pos++] = pIdStr[i*2 + 1];

    if (i < PROVISIONER_FORMAT_BUF_ARG_LEN - 1)
    {
      pStr[pos++] = ':';
    }
  }

  pStr[pos++] = '\0';

  /* Return pointer in case caller wishes to append more data. */
  return &pStr[pos - 1];
}

/*************************************************************************************************/
/*!
 *  \brief  Add an error code to the end of a given string.
 *
 *  \param  pStr        String buffer
 *  \param  error       Error code
 *
 *  \return None.
 */
/*************************************************************************************************/
static void strCatError(char *pStr, uint8_t error)
{
  static const char hex[] = "0123456789ABCDEF";

  strcat(pStr, "\r\nError: ");
  pStr += strlen(pStr);

  *pStr++ = hex[error >> 4];
  *pStr++ = hex[error & 0x0F];
  *pStr = '\0';
}

/*************************************************************************************************/
/*!
 *  \brief  Add room switch action
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerAddRoomSwitch(void)
{
  /* Update state */
  provisionerUiState = PROVISIONER_STATE_ADDING_RM_SW;

  /* Display Dialog */
  UiLoadDialog(&provisionerDlgAddRmSwitch);

  /* Begin Provisioning */
  ProvisionerProvisionDevice(PROVISIONER_PRV_ROOM_SWITCH);
}

/*************************************************************************************************/
/*!
 *  \brief  Add master switch action
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerAddMasterSwitch(void)
{
  /* Update state */
  provisionerUiState = PROVISIONER_STATE_ADDING_MSTR_SW;

  /* Display Dialog */
  UiLoadDialog(&provisionerDlgAddMstrSwitch);

  /* Begin Provisioning */
  ProvisionerProvisionDevice(PROVISIONER_PRV_MASTER_SWITCH);
}

/*************************************************************************************************/
/*!
 *  \brief  Add light action
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerAddLight(void)
{
  /* Update state */
  provisionerUiState = PROVISIONER_STATE_ADDING_LIGHT;

  /* Display Dialog */
  UiLoadDialog(&provisionerDlgAddLight);

  /* Begin Provisioning */
  ProvisionerProvisionDevice(PROVISIONER_PRV_LIGHT);
}

/*************************************************************************************************/
/*!
 *  \brief  Complete add room switch process.
 *
 *  \param  status      Status code
 *  \param  pUuid       Device UUID
 *  \param  pDevKey     Device key of provisioned node.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerAddRoomSwitchCmpl(uint8_t status, uint8_t *pUuid, uint8_t *pDevKey)
{
  char *p;

  /* Return to IDLE state */
  provisionerUiState = PROVISIONER_STATE_IDLE;

  p = provisionerDlgMsgStr;

  strcpy(p, "ID: ");
  p += strlen(p);

  /* Set UUID */
  p = format128BitBuf(p, pUuid);

  /* Add new line for device key */
  strcpy(p, "\r\nDevice Key: ");
  p += strlen(p);
  (void) format128BitBuf(p, pDevKey);

  /* Display next dialog */
  if (status == MESH_SUCCESS)
  {
    UiLoadDialog(&provisionerDlgRmSwAdded);
  }
  else
  {
    strCatError(provisionerDlgMsgStr, status);
    UiLoadDialog(&provisionerDlgRmSwError);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Complete add master switch process.
 *
 *  \param  status      Status code
 *  \param  pUuid       Device UUID
 *  \param  pDevKey     Device key of provisioned node.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerAddMstrSwitchCmpl(uint8_t status, uint8_t *pUuid, uint8_t *pDevKey)
{
  char *p;

  /* Return to IDLE state */
  provisionerUiState = PROVISIONER_STATE_IDLE;

  p = provisionerDlgMsgStr;

  strcpy(p, "ID: ");
  p += strlen(p);

  /* Set UUID */
  p = format128BitBuf(p, pUuid);

  /* Add new line for device key */
  strcpy(p, "\r\nDevice Key: ");
  p += strlen(p);
  (void) format128BitBuf(p, pDevKey);

  /* Display next dialog */
  if (status == MESH_SUCCESS)
  {
    UiLoadDialog(&provisionerDlgMstrSwAdded);
  }
  else
  {
    strCatError(provisionerDlgMsgStr, status);
    UiLoadDialog(&provisionerDlgMstrSwError);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Complete add light process.
 *
 *  \param  status      Status code
 *  \param  pUuid       Device UUID
 *  \param  pDevKey     Device key of provisioned node.\
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerAddLightCmpl(uint8_t status, uint8_t *pUuid, uint8_t *pDevKey)
{
  char *p;

  /* Return to IDLE state */
  provisionerUiState = PROVISIONER_STATE_IDLE;

  p = provisionerDlgMsgStr;

  strcpy(p, "ID: ");
  p += strlen(p);

  /* Set UUID */
  p = format128BitBuf(p, pUuid);

  /* Add new line for device key */
  strcpy(p, "\r\nDevice Key: ");
  p += strlen(p);
  (void) format128BitBuf(p, pDevKey);

  /* Display next dialog */
  if (status == MESH_SUCCESS)
  {
    UiLoadDialog(&provisionerDlgLightAdded);
  }
  else
  {
    strCatError(provisionerDlgMsgStr, status);
    UiLoadDialog(&provisionerDlgLightError);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process menu selection for Main Menu.
 *
 *  \param  pMenu       Pointer to the current menu.
 *  \param  selection   User selection.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcSelMain(const void *pMenu, uint8_t selection)
{
  switch(selection)
  {
  case PROVISIONER_SEL_ADD_RM_SW:
    provisionerAddRoomSwitch();
    break;

  case PROVISIONER_SEL_ADD_MSTR_SW:
    provisionerAddMasterSwitch();
    break;

  default:
    break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Called on dialog selection when only option is to return to the Main Menu.
 *
 *  \param  pDialog     Pointer to the current dialog.
 *  \param  selection   User selection.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcSelGoToMainMenu(const void *pDialog, uint8_t selection)
{
  switch(provisionerUiState)
  {
    case PROVISIONER_STATE_ADDING_RM_SW:   /* Fallthrough */
    case PROVISIONER_STATE_ADDING_MSTR_SW: /* Fallthrough */
    case PROVISIONER_STATE_ADDING_LIGHT:   /* Fallthrough */
    default:
      ProvisionerCancelProvisioning();
      break;
  }

  /* Return to idle state. */
  provisionerUiState = PROVISIONER_STATE_IDLE;

  UiLoadMenu(&provisionerMain);
}

/*************************************************************************************************/
/*!
 *  \brief  Called on dialog selection when only option is to go to Add Room Switch dialog.
 *
 *  \param  pDialog     Pointer to the current dialog.
 *  \param  selection   User selection.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcSelGoToAddRmSwitch(const void *pDialog, uint8_t selection)
{
  switch(selection)
  {
    case PROVISIONER_SEL_RETRY_ADD_ANOTHER:
      provisionerAddRoomSwitch();
      break;

    case PROVISIONER_SEL_CANCEL_DONE:
      /* Return to idle state. */
      provisionerUiState = PROVISIONER_STATE_IDLE;
      UiLoadMenu(&provisionerMain);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Called on dialog selection when only option is to go to Add Master Switch dialog.
 *
 *  \param  pDialog     Pointer to the current dialog.
 *  \param  selection   User selection.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcSelGoToAddMstrSwitch(const void *pDialog, uint8_t selection)
{
  switch(selection)
  {
    case PROVISIONER_SEL_RETRY_ADD_ANOTHER:
      provisionerAddMasterSwitch();
      break;

    case PROVISIONER_SEL_CANCEL_DONE:
      /* Return to idle state. */
      provisionerUiState = PROVISIONER_STATE_IDLE;
      UiLoadMenu(&provisionerMain);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Called on dialog selection when only option is to go to Add Light dialog.
 *
 *  \param  pDialog     Pointer to the current dialog.
 *  \param  selection   User selection.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcSelGoToAddLight(const void *pDialog, uint8_t selection)
{
  provisionerAddLight();
}

/*************************************************************************************************/
/*!
 *  \brief  Process dialog selection for Light Added dialog.
 *
 *  \param  pDialog     Pointer to the current dialog.
 *  \param  selection   User selection.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcSelRetryAddLight(const void *pDialog, uint8_t selection)
{
  switch (selection)
  {
    case PROVISIONER_SEL_RETRY_ADD_ANOTHER:
      provisionerAddLight();
      break;

    case PROVISIONER_SEL_CANCEL_DONE:
     /* Return to idle state. */
     provisionerUiState = PROVISIONER_STATE_IDLE;
     UiLoadMenu(&provisionerMain);
     break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process dialog selection for Light Added dialog.
 *
 *  \param  pDialog     Pointer to the current dialog.
 *  \param  selection   User selection.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void provisionerProcSelLightAdded(const void *pDialog, uint8_t selection)
{
  switch (selection)
  {
  case PROVISIONER_SEL_RETRY_ADD_ANOTHER:
    provisionerAddLight();
    break;

  case PROVISIONER_SEL_CANCEL_DONE:
    UiLoadMenu(&provisionerMain);
    break;

  default:
    break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Notify application menu of system events.
 *
 *  \param  status  status of event.
 *  \param  pUuid   Device UUID or NULL if not used.
 *  \param  pDevKey     Device key of provisioned node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerMenuHandleEvent(uint8_t status, uint8_t *pUuid, uint8_t *pDevKey)
{
  /* Complete activity */
  switch (provisionerUiState)
  {
    case PROVISIONER_STATE_ADDING_RM_SW:
      provisionerAddRoomSwitchCmpl(status, pUuid, pDevKey);
      break;

    case PROVISIONER_STATE_ADDING_MSTR_SW:
      provisionerAddMstrSwitchCmpl(status, pUuid, pDevKey);
      break;

    case PROVISIONER_STATE_ADDING_LIGHT:
      provisionerAddLightCmpl(status, pUuid, pDevKey);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Provisioner application User Interface initialization.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ProvisionerUiInit(void)
{
  /* Initialize UI */
  UiTimerInit();
  UiConsoleInit();
  UiInit(&provisionerSplash, &provisionerMain);
}
