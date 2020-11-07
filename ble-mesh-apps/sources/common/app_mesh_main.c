/*************************************************************************************************/
/*!
 *  \file   app_mesh_main.c
 *
 *  \brief  Mesh application framework main module.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
 *
 *  $$LICENSE$$
 */
/*************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_timer.h"
#include "wsf_nvm.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"

#include "app_mesh_api.h"
#include "mesh_replay_protection.h"
#include "mesh_local_config.h"

#include "pal_sys.h"
#include "pal_cfg.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh App control block structure */
typedef struct appMeshCb_tag
{
  const char    *pAppVersion;                       /*!< Pointer to application version string */
  uint8_t       meshNvmInstanceId;                  /*!< Mesh Stack NVM instance. */
  uint8_t       mmdlNvmInstanceId;                  /*!< Mesh Models NVM instance. */
} appMeshCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Configuration pointer for Provisioning Server */
meshPrvSrCfg_t *pMeshPrvSrCfg;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh application control block */
static appMeshCb_t appMeshCb;

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize common Mesh Application functionality for a Mesh Node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshNodeInit(void)
{
  /* Load Device UUID. */
  PalCfgLoadData(PAL_CFG_ID_UUID, pMeshPrvSrCfg->devUuid, MESH_PRV_DEVICE_UUID_SIZE);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the application version.
 *
 *  \param[in] pVersion  Pointer to version number string.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppMeshSetVersion(const char *pVersion)
{
  appMeshCb.pAppVersion = pVersion;
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the application version.
 *
 *  \return Pointer to version number string.
 */
/*************************************************************************************************/
const char *AppMeshGetVersion(void)
{
  return appMeshCb.pAppVersion;
}

/*************************************************************************************************/
/*!
 *  \brief  Clears the NVM for Mesh Stack and models.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshClearNvm(void)
{
  MeshLocalCfgEraseNvm();
  MeshRpNvmErase();
}

/*************************************************************************************************/
/*!
 *  \brief  Initiate software system reset.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppMeshReset(void)
{
    /* Stub */
}
