/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_messages.h
 *
 *  \brief  Configuration Messages internal interface.
 *
 *  Copyright (c) 2010-2018 Arm Ltd.
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

#ifndef MESH_CFG_MDL_MESSAGES_H
#define MESH_CFG_MDL_MESSAGES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_types.h"
#include "util/bstream.h"
#include "mesh_cfg_mdl_defs.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! BSTREAM wrapper for serializing vendor model ID's */
#define VEND_MDL_TO_BSTREAM(p, n) { UINT16_TO_BSTREAM((p), MESH_VENDOR_MODEL_ID_TO_COMPANY_ID((n)));\
                                    UINT16_TO_BSTREAM((p), MESH_VENDOR_MODEL_ID_TO_MODEL_ID((n))); }

/*! BSTREAM wrapper for deserializing vendor model ID's */
#define BSTREAM_TO_VEND_MDL(n, p) { uint16_t c, m;\
                                    BSTREAM_TO_UINT16(c, (p)); BSTREAM_TO_UINT16(m, (p));\
                                    n = MESH_VENDOR_MODEL_MK(c, m);}

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

uint8_t meshCfgMsgPackSingleKeyIndex(uint8_t *pBuf, uint16_t keyIndex);

uint8_t meshCfgMsgPackTwoKeyIndex(uint8_t *pBuf, uint16_t keyIndex1, uint16_t keyIndex2);

uint8_t meshCfgMsgUnpackSingleKeyIndex(uint8_t *pBuf, uint16_t *pKeyIndex);

uint8_t meshCfgMsgUnpackTwoKeyIndex(uint8_t *pBuf, uint16_t *pKeyIndex1, uint16_t *pKeyIndex2);

void meshCfgMsgUnpackCompData(uint8_t *pBuf, uint16_t dataLength, meshCompData_t *pCompData);

uint16_t meshCfgMsgGetPackedCompDataPg0Size(void);

void meshCfgMsgPackCompData(uint8_t *pBuf, uint8_t pageNumber);

void meshCfgMsgPackRelay(uint8_t *pBuf, meshRelayStates_t *pRelayState,
                         meshRelayRetransState_t *pRetranState);

void meshCfgMsgUnpackRelay(uint8_t *pBuf, meshRelayStates_t *pRelayState,
                           meshRelayRetransState_t *pRetranState);

void meshCfgMsgPackModelPubGet(uint8_t *pBuf, meshAddress_t elemAddr, meshSigModelId_t sigId,
                               meshVendorModelId_t vendorId, bool_t isSig);

void meshCfgMsgUnpackModelPubGet(uint8_t *pBuf, meshAddress_t *pElemAddr, meshSigModelId_t *pSigId,
                                 meshVendorModelId_t *pVendorId, bool_t isSig);

void meshCfgMsgPackModelPubParam(uint8_t *pBuf, meshModelPublicationParams_t *pParams,
                                 meshSigModelId_t sigId, meshVendorModelId_t vendorId,
                                 bool_t isSig);

void meshCfgMsgUnpackModelPubParam(uint8_t *pBuf, meshModelPublicationParams_t *pParams,
                                   meshSigModelId_t *pSigId, meshVendorModelId_t *pVendorId,
                                   bool_t isSig);

void meshCfgMsgPackModelAppBind(uint8_t *pBuf, meshAddress_t elemAddr,
                                uint16_t appKeyIndex, meshSigModelId_t sigModelId,
                                meshVendorModelId_t vendorModelId, bool_t isSig);

void meshCfgMsgUnpackModelAppBind(uint8_t *pBuf, meshAddress_t *pElemAddr,
                                  uint16_t *pAppKeyIndex, meshSigModelId_t *pSigModelId,
                                  meshVendorModelId_t *pVendorModelId, bool_t isSig);

void meshCfgMsgPackHbPub(uint8_t *pBuf, meshHbPub_t *pHbPub);

void meshCfgMsgUnpackHbPub(uint8_t *pBuf, meshHbPub_t *pHbPub);

void meshCfgMsgPackHbSubSet(uint8_t *pBuf, meshHbSub_t *pHbSub);

void meshCfgMsgUnpackHbSubSet(uint8_t *pBuf, meshHbSub_t *pHbSub);

void meshCfgMsgPackHbSubState(uint8_t *pBuf, meshHbSub_t *pHbSub);

void meshCfgMsgUnpackHbSubState(uint8_t *pBuf, meshHbSub_t *pHbSub);

void meshCfgMsgPackLpnPollTimeout(uint8_t *pBuf, meshAddress_t addr, uint32_t time);

void meshCfgMsgUnpackLpnPollTimeout(uint8_t *pBuf, meshAddress_t *pAddr, uint32_t *pTime);

void meshCfgMsgPackNwkTrans(uint8_t *pBuf, meshNwkTransState_t *pState);

void meshCfgMsgUnpackNwkTrans(uint8_t *pBuf, meshNwkTransState_t *pState);

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_MESSAGES_H */
