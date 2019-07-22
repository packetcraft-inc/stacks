/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  LL initialization for controller configuration.
 *
 *  Copyright (c) 2013-2018 Arm Ltd.
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

#include "mac_154_int.h"
#include "chci_154_int.h"
#include "bb_154_int.h"
#include "chci_api.h"
#include "sch_api.h"

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize 802.15.4.
 *
 *  \param      initHandler   Initialize handler if TRUE
 *
 *  \return     TRUE if 802.15.4 initialized.
 */
/*************************************************************************************************/
bool_t Mac154Init(bool_t initHandler)
{
  /**** MAC154 Baseband ****/
  Bb154Init();
  Bb154TestInit();
  Bb154AssocInit();
  Bb154DataInit();

  /**** MAC154 main ****/
  Mac154TestInit();
  Mac154ScanInit();
  Mac154DataInit();
  if (initHandler)
  {
    Mac154InitPIB();
    Mac154HandlerInit();
  }

  /**** MAC154 HCI ****/
  Chci154TestInit();
  //Chci154VsInit();
  Chci154DataInit();
  Chci154AssocInit();
  Chci154MiscInit();
  Chci154ScanInit();
  Chci154PibInit();
  /* Chci154DiagInit(); */ /* Not currently used */
  if (initHandler)
  {
    Chci154HandlerInit();
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      De-initialize 802.15.4.
 *
 *  \param      None.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154DeInit(void)
{
  Bb154DataDeInit();
}
