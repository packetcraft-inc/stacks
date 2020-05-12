/*************************************************************************************************/
/*!
 *  \file   mesh_network_if.h
 *
 *  \brief  Network interfaces module interface.
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

#ifndef MESH_NETWORK_IF_H
#define MESH_NETWORK_IF_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Network layer interface filter list types enumeration.
 *
 * \brief Types used while adding or removing an address from the interface filter lists.
 */
enum meshNwkFilterType
{
  MESH_NWK_WHITE_LIST = 0,   /*!< Filter type is white list */
  MESH_NWK_BLACK_LIST = 1,   /*!< Filter type is black list */
};

/*! Mesh Newtork layer interface filter type. See ::meshNwkFilterType */
typedef uint8_t meshNwkFilterType_t;

/*! Mesh Network interface filter definition */
typedef struct meshNwkIfFilter_tag
{
  meshNwkFilterType_t filterType;     /*!< Fiter type */
  uint16_t            filterSize;     /*!< Size of an interface filter */
  meshAddress_t       *pAddrList;     /*!< Pointer to list of addresses */
} meshNwkIfFilter_t;

/*! Mesh Network interface definition */
typedef struct meshNwkIf_tag
{
  meshBrInterfaceId_t brIfId;        /*!< Interface ID */
  meshBrType_t        brIfType;      /*!< Interface type: together with ID form unique pair */
  meshNwkIfFilter_t   outputFilter;  /*!< Output filter for the interface */
} meshNwkIf_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Gets the network interface structure corresponding to the bearer interface.
 *
 *  \param[in] brIfId  ID number for the network interface.
 *
 *  \return    Pointer ::meshNwkIf_t, or NULL if interface does not exist.
 */
/*************************************************************************************************/
meshNwkIf_t* MeshNwkIfGet(meshBrInterfaceId_t brIfId);

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
void MeshNwkIfSetFilterType(meshBrInterfaceId_t brIfId, meshNwkFilterType_t filterType);

/*************************************************************************************************/
/*!
 *  \brief     Adds the given addresses to the given filter list.
 *
 *  \param[in] brIfId   ID number for the network interface.
 *  \param[in] address  Mesh address to be added to the filter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkIfAddAddressToFilter(meshBrInterfaceId_t brIfId, meshAddress_t address);

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
void MeshNwkIfRemoveAddressFromFilter(meshBrInterfaceId_t brIfId, meshAddress_t address);

#ifdef __cplusplus
}
#endif

#endif /* MESH_NETWORK_IF_H */
