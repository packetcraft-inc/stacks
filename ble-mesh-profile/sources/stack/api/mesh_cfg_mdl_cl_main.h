/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_cl_main.h
 *
 *  \brief  Configuration Client internal module interface.
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

#ifndef MESH_CFG_MDL_CL_MAIN_H
#define MESH_CFG_MDL_CL_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Default timeout for Configuration Client requests */
#define MESH_CFG_MDL_CL_OP_TIMEOUT_DEFAULT_SEC  (10)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Configuration Client WSF message events */
enum meshCfgMdlClWsfMsgEvents
{
  MESH_CFG_MDL_CL_MSG_API_SEND         = MESH_CFG_MDL_CL_MSG_START, /*!< API send event */
  MESH_CFG_MDL_CL_MSG_RSP_TMR_EXPIRED,                              /*!< Response timer expired */
};

/*! Config Client database for remote servers */
typedef struct meshCfgMdlClRemCfgMdlSrDbEntry_tag
{
  meshAddress_t cfgMdlSrAddr;                      /*!< Configuration Server address */
  uint8_t       cfgMdlSrDevKey[MESH_KEY_SIZE_128]; /*!< Configuration Server Device Key */
  uint8_t       refCount;                          /*!< Number of requests using the entry */
} meshCfgMdlClRemCfgMdlSrDbEntry_t;

/*! Mesh Configuration Client Control Block */
typedef struct meshCfgMdlClCb_tag
{
  meshCfgMdlClCback_t           cback;             /*!< Upper layer procedure callback */
  meshCfgMdlClRemCfgMdlSrDbEntry_t *pCfgMdlSrDb;   /*!< Pointer to Configuration Server database
                                                     *   containing remote server address and Device Key
                                                     */
  wsfQueue_t                 opQueue;               /*!< Pending operations queue */
  uint16_t                   opTimeoutSec;          /*!< Operation timeout in seconds */
  uint16_t                   cfgMdlSrDbNumEntries;  /*!< Number of entries in the database */
  uint16_t                   rspTmrUidGen;          /*!< Response timer unique ID generator */
} meshCfgMdlClCb_t;

/*! Mesh Configuration Client Operation request parameters */
typedef struct meshCfgMdlClOpReqParams_tag
{
  void            *pNext;               /*!< Pointer to next element */
  meshAddress_t   cfgMdlSrAddr;         /*!< Configuration Server address */
  uint16_t        cfgMdlSrNetKeyIndex;  /*!< Identifier of the Network used to communicate with the
                                         *   server
                                         */
  wsfTimer_t      rspTmr;               /*!< Response timer timeout */
  meshCfgMdlClOpId_t reqOpId;           /*!< Request operation identifier */
  meshCfgMdlSrOpId_t rspOpId;           /*!< Response operation identifier */
  uint8_t         apiEvt;               /*!< API event. See ::meshCfgMdlEventValues */
} meshCfgMdlClOpReqParams_t;

/*! Mesh Configuration Client Operation Request */
typedef struct meshCfgMdlClOpReq_tag
{
  wsfMsgHdr_t            hdr;             /*!< Header structure */
  meshCfgMdlClOpReqParams_t *pReqParam;   /*!< Pointer to allocated request parameters */
  uint8_t                *pMsgParam;      /*!< Pointer to packed message parameters */
  uint16_t               msgParamLen;     /*!< Message parameters length in bytes */
} meshCfgMdlClOpReq_t;


/*! Mesh Configuration Client operation response action handler */
typedef bool_t (*meshCfgMdlClOpRspAct_t)(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

bool_t meshCfgMdlClAddToSrDbSafe(meshAddress_t cfgMdlSrAddr, const uint8_t *pDevKey);
void meshCfgMdlClRemFromSrDbSafe(meshAddress_t cfgMdlSrAddr);

void meshCfgMdlClEmptyCback(meshCfgMdlClEvt_t* pEvt);
void meshCfgMdlClWsfMsgHandlerCback(wsfMsgHdr_t *pMsg);

void meshCfgMdlClAccMsgRcvCback(uint8_t opcodeIdx, uint8_t *pMsgParam,
                                uint16_t msgParamLen, meshAddress_t src,
                                meshElementId_t elemId, uint8_t ttl,
                                uint16_t netKeyIndex);
bool_t meshCfgMdlClHandleBeaconStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen);
bool_t meshCfgMdlClHandleCompDataStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen);
bool_t meshCfgMdlClHandleDefaultTtlStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                          uint16_t msgParamLen);
bool_t meshCfgHandleGattProxyStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                       uint16_t msgParamLen);
bool_t meshCfgMdlClHandleRelayStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                     uint16_t msgParamLen);
bool_t meshCfgMdlClHandleModelPubStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen);
bool_t meshCfgMdlClHandleModelSubscrStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                           uint16_t msgParamLen);
bool_t meshCfgMdlClHandleModelSubscrSigList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                            uint16_t msgParamLen);
bool_t meshCfgMdlClHandleModelSubscrVendorList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                               uint16_t msgParamLen);
bool_t meshCfgMdlClHandleNetKeyStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen);
bool_t meshCfgMdlClHandleNetKeyList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                    uint16_t msgParamLen);
bool_t meshCfgMdlClHandleAppKeyStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen);
bool_t meshCfgMdlClHandleAppKeyList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                    uint16_t msgParamLen);
bool_t meshCfgMdlClHandleNodeIdentityStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                            uint16_t msgParamLen);
bool_t meshCfgMdlClHandleModelAppStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen);
bool_t meshCfgMdlClHandleModelAppSigList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                         uint16_t msgParamLen);
bool_t meshCfgMdlClHandleModelAppVendorList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                            uint16_t msgParamLen);
bool_t meshCfgMdlClHandleNodeResetStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                         uint16_t msgParamLen);
bool_t meshCfgMdlClHandleFriendStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen);
bool_t meshCfgHandleKeyRefPhaseStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen);
bool_t meshCfgMdlClHandleHbPubStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                     uint16_t msgParamLen);
bool_t meshCfgMdlClHandleHbSubStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                     uint16_t msgParamLen);
bool_t meshCfgMdlClHandleLpnPollTimeoutStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                              uint16_t msgParamLen);
bool_t meshCfgMdlClHandleNwkTransStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! Mesh Configuration Client control block */
extern meshCfgMdlClCb_t meshCfgMdlClCb;

/*! Mesh Configuration Client operation response action table. */
extern const meshCfgMdlClOpRspAct_t meshCfgMdlClOpRspActTbl[];

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_CL_MAIN_H */
