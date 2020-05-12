/*************************************************************************************************/
/*!
 *  \file   mesh_prv_cl_api.h
 *
 *  \brief  Provisioning Client API.
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
 * \addtogroup MESH_PROVISIONING_CLIENT_API Mesh Provisioning Client API
 * @{
 *************************************************************************************************/

#ifndef MESH_PRV_CL_API_H
#define MESH_PRV_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mesh_prv.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*!
 *  Mesh Provisioning Client event types enumeration
 */
enum meshPrvClEvtTypes
{
  MESH_PRV_CL_LINK_OPENED_EVENT,                            /*!< Provisioning link open, ACK
                                                             *   received and provisioning process
                                                             *   is underway; this event is generated
                                                             *   only when PB-ADV is used; for PB-GATT
                                                             *   the link is already open.
                                                             */
  MESH_PRV_CL_RECV_CAPABILITIES_EVENT,                      /*!< The unprovisioned device has
                                                             *   sent its capabilities and the
                                                             *   Provisioner has to select the
                                                             *   authentication method
                                                             */
  MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT,                       /*!< The application has to provide the
                                                             *   Output OOB information displayed
                                                             *   by the unprovisioned device
                                                             */
  MESH_PRV_CL_DISPLAY_INPUT_OOB_EVENT,                      /*!< The application has to display the
                                                             *   Input OOB information to be input
                                                             *   by the user on the unprovisioned device
                                                             */
  MESH_PRV_CL_PROVISIONING_COMPLETE_EVENT,                  /*!< The provisioning process is
                                                             *   complete
                                                             */
  MESH_PRV_CL_PROVISIONING_FAILED_EVENT,                    /*!< An error occurred during the
                                                             *   provisioning process
                                                             */
};

/*! \brief Mesh Provisioning Client callback events end */
#define MESH_PRV_CL_MAX_EVENT             MESH_PRV_CL_PROVISIONING_FAILED_EVENT

/*!
 *  Mesh Provisioning Client OOB authentication method enumeration values.
 *  Values are explicit in order to match PDU values.
 */
enum meshPrvClOobAuthMethodValues
{
  MESH_PRV_CL_NO_OBB_AUTH       = 0x00,   /*!< No OOB authentication is used. Provisioning is insecure. */
  MESH_PRV_CL_USE_STATIC_OOB    = 0x01,   /*!< Use 16-octet static OOB data for authentication. */
  MESH_PRV_CL_USE_OUTPUT_OOB    = 0x02,   /*!< Use output OOB data. */
  MESH_PRV_CL_USE_INPUT_OOB     = 0x03,   /*!< Use input OOB data. */
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/
/*!
 * Provisioning session information
 */
typedef struct meshPrvClSessionInfo_tag
{
  uint8_t                   *pDeviceUuid;       /*!< Pointer to Device UUID. */
  meshPrvOobPublicKey_t     *pDevicePublicKey;  /*!< Pointer to OOB device public key,
                                                 *   or NULL if not available. */
  uint8_t                   *pStaticOobData;    /*!< Pointer to Static OOB authentication data,
                                                 *   or NULL if not available. */
  meshPrvEccKeys_t          *pAppEccKeys;       /*!< Pointer to Provisioner's public key,
                                                 *   or NULL if it should be generated internally. */
  meshPrvProvisioningData_t *pData;             /*!< Pointer to provisioning data to be sent
                                                 *   to the device. Shall not be NULL. */
  uint8_t                   attentionDuration;  /*!< Attention timer value for provisioning. */
} meshPrvClSessionInfo_t;

/*! Mesh Provisioning Client OOB authentication method type.
 *  See ::meshPrvClOobAuthMethodValues
 */
typedef uint8_t meshPrvClOobAuthMethod_t;

/*! Union of input and output OOB actions. */
typedef union meshPrvClOobAuthAction_tag
{
  meshPrvOutputOobAction_t  outputOobAction; /*!< Used for the MESH_PRV_CL_USE_OUTPUT_OOB method. */
  meshPrvInputOobAction_t   inputOobAction;  /*!< Used for the MESH_PRV_CL_USE_INPUT_OOB method. */
} meshPrvClOobAuthAction_t;

/*!
 *  Mesh Provisioning Client selected authentication parameter structure
 */
typedef struct meshPrvClSelectAuth_tag
{
  bool_t                    useOobPublicKey;  /*!< Use OOB public key. This may be TRUE only if
                                               * the device's OOB public key was provided to the
                                               * Provisioning Client when provisioning was started.
                                               * Otherwise, this shall be FALSE. */
  meshPrvClOobAuthMethod_t  oobAuthMethod;    /*!< OOB method. This may be MESH_PRV_CL_USE_STATIC_OOB
                                               * only if static OOB data has been provided to the
                                               * Provisioning Client when provisioning was started.
                                               * Otherwise, this may be MESH_PRV_CL_USE_INPUT_OOB or
                                               * MESH_PRV_CL_USE_OUTPUT_OOB only if the device has indicated
                                               * respective support in the provisioning capabilities.
                                               * Otherwise, this shall be MESH_PRV_CL_NO_OBB_AUTH. */
  uint8_t                   oobSize;          /*!< OOB data size. This parameter is used only for
                                               * the MESH_PRV_CL_USE_INPUT_OOB and MESH_PRV_CL_USE_OUTPUT_OOB
                                               * methods. Otherwise it is ignored.
                                               * If used, this shall be less than or equal to the maximum size
                                               * indicated by the device in the provisioning capabilities,
                                               * and it shall be greater than 0. */
  meshPrvClOobAuthAction_t  oobAction;        /*!< OOB action. This parameter is used only for
                                               * the MESH_PRV_CL_USE_INPUT_OOB and MESH_PRV_CL_USE_OUTPUT_OOB
                                               * methods. Otherwise it is ignored.
                                               * If used, then only one valid bit shall be set to 1, and
                                               * all other bits shall be set to 0. The bit that is set to 1
                                               * must also be set to 1 in the received provisioning capabilities. */
} meshPrvClSelectAuth_t;

/*! \brief Mesh Provisioning Client notification event type. See ::meshPrvClEvtTypes */
typedef uint8_t meshPrvClEvtType_t;

/*! \brief Parameters structure for ::MESH_PRV_CL_RECV_CAPABILITIES_EVENT event */
typedef struct meshPrvClEvtRecvCapabilities_tag
{
  wsfMsgHdr_t               hdr;              /*!< Header structure */
  meshPrvCapabilities_t     capabilities;     /*!< Peer device capabilities.
                                               *   See ::meshPrvCapabilities_t
                                               */
} meshPrvClEvtRecvCapabilities_t;

/*! \brief Parameters structure for ::MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT event */
typedef struct meshPrvClEvtEnterOutputOob_tag
{
  wsfMsgHdr_t               hdr;              /*!< Header structure */
  meshPrvOutputOobAction_t  outputOobAction;  /*!< Output OOB action performed by the peer device.
                                               *   See ::meshPrvOutputOobAction_t
                                               */
} meshPrvClEvtEnterOutputOob_t;

/*! \brief Parameters structure for ::MESH_PRV_CL_DISPLAY_INPUT_OOB_EVENT event */
typedef struct meshPrvClEvtDisplayInputOob_tag
{
  wsfMsgHdr_t               hdr;              /*!< Header structure */
  meshPrvInputOobAction_t   inputOobAction;   /*!< Selected Input OOB Action to be performed
                                               *   by the user on the unprovisioned device.
                                               *   Only the selected action bit is set.
                                               *   If the selected action bit is
                                               *   MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM,
                                               *   then inputData is an array of characters
                                               *   of size inputOobSize;
                                               *   otherwise, it is a numeric value and
                                               *   the inputOobSize parameter is not used.
                                               *   See ::meshPrvInputOobAction_t
                                               */
  meshPrvInputOobSize_t     inputOobSize;     /*!< Size of alphanumeric Input OOB data.
                                               *   Used only when the inputOobAction bit is
                                               *   MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM,
                                               *   otherwise this is set to 0 and shall be
                                               *   ignored.
                                               *   See ::meshPrvInputOobSize_t
                                               */
  meshPrvInOutOobData_t     inputOobData;     /*!< Input OOB data to be input on the device.
                                               *   If the inputOobAction bit is equal to
                                               *   MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM,
                                               *   then this shall be displayed as an array of
                                               *   characters of size inputOobSize; otherwise,
                                               *   this shall be used as a number.
                                               *   See ::meshPrvInOutOobData_t
                                               */
} meshPrvClEvtDisplayInputOob_t;

/*! \brief Parameters structure for ::MESH_PRV_CL_PROVISIONING_COMPLETE_EVENT event */
typedef struct meshPrvClEvtPrvComplete_tag
{
  wsfMsgHdr_t        hdr;                               /*!< Header structure */
  uint8_t            uuid[MESH_PRV_DEVICE_UUID_SIZE];   /*!< Pointer to the device UUID of the new node */
  meshAddress_t      address;                           /*!< Unicast address of the new node */
  uint8_t            numOfElements;                     /*!< Number of elements on the new node */
  uint8_t            devKey[MESH_KEY_SIZE_128];         /*!< Pointer to the device key of the new node */
} meshPrvClEvtPrvComplete_t;

/*! \brief Parameters structure for ::MESH_PRV_CL_PROVISIONING_FAILED_EVENT event */
typedef struct meshPrvClEvtPrvFailed_tag
{
  wsfMsgHdr_t         hdr;      /*!< Header structure */
  meshPrvFailReason_t reason;   /*!< Address assigned to the primary element */
} meshPrvClEvtPrvFailed_t;

/*! \brief Generic Provisioning Client event callback parameters structure */
typedef union meshPrvClEvt_tag
{
  wsfMsgHdr_t                     hdr;            /*!< Generic WSF header. Used for the following events:
                                                   *   ::MESH_PRV_CL_LINK_OPENED_EVENT,
                                                   */

  meshPrvClEvtRecvCapabilities_t  recvCapab;      /*!< Capabilities received event data.
                                                   *   Used for ::MESH_PRV_CL_RECV_CAPABILITIES_EVENT
                                                   */

  meshPrvClEvtEnterOutputOob_t    enterOutputOob; /*!< Capabilities received event data.
                                                   *   Used for ::MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT
                                                   */
  meshPrvClEvtDisplayInputOob_t   inputOob;       /*!< Display Input OOB event data.
                                                   *   Used for ::MESH_PRV_CL_DISPLAY_INPUT_OOB_EVENT
                                                   */
  meshPrvClEvtPrvComplete_t       prvComplete;    /*!< Provisioning complete event data.
                                                   *   Used for ::MESH_PRV_CL_PROVISIONING_COMPLETE_EVENT
                                                   */
  meshPrvClEvtPrvFailed_t         prvFailed;      /*!< Provisioning failed event data.
                                                   *   Used for ::MESH_PRV_CL_PROVISIONING_FAILED_EVENT
                                                   */
} meshPrvClEvt_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Client layer event notification callback function pointer type.
 *
 *  \param[in] pEvent  Pointer to Provisioning Client event. See ::meshPrvClEvt_t
 *
 *  \return    None.
 *
 *  \note      This notification callback should be used by the application to process the
 *             provisioning events and take appropriate action.
 */
/*************************************************************************************************/
typedef void (*meshPrvClEvtNotifyCback_t)(meshPrvClEvt_t *pEvent);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Client.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvClInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the WSF handler for the Provisioning Client.
 *
 *  \param[in] handlerId  WSF handler ID of the application using this service.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF event handler for Mesh Provisioning Client API.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Registers the Provisioning Client event callback function.
 *
 *  \param[in] evtCback  Pointer to the callback to be invoked on provisioning events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClRegister(meshPrvClEvtNotifyCback_t evtCback);

/*************************************************************************************************/
/*!
 *  \brief     Starts the provisioning procedure over PB-ADV for the device with a given UUID.
 *
 *  \param[in] ifId          Interface id.
 *  \param[in] pSessionInfo  Pointer to static provisioning session information.
 *
 *  \remarks   The structure pointed by pSessionInfo shall be static for the provisioning session.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClStartPbAdvProvisioning(uint8_t ifId,
                                     meshPrvClSessionInfo_t const *pSessionInfo);

/*************************************************************************************************/
/*!
 *  \brief     Starts the provisioning procedure over PB-GATT for the device with a given UUID.
 *
 *  \param[in] connId        GATT connection id.
 *  \param[in] pSessionInfo  Pointer to static provisioning session information.
 *
 *  \remarks   The structure pointed by pSessionInfo shall be static for the provisioning session.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClStartPbGattProvisioning(uint8_t connId,
                                      meshPrvClSessionInfo_t const *pSessionInfo);

/*************************************************************************************************/
/*!
 *  \brief  Cancels any on-going provisioning procedure.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvClCancel(void);

/*************************************************************************************************/
/*!
*  \brief     Selects the authentication parameters to continue provisioning.
*
*  \param[in] pSelectAuth  Pointer to selected authentication parameters.
*
*  \return    None.
*
*  \remarks   This API shall be called when the MESH_PRV_CL_RECV_CAPABILITIES_EVENT
*             event has been generated. The authentication parameters shall be
*             set to valid values based on the received capabilities and on the
*             availability of OOB public key and OOB static data.
*             See ::meshPrvClSelectAuth_t.
*             If invalid parameters are provided, the API call will be ignored
*             and provisioning will timeout.
*/
/*************************************************************************************************/
void MeshPrvClSelectAuthentication(meshPrvClSelectAuth_t *pSelectAuth);

/*************************************************************************************************/
/*!
 *  \brief     Provisioning Client application calls this function when the
 *             MESH_PRV_CL_ENTER_OUTPUT_OOB_EVENT event has been generated and the user
 *             has input the data displayed by the device.
 *
 *  \param[in] outputOobSize  Size of alphanumeric Output OOB data, used only when the Output OOB
 *                            Action was MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM; otherwise,
 *                            the Output OOB data is numeric, and this parameter shall be set to 0.
 *
 *  \param[in] outputOobData  Array of outputOobSize octets containing the alphanumeric Output OOB
 *                            data, if the Output OOB action selected by the Provisioner was
 *                            MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM action; otherwise,
 *                            this is a numeric 4-octet value and outputOobSize is ignored.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvClEnterOutputOob(meshPrvOutputOobSize_t outputOobSize,
                             meshPrvInOutOobData_t  outputOobData);

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Provisioning Client callback event.
 *
 *  \param[in] pMeshPrvClEvt  Mesh Provisioning Client callback event.
 *
 *  \return    Size of Mesh Provisioning Client callback event.
 */
/*************************************************************************************************/
uint16_t MeshPrvClSizeOfEvt(meshPrvClEvt_t *pMeshPrvClEvt);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
