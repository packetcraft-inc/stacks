/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Provisioning Service client.
 *
 *  Copyright (c) 2012-2018 Arm Ltd. All Rights Reserved.
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
 */
/*************************************************************************************************/
#ifndef MPRVS_API_H
#define MPRVS_API_H

#include "att_api.h"
#include "mesh_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup MESH_PROVISIONING_PROFILE Mesh Provisioning Profile API
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief  This function is called by the application when a message that requires
 *          processing by the Mesh Provisioning server is received.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprvsProcMsg(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief  ATTS write callback for mesh provisioning service. Use this function as  a parameter
 *          to SvcMprvsRegister().
 *
 *  \param  connId     DM connection ID.
 *  \param  handle     ATT handle.
 *  \param  operation  ATT operation.
 *  \param  offset     Write offset.
 *  \param  len        Write value length.
 *  \param  pValue     Data to write.
 *  \param  pAttr      Attribute to write.
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t MprvsWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                        uint16_t len, uint8_t *pValue, attsAttr_t *pAttr);

/*************************************************************************************************/
/*!
 *  \brief  Set the CCCD index used by the application for mesh provisioning service characteristics.
 *
 *  \param  dataOutCccIdx   Data Out CCCD index.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprvsSetCccIdx(uint8_t dataOutCccIdx);

/*************************************************************************************************/
/*!
 *  \brief  Send data on the Mesh Provisioning Server.
 *
 *  \param[in]  pEvt  GATT Proxy PDU send event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprvsSendDataOut(meshGattProxyPduSendEvt_t *pEvt);

/*! \} */    /* MESH_PROVISIONING_PROFILE */

#ifdef __cplusplus
};
#endif

#endif /* MPRVS_API_H */
