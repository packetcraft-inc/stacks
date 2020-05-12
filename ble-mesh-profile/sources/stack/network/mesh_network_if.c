/*************************************************************************************************/
/*!
 *  \file   mesh_network_if.c
 *
 *  \brief  Network Cache implementation.
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
#include "wsf_math.h"
#include "wsf_msg.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_utils.h"
#include "mesh_error_codes.h"
#include "mesh_adv_bearer.h"
#include "mesh_gatt_bearer.h"
#include "mesh_bearer.h"
#include "mesh_network.h"
#include "mesh_network_if.h"
#include "mesh_network_main.h"

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Network Interfaces control block */
meshNwkIfCb_t nwkIfCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Resets the Filter Address List for an interface.
 *
 *  \param[in] pNwkIf  Network Interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static inline void meshNwkIfResetFilterAddressList(meshNwkIf_t *pNwkIf)
{
  uint8_t idx;

  /* Set addresses to unassigned. */
  for (idx = 0; idx < nwkIfCb.maxFilterSize; idx++)
  {
    pNwkIf->outputFilter.pAddrList[idx] = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Set filter size to zero. */
  pNwkIf->outputFilter.filterSize = 0;
}

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory for Nwk Cache to be provided based on the given
 *          configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t meshNwkIfGetRequiredMemory(uint8_t filterSize)
{
  /* Compute required memory size for the Network Interface Filter. */
  return MESH_UTILS_ALIGN(sizeof(meshAddress_t) * filterSize * MESH_BR_MAX_INTERFACES);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Network Interfaces and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void meshNwkIfInit(void)
{
  uint8_t ifIdx;
  uint8_t *pMemBuff;
  uint32_t reqMem = meshNwkIfGetRequiredMemory(pMeshConfig->pMemoryConfig->nwkOutputFilterSize);

  pMemBuff = meshCb.pMemBuff;

  /* Reset internal data. Allocate Memory for output filter. */
  for (ifIdx = 0; ifIdx < MESH_BR_MAX_INTERFACES; ifIdx++)
  {
    nwkIfCb.interfaces[ifIdx].brIfId = MESH_BR_INVALID_INTERFACE_ID;

    /* Save the pointer for the interface output filter. */
    nwkIfCb.interfaces[ifIdx].outputFilter.pAddrList = (meshAddress_t *)pMemBuff;

    /* Increment the memory buffer pointer. */
    pMemBuff += (sizeof(meshAddress_t) * pMeshConfig->pMemoryConfig->nwkOutputFilterSize);
  }

  /* Set interface output filter size. */
  nwkIfCb.maxFilterSize = pMeshConfig->pMemoryConfig->nwkOutputFilterSize;

  /* Save the updated address. */
  meshCb.pMemBuff += reqMem;

  /* Subtract the reserved size from memory buffer size. */
  meshCb.memBuffSize -= reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief     Converts a bearer interface (instance) ID to a network interface.
 *
 *  \param[in] brIfId  Unique identifier of the interface.
 *
 *  \return    meshNwkIf_t* or NULL if interface is not found.
 *
 *  \remarks   If the invalid bearer interface ID is passed as parameter, this function also
 *             searches the first empty slot of the network interfaces.
 */
/*************************************************************************************************/
meshNwkIf_t *meshNwkIfBrIdToNwkIf(meshBrInterfaceId_t brIfId)
{
  uint8_t idx = 0;

  for (idx = 0; idx < MESH_BR_MAX_INTERFACES; idx++)
  {
    /* Search for an matching entry. */
    if (nwkIfCb.interfaces[idx].brIfId == brIfId)
    {
      break;
    }
  }

  /* If no entry found, return NULL. */
  if (idx == MESH_BR_MAX_INTERFACES)
  {
    return NULL;
  }

  /* Return pointer to interface found. */
  return (&(nwkIfCb.interfaces[idx]));
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a new interface in the network layer.
 *
 *  \param[in] brIfId    Unique identifier of the interface.
 *  \param[in] brIfType  Type of the bearer associated with the interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshNwkIfAddInterface(meshBrInterfaceId_t brIfId, meshBrType_t brIfType)
{
  meshNwkIf_t *pNwkIf;

  WSF_ASSERT(brIfType < MESH_INVALID_BEARER);
  WSF_ASSERT(brIfId < MESH_BR_INVALID_INTERFACE_ID);

  /* Search for an empty slot. */
  pNwkIf = meshNwkIfBrIdToNwkIf(MESH_BR_INVALID_INTERFACE_ID);

  /* If slot is not found abort. */
  if (pNwkIf == NULL)
  {
    /* Should never happen as this is synced by the bearer. */
    WSF_ASSERT(pNwkIf != NULL);
    return;
  }

  /* Set ID and type. */
  pNwkIf->brIfId = brIfId;
  pNwkIf->brIfType = brIfType;

  /* Set empty white-list filter for GATT, empty black-list filter for ADV. */
  if (brIfType == MESH_ADV_BEARER)
  {
    pNwkIf->outputFilter.filterType = MESH_NWK_BLACK_LIST;
  }
  else
  {
    pNwkIf->outputFilter.filterType = MESH_NWK_WHITE_LIST;
  }

  /* Reset Address List. */
  meshNwkIfResetFilterAddressList(pNwkIf);
}

/*************************************************************************************************/
/*!
 *  \brief     Removes an interface in the network layer.
 *
 *  \param[in] brIfId  Unique identifier of the interface.
 *
 *  \return    None.
 *
 *  \remarks   brIfId sanity check occurs prior to this function beeing called.
 */
/*************************************************************************************************/
void meshNwkIfRemoveInterface(meshBrInterfaceId_t brIfId)
{
  meshNwkIf_t *pNwkIf;

  pNwkIf = meshNwkIfBrIdToNwkIf(brIfId);

  WSF_ASSERT(brIfId < MESH_BR_INVALID_INTERFACE_ID);

  MESH_TRACE_WARN1("MESH NWK: removing interface %d", brIfId);

  /* If the interface is not found abort */
  if (pNwkIf == NULL)
  {
    /* Should never happen as this is verified by the bearer. */
    WSF_ASSERT(pNwkIf != NULL);
    return;
  }

  /* Reset interface slot */
  pNwkIf->brIfId = MESH_BR_INVALID_INTERFACE_ID;
}

/*************************************************************************************************/
/*! \brief     Decides if a Network PDU should be filtered out on a specific interface.
 *
 *  \param[in] pIfFilter    Pointer to an output filter of a Network Interface.
 *  \param[in] pNwkPduMeta  Destination address.
 *
 *  \retval    TRUE The PDU should be filtered out.
 *  \retval    FALSE The PDU should be sent.
 */
/*************************************************************************************************/
bool_t meshNwkIfFilterOutMsg(meshNwkIfFilter_t *pIfFilter, meshAddress_t dstAddr)
{
  uint8_t idx = 0;
  bool_t  isInList = FALSE;

  /* Filter an unassigned destination address */
  if (dstAddr == MESH_ADDR_TYPE_UNASSIGNED)
  {
    return TRUE;
  }

  /* Find if address is in filter list */
  for (idx = 0; idx < nwkIfCb.maxFilterSize; idx++)
  {
    /* The filter list is populated from left to right */
    if (pIfFilter->pAddrList[idx] == MESH_ADDR_TYPE_UNASSIGNED)
    {
      break;
    }

    /* Check if the address stored in the filter is the same */
    if (dstAddr == pIfFilter->pAddrList[idx])
    {
      isInList = TRUE;
      break;
    }
  }

  /* If the address is in the filter list. */
  if (isInList)
  {
    /* NWK should filter out the message if the filter type is black-list. */
    return (pIfFilter->filterType == MESH_NWK_BLACK_LIST);
  }

  /* NWK should filter out the message if the filter type is white-list. */
  return (pIfFilter->filterType == MESH_NWK_WHITE_LIST);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the output filter type for an interface.
 *
 *  \param[in] brIfId      ID number for the network interface.
 *  \param[in] filterType  Type of filter. See ::meshNwkFilterType_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkIfSetFilterType(meshBrInterfaceId_t brIfId, meshNwkFilterType_t filterType)
{
  meshNwkIf_t *pNwkIf;

  /* Search for the interface. */
  pNwkIf = meshNwkIfBrIdToNwkIf(brIfId);

  /* If slot is not found abort. */
  if (pNwkIf == NULL)
  {
    /* Should never happen as this is verified by the bearer. */
    WSF_ASSERT(pNwkIf != NULL);
    return;
  }

  /* Update filter type. */
  pNwkIf->outputFilter.filterType = filterType;

  /* Reset Address List. */
  meshNwkIfResetFilterAddressList(pNwkIf);
}

/*************************************************************************************************/
/*!
 *  \brief     Adds the given address to the filter list of the given interface.
 *
 *  \param[in] brIfId   ID number for the network interface.
 *  \param[in] address  Mesh address to be added to the filter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkIfAddAddressToFilter(meshBrInterfaceId_t brIfId, meshAddress_t address)
{
  meshNwkIf_t *pNwkIf;
  uint16_t idx;

  /* Search for the interface. */
  pNwkIf = meshNwkIfBrIdToNwkIf(brIfId);

  /* If slot is not found abort. */
  if (pNwkIf == NULL)
  {
    /* Should never happen as this is verified by the bearer. */
    WSF_ASSERT(pNwkIf != NULL);
    return;
  }

  /* Output filter is full. */
  if (pNwkIf->outputFilter.filterSize == nwkIfCb.maxFilterSize)
  {
    return;
  }

  /* Skip invalid addresses */
  if (MESH_IS_ADDR_UNASSIGNED(address))
  {
    return;
  }

  /* Search address from received list in current filter list. */
  for (idx = 0; idx < pNwkIf->outputFilter.filterSize; idx++)
  {
    if (pNwkIf->outputFilter.pAddrList[idx] == address)
    {
      /* Found match */
      break;
    }
  }

  /* No match found. Update filter with new address */
  if (idx == pNwkIf->outputFilter.filterSize)
  {
    pNwkIf->outputFilter.pAddrList[idx] = address;
    pNwkIf->outputFilter.filterSize++;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Removes the given addresses from the filter list of the given interface.
 *
 *  \param[in] brIfId   ID number for the network interface.
 *  \param[in] address  Mesh address to be added to the filter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkIfRemoveAddressFromFilter(meshBrInterfaceId_t brIfId, meshAddress_t address)
{
  meshNwkIf_t *pNwkIf;
  uint16_t idx;

  /* Search for the interface. */
  pNwkIf = meshNwkIfBrIdToNwkIf(brIfId);

  /* If slot is not found abort. */
  if (pNwkIf == NULL)
  {
    /* Should never happen as this is verified by the bearer. */
    WSF_ASSERT(pNwkIf != NULL);
    return;
  }

  /* Output filter is empty. */
  if (pNwkIf->outputFilter.filterSize == 0)
  {
    return;
  }

  /* Skip invalid addresses. */
  if (MESH_IS_ADDR_UNASSIGNED(address))
  {
    return;
  }

  /* Search address from received list in current filter list. */
  for (idx = 0; idx < pNwkIf->outputFilter.filterSize; idx++)
  {
    if (pNwkIf->outputFilter.pAddrList[idx] == address)
    {
      /* Found match */
      break;
    }
  }

  /* Match found. Decrease filter size */
  if (idx != pNwkIf->outputFilter.filterSize)
  {
    pNwkIf->outputFilter.filterSize--;

    /* Remove address from filter by overwriting it with the last one.*/
    if (idx < pNwkIf->outputFilter.filterSize)
    {
      pNwkIf->outputFilter.pAddrList[idx] = pNwkIf->outputFilter.pAddrList[pNwkIf->outputFilter.filterSize];
    }

    /* Mark last one as unassigned. */
    pNwkIf->outputFilter.pAddrList[pNwkIf->outputFilter.filterSize] = MESH_ADDR_TYPE_UNASSIGNED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the network interface structure corresponding to the bearer interface.
 *
 *  \param[in] brIfId  ID number for the network interface.
 *
 *  \return    Pointer ::meshNwkIf_t, or NULL if interface does not exist.
 */
/*************************************************************************************************/
meshNwkIf_t* MeshNwkIfGet(meshBrInterfaceId_t brIfId)
{
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);

  return meshNwkIfBrIdToNwkIf(brIfId);
}
