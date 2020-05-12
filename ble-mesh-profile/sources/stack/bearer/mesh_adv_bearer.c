/*************************************************************************************************/
/*!
 *  \file   mesh_adv_bearer.c
 *
 *  \brief  ADV bearer module implementation.
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

#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "cfg_mesh_stack.h"
#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"

#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_utils.h"
#include "mesh_bearer_defs.h"
#include "mesh_adv_bearer.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Invalid Advertising Bearer interface ID value */
#define MESH_ADV_INVALID_INTERFACE_ID                       0xFF

/*! Invalid index in array of advertising interfaces */
#define MESH_ADV_INVALID_INDEX                              MESH_ADV_MAX_INTERFACES

/*! Defines the bit mask for a valid interface */
#define MESH_ADV_VALID_INTERFACE_MASK                       0x0F

/*! Checks whether the interface id is a valid advertising interface id */
#define MESH_ADV_IS_VALID_INTERFACE_ID(id) \
        (MESH_UTILS_BITMASK_XCL(id, MESH_ADV_VALID_INTERFACE_MASK))

/*! Maximum ADV Bearer PDU buffer size */
#define MESH_ADV_MAX_PDU_SIZE                               31

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Structure containing information stored for each item in the queue */
typedef struct meshAdvQueuedItem_tag
{
  uint8_t       *pBrPdu;  /*!< Bearer PDU data */
  uint8_t       pduLen;   /*!< Bearer PDU length */
  meshAdvType_t advType;  /*!< Advertising type */
} meshAdvQueuedItem_t;

/*! Definition of the Advertising TX queue */
typedef struct meshAdvQueue_tag
{
  meshAdvQueuedItem_t queueItem[MESH_ADV_QUEUE_SIZE];  /*!< FIFO queue items */
  uint8_t             queueSize;                       /*!< Size of the FIFO queue */
  uint8_t             queueHead;                       /*!< Index of the first element in the
                                                        *   FIFO queue
                                                        */
} meshAdvQueue_t;

/*! Definition of the Advertising Interface data  */
typedef struct meshAdvInterface_tag
{
  meshAdvQueue_t  advTxQueue;  /*!< Queue used by the advertising bearer to store information about
                                *   the packets prepared by the network layer to send over-the-air
                                */
  meshAdvIfId_t   advIfId;     /*!< Unique identifier for the interface */
  bool_t          advIfBusy;   /*!< States whether the advertising interface is busy sending a
                                *   packet over-the-air
                                */
} meshAdvInterface_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Advertising Bearer control block */
static struct meshAdvCb_tag
{
  meshAdvPduSendCback_t     advPduSendCback;                  /*!< Send PDU to advertising module */
  meshAdvRecvCback_t        advPduRecvCback;                  /*!< Advertising PDU received callback
                                                               *   for bearer layer
                                                               */
  meshAdvEventNotifyCback_t advBrNotifCback;                  /*!< Event notification callback for
                                                               *   bearer layer
                                                               */
  meshAdvInterface_t  advInterfaces[MESH_ADV_MAX_INTERFACES]; /*!< Array of advertising interfaces
                                                               *   supported by the stack
                                                               */
} advBrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Searches for a registered advertising interface by the identifier in the
 *             advertising interfaces array.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *
 *  \return    Index in the array, or MESH_ADV_INVALID_INDEX.
 */
/*************************************************************************************************/
static uint8_t meshAdvGetAdvInterfaceById(meshAdvIfId_t advIfId)
{
  /* Interface identifier is always valid */
  WSF_ASSERT(MESH_ADV_IS_VALID_INTERFACE_ID(advIfId));

  /* Search the array for a matching advertising interface id*/
  for (uint8_t idx = 0; idx < MESH_ADV_MAX_INTERFACES; idx++)
  {
    if (advBrCb.advInterfaces[idx].advIfId == advIfId)
    {
      return idx;
    }
  }

  /* No entry was found. Return invalid index */
  return MESH_ADV_INVALID_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     Allocates an advertising PDU and sends it outside the Mesh Stack to be sent
 *             over-the-air.
 *
 *  \param[in] pAdvIf   Pointer to interface entry in the interface array
 *  \param[in] advType  ADV type received. See ::meshAdvType
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU
 *  \param[in] pduLen   Size of the Mesh ADV Bearer PDU
 *
 *  \return    TRUE if packet is transmited, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshAdvTransmitPacket(meshAdvInterface_t *pAdvIf, meshAdvType_t advType,
                                             const uint8_t *pBrPdu, uint8_t pduLen)
{
  meshAdvPduSendEvt_t evt;

  if ((pduLen + sizeof(advType) + sizeof(pduLen)) > MESH_ADV_MAX_PDU_SIZE)
  {
    MESH_TRACE_ERR1("MESH ADV BEARER: PDU too long %d", pduLen);
    return FALSE;
  }

  MESH_TRACE_INFO1("MESH ADV BEARER: Sending PDU of length %d", pduLen);

  evt.hdr.event = MESH_CORE_ADV_PDU_SEND_EVENT;
  evt.ifId = pAdvIf->advIfId;
  evt.adType = advType;
  evt.pAdvPdu = pBrPdu;
  evt.advPduLen = pduLen;

  /* Send PDU to advertising interface */
  advBrCb.advPduSendCback(&evt);

  /* Mark interface as busy */
  pAdvIf->advIfBusy = TRUE;

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the advertising TX queue.
 *
 *  \param[in] pQueue  Pointer to the queue structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAdvQueueInit(meshAdvQueue_t *pQueue)
{
  pQueue->queueHead = 0;
  pQueue->queueSize = 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Queues a bearer PDU in the specified TX queue of the advertising interface.
 *
 *  \param[in] pQueue   Pointer to the queue structure.
 *  \param[in] advType  ADV type received. See ::meshAdvType.
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen   Size of the Mesh ADV Bearer PDU.
 *
 *  \return    TRUE if queue is not full, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshAdvQueueAdd(meshAdvQueue_t *pQueue, meshAdvType_t advType,
                                       const uint8_t *pBrPdu, uint8_t pduLen)

{
  uint8_t tail;

  /* Check if queue is not full */
  if (pQueue->queueSize < MESH_ADV_QUEUE_SIZE)
  {
    tail = (pQueue->queueHead + pQueue->queueSize) % MESH_ADV_QUEUE_SIZE;

    /* Copy in queued data */
    pQueue->queueItem[tail].advType = advType;
    pQueue->queueItem[tail].pBrPdu = (uint8_t *)pBrPdu;
    pQueue->queueItem[tail].pduLen = pduLen;

    /* Increase queue size by one item */
    pQueue->queueSize++;

    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Peeks the head element in the specified TX queue.
 *
 *  \param[in]  pQueue      Pointer to the queue structure.
 *  \param[out] ppPeekItem  Pointer to the address of the peeked item.
 *
 *  \return     TRUE if queue is not empty, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshAdvQueuePeekHead(meshAdvQueue_t *pQueue, meshAdvQueuedItem_t **ppPeekItem)
{
  /* Check if queue is not empty */
  if (pQueue->queueSize > 0)
  {
    /* Peek head item */
    *ppPeekItem = &pQueue->queueItem[pQueue->queueHead];

    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Removes the head element from the specified TX queue.
 *
 *  \param[in] pQueue  Pointer to the queue structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAdvQueueRemoveHead(meshAdvQueue_t *pQueue)
{
  /* Check if queue is not empty */
  if (pQueue->queueSize > 0)
  {
    /* Move queue head */
    pQueue->queueHead++;

    /* Roll over queue head if we reach the maximum queue size */
    if (pQueue->queueHead == MESH_ADV_QUEUE_SIZE)
    {
      pQueue->queueHead = 0;
    }

    /* Decrease queue size by one item */
    pQueue->queueSize--;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Empties the TX queue of the specified advertising interface. This triggers
 *             sending events to the network layer for each queued item.
 *
 *  \param[in] pAdvIf   Pointer to interface entry in the interface array.
 *  \param[in] advType  ADV type received. See ::meshAdvType.
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen   Size of the Mesh ADV Bearer PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAdvEmptyQueue(meshAdvInterface_t *pAdvIf)
{
  meshAdvQueuedItem_t *pQueuedItem;

  /* Callback should be assigned at initialization */
  WSF_ASSERT(advBrCb.advBrNotifCback != NULL);

  /* Go through all queued items and send status to Network layer*/
  while (meshAdvQueuePeekHead(&pAdvIf->advTxQueue, &pQueuedItem))
  {
    meshAdvBrPduStatus_t pduStatus;

    /* Signal the Network layer that the packet has been processed by the bearer.
     * This will help the layer remove any references of this packet.
     */
    if (pQueuedItem->advType == MESH_AD_TYPE_PACKET || pQueuedItem->advType == MESH_AD_TYPE_PB ||
        pQueuedItem->advType == MESH_AD_TYPE_BEACON)
    {
      pduStatus.adType = pQueuedItem->advType;
      pduStatus.pPdu = pQueuedItem->pBrPdu;
      advBrCb.advBrNotifCback(pAdvIf->advIfId, MESH_ADV_PACKET_PROCESSED,
                              (meshAdvBrEventParams_t *)&pduStatus);
    }

    /* Remove peeked item from the queue */
    meshAdvQueueRemoveHead(&pAdvIf->advTxQueue);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Remove interface.
 *
 *  \param[in] entryIdx  Entry in the interfaces array.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAdvRemoveInterface(uint8_t entryIdx)
{
  /* Callback should be assigned at initialization */
  WSF_ASSERT(advBrCb.advBrNotifCback != NULL);

  /* Signal the network layer that the interface was closed */
  advBrCb.advBrNotifCback(advBrCb.advInterfaces[entryIdx].advIfId, MESH_ADV_INTERFACE_CLOSED, NULL);

  /* Empty Advertising interface queue */
  meshAdvEmptyQueue(&(advBrCb.advInterfaces[entryIdx]));

  /* Reset information for the specified advertising interface */
  advBrCb.advInterfaces[entryIdx].advIfId = MESH_ADV_INVALID_INTERFACE_ID;
}

/*************************************************************************************************/
/*!
 *  \brief     Empty notification callback to upper layer.
 *
 *  \param[in] ifId          Interface Id.
 *  \param[in] event         Event.
 *  \param[in] pEventParams  Event parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void advBrEmptyNotifCback(meshAdvIfId_t ifId, meshAdvEvent_t event,
                                 const meshAdvBrEventParams_t *pEventParams)
{
  (void)ifId;
  (void)event;
  (void)pEventParams;
  MESH_TRACE_ERR0("MESH ADV BEARER: Notif cback not registered ");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty PDU receive callback to upper layer.
 *
 *  \param[in] advIfId  Interface Id.
 *  \param[in] advType  Advertising Type.
 *  \param[in] pBrPdu   Pointer to bearer PDU.
 *  \param[in] pduLen   Length of the PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void advBrEmptyPduRecvCback(meshAdvIfId_t advIfId, meshAdvType_t advType,
                                   const uint8_t *pBrPdu, uint8_t pduLen)
{
  (void)advIfId;
  (void)advType;
  (void)pBrPdu;
  (void)pduLen;
  MESH_TRACE_ERR0("MESH ADV BEARER: PDU receive cback not registered ");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty PDU send callback to external module.
 *
 *  \param[in] advIfId  Interface Id.
 *  \param[in] advType  Advertising Type.
 *  \param[in] pBrPdu   Pointer to bearer PDU.
 *  \param[in] pduLen   Length of the PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void advBrEmptyPduSendCback(meshAdvPduSendEvt_t *pEvt)
{
  (void)pEvt;
  MESH_TRACE_ERR0("MESH ADV BEARER: PDU send cback not registerd ");
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh ADV Bearer layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAdvInit(void)
{
  MESH_TRACE_INFO0("MESH ADV BEARER: init");

  /* Set callbacks */
  advBrCb.advBrNotifCback = advBrEmptyNotifCback;
  advBrCb.advPduRecvCback = advBrEmptyPduRecvCback;
  advBrCb.advPduSendCback = advBrEmptyPduSendCback;

  /* Initialize the interfaces array */
  for (uint8_t i = 0; i < MESH_ADV_MAX_INTERFACES; i++)
  {
    /* Empty Advertising interface queue */
    meshAdvQueueInit(&advBrCb.advInterfaces[i].advTxQueue);

    /* Reset information for the specified advertising interface */
    advBrCb.advInterfaces[i].advIfId = MESH_ADV_INVALID_INTERFACE_ID;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Register upper layer callbacks.
 *
 *  \param[in] recvCback   Callback to be invoked when a Mesh ADV PDU is received on a specific ADV
 *                         interface
 *  \param[in] notifCback  Callback to be invoked when an event on a specific advertising interface
 *                         is triggered
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvRegister(meshAdvRecvCback_t pduRecvCback, meshAdvEventNotifyCback_t evtCback)
{
  if ((evtCback != NULL) && (pduRecvCback != NULL))
  {
    advBrCb.advBrNotifCback = evtCback;
    advBrCb.advPduRecvCback = pduRecvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Register callback to send PDU to bearer module.
 *
 *  \param[in] cback  Callback to be invoked to send a Mesh ADV PDU outside the stack.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvRegisterPduSendCback(meshAdvPduSendCback_t cback)
{
  if (cback != NULL)
  {
    advBrCb.advPduSendCback = cback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Allocates a Mesh ADV bearer instance.
 *
 *  \param[in] advIfId          Unique identifier for the interface.
 *  \param[in] statusCback      Status callback for the advIfId interface.
 *  \param[in] advPduRecvCback  Callback invoked by the Mesh Stack to send an advertising PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvAddInterface(meshAdvIfId_t advIfId)
{
  meshAdvIfEvt_t evt =
  {
    .hdr.event = MESH_CORE_EVENT,
    .hdr.status = MESH_SUCCESS,
    .hdr.param = MESH_CORE_ADV_IF_ADD_EVENT,
    .ifId = advIfId
  };
  uint8_t emptyIdx;

  MESH_TRACE_INFO1("MESH ADV BEARER: adding interface id %d", advIfId);

  /* Interface Id should have a valid value */
  WSF_ASSERT(MESH_ADV_IS_VALID_INTERFACE_ID(advIfId));

  /* Check for duplicate advertising interface id*/
  if (meshAdvGetAdvInterfaceById(advIfId) != MESH_ADV_INVALID_INDEX)
  {
    MESH_TRACE_WARN1("MESH ADV BEARER: duplicate interface id %d", advIfId);

    /* Set event status to error. */
    evt.hdr.status = MESH_ALREADY_EXISTS;
  }
  else
  {
    /* Search for an empty entry */
    for (emptyIdx = 0; emptyIdx < MESH_ADV_MAX_INTERFACES; emptyIdx++)
    {
      if (advBrCb.advInterfaces[emptyIdx].advIfId == MESH_ADV_INVALID_INTERFACE_ID)
      {
        /* Empty entry found. Populate information */
        advBrCb.advInterfaces[emptyIdx].advIfId = advIfId;

        /* Initially, the interface is not busy sending packets */
        advBrCb.advInterfaces[emptyIdx].advIfBusy = TRUE;

        /* Initialize advertising interface queue */
        meshAdvQueueInit(&(advBrCb.advInterfaces[emptyIdx].advTxQueue));

        /* Signal the network layer that the interface was opened */
        advBrCb.advBrNotifCback(advIfId, MESH_ADV_INTERFACE_OPENED, NULL);

        /* Entry found. Break search sequence */
        break;
      }
    }

    /* No empty interface was found */
    if (emptyIdx == MESH_ADV_MAX_INTERFACES)
    {
      /* Set event status to error. */
      evt.hdr.status = MESH_NO_RESOURCES;
    }
  }

  /* Trigger generic callback. */
  meshCb.evtCback((meshEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Frees a Mesh ADV bearer instance.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvRemoveInterface(meshAdvIfId_t advIfId)
{
  meshAdvIfEvt_t evt =
  {
    .hdr.event = MESH_CORE_EVENT,
    .hdr.status = MESH_SUCCESS,
    .hdr.param = MESH_CORE_ADV_IF_REMOVE_EVENT,
    .ifId = advIfId
  };
  uint8_t entryIdx;

  MESH_TRACE_WARN1("MESH ADV BEARER: removing interface id %d", advIfId);

  /* Interface Id should have a valid value */
  WSF_ASSERT(MESH_ADV_IS_VALID_INTERFACE_ID(advIfId));

  /* Get entry index in the advertising interface array */
  entryIdx = meshAdvGetAdvInterfaceById(advIfId);

  /* If interface is not found, return error */
  if (entryIdx == MESH_ADV_INVALID_INDEX)
  {
    /* Set event status to error. */
    evt.hdr.status = MESH_INVALID_PARAM;
  }
  else
  {
    meshAdvRemoveInterface(entryIdx);
  }

  /* Trigger generic callback. */
  meshCb.evtCback((meshEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Processes an ADV PDU received on an ADV instance.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *  \param[in] pAdvPdu  Pointer to a buffer containing an ADV PDU.
 *  \param[in] pduLen   Size of the ADV PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvProcessPdu(meshAdvIfId_t advIfId, const uint8_t *pAdvPdu, uint8_t pduLen)
{
  meshAdvType_t advType;

  MESH_TRACE_INFO1("Receiving PDU of length %d on advertising interface", pduLen);

  MESH_TRACE_INFO1("MESH ADV BEARER: Receiving PDU of length %d", pduLen);

  /* Extract mesh AD type */
  advType = pAdvPdu[MESH_ADV_TYPE_POS];

  /* Check for valid mesh AD type value. AD type should be Mesh Beacon, PB-ADV or Mesh Packet */
  if((advType >= MESH_AD_TYPE_PB) && (advType <= MESH_AD_TYPE_BEACON))
  {
    /* Interface Id should have a valid value */
    WSF_ASSERT(MESH_ADV_IS_VALID_INTERFACE_ID(advIfId));

    /* Check if advertising interface is valid */
    if (meshAdvGetAdvInterfaceById(advIfId) != MESH_ADV_INVALID_INDEX)
    {
      /* Extract advertising data and send it as a Bearer PDU to the Bearer layer.
       * First octet is AD type. Decrease PDU length by size of AD type and AD length.
       */
      advBrCb.advPduRecvCback(advIfId, advType, &pAdvPdu[MESH_ADV_PDU_POS],
                              pduLen - (sizeof(uint8_t) + sizeof(meshAdvType_t)));
      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Signals the Advertising Bearer that the interface is ready to transmit packets.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvSignalInterfaceReady(meshAdvIfId_t advIfId)
{
  meshAdvIfEvt_t evt =
  {
    .hdr.event = MESH_CORE_EVENT,
    .hdr.status = MESH_SUCCESS,
    .hdr.param = MESH_CORE_ADV_SIGNAL_IF_RDY_EVENT,
    .ifId = advIfId
  };

  uint8_t advIfIndex;
  meshAdvQueuedItem_t *pQueuedItem = NULL;
  meshAdvBrPduStatus_t pduStatus;

  /* Interface Id should have a valid value */
  WSF_ASSERT(MESH_ADV_IS_VALID_INTERFACE_ID(advIfId));

  /* Get interface ID */
  advIfIndex = meshAdvGetAdvInterfaceById(advIfId);

  /* Check if advertising interface ID is valid */
  if (advIfIndex == MESH_ADV_INVALID_INDEX)
  {
    /* Set event status to error. */
    evt.hdr.status = MESH_INVALID_INTERFACE;
  }
  else
  {
    /* The packet from the head of the queue was sent. Peek into it */
    if (meshAdvQueuePeekHead(&(advBrCb.advInterfaces[advIfIndex].advTxQueue), &pQueuedItem))
    {
      /* Signal the Network layer or Provisioning Bearer that the packet has been processed by the
       * ADV bearer. This will help the layer remove any references of this packet.
       */
      if (pQueuedItem->advType == MESH_AD_TYPE_PACKET || pQueuedItem->advType == MESH_AD_TYPE_PB ||
          pQueuedItem->advType == MESH_AD_TYPE_BEACON)
      {
        pduStatus.adType = pQueuedItem->advType;
        pduStatus.pPdu = pQueuedItem->pBrPdu;
        advBrCb.advBrNotifCback(advIfId, MESH_ADV_PACKET_PROCESSED,
                                (meshAdvBrEventParams_t *)&pduStatus);
      }

      /* Remove peeked item from the queue */
      meshAdvQueueRemoveHead(&(advBrCb.advInterfaces[advIfIndex].advTxQueue));
    }

    /* Peek next item. If found, send it over-the-air */
    if (meshAdvQueuePeekHead(&(advBrCb.advInterfaces[advIfIndex].advTxQueue), &pQueuedItem))
    {
      /* Interface is available for sending packets over-the-air */
      if (!meshAdvTransmitPacket(&(advBrCb.advInterfaces[advIfIndex]), pQueuedItem->advType,
                                  pQueuedItem->pBrPdu, pQueuedItem->pduLen))
      {
        /* Transmit failed. Empty Advertising interface queue */
        meshAdvEmptyQueue(&(advBrCb.advInterfaces[advIfIndex]));
      }
    }
    else
    {
      /* No more queued items. Mark interface as not busy */
      advBrCb.advInterfaces[advIfIndex].advIfBusy = FALSE;
    }
  }

  /* Trigger generic callback. */
  meshCb.evtCback((meshEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Bearer PDU on an ADV bearer instance.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *  \param[in] advType  ADV type received. See ::meshAdvType
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen   Size of the Mesh ADV Bearer PDU.
 *
 *  \return    TRUE if message is sent or queued for later transmission, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshAdvSendBrPdu(meshAdvIfId_t advIfId, meshAdvType_t advType, const uint8_t *pBrPdu,
                        uint8_t pduLen)
{
  uint8_t advIfIndex;

  /* Interface Id should have a valid value */
  WSF_ASSERT(MESH_ADV_IS_VALID_INTERFACE_ID(advIfId));

  /* Check for valid input data */
  if ((pBrPdu == NULL) || (pduLen == 0))
  {
    return FALSE;
  }

  /* Check for valid AD type */
  if ((advType < MESH_AD_TYPE_PB) || (advType > MESH_AD_TYPE_BEACON))
  {
    return FALSE;
  }

  /* Get interface ID */
  advIfIndex = meshAdvGetAdvInterfaceById(advIfId);

  /* Check if advertising interface ID is valid */
  if (advIfIndex == MESH_ADV_INVALID_INDEX)
  {
    MESH_TRACE_ERR0("Mesh ADV: Invalid Interface ID. ");
    return FALSE;
  }

  /* Queue incoming message */
  if (meshAdvQueueAdd(&(advBrCb.advInterfaces[advIfIndex].advTxQueue), advType, pBrPdu, pduLen))
  {
    /* Check availability of interface*/
    if (!(advBrCb.advInterfaces[advIfIndex].advIfBusy))
    {
      /* Interface is available for sending the packet over-the-air */
      return meshAdvTransmitPacket(&(advBrCb.advInterfaces[advIfIndex]), advType, pBrPdu, pduLen);
    }
    else
    {
      /* Packet remains queued */
      return TRUE;
    }
  }

  /* Packet cannot be sent or queued */
  return FALSE;
}
