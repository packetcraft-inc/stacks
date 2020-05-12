/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Stack initialization for Provisioner.
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

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_handler.h"
#include "mesh_prv.h"
#include "mesh_prv_cl_api.h"

#include "mmdl_types.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "provisioner_api.h"
#include "provisioner_mmdl_handler.h"
#include "provisioner_terminal.h"
#include "app_mesh_terminal.h"
#include "app_mesh_cfg_mdl_cl_terminal.h"

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
#include "pal_uart.h"

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/
void StackInitProvisioner(void);

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
void StackInitProvisioner(void)
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

  DmConnInit();
#if (LL_VER >= LL_VER_BT_CORE_SPEC_5_0)
  DmExtConnMasterInit();
  DmExtConnSlaveInit();
#else
  DmConnMasterInit();
  DmConnSlaveInit();
#endif

  DmSecInit();
  DmSecLescInit();
  DmPrivInit();
  DmHandlerInit(handlerId);

  handlerId = WsfOsSetNextHandler(L2cSlaveHandler);
  L2cSlaveHandlerInit(handlerId);
  L2cInit();
  L2cMasterInit();
  L2cSlaveInit();

  handlerId = WsfOsSetNextHandler(AttHandler);
  AttHandlerInit(handlerId);
  AttsInit();
  AttsIndInit();
  AttcInit();

  handlerId = WsfOsSetNextHandler(SmpHandler);
  SmpHandlerInit(handlerId);
  SmpiInit();
  SmprInit();
  SmpiScInit();
  SmprScInit();
  HciSetMaxRxAclLen(100);

  /* Initialize Mesh handlers. */
  handlerId = WsfOsSetNextHandler(MeshHandler);
  MeshHandlerInit(handlerId);

  /* Initialize Mesh Security handler. */
  handlerId = WsfOsSetNextHandler(MeshSecurityHandler);
  MeshSecurityHandlerInit(handlerId);

  /* Initialize Mesh Provisioning Server handler. */
  handlerId = WsfOsSetNextHandler(MeshPrvClHandler);
  MeshPrvClHandlerInit(handlerId);

  /* Initialize Mesh Models handler. */
  handlerId = WsfOsSetNextHandler(ProvisionerMmdlHandler);

  /* Initialize Health Server model handler. */
  MeshHtSrHandlerInit(handlerId);

  /* Initialize Generic OnOff Client model handler. */
  MmdlGenOnOffClHandlerInit(handlerId);

  /* Initialize Light HSL Client model handler. */
  MmdlLightHslClHandlerInit(handlerId);

  /* Initialize application handler. */
  handlerId = WsfOsSetNextHandler(ProvisionerHandler);
  ProvisionerHandlerInit(handlerId);
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize configuration for stack.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void StackInitCfgProvisioner(void)
{
  ProvisionerConfigInit();
}
