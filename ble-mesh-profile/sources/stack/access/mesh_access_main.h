/*************************************************************************************************/
/*!
 *  \file   mesh_access_main.h
 *
 *  \brief  Access internal module interface.
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

#ifndef MESH_ACCESS_MAIN_H
#define MESH_ACCESS_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Tick value in milliseconds for published messages retransmission */
#ifndef MESH_ACC_PUB_RETRANS_TMR_TICK_MS
#define MESH_ACC_PUB_RETRANS_TMR_TICK_MS     (10)
#endif

/*! Extracts SIG model instance from mesh configuration array */
#define SIG_MODEL_INSTANCE(elemId, modelIdx) (pMeshConfig->pElementArray[(elemId)].\
                                              pSigModelArray[(modelIdx)])

/*! Extracts vendor model instance from mesh configuration array */
#define VENDOR_MODEL_INSTANCE(elemId, modelIdx)  (pMeshConfig->pElementArray[(elemId)].\
                                                  pVendorModelArray[(modelIdx)])

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Access Layer WSF message events */
enum meshAccWsfMsgEvents
{
  MESH_ACC_MSG_RETRANS_TMR_EXPIRED = MESH_ACC_MSG_START, /*!< Retransmission timer expired */
  MESH_ACC_MSG_DELAY_TMR_EXPIRED,                        /*!< Random Delay timer expired */
  MESH_ACC_MSG_PP_TMR_EXPIRED,                           /*!< Periodic publishing timer expired */
};

/*! Periodic publishing state changed callback */
typedef void (*meshAccPpChangedCback_t)(meshElementId_t elemId, meshModelId_t *pModelId);

/*! Mesh Access Control Block */
typedef struct meshAccCb_tag
{
  wsfQueue_t               coreMdlQueue;                         /*!< Queue of core models */
  wsfQueue_t               pubRetransQueue;                      /*!< Queue used for retransmitting
                                                                  *   published messages
                                                                  */
  wsfQueue_t               msgSendQueue;                         /*!< Queue used for delaying
                                                                  *   access messages
                                                                  */
  meshAccPpChangedCback_t  ppChangedCback;                       /*!< Callback to inform the
                                                                  *   Periodic publishing module
                                                                  *   that a state was changed
                                                                  */
  meshWsfMsgHandlerCback_t ppWsfMsgCback;                        /*!< Periodic publishing WSF
                                                                  *   message callback
                                                                  */
  meshAccFriendAddrFromSubnetCback_t friendAddrFromSubnetCback;  /*!< Callback to get the Friend
                                                                  *   address from sub-net
                                                                  */
  uint16_t                 tmrUidGen;                            /*!< Timer unique identifier
                                                                  *   generator variable
                                                                  */
} meshAccCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Control block for Mesh Access */
extern meshAccCb_t meshAccCb;

#ifdef __cplusplus
}
#endif

#endif /* MESH_ACCESS_MAIN_H */
