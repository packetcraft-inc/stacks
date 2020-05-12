/*************************************************************************************************/
/*!
 *  \file   mesh_security_main.c
 *
 *  \brief  Security main implementation.
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
 *
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"

#include "mesh_defs.h"
#include "mesh_network_beacon_defs.h"
#include "mesh_security_defs.h"

#include "mesh_types.h"
#include "mesh_error_codes.h"

#include "mesh_utils.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_seq_manager.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_security_toolbox.h"
#include "mesh_security.h"
#include "mesh_security_main.h"
#include "mesh_security_deriv.h"
#include "mesh_security_crypto.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Storage for security material. */
meshSecMaterial_t secMatLocals =
{
  .appKeyInfoListSize = 0,
  .netKeyInfoListSize = 0,
  .friendMatListSize = 0,
};

/*! Security control block */
meshSecCb_t meshSecCb = { NULL };

/*! Request sources for crypto operations. */
meshSecCryptoRequests_t   secCryptoReq;

/*! Request sources for key derivation operations. */
meshSecKeyDerivRequests_t secKeyDerivReq;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured number of Application Keys.
 *
 *  \param[in] numAppKeys  Maximum number of Application Keys.
 *
 *  \return    Required memory in bytes for Application Keys material and information.
 */
/*************************************************************************************************/
static inline uint32_t meshSecGetAppKeyMatRequiredMemory(uint16_t numAppKeys)
{
  return MESH_UTILS_ALIGN(numAppKeys * sizeof(meshSecAppKeyInfo_t));
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured number of Network Keys.
 *
 *  \param[in] numNetKeys  Maximum number of Network Keys.
 *
 *  \return    Required memory in bytes for Network Keys material and information.
 */
/*************************************************************************************************/
static inline uint32_t meshSecGetNetKeyMatRequiredMemory(uint16_t numNetKeys)
{
  return MESH_UTILS_ALIGN(numNetKeys * sizeof(meshSecNetKeyInfo_t));
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured number of friendships.
 *
 *  \param[in] numFriendships  Maximum number of friendships.
 *
 *  \return    Required memory in bytes for friendship credentials material and information.
 */
/*************************************************************************************************/
static inline uint32_t meshSecGetFriendMatRequiredMemory(uint16_t numFriendships)
{
  return MESH_UTILS_ALIGN(numFriendships * sizeof(meshSecFriendMat_t));
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the global configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of error.
 */
/*************************************************************************************************/
uint32_t MeshSecGetRequiredMemory(void)
{
  uint32_t reqMem = MESH_MEM_REQ_INVALID_CFG;

  if (pMeshConfig->pMemoryConfig->netKeyListSize == 0)
  {
    return reqMem;
  }

  /* Get memory required by AppKey, NetKey and Friendship material. */
  uint32_t totalMem =
    meshSecGetAppKeyMatRequiredMemory(pMeshConfig->pMemoryConfig->appKeyListSize) +
    meshSecGetNetKeyMatRequiredMemory(pMeshConfig->pMemoryConfig->netKeyListSize) +
    meshSecGetFriendMatRequiredMemory(pMeshConfig->pMemoryConfig->maxNumFriendships);

  return totalMem;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Security module and allocates configuration memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSecInit(void)
{
  uint32_t memReq = 0;
  uint16_t idx;

  memReq = MeshSecGetRequiredMemory();

  /* Set number of AppKeys. */
  secMatLocals.appKeyInfoListSize = pMeshConfig->pMemoryConfig->appKeyListSize;
  /* Set number of NetKeys. */
  secMatLocals.netKeyInfoListSize = pMeshConfig->pMemoryConfig->netKeyListSize;
  /* Set number of friendships. */
  secMatLocals.friendMatListSize  = pMeshConfig->pMemoryConfig->maxNumFriendships;

  /* Set start of memory for AppKey material. */
  secMatLocals.pAppKeyInfoArray = (meshSecAppKeyInfo_t *)(meshCb.pMemBuff);
  /* Forward pointer. */
  meshCb.pMemBuff += meshSecGetAppKeyMatRequiredMemory(secMatLocals.appKeyInfoListSize);

  /* Set start of memory for the NetKey material. */
  secMatLocals.pNetKeyInfoArray = (meshSecNetKeyInfo_t *)(meshCb.pMemBuff);
  /* Forward pointer. */
  meshCb.pMemBuff += meshSecGetNetKeyMatRequiredMemory(secMatLocals.netKeyInfoListSize);

  /* Set start of memory for the Friendship material. */
  secMatLocals.pFriendMatArray = (meshSecFriendMat_t *)(meshCb.pMemBuff);
  /* Forward pointer. */
  meshCb.pMemBuff += meshSecGetFriendMatRequiredMemory(secMatLocals.friendMatListSize);

  /* Subtract used memory. */
  meshCb.memBuffSize -= memReq;

  /* Reset Network Key derivation material. */
  memset(secMatLocals.pNetKeyInfoArray, 0, secMatLocals.netKeyInfoListSize * sizeof(meshSecNetKeyInfo_t));

  /* Reset Application Key derivation material. */
  memset(secMatLocals.pAppKeyInfoArray, 0, secMatLocals.appKeyInfoListSize * sizeof(meshSecAppKeyInfo_t));

  /* Reset Friendship material. */
  for (idx = 0; idx < secMatLocals.friendMatListSize; idx++)
  {
    secMatLocals.pFriendMatArray[idx].netKeyInfoIndex = MESH_SEC_INVALID_ENTRY_INDEX;
    secMatLocals.pFriendMatArray[idx].hasUpdtMaterial = FALSE;
  }

  /* Reset key derivation requests. */
  secKeyDerivReq.friendMatDerivReq.friendListIdx = MESH_SEC_INVALID_ENTRY_INDEX;
  secKeyDerivReq.netKeyDerivReq.netKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;
  secKeyDerivReq.appKeyDerivReq.appKeyListIdx = MESH_SEC_INVALID_ENTRY_INDEX;

  /* Reset Upper Transport security requests. */
  secCryptoReq.utrEncReq.cback = NULL;
  secCryptoReq.utrDecReq.cback = NULL;

  /* Reset Network security requests. */
  secCryptoReq.nwkEncObfReq[MESH_SEC_NWK_ENC_SRC_NWK].cback    = NULL;
  secCryptoReq.nwkEncObfReq[MESH_SEC_NWK_ENC_SRC_PROXY].cback  = NULL;
  secCryptoReq.nwkEncObfReq[MESH_SEC_NWK_ENC_SRC_FRIEND].cback = NULL;

  secCryptoReq.nwkDeobfDecReq[MESH_SEC_NWK_DEC_SRC_NWK_FRIEND].cback = NULL;
  secCryptoReq.nwkDeobfDecReq[MESH_SEC_NWK_DEC_SRC_PROXY].cback      = NULL;

  /* Reset Beacon Authentication requests. */
  secCryptoReq.beaconAuthReq.cback     = NULL;
  secCryptoReq.beaconCompAuthReq.cback = NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the reader function for remote Device Keys.
 *
 *  \param[in] devKeyReader  Function (callback) called when Security needs to read a remote node's
 *                           Device Key.
 *
 *  \return    None.
 *
 *  \note      This function should be called only when an instance of Configuration Client is
 *             present on the local node.
 */
/*************************************************************************************************/
void MeshSecRegisterRemoteDevKeyReader(meshSecRemoteDevKeyReadCback_t devKeyReader)
{
  /* Store the callback provided by the caller for reading remote Device Keys. */
  meshSecCb.secRemoteDevKeyReader = devKeyReader;
}

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
/*************************************************************************************************/
/*!
 *  \brief     Alters the NetKey list size in Security for Mesh Test.
 *
 *  \param[in] listSize  NetKey list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestSecAlterNetKeyListSize(uint16_t listSize)
{
  secMatLocals.netKeyInfoListSize = listSize;
}
#endif
