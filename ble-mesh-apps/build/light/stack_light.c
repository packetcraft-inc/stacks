/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Stack initialization for Light.
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
#include "mmdl_defs.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_bindings_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_scene_sr_api.h"

#include "light_api.h"
#include "light_mmdl_handler.h"

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
void StackInitLight(void);

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
void StackInitLight(void)
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

  /* Initialize Mesh handler. */
  handlerId = WsfOsSetNextHandler(MeshHandler);
  MeshHandlerInit(handlerId);

  /* Initialize Mesh Security handler. */
  handlerId = WsfOsSetNextHandler(MeshSecurityHandler);
  MeshSecurityHandlerInit(handlerId);

  /* Initialize Mesh Provisioning Server handler. */
  handlerId = WsfOsSetNextHandler(MeshPrvSrHandler);
  MeshPrvSrHandlerInit(handlerId);

  /* Initialize Mesh Models handler. */
  handlerId = WsfOsSetNextHandler(LightMmdlHandler);

  /* Initialize Health Server model handler. */
  MeshHtSrHandlerInit(handlerId);

  /* Initialize Generic On Off Server model handler. */
  MmdlGenOnOffSrHandlerInit(handlerId);

  /* Initialize Generic Power On Off Server model handler. */
  MmdlGenPowOnOffSrHandlerInit(handlerId);
  MmdlGenPowOnOffSetupSrHandlerInit(handlerId);

  /* Initialize Generic Level Server model handler. */
  MmdlGenLevelSrHandlerInit(handlerId);

  /* Initialize Scene Server model handler. */
  MmdlSceneSrHandlerInit(handlerId);

  /* Initialize Generic Default Transition Server model handler. */
  MmdlGenDefaultTransSrHandlerInit(handlerId);

  /* Initialize Light Lightness Server model handler. */
  MmdlLightLightnessSrHandlerInit(handlerId);
  MmdlLightLightnessSetupSrHandlerInit(handlerId);

  /* Initialize Light HSL Client and Server model handler. */
  MmdlLightHslSrHandlerInit(handlerId);
  MmdlLightHslHueSrHandlerInit(handlerId);
  MmdlLightHslSatSrHandlerInit(handlerId);

  /* Initialize application handler. */
  handlerId = WsfOsSetNextHandler(LightHandler);
  LightHandlerInit(handlerId);
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize configuration for stack.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void StackInitCfgLight(void)
{
  LightConfigInit();
}
