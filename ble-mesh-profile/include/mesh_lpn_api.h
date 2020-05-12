/*************************************************************************************************/
/*!
 *  \file   mesh_lpn_api.h
 *
 *  \brief  Mesh Low Power Node API.
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

/*! ***********************************************************************************************
*
* @addtogroup MESH_LPN_API Mesh Low Power Node API
* @{
************************************************************************************************ */

#ifndef MESH_LPN_API_H
#define MESH_LPN_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Mesh LPN callback events */
enum meshLpnEventValues
{
  MESH_LPN_FRIENDSHIP_ESTABLISHED_EVENT,        /*!< Friendship Established event */
  MESH_LPN_FRIENDSHIP_TERMINATED_EVENT,         /*!< Friendship Terminated event */
  MESH_LPN_MAX_EVENT,                           /*!< Max LPN event */
};

/*! \brief LPN Friendship Established event type for ::MESH_LPN_FRIENDSHIP_ESTABLISHED_EVENT */
typedef struct meshLpnFriendshipEstablishedEvt_tag
{
  wsfMsgHdr_t        hdr;          /*!< Header structure */
  uint16_t           netKeyIndex;  /*!< Connection identifier */
} meshLpnFriendshipEstablishedEvt_t;

/*! \brief LPN Friendship Established event type for ::MESH_LPN_FRIENDSHIP_TERMINATED_EVENT */
typedef struct meshLpnFriendshipTerminatedEvt_tag
{
  wsfMsgHdr_t        hdr;          /*!< Header structure */
  uint16_t           netKeyIndex;  /*!< Connection identifier */
} meshLpnFriendshipTerminatedEvt_t;

/*! \brief Generic LPN event callback parameters structure */
typedef union meshLpnEvt_tag
{
  wsfMsgHdr_t                        hdr;                     /*!< Generic WSF header */
  meshLpnFriendshipEstablishedEvt_t  friendshipEstablished;   /*!< Friendship Established event */
  meshLpnFriendshipTerminatedEvt_t   friendshipTerminated;    /*!< Friendship Terminated event */
} meshLpnEvt_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN event notification callback function pointer type.
 *
 *  \param[in] pEvent  Pointer to LPN event. See ::meshLpnEvt_t
 *
 *  \return    None.
 *
 *  \note      This notification callback should be used by the application to process the
 *             LPN events and take appropriate action.
 */
/*************************************************************************************************/
typedef void (*meshLpnEvtNotifyCback_t)(const meshLpnEvt_t *pEvent);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes memory requirements for LPN feature.
 *
 *  \return Required memory in bytes.
 */
/*************************************************************************************************/
uint32_t MeshLpnGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief     Initialize Low Power Node memory requirements.
 *
 *  \param[in] pFreeMem     Pointer to free memory.
 *  \param[in] freeMemSize  Size of pFreeMem.
 *
 *  \return    Amount of free memory consumed.
 *
 *  This function initializes the Low Power Node feature.
 *
 *  \note      This function must be called once after Mesh Stack initialization.
 */
/*************************************************************************************************/
uint32_t MeshLpnMemInit(uint8_t *pFreeMem, uint32_t freeMemSize);

/*************************************************************************************************/
/*!
 *  \brief     Initialize Low Power Node feature.
 *
 *  \return    None.
 *
 *  This function initializes the Low Power Node feature.
 *
 *  \note      This function and MeshFriendInit() are mutually exclusive.
 */
/*************************************************************************************************/
void MeshLpnInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the Mesh LPN events callback.
 *
 *  \param[in] eventCback  Callback function for LPN events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLpnRegister(meshLpnEvtNotifyCback_t eventCback);

/*************************************************************************************************/
/*!
 *  \brief     Tries to establish a Friendship based on specific criteria for a subnet.
 *
 *  \param[in] netKeyIndex          NetKey index.
 *  \param[in] pLpnCriteria         Pointer to LPN criteria structure.
 *  \param[in] sleepDurationMs      Sleep duration in milliseconds.
 *  \param[in] recvDelayMs          Receive Delay in milliseconds, see ::meshFriendshipRecvDelayValues.
 *  \param[in] establishRetryCount  Number of retries for Friendship establish procedures.
 *
 *  \return    TRUE if success, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLpnEstablishFriendship(uint16_t netKeyIndex, meshFriendshipCriteria_t *pLpnCriteria,
                                  uint32_t sleepDurationMs, uint8_t recvDelayMs,
                                  uint8_t establishRetryCount);

/*************************************************************************************************/
/*!
 *  \brief     Terminates an established Friendship for a subnet.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLpnTerminateFriendship(uint16_t netKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief  Returns the period in milliseconds until another scheduled action is performed by LPN.
 *
 *  \return Period value in milliseconds.
 */
/*************************************************************************************************/
uint32_t MeshLpnGetRemainingSleepPeriod(void);

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Low Power Node callback event.
 *
 *  \param[in] pMeshLpnEvt  Mesh Low Power Node callback event.
 *
 *  \return    Size of Mesh Low Power Node callback event.
 */
/*************************************************************************************************/
uint16_t MeshLpnSizeOfEvt(wsfMsgHdr_t *pMeshLpnEvt);

#ifdef __cplusplus
}
#endif

#endif /* MESH_LPN_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
