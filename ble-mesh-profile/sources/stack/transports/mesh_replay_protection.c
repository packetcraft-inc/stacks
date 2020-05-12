/*************************************************************************************************/
/*!
 *  \file   mesh_replay_protection.c
 *
 *  \brief  Message replay protection feature implementation.
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
 *
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_assert.h"
#include "wsf_nvm.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_utils.h"
#include "mesh_local_config.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_replay_protection.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Replay Protection List entry type */
typedef struct meshRpListEntry_tag
{
  meshSeqNumber_t seqNo;    /*!< Sequence number */
  uint32_t        ivIndex;  /*!< IV index */
  meshAddress_t   srcAddr;  /*!< SRC address */
} meshRpListEntry_t;

/*! Replay Protection List type */
typedef struct meshRpList_tag
{
  meshRpListEntry_t     *pRpl;        /*!< Mesh Replay Protection List memory pointer */
} meshRpList_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Replay Protection List control block */
static meshRpList_t meshRplCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured size of Replay Protection List.
 *
 *  \param[in] meshRpListSize  Replay Protection List.
 *
 *  \return    Required memory in bytes for Replay Protection List.
 */
/*************************************************************************************************/
static inline uint32_t meshRpGetRequiredMemory(uint16_t meshRpListSize)
{
  /* Compute required memory size for Replay Protection List. */
  return  MESH_UTILS_ALIGN(sizeof(meshRpListEntry_t) * meshRpListSize);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Replay Protection List and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshRpInit(void)
{
  uint32_t reqMemRpl;
  bool_t retVal;

  /* Save the pointer for RPL. */
  meshRplCb.pRpl = (meshRpListEntry_t *)meshCb.pMemBuff;

  /* Increment the memory buffer pointer. */
  reqMemRpl = meshRpGetRequiredMemory(pMeshConfig->pMemoryConfig->rpListSize);

  meshCb.pMemBuff += reqMemRpl;
  meshCb.memBuffSize -= reqMemRpl;

  /* Initialize list. */
  memset(meshRplCb.pRpl, 0, sizeof(meshRpListEntry_t) * pMeshConfig->pMemoryConfig->rpListSize);

  retVal = WsfNvmReadData((uint64_t)MESH_RP_NVM_LIST_DATASET_ID, (uint8_t *)meshRplCb.pRpl,
                             sizeof(meshRpListEntry_t) * pMeshConfig->pMemoryConfig->rpListSize, NULL);

  /* Suppress compiler warnings. */
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshRpGetRequiredMemory(void)
{
  uint32_t reqMem = MESH_MEM_REQ_INVALID_CFG;
  if ((pMeshConfig->pMemoryConfig == NULL) ||
      (pMeshConfig->pMemoryConfig->rpListSize < MESH_RP_MIN_LIST_SIZE))
  {
    return reqMem;
  }

  /* Compute the required memory. */
  reqMem = meshRpGetRequiredMemory(pMeshConfig->pMemoryConfig->rpListSize);

  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief     Verifies a PDU for replay attacks.
 *
 *  \param[in] srcAddr  Address of originating element.
 *  \param[in] seqNo    Sequence number used to identify the PDU.
 *  \param[in] ivIndex  The IV used with the PDU.
 *
 *  \return    TRUE if PDU is replay attack, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshRpIsReplayAttack(meshAddress_t srcAddr, meshSeqNumber_t seqNo, uint32_t ivIndex)
{
  uint16_t index = 0;

  /* Check for invalid parameters. */
  WSF_ASSERT((MESH_IS_ADDR_UNICAST(srcAddr)) && (MESH_SEQ_IS_VALID(seqNo)));

  /* Check for invalid replay protection list. */
  WSF_ASSERT(meshRplCb.pRpl != NULL);

  /* Parse the Replay Protection List. */
  while (!MESH_IS_ADDR_UNASSIGNED(meshRplCb.pRpl[index].srcAddr) &&
         (index < pMeshConfig->pMemoryConfig->rpListSize))
  {
    /* Search for an existing reference to the element in the RPL. */
    if (meshRplCb.pRpl[index].srcAddr == srcAddr)
    {
      if (ivIndex < meshRplCb.pRpl[index].ivIndex)
      {
        return TRUE;
      }

      if (ivIndex == meshRplCb.pRpl[index].ivIndex)
      {
        if (seqNo <= meshRplCb.pRpl[index].seqNo)
        {
          return TRUE;
        }

        return FALSE;
      }

      return FALSE;
    }

    index++;
  }

  /* There is no entry yet for this element in the RPL and the list is not full. */
  if (index < pMeshConfig->pMemoryConfig->rpListSize)
  {
    return FALSE;
  }

  /* There is no entry yet for this element in the RPL and the list is full.
   * The node does not have enough resources to perform replay protection for this source address
   * and the message shall be discarded.
   */
  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Updates the Replay Protection List for a given element with a specific sequence number.
 *
 *  \param[in] srcAddr  Address of originating element.
 *  \param[in] seqNo    Sequence number used to identify the PDU.
 *  \param[in] ivIndex  The IV used with the PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRpUpdateList(meshAddress_t srcAddr, meshSeqNumber_t seqNo, uint32_t ivIndex)
{
  uint16_t index = 0;

  /* Check for invalid parameters. */
  WSF_ASSERT((MESH_IS_ADDR_UNICAST(srcAddr)) && (MESH_SEQ_IS_VALID(seqNo)));

  /* Check for invalid replay protection list. */
  WSF_ASSERT(meshRplCb.pRpl != NULL);

  /* Parse the Replay Protection List. */
  while (!MESH_IS_ADDR_UNASSIGNED(meshRplCb.pRpl[index].srcAddr) &&
         (index < pMeshConfig->pMemoryConfig->rpListSize))
  {
    /* Search for the existing reference to the element in the RPL. */
    if (meshRplCb.pRpl[index].srcAddr == srcAddr)
    {
      meshRplCb.pRpl[index].seqNo = seqNo;
      meshRplCb.pRpl[index].ivIndex = ivIndex;

      /* Store entry to NVM. */
      WsfNvmWriteData((uint64_t)MESH_RP_NVM_LIST_DATASET_ID, (uint8_t *)meshRplCb.pRpl,
                                   sizeof(meshRpListEntry_t) * pMeshConfig->pMemoryConfig->rpListSize, NULL);
      return;
    }

    index++;
  }

  /* Cannot get to this point if the RPL is full */
  WSF_ASSERT(index < pMeshConfig->pMemoryConfig->rpListSize);

  /* The element is new - add it Replay Protection List. */
  meshRplCb.pRpl[index].seqNo = seqNo;
  meshRplCb.pRpl[index].srcAddr = srcAddr;
  meshRplCb.pRpl[index].ivIndex = ivIndex;

  /* Store entry to NVM. */
  WsfNvmWriteData((uint64_t)MESH_RP_NVM_LIST_DATASET_ID, (uint8_t *)meshRplCb.pRpl,
                               sizeof(meshRpListEntry_t) * pMeshConfig->pMemoryConfig->rpListSize, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Clears the Replay Protection List.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshRpClearList(void)
{
  /* Check for invalid replay protection list. */
  WSF_ASSERT(meshRplCb.pRpl != NULL);

  /* Clear Replay Protection List. */
  memset(meshRplCb.pRpl, 0, sizeof(meshRpListEntry_t) * pMeshConfig->pMemoryConfig->rpListSize);

  /* Clear NVM entry also. */
  WsfNvmWriteData((uint64_t)MESH_RP_NVM_LIST_DATASET_ID, (uint8_t *)meshRplCb.pRpl,
                               sizeof(meshRpListEntry_t) * pMeshConfig->pMemoryConfig->rpListSize, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Clears the Replay Protection List.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshRpNvmErase(void)
{
  WsfNvmEraseData((uint64_t)MESH_RP_NVM_LIST_DATASET_ID, NULL);
}
