/*************************************************************************************************/
/*!
 *  \file   mesh_test_main.c
 *
 *  \brief  Mesh Stack Test module implementation.
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

#if defined(MESH_ENABLE_TEST)

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_os.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_replay_protection.h"
#include "mesh_sar_rx_history.h"
#include "mesh_lower_transport.h"
#include "mesh_upper_transport.h"
#include "mesh_local_config.h"
#include "mesh_network_beacon.h"

#include "mesh_test_api.h"

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*! Mesh Stack Test event notification callback */
static void meshTestEmptyCback (meshTestEvt_t *pEvt)
{
  (void)pEvt;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Stack Test module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshTestInit(void)
{
  meshTestCb.listenMask = MESH_TEST_LISTEN_OFF;
  meshTestCb.testCback = meshTestEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Mesh Stack Test events callback.
 *
 *  \param[in] meshTestCback  Callback function for Mesh Stack Test events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestRegister(meshTestCback_t meshTestCback)
{
  meshTestCb.testCback = meshTestCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Test Listen mask. Only masked events are reported.
 *
 *  \param[in] mask  Listen events mask to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestSetListenMask(uint16_t mask)
{
  meshTestCb.listenMask = mask;
}

/*************************************************************************************************/
/*!
 *  \brief  Clear Replay Protection list.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshTestRpClearList(void)
{
  /* Clear replay protection list. */
  MeshRpClearList();

  /* Clear SAR history. */
  MeshSarRxHistoryReset();
}

/*************************************************************************************************/
/*!
 *  \brief     Alters the NetKey list size to a lower value than the one set at compile time.
 *
 *  \param[in] listSize  NetKey list size.
 *
 *  \return    NetKey list size set.
 *
 *  \remarks   If a value larger than the one set at compile time or 0 is passed, the value set at
 *             compile time is restored.
 */
/*************************************************************************************************/
uint16_t MeshTestAlterNetKeyListSize(uint16_t listSize)
{
  if ((listSize != 0) && (listSize < pMeshConfig->pMemoryConfig->netKeyListSize))
  {
    MeshTestLocalCfgAlterNetKeyListSize(listSize);
    MeshTestSecAlterNetKeyListSize(listSize);

    return listSize;
  }

  /* Set actual netKey list size. */
  MeshTestLocalCfgAlterNetKeyListSize(pMeshConfig->pMemoryConfig->netKeyListSize);
  MeshTestSecAlterNetKeyListSize(pMeshConfig->pMemoryConfig->netKeyListSize);

  return pMeshConfig->pMemoryConfig->netKeyListSize;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh Control Message.
 *
 *  \param[in] src          SRC address
 *  \param[in] dst          DST address
 *  \param[in] netKeyIndex  NetKey index to be used for encrypting the Transport PDU
 *  \param[in] ttl          TTL to be used. If invalid, Default TTL will be used
 *  \param[in] opcode       Control Message opcode
 *  \param[in] ackRequired  Acknowledgement is requested for this PDU
 *  \param[in] pCtlPdu      Pointer to the Control PDU
 *  \param[in] pduLen       Size of the PDU
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestSendCtlMsg(meshAddress_t dstAddr, uint16_t netKeyIndex, uint8_t opcode, uint8_t ttl,
                        bool_t ackRequired, const uint8_t *pCtlPdu, uint16_t pduLen)
{
  meshUtrCtlPduInfo_t ctlPduInfo;

  /* Set primary element address as source address. */
  MeshLocalCfgGetAddrFromElementId(0, &ctlPduInfo.src);

  /* Set ctlPduInfo. */
  ctlPduInfo.dst = dstAddr;
  ctlPduInfo.netKeyIndex = netKeyIndex;
  ctlPduInfo.opcode = opcode;
  ctlPduInfo.ttl = ttl;
  ctlPduInfo.ackRequired = ackRequired;
  ctlPduInfo.pCtlPdu = pCtlPdu;
  ctlPduInfo.pduLen = pduLen;
  ctlPduInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  ctlPduInfo.ifPassthr = FALSE;
  ctlPduInfo.prioritySend = FALSE;

  /* Send CTL PDU. */
  MeshUtrSendCtlPdu(&ctlPduInfo);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends beacons on all available interfaces for one or all NetKeys as a result of a
 *             trigger.
 *
 *  \param[in] netKeyIndex Index of the NetKey that triggered the beacon sending or
 *                         0xFFFF for all netkeys.
 *  \return None.
 */
/*************************************************************************************************/
void MeshTestSendNwkBeacon(uint16_t netKeyIndex)
{
  MeshNwkBeaconTriggerSend(netKeyIndex);
}

#endif //defined(MESH_ENABLE_TEST)
