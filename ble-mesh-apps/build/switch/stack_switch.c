/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Stack initialization for Switch.
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

#include "wsf_types.h"
#include "wsf_os.h"
#include "wsf_bufio.h"
#include "wsf_timer.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_handler.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"

#include "mmdl_types.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "switch_api.h"
#include "switch_mmdl_handler.h"
#include "switch_terminal.h"
#include "app_mesh_terminal.h"

#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "hci_core.h"
#include "sec_api.h"
#include "pal_crypto.h"

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/
void StackInitSwitch(void);

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize stack.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void StackInitSwitch(void)
{
  wsfHandlerId_t handlerId;

  SecInit();
  SecAesInit();
  SecAesRevInit();
  SecCmacInit();
  SecEccInit();
  SecCcmInit();

  /* Initialize stack handlers. */
  handlerId = WsfOsSetNextHandler(HciHandler);
  HciHandlerInit(handlerId);

  handlerId = WsfOsSetNextHandler(DmHandler);
  DmDevVsInit(0);

#if (LL_VER >= LL_VER_BT_CORE_SPEC_5_0)
  DmExtScanInit();
  DmExtAdvInit();
#else
  DmScanInit();
  DmAdvInit();
#endif

  DmSecInit();
  DmHandlerInit(handlerId);

  handlerId = WsfOsSetNextHandler(L2cSlaveHandler);
  L2cSlaveHandlerInit(handlerId);
  L2cInit();
  L2cSlaveInit();

  handlerId = WsfOsSetNextHandler(AttHandler);
  AttHandlerInit(handlerId);
  AttsInit();
  AttsIndInit();

  handlerId = WsfOsSetNextHandler(SmpHandler);
  SmpHandlerInit(handlerId);
  SmprInit();
  SmprScInit();
  HciSetMaxRxAclLen(100);

  /* Initialize Mesh handlers. */
  handlerId = WsfOsSetNextHandler(MeshHandler);
  MeshHandlerInit(handlerId);

  /* Initialize Mesh Security handler. */
  handlerId = WsfOsSetNextHandler(MeshSecurityHandler);
  MeshSecurityHandlerInit(handlerId);

  /* Initialize Mesh Provisioning Server handler. */
  handlerId = WsfOsSetNextHandler(MeshPrvSrHandler);
  MeshPrvSrHandlerInit(handlerId);

  /* Initialize Mesh Models handler. */
  handlerId = WsfOsSetNextHandler(SwitchMmdlHandler);

  /* Initialize Health Server model handler. */
  MeshHtSrHandlerInit(handlerId);

  /* Initialize Generic OnOff Client model handler. */
  MmdlGenOnOffClHandlerInit(handlerId);

  /* Initialize Generic Power On Off Client model handler. */
  MmdlGenPowOnOffClHandlerInit(handlerId);

  /* Initialize Generic Level Client model handler. */
  MmdlGenLevelClHandlerInit(handlerId);

  /* Initialize Light Lightness Client model handler. */
  MmdlLightLightnessClHandlerInit(handlerId);

  /* Initialize Light HSL Client model handler. */
  MmdlLightHslClHandlerInit(handlerId);

  /* Initialize application handler. */
  handlerId = WsfOsSetNextHandler(SwitchHandler);
  SwitchHandlerInit(handlerId);
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize configuration for stack.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void StackInitCfgSwitch(void)
{
  SwitchConfigInit();
}
