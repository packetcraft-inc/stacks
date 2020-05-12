/*************************************************************************************************/
/*!
 *  \file   mmdl_vendor_test_cl_main.c
 *
 *  \brief  Implementation of the Vendor Test Client model.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_os.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mmdl_common.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_vendor_test_cl_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Initializer of a message info for the specified model ID and opcode */
#define MSG_INFO(modelId, opcode) {{(meshVendorModelId_t)modelId }, {opcode},\
                                    0xFF, NULL, MESH_ADDR_TYPE_UNASSIGNED, 0xFF, 0xFF}

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Vendor Test Client control block type definition */
typedef struct mmdlVendorTestClCb_tag
{
  mmdlVendorTestClRecvCback_t recvCback;    /*!< Model Vendor Test received callback */
} mmdlVendorTestClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlVendorTestClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlVendorTestClRcvdOpcodes[] =
{
  { {UINT24_OPCODE_TO_BYTES(MMDL_VENDOR_TEST_STATUS_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Vendor Test Client control block */
static mmdlVendorTestClCb_t  vendorTestClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Model Vendor Test received callback
 *
 *  \param[in] pEvent  Pointer to Vendor Test Client Event
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlVendorTestClRecvEmptyCback(const mmdlVendorTestClEvent_t *pEvent)
{
  MESH_TRACE_WARN0("VENDOR TEST CL: Receive callback not set!");
  (void)pEvent;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Vendor Test Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlVendorTestClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlVendorTestClStatusEvent_t event;

  /* Set event type and status */
  event.hdr.event = MMDL_VENDOR_TEST_CL_STATUS_EVENT;
  event.hdr.status = MMDL_VENDOR_TEST_CL_SUCCESS;

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;
  event.pMsgParams = pMsg->pMessageParams;
  event.ttl = pMsg->ttl;
  event.messageParamsLen = pMsg->messageParamsLen;

  /* Send event to the upper layer */
  vendorTestClCb.recvCback((mmdlVendorTestClEvent_t *)&event);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for On Off Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlVendorTestClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlVendorTestClHandlerId = handlerId;

  /* Initialize control block */
  vendorTestClCb.recvCback = mmdlVendorTestClRecvEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for On Off Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlVendorTestClHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;


  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        /* Process Status message */
        mmdlVendorTestClHandleStatus(pModelMsg);
        break;

      default:
        MESH_TRACE_WARN0("VENDOR TEST CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlVendorTestClRegister(mmdlVendorTestClRecvCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    vendorTestClCb.recvCback = recvCback;
  }
}
