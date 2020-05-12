/*************************************************************************************************/
/*!
 *  \file   mesh_prv_beacon.h
 *
 *  \brief  Mesh Provisioning beacon module interface.
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

#ifndef MESH_PRV_BEACON_H
#define MESH_PRV_BEACON_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Beacon functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvBeaconInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initiates the sending of an Unprovisioned Device beacon on the specified interface
 *
 *  \param[in] brIfId          PB-ADV interface ID.
 *  \param[in] beaconInterval  Unprovisioned Device beacon interval in ms.
 *  \param[in] pUuid           16 bytes of UUID data.
 *  \param[in] oobInfoSrc      OOB information indicating the availability of OOB data.
 *  \param[in] pUriData        Uniform Resource Identifier (URI) data.
 *  \param[in] uriLen          URI data length.
 *
 *  \return    None.
 */
 /*************************************************************************************************/
void MeshPrvBeaconStart(meshBrInterfaceId_t brIfId, uint32_t beaconInterval, const uint8_t *pUuid,
                        uint16_t oobInfoSrc, const uint8_t *pUriData,
                        uint8_t uriLen);

/*************************************************************************************************/
/*!
 *  \brief  Stops the sending of Unprovisioned Device beacons.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvBeaconStop(void);

/*************************************************************************************************/
/*!
 *  \brief  Matches the UUID with the one in the Unprovisioned Device beacon.
 *
 *  \return TRUE if matches, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshPrvBeaconMatch(const uint8_t *pUuid);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_BEACON_H */
