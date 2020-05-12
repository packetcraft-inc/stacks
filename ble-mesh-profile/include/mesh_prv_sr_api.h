/*************************************************************************************************/
/*!
 *  \file   mesh_prv_sr_api.h
 *
 *  \brief  Provisioning Server API.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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
 * \addtogroup MESH_PROVISIONING_SERVER_API Mesh Provisioning Server API
 * @{
 *************************************************************************************************/

#ifndef MESH_PRV_SR_API_H
#define MESH_PRV_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mesh_prv.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*!
 *  Mesh Provisioning Server event types enumeration
 *
 *  \remarks These events will be generated at various steps in the provisioning process to notify
 *           the user
 */
enum meshPrvSrEvtTypes
{
  MESH_PRV_SR_LINK_OPENED_EVENT,                           /*!< Provisioning link opened, ACK sent and
                                                            *   provisioning process is underway; this
                                                            *   event is generated only when PB-ADV is
                                                            *   used for PB-GATT the link is already open.
                                                            */
  MESH_PRV_SR_OUTPUT_OOB_EVENT,                            /*!< Device should output the OOB informations
                                                            *   as specified by the event parameters.
                                                            *   See ::meshPrvSrEvtOutputOob_t
                                                            */
  MESH_PRV_SR_OUTPUT_CONFIRMED_EVENT,                      /*!< Device can stop outputting OOB information
                                                            *   now
                                                            */
  MESH_PRV_SR_INPUT_OOB_EVENT,                             /*!< The user has to enter the
                                                            *   Input OOB information displayed
                                                            *   by the Provisioner device.
                                                            */
  MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT,                 /*!< The provisioning process is complete.
                                                            *   The address of the first element is
                                                            *   provided in the event parameters.
                                                            *   See ::meshPrvSrEvtPrvComplete_t
                                                            */
  MESH_PRV_SR_PROVISIONING_FAILED_EVENT,                   /*!< An error occurred during the provisioning
                                                            *   process
                                                            *   See ::meshPrvSrEvtPrvFailed_t
                                                            */
};

/*! \brief Mesh Provisioning Server callback events end */
#define MESH_PRV_SR_MAX_EVENT             MESH_PRV_SR_PROVISIONING_FAILED_EVENT

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Mesh Unprovisioned Device Information */
typedef struct meshPrvSrUnprovisionedDeviceInfo_tag
{
  const meshPrvCapabilities_t  *pCapabilities;     /*!< Pointer to device capabilities */
  uint8_t                      *pDeviceUuid;       /*!< Pointer to the device UUID */
  meshPrvOobInfoSource_t       oobInfoSrc;         /*!< OOB information source */
  uint8_t                      *pStaticOobData;    /*!< Pointer to static OOB data,
                                                    *   or NULL if unsupported
                                                    */
  uint8_t                      uriLen;             /*!< Length of URI data
                                                    *   (size of the array pointed by pUriData)
                                                    */
  uint8_t                      *pUriData;          /*!< Pointer to URI data */
  meshPrvEccKeys_t             *pAppOobEccKeys;    /*!< The OOB ECC key pair provided by the application. */
} meshPrvSrUnprovisionedDeviceInfo_t;

/*! \brief Parameters structure for ::MESH_PRV_SR_OUTPUT_OOB_EVENT event */
typedef struct meshPrvSrEvtOutputOob_tag
{
  wsfMsgHdr_t               hdr;              /*!< Header structure */
  meshPrvOutputOobAction_t  outputOobAction;  /*!< Selected Output OOB Action to be performed
                                               *   by the application on the unprovisioned device.
                                               *   Only the selected action bit is 1, the rest are 0.
                                               *   If the selected action bit is
                                               *   MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM,
                                               *   then outputData is an array of characters
                                               *   of size outputOobSize;
                                               *   otherwise, it is a numeric value and
                                               *   the outputOobSize parameter is not used.
                                               *   See ::meshPrvInputOobAction_t
                                               */
  meshPrvOutputOobSize_t    outputOobSize;    /*!< Size of alphanumeric Output OOB data.
                                               *   Used only when the outputOobAction bit is
                                               *   MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM,
                                               *   otherwise this is set to 0 and shall be
                                               *   ignored.
                                               *   See ::meshPrvOutputOobSize_t
                                               */
  meshPrvInOutOobData_t     outputOobData;    /*!< Output OOB data to be output by the device.
                                               *   If the outputOobAction bit is equal to
                                               *   MESH_PRV_OUTPUT_OOB_ACTION_OUTPUT_ALPHANUM,
                                               *   then this shall be displayed as an array of
                                               *   characters of size outputOobSize; otherwise,
                                               *   this shall be used as a number.
                                               *   See ::meshPrvInOutOobData_t
                                               */
} meshPrvSrEvtOutputOob_t;

/*! \brief Parameters structure for ::MESH_PRV_SR_INPUT_OOB_EVENT event */
typedef struct meshPrvSrEvtInputOob_tag
{
  wsfMsgHdr_t               hdr;              /*!< Header structure */
  meshPrvInputOobAction_t   inputOobAction;   /*!< Selected Input OOB Action to be performed.
                                               *   Only the selected action bit is set.
                                               *   See ::meshPrvInputOobAction_t
                                               */
} meshPrvSrEvtInputOob_t;

/*! \brief Parameters structure for ::MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT event */
typedef struct meshPrvSrEvtPrvComplete_tag
{
  wsfMsgHdr_t   hdr;                          /*!< Header structure */
  uint8_t       devKey[MESH_KEY_SIZE_128];    /*!< Device Key */
  uint8_t       netKey[MESH_KEY_SIZE_128];    /*!< Network Key */
  uint16_t      netKeyIndex;                  /*!< Network Key Index */
  uint8_t       flags;                        /*!< Flags bitmask */
  uint32_t      ivIndex;                      /*!< Current value of the IV Index */
  uint16_t      address;                      /*!< Address assigned to the primary element */
} meshPrvSrEvtPrvComplete_t;

/*! \brief Parameters structure for ::MESH_PRV_SR_PROVISIONING_FAILED_EVENT event */
typedef struct meshPrvSrEvtPrvFailed_tag
{
  wsfMsgHdr_t         hdr;      /*!< Header structure */
  meshPrvFailReason_t reason;   /*!< Reason why provisioning failed */
} meshPrvSrEvtPrvFailed_t;

/*! \brief Generic Provisioning Server event callback parameters structure */
typedef union meshPrvSrEvt_tag
{
  wsfMsgHdr_t                 hdr;         /*!< Generic WSF header. Used for the following events:
                                            *   ::MESH_PRV_SR_LINK_OPENED_EVENT,
                                            *   ::MESH_PRV_SR_OUTPUT_CONFIRMED_EVENT,
                                            */
  meshPrvSrEvtOutputOob_t   outputOob;     /*!< Output OOB event received data.
                                            *   Used for ::MESH_PRV_SR_OUTPUT_OOB_EVENT
                                            */
  meshPrvSrEvtInputOob_t    inputOob;      /*!< Input OOB event data.
                                            *   Used for ::MESH_PRV_SR_INPUT_OOB_EVENT
                                            *   After the user inputs the OOB data, the
                                            *   MeshPrvSrInputComplete API has to be called.
                                            */
  meshPrvSrEvtPrvComplete_t prvComplete;   /*!< Provisioning complete event received data.
                                            *   Used for ::MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT
                                            */
  meshPrvSrEvtPrvFailed_t   prvFailed;     /*!< Provisioning failed event.
                                            *   Used for ::MESH_PRV_SR_PROVISIONING_FAILED_EVENT
                                            */
} meshPrvSrEvt_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Server layer event notification callback function pointer type.
 *
 *  \param[in] pEvent  Pointer to Provisioning Server event. See ::meshPrvSrEvt_t
 *
 *  \return    None.
 *
 *  \note      This notification callback should be used by the application to process the
 *             provisioning related events and take appropriate action. Such as, starting
 *             outputting the OOB information upon receiving ::MESH_PRV_SR_OUTPUT_OOB_EVENT event.
 */
/*************************************************************************************************/
typedef void (*meshPrvSrEvtNotifyCback_t)(meshPrvSrEvt_t *pEvent);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes and configures the Provisioning Server.
 *
 *  \param[in] pUpdInfo  Pointer to static unprovisioned device information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrInit(meshPrvSrUnprovisionedDeviceInfo_t const *pUpdInfo);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh Provisioning Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF event handler for Mesh Provisioning Server API.
 *
 *  \param[in] event WSF event mask.
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Registers the Provisioning Server event callback function.
 *
 *  \param[in] eventCback  Pointer to the callback to be invoked on provisioning events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrRegister(meshPrvSrEvtNotifyCback_t eventCback);

/*************************************************************************************************/
/*!
 *  \brief     Begins provisioning over PB-ADV by waiting for a PB-ADV link.
 *
 *  \param[in] ifId            ID of the PB-ADV bearer interface.
 *  \param[in] beaconInterval  Unprovisioned Device beacon interval in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrEnterPbAdvProvisioningMode(uint8_t ifId, uint32_t beaconInterval);

/*************************************************************************************************/
/*!
 *  \brief     Begins provisioning over PB-GATT.
 *
 *  \param[in] connId  ID of the GATT connection.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrEnterPbGattProvisioningMode(uint8_t connId);

/*************************************************************************************************/
/*!
 *  \brief     Provisioner Server application calls this function when it obtains the OOB input
 *             numbers or characters from the user.
 *
 *  \param[in] inputOobSize  Size of alphanumeric Input OOB data, used only when the Input OOB
 *                           Action was MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM; otherwise,
 *                           the Input OOB data is numeric, and this parameter shall be set to 0.
 *
 *  \param[in] inputOobData  Array of inputOobSize octets containing the alphanumeric Input OOB
 *                           data, if the Input OOB action selected by the Provisioner was
 *                           MESH_PRV_INPUT_OOB_ACTION_INPUT_ALPHANUM action; otherwise,
 *                           this is a numeric 4-octet value and inputOobSize is ignored.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvSrInputComplete(meshPrvInputOobSize_t inputOobSize, meshPrvInOutOobData_t inputOobData);

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Provisioning Server callback event.
 *
 *  \param[in] pMeshPrvSrEvt  Mesh Provisioning Server callback event.
 *
 *  \return    Size of Mesh Provisioning Server callback event.
 */
/*************************************************************************************************/
uint16_t MeshPrvSrSizeOfEvt(meshPrvSrEvt_t *pMeshPrvSrEvt);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
