/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_sr_main.h
 *
 *  \brief  Configuration Server internal module interface.
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

#ifndef MESH_CFG_MDL_SR_MAIN_H
#define MESH_CFG_MDL_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Configuration Server operation request action handler */
typedef void (*meshCfgMdlSrOpReqAct_t)(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                       uint8_t ttl, uint16_t netKeyIndex);

/*! Mesh Configuration Server control block */
typedef struct meshCfgMdlSrCb_tag
{
  meshCfgMdlSrCback_t                cback;                /*!< User callback */
  meshCfgMdlSrFriendStateChgCback_t  friendStateChgCback;  /*!< Friend State changed callback */
  meshCfgMdlSrNetKeyDelNotifyCback_t netKeyDelNotifyCback; /*!< NetKey deleted notification callback */
  meshCfgMdlSrPollTimeoutGetCback_t  pollTimeoutGetCback;  /*!< Poll Timeout get callback */
} meshCfgMdlSrCb_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void meshCfgMdlSrAccMsgRcvCback(uint8_t opcodeIdx, uint8_t *pMsgParam,
                               uint16_t msgParamLen, meshAddress_t src,
                               meshElementId_t elemId, uint8_t ttl,
                               uint16_t netKeyIndex);

void meshCfgMdlSrEmptyCback(const meshCfgMdlSrEvt_t *pEvt);
void meshCfgMdlSrEmptyFriendStateChgCback(void);
uint32_t meshCfgMdlSrEmptyPollTimeoutGetCback(meshAddress_t lpnAddr);
void meshCfgMdlSrEmptyNetKeyDelNotifyCback(uint16_t netKeyIndex);
void meshCfgMdlSrSendRsp(meshCfgMdlSrOpId_t opId, uint8_t *pMsgParam, uint16_t msgParamLen,
                         meshAddress_t cfgMdlClAddr, uint8_t recvTtl, uint16_t cfgMdlClNetKeyIndex);

void meshCfgMdlSrHandleBeaconGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleBeaconSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleCompositionDataGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleDefaultTtlGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                     uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleDefaultTtlSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                     uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleGattProxyGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                    uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleGattProxySet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                    uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleRelayGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleRelaySet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleModelPubGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                   uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelPubSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                   uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelPubVirtSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                       uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleModelSubscrAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrVirtAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrVirtDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrOvr(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrVirtOvr(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrDelAll(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrSigGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelSubscrVendorGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                            uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleNetKeyAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleNetKeyUpdt(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                  uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleNetKeyDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleNetKeyGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleAppKeyAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleAppKeyUpdt(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                  uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleAppKeyDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleAppKeyGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleNodeIdentityGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                       uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleNodeIdentitySet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                       uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleModelAppBind(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                    uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelAppUnbind(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelAppSigGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleModelAppVendorGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleNodeReset(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleFriendGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleFriendSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex);


void meshCfgMdlSrHandleKeyRefPhaseGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleKeyRefPhaseSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleHbPubGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleHbPubSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleHbSubGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleHbSubSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleLpnPollTimeoutGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex);

void meshCfgMdlSrHandleNwkTransGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                   uint8_t ttl, uint16_t netKeyIndex);
void meshCfgMdlSrHandleNwkTransSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                   uint8_t ttl, uint16_t netKeyIndex);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! Mesh Configuration Server control block */
extern meshCfgMdlSrCb_t meshCfgMdlSrCb;

/*! Mesh Configuration Server operation request action table. */
extern const meshCfgMdlSrOpReqAct_t meshCfgMdlSrOpReqActTbl[];

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_SR_MAIN_H */
