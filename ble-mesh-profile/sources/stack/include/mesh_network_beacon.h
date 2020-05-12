/*************************************************************************************************/
/*!
 *  \file   mesh_nwk_beacon.h
 *
 *  \brief  Secure Network Beacon module interface.
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

#ifndef MESH_NWK_BEACON_H
#define MESH_NWK_BEACON_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Secure Network Beacon broadcast interval */
#ifndef MESH_NWK_BEACON_INTVL_SEC
#define MESH_NWK_BEACON_INTVL_SEC 10
#endif

/*! Queue limit for received beacons */
#ifndef MESH_NWK_BEACON_RX_QUEUE_LIMIT
#define MESH_NWK_BEACON_RX_QUEUE_LIMIT 10
#endif

/*! Macro used to indicate that beacons are sent on all network keys */
#define MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS     0xFFFF

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*************************************************************************************************/
/*!
 * \brief     Beacon generate complete callback.
 * \param[in] isSuccess    TRUE if beacon was generated successfully.
 * \param[in] netKeyIndex  Global identifier of the NetKey.
 * \param[in] pBeacon      Pointer to generated beacon, provided in the request.
 */
/*************************************************************************************************/
typedef void (*meshBeaconGenOnDemandCback_t)(bool_t isSuccess, uint16_t netKeyIndex,
                                             uint8_t *pBeacon);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Secure Network Beacon module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshNwkBeaconInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Informs the module that the Beacon state has changed.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshNwkBeaconHandleStateChanged(void);

/*************************************************************************************************/
/*!
 *  \brief     Sends beacons on all available interfaces for one or all NetKeys as a result of a
 *             trigger.
 *
 *  \param[in] netKeyIndex  Index of the NetKey that triggered the beacon sending or
 *                          MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkBeaconTriggerSend(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Generates a Secure Network Beacon for a given subnet.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey associated to a subnet.
 *  \param[in] pBuf         Pointer to buffer where to store the beacon.
 *  \param[in] cback        Beacon generation complete callback.
 *
 *  \return    TRUE if beacon generation starts, FALSE otherwise.
 *
 *  \remarks   pBuf must point to a buffer of at least ::MESH_NWK_BEACON_NUM_BYTES bytes
 */
/*************************************************************************************************/
bool_t MeshNwkBeaconGenOnDemand(uint16_t netKeyIndex, uint8_t *pBuf,
                                meshBeaconGenOnDemandCback_t cback);

#ifdef __cplusplus
}
#endif

#endif /* MESH_NWK_BEACON_H */
