/*************************************************************************************************/
/*!
 *  \file   mesh_network_cache.c
 *
 *  \brief  Network Cache implementation.
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
#include "wsf_assert.h"

#include "mesh_defs.h"

#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_error_codes.h"
#include "mesh_seq_manager.h"
#include "mesh_utils.h"
#include "mesh_adv_bearer.h"
#include "mesh_gatt_bearer.h"
#include "mesh_bearer.h"
#include "mesh_network.h"
#include "mesh_network_if.h"
#include "mesh_network_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Network Message Cache Level 1 entry type */
typedef uint32_t meshNwkCacheL1Entry_t;

/*! Network Message Cache Level 1 type*/
typedef struct meshNwkCacheL1_tag
{
  uint16_t               head;        /*!< Message cache head index */
  uint16_t               tail;        /*!< Message cache tail index */
  meshNwkCacheL1Entry_t  *pMsgCache;  /*!< Message cache memory pointer */
  bool_t                 l1IsFull;    /*!< Mesh Network Message Cache Level 1 is full */
} meshNwkCacheL1_t;

/*! Network Message Cache Level 2 entry type */
typedef struct meshNwkCacheL2Entry_tag
{
  meshSeqNumber_t seqNo;    /*!< Sequence number */
  uint16_t        srcAddr;  /*!< SRC address */
} meshNwkCacheL2Entry_t;

/*! Network Message Cache Level 2 type*/
typedef struct meshNwkCacheL2_tag
{
  uint16_t               head;        /*!< Message cache head index */
  uint16_t               tail;        /*!< Message cache tail index */
  meshNwkCacheL2Entry_t  *pMsgCache;  /*!< Message cache memory pointer */
  bool_t                 l2IsFull;    /*!< Mesh Network Message Cache Level 2 is full */
} meshNwkCacheL2_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Network Message Cache control block */
static struct meshNwkCacheCb_tag
{
  meshNwkCacheL1_t  l1;        /*!< Mesh Network Message Cache Level 1 */
  meshNwkCacheL2_t  l2;        /*!< Mesh Network Message Cache Level 2 */
  uint8_t           l1Size;    /*!< Mesh Network Message Cache Level 1 maximum size */
  uint8_t           l2Size;    /*!< Mesh Network Message Cache Level 2 maximum size */
} meshNwkCacheCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Checks if the Network PDU is already in the Network Message Cache and adds it if not.
 *
 *  \param[in] cacheType  Network Cache type in which to add the PDU.
 *  \param[in] pNwkPdu    Pointer to the Network PDU received from bearer.
 *  \param[in] pduLen     Size of the PDU.
 *
 *  \retval    MESH_SUCCESS                  If the given PDU was successfully added to the Cache.
 *  \retval    MESH_NWK_CACHE_ALREADY_EXISTS If given PDU is present in the Network Message Cache.
 */
/*************************************************************************************************/
static meshNwkCacheRetVal_t meshNwkCacheAddToFifo(meshNwkCacheType_t cacheType,
                                                  const uint8_t *pNwkPdu, uint8_t pduLen)
{
  /* Temporary entries used for computations. */
  meshNwkCacheL1Entry_t cacheEntryL1;
  meshNwkCacheL2Entry_t cacheEntryL2;
  uint16_t index;
  /* Mesh Network Cache queue empty flag.  */
  bool_t isCacheNotEmpty = FALSE;

  /* Select message cache type. */
  switch (cacheType)
  {
  case MESH_NWK_CACHE_L1:
    index = meshNwkCacheCb.l1.tail;

    /* Cache the last 4 bytes of the NetMIC for tracking. */
    cacheEntryL1 = ((uint32_t)pNwkPdu[pduLen - 4] << 24) |
                   ((uint32_t)pNwkPdu[pduLen - 3] << 16) |
                   ((uint32_t)pNwkPdu[pduLen - 2] << 8) |
                   ((uint32_t)pNwkPdu[pduLen - 1]);

    /* Signal message cache not empty. */
    isCacheNotEmpty = (meshNwkCacheCb.l1.head !=
                       meshNwkCacheCb.l1.tail);

    if (meshNwkCacheCb.l1.l1IsFull)
    {
      /* Search from tail to tail. */
      do
      {
        /* Return if the entry was found. */
        if (meshNwkCacheCb.l1.pMsgCache[index] == cacheEntryL1)
        {
          return MESH_NWK_CACHE_ALREADY_EXISTS;
        }

        /* Increment index. The index rollover when the maximum depth was reached. */
        index = (index + 1) % meshNwkCacheCb.l1Size;
      } while (index != meshNwkCacheCb.l1.tail);
    }
    else
    {
      /* Search from tail to head. */
      while (index != meshNwkCacheCb.l1.head)
      {
        /* Return if the entry was found. */
        if (meshNwkCacheCb.l1.pMsgCache[index] == cacheEntryL1)
        {
          return MESH_NWK_CACHE_ALREADY_EXISTS;
        }

        /* Increment index. The index rollover when the maximum depth was reached. */
        index = (index + 1) % meshNwkCacheCb.l1Size;
      }
    }

    /* A new Network PDU has been received - add it to the queue. */
    meshNwkCacheCb.l1.pMsgCache[meshNwkCacheCb.l1.head++] = cacheEntryL1;

    if (meshNwkCacheCb.l1.head == meshNwkCacheCb.l1Size)
    {
      /* Rollover message cache head index. */
      meshNwkCacheCb.l1.head = 0;

      /* Signal Cache Full. */
      meshNwkCacheCb.l1.l1IsFull = TRUE;
    }

    if (isCacheNotEmpty && (meshNwkCacheCb.l1.head == meshNwkCacheCb.l1.tail))
    {
      meshNwkCacheCb.l1.tail += 1;

      if (meshNwkCacheCb.l1.tail == meshNwkCacheCb.l1Size)
      {
        /* Rollover message cache tail index. */
        meshNwkCacheCb.l1.tail = 0;
      }
    }
    break;

  case MESH_NWK_CACHE_L2:
    index = meshNwkCacheCb.l2.tail;

    /* Cache the Source Address, Destination Addres and Sequence Number for tracking. */
    cacheEntryL2.seqNo = ((uint32_t)pNwkPdu[2] << 16) |
                         ((uint32_t)pNwkPdu[3] << 8) |
                         ((uint32_t)pNwkPdu[4]);

    cacheEntryL2.srcAddr = ((uint16_t)pNwkPdu[5] << 8) | ((uint16_t)pNwkPdu[6]);

    isCacheNotEmpty = (meshNwkCacheCb.l2.head != meshNwkCacheCb.l2.tail);

    if (meshNwkCacheCb.l2.l2IsFull)
    {
      /* Search from tail to tail. */
      do
      {
        /* Return if the entry was found. */
        if ((meshNwkCacheCb.l2.pMsgCache[index].srcAddr == cacheEntryL2.srcAddr) &&
            (meshNwkCacheCb.l2.pMsgCache[index].seqNo == cacheEntryL2.seqNo))
        {
          return MESH_NWK_CACHE_ALREADY_EXISTS;
        }

        /* Increment index. The index rollover when the maximum depth was reached. */
        index = (index + 1) % meshNwkCacheCb.l2Size;
      } while (index != meshNwkCacheCb.l2.tail);
    }
    else
    {
      /* Search from tail to head. */
      while (index != meshNwkCacheCb.l2.head)
      {
        /* Return if the entry was found. */
        if ((meshNwkCacheCb.l2.pMsgCache[index].srcAddr == cacheEntryL2.srcAddr) &&
            (meshNwkCacheCb.l2.pMsgCache[index].seqNo == cacheEntryL2.seqNo))
        {
          return MESH_NWK_CACHE_ALREADY_EXISTS;
        }

        /* Increment index. The index rollover when the maximum depth was reached. */
        index = (index + 1) % meshNwkCacheCb.l2Size;
      }
    }

    /* A new Network PDU has been received - add it to the queue. */
    meshNwkCacheCb.l2.pMsgCache[meshNwkCacheCb.l2.head].srcAddr  = cacheEntryL2.srcAddr;
    meshNwkCacheCb.l2.pMsgCache[meshNwkCacheCb.l2.head++].seqNo = cacheEntryL2.seqNo;

    if (meshNwkCacheCb.l2.head == meshNwkCacheCb.l2Size)
    {
      /* Rollover message cache head index. */
      meshNwkCacheCb.l2.head = 0;

      /* Signal Cache Full. */
      meshNwkCacheCb.l2.l2IsFull = TRUE;
    }

    if (isCacheNotEmpty && (meshNwkCacheCb.l2.head == meshNwkCacheCb.l2.tail))
    {
      meshNwkCacheCb.l2.tail += 1;

      if (meshNwkCacheCb.l2.tail == meshNwkCacheCb.l2Size)
      {
        /* Rollover message cache tail index. */
        meshNwkCacheCb.l2.tail = 0;
      }
    }
    break;
  default:
    return MESH_NWK_CACHE_INVALID_PARAM;
  }

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured size of Network Cache Level 1.
 *
 *  \param[in] meshNwkCacheL1Size  Network Cache Level 1 size.
 *
 *  \return    Required memory in bytes for Network Cache Level 1.
 */
/*************************************************************************************************/
static inline uint16_t meshNwkCacheGetRequiredMemoryL1(uint8_t meshNwkCacheL1Size)
{
  /* Compute required memory size for L1 cache. */
  return  MESH_UTILS_ALIGN(sizeof(meshNwkCacheL1Entry_t) * meshNwkCacheL1Size);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured size of Network Cache Level 2.
 *
 *  \param[in] meshNwkCacheL2Size  Network Cache Level 2 size.
 *
 *  \return    Required memory in bytes for Network Cache Level 2.
 */
/*************************************************************************************************/
static inline uint16_t meshNwkCacheGetRequiredMemoryL2(uint8_t meshNwkCacheL2Size)
{
  /* Compute required memory size for L2 cache. */
  return  MESH_UTILS_ALIGN(sizeof(meshNwkCacheL2Entry_t) * meshNwkCacheL2Size);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory for Nwk Cache to be provided based on the given
 *          configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t meshNwkCacheGetRequiredMemory(void)
{
  uint32_t reqMem;

  /* Compute the required memory. */
  reqMem = meshNwkCacheGetRequiredMemoryL1(pMeshConfig->pMemoryConfig->nwkCacheL1Size) +
           meshNwkCacheGetRequiredMemoryL2(pMeshConfig->pMemoryConfig->nwkCacheL2Size);

  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Network Message Caches and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void meshNwkCacheInit(void)
{
  uint8_t *pMemBuff;
  uint16_t reqMemL1;
  uint16_t reqMemL2;

  pMemBuff = meshCb.pMemBuff;

  /* Save the pointer for L1 cache. */
  meshNwkCacheCb.l1.pMsgCache = (meshNwkCacheL1Entry_t *)pMemBuff;

  /* Increment the memory buffer pointer. */
  reqMemL1 = meshNwkCacheGetRequiredMemoryL1(pMeshConfig->pMemoryConfig->nwkCacheL1Size);
  pMemBuff += reqMemL1;

  /* Save the pointer for L2 cache. */
  meshNwkCacheCb.l2.pMsgCache = (meshNwkCacheL2Entry_t *)pMemBuff;

  /* Increment the memory buffer pointer. */
  reqMemL2 = meshNwkCacheGetRequiredMemoryL2(pMeshConfig->pMemoryConfig->nwkCacheL2Size);
  pMemBuff += reqMemL2;

  /* Save the updated address. */
  meshCb.pMemBuff = pMemBuff;

  /* Subtract the reserved size from memory buffer size. */
  meshCb.memBuffSize -= (reqMemL1 + reqMemL2);

  /* Set L1 cache size. */
  meshNwkCacheCb.l1Size = pMeshConfig->pMemoryConfig->nwkCacheL1Size;

  /* Set L2 cache size. */
  meshNwkCacheCb.l2Size = pMeshConfig->pMemoryConfig->nwkCacheL2Size;

  /* Clear all message caches. */
  meshNwkCacheClear();
}

/*************************************************************************************************/
/*!
 *  \brief     Checks if the Network PDU is already in the Network Message Cache and adds it if not.
 *
 *  \param[in] cacheType  Network Cache type in which to add the PDU.
 *  \param[in] pNwkPdu    Pointer to the Network PDU received from bearer.
 *  \param[in] pduLen     Size of the PDU.
 *
 *  \retval    MESH_SUCCESS                  If the given PDU was successfully added to the Cache.
 *  \retval    MESH_NWK_CACHE_ALREADY_EXISTS If given PDU is present in the Network Message Cache.
 *  \retval    MESH_NWK_CACHE_INVALID_PARAM  If one or more parameters are invalid.
 */
/*************************************************************************************************/
meshNwkCacheRetVal_t meshNwkCacheAdd(meshNwkCacheType_t cacheType, const uint8_t *pNwkPdu,
                                     uint8_t pduLen)
{
  /* Check for invalid parameters. */
  if ((pNwkPdu == NULL) || (pduLen > MESH_NWK_MAX_PDU_LEN) || (pduLen < MESH_NWK_HEADER_LEN))
  {
    return MESH_NWK_CACHE_INVALID_PARAM;
  }

  return meshNwkCacheAddToFifo(cacheType, pNwkPdu, pduLen);
}

/*************************************************************************************************/
/*!
 *  \brief  Clears the Network Message Caches.
 *
 *  \return None.
 */
/*************************************************************************************************/
void meshNwkCacheClear(void)
{
  /* Clear Level 1 cache. */
  memset(meshNwkCacheCb.l1.pMsgCache, 0, (sizeof(meshNwkCacheL1Entry_t) * meshNwkCacheCb.l1Size));

  /* Clear Level 2 cache. */
  memset(meshNwkCacheCb.l2.pMsgCache, 0, (sizeof(meshNwkCacheL2Entry_t) * meshNwkCacheCb.l2Size));

  /* Reset L1 cache internals. */
  meshNwkCacheCb.l1.head = 0;
  meshNwkCacheCb.l1.tail = 0;
  meshNwkCacheCb.l1.l1IsFull = FALSE;

  /* Reset L1 cache internals. */
  meshNwkCacheCb.l2.head = 0;
  meshNwkCacheCb.l2.tail = 0;
  meshNwkCacheCb.l2.l2IsFull = FALSE;
}
