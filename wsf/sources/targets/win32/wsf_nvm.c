/*************************************************************************************************/
/*!
 *  \file   wsf_nvm.c
 *
 *  \brief  Win32 host tester simulation of NVM.
 *
 *  Copyright (c) 2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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
#include <stdio.h>
#include <sys/stat.h>
#include <direct.h>
#include "wsf_types.h"
#include "wsf_nvm.h"
#include "wsf_assert.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* Device instance string max length. */
#define WSF_NVM_INSTANCE_LEN              16

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/* Device instance - to allow simulation of multiple devices simultaneously. */
static char wsfNvmInstance[WSF_NVM_INSTANCE_LEN];

/* NVM simulation enabled. */
static bool_t wsfNvmEnabled = FALSE;

/*************************************************************************************************/
/*!
 *  \brief  Initialize the WSF NVM.
 */
/*************************************************************************************************/
void WsfNvmInit(void)
{
}

/*************************************************************************************************/
/*!
 *  \brief  Set the instance name of the device for NVM simulation.
 *
 *  \param  str        Instance string.
 *
 *  \note   This function is used to create unique files for storing NVM data on the win32 host.
 *          tester.  This function should be called from host test scripts that use NVM.
 */
/*************************************************************************************************/
void WsfNvmSetInstanceStr(char *str)
{
  struct stat st = {0};
  char path[32];

  strncpy(wsfNvmInstance, str, WSF_NVM_INSTANCE_LEN);

  /* Ensure directory exists. */
  if (stat("nvm", &st) == -1)
  {
    _mkdir("nvm");
  }

  sprintf(path, "nvm\\%s", wsfNvmInstance);

  if (stat(path, &st) == -1)
  {
    _mkdir(path);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Called to enable/disable NVM simulation on the host tester.
 *
 *  \param  enabled    Enabled boolean.
 */
/*************************************************************************************************/
void WsfNvmEnableSimulation(bool_t enabled)
{
  wsfNvmEnabled = enabled;
}

/*************************************************************************************************/
/*!
 *  \brief  Read data.
 *
 *  \param  id         Read ID.
 *  \param  pData      Buffer to read to.
 *  \param  len        Data length to read.
 *  \param  compCback  Read callback.
 *
 *  \return TRUE if NVM operation is successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t WsfNvmReadData(uint64_t id, uint8_t *pData, uint16_t len, WsfNvmCompEvent_t compCback)
{
  if (wsfNvmEnabled)
  {
    FILE *pFile;
    char filename[32];

    sprintf(filename, "nvm\\%s\\nvm_%d.dat", wsfNvmInstance, (uint32_t) id);
    pFile = fopen(filename, "rb");

    if (pFile)
    {
      len = (uint16_t) fread(pData, 1, len, pFile);
      fclose(pFile);
    }
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Write data.
 *
 *  \param  id         Write ID.
 *  \param  pData      Buffer to write.
 *  \param  len        Data length to write.
 *  \param  compCback  Write callback.
 *
 *  \return TRUE if NVM operation is successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t WsfNvmWriteData(uint64_t id, const uint8_t *pData, uint16_t len, WsfNvmCompEvent_t compCback)
{
  if (wsfNvmEnabled)
  {
    FILE *pFile;
    char filename[32];

    sprintf(filename, "nvm\\%s\\nvm_%d.dat", wsfNvmInstance, (uint32_t) id);
    pFile = fopen(filename, "wb");

    if (pFile)
    {
      len = (uint16_t) fwrite(pData, 1, len, pFile);
      fclose(pFile);
    }
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Erase data.
 *
 *  \param  id         Erase ID.
 *  \param  compCback  Write callback.
 *
 *  \return TRUE if NVM operation is successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t WsfNvmEraseData(uint64_t id, WsfNvmCompEvent_t compCback)
{
  if (wsfNvmEnabled)
  {
    char filename[32];
    sprintf(filename, "nvm\\%s\\nvm_%d.dat", wsfNvmInstance, (uint32_t) id);
    remove("fileName");
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Erase all data existing in NVM storage.
 *
 *  \param  compCback          Erase callback.
 *
 *  \note   Security Risk Warning. NVM storage could be shared by multiple Apps.
 */
/*************************************************************************************************/
void WsfNvmEraseDataAll(WsfNvmCompEvent_t compCback)
{
}
