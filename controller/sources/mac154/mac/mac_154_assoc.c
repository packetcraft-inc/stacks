/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  802.15.4 MAC association.
 *
 *  Copyright (c) 2013-2018 Arm Ltd.
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

#include "wsf_types.h"
#include "wsf_buf.h"
#include "util/bstream.h"
#include "pal_bb.h"
#include "bb_154_int.h"
#include "bb_154.h"
#include "chci_154_int.h"
#include "mac_154_int.h"
#include "mac_154_defs.h"
#include "sch_api.h"
#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Data
**************************************************************************************************/

/* TODO: Control block needed? Multiple instances? */

/**************************************************************************************************
  Forward declarations
**************************************************************************************************/

/**************************************************************************************************
  Subroutines
**************************************************************************************************/


/**************************************************************************************************
  BOD cleanup
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Cleanup test BOD at end of association request
 *
 *  \param      pOp    Pointer to the executing BOD.
 *
 *  \return     None.
 *  \note       Called from scheduler context, not ISR.
 */
/*************************************************************************************************/
static void mac154AssocEndCback(BbOpDesc_t *pOp)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Assoc_t * const pAssoc = &p154->op.assoc;

  switch (pAssoc->cmd)
  {
    case MAC_154_CMD_FRAME_TYPE_ASSOC_REQ:
    case MAC_154_CMD_FRAME_TYPE_DATA_REQ:
      {
        /* Cases D1aNG and D1bNG */
        Chci154AssocSendAssocCfm(pPib->shortAddr, pAssoc->status);
      }
      break;
#if MAC_154_OPT_DISASSOC
    case MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF:
      {
        /* Case D3 */
        Chci154AssocSendDisassocCfm(&p154->op.disassoc.deviceAddr, pAssoc->status);
        if ((pPib->deviceType == MAC_154_DEV_TYPE_DEVICE) &&
            (pAssoc->status != MAC_154_ENUM_CHANNEL_ACCESS_FAILURE))
        {
          /* If device (not coordinator), set PIB attributes to defaults. [SR 182,3] */
          pPib->panId = MAC_154_PIB_PAN_ID_DEF;
          pPib->shortAddr = MAC_154_PIB_SHORT_ADDRESS_DEF;
          pPib->associatedPanCoord = MAC_154_PIB_ASSOCIATED_PAN_COORD_DEF;
          pPib->coordShortAddr = MAC_154_PIB_COORD_SHORT_ADDRESS_DEF;
          pPib->coordExtAddr = 0;
        }
      }
      break;
#endif /* MAC_154_OPT_DISASSOC */
    default:
      break;
  }

  /* Stop 15.4 baseband operation */
  BbStop(BB_PROT_15P4);
  if (pAssoc->pTxDesc != NULL)
  {
    WsfBufFree(pAssoc->pTxDesc);
  }
  WsfBufFree(p154);
  WsfBufFree(pOp);
}

/*************************************************************************************************/
/*!
 *  \brief      Start association operation
 *
 *  \param      p154          15.4 operation to (re)start
 *
 *  \return     Status code.
 */
/*************************************************************************************************/
static uint8_t mac154AssocStartOp(void *pPendingTxHandle)
{
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();
  BbOpDesc_t *pOp;
  Bb154Data_t * const p154 = (Bb154Data_t *)pPendingTxHandle;

  /* Allocate storage for data transmit BOD */
  if ((pOp = WsfBufAlloc(sizeof(BbOpDesc_t))) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }
  memset(pOp, 0, sizeof(BbOpDesc_t));
  pOp->prot.p154 = p154;

  /* Initialize association BOD protocol */
  pOp->reschPolicy = BB_RESCH_MOVEABLE_PREFERRED;
  pOp->protId = BB_PROT_15P4;
  pOp->endCback = mac154AssocEndCback;
  pOp->abortCback = mac154AssocEndCback;

  /* Set the 802.15.4 operation type */
  p154->opType = BB_154_OP_ASSOC;

  /* Set 802.15.4 operational parameters */
  p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK;
  p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

  /* Set baseband operation */
  Bb154AssocInit();

  /* Claim baseband for 15.4 use */
  BbStart(BB_PROT_15P4);

  p154->chan.channel = pPhyPib->chan;
  p154->chan.txPower = pPhyPib->txPower;
  p154->op.assoc.status = MAC_154_ENUM_TRANSACTION_OVERFLOW;/* Default status if aborted early */
  SchInsertNextAvailable(pOp);
  return MAC_154_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      Start association request.
 *
 *  \param      pCoordAddr    Pointer to coordinator address
 *  \param      capInfo       Capability information
 *
 *  \return     Result code
 */
/*************************************************************************************************/
uint8_t Mac154AssocReqStart(Mac154Addr_t *pCoordAddr, uint8_t capInfo)
{
  Bb154Data_t *p154;
  PalBb154TxBufDesc_t *pDesc;
  Bb154AssocReq_t *pAssocReq;

  /* Allocate storage for data transmit BOD's 15.4 specific data */
  if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) != NULL)
  {
    memset(p154, 0, sizeof(Bb154Data_t));
  }
  else
  {
    return MAC_154_ERROR;
  }

  /* Build association request to Tx */
  if ((pDesc = Bb154BuildAssocReq(pCoordAddr, capInfo)) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }

  /* Store the general association parameters */
  pAssocReq = &p154->op.assocReq;
  pAssocReq->assoc.cmd = MAC_154_CMD_FRAME_TYPE_ASSOC_REQ;
  pAssocReq->assoc.pTxDesc = pDesc;
  /* Store the specific association request parameters */
  pAssocReq->coordAddr = *pCoordAddr;

  /* Start the baseband operation */
  return mac154AssocStartOp(p154);
}

/*************************************************************************************************/
/*!
 *  \brief      Start association response.
 *
 *  \param      deviceAddr    Device address
 *  \param      assocShtAddr  Associated short address
 *  \param      status        Status of association request
 *
 *  \return     Result code
 */
/*************************************************************************************************/
uint8_t Mac154AssocRspStart(uint64a_t deviceAddr, uint16a_t assocShtAddr, uint8_t status)
{
  PalBb154TxBufDesc_t *pTxDesc;

  /*
   * Do not start any operation. The frame is simply built and queued
   * in the indirect queue.
   */
  if ((pTxDesc = Bb154BuildAssocRsp(deviceAddr, assocShtAddr, status)) == NULL)
  {
    return MAC_154_ERROR;
  }
  pTxDesc->handle = 0;
  Bb154QueueTxIndirectBuf(pTxDesc);

  /* Schedule data receive for the poll */
  Mac154ScheduleDataRx();
  return MAC_154_SUCCESS;
}

#if MAC_154_OPT_DISASSOC

/*************************************************************************************************/
/*!
 *  \brief      Compare device address with Coordinator PIB address [SR 87,39]
 *
 *  \param      pDevAddr      Device address
 *
 *  \return     TRUE: Matches coordinator address.
 */
/*************************************************************************************************/
bool_t Mac154AssocDisassocToCoord(Mac154Addr_t *pDevAddr)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();

  if (pDevAddr->addrMode == MAC_154_ADDR_MODE_SHORT)
  {
    uint16_t shortAddr;
    BYTES_TO_UINT16(shortAddr, pDevAddr->addr);
    if (shortAddr == pPib->coordShortAddr)
    {
      return TRUE;
    }
  }
  else if (pDevAddr->addrMode == MAC_154_ADDR_MODE_EXTENDED)
  {
    uint64_t extAddr;
    BYTES_TO_UINT64(extAddr, pDevAddr->addr);
    if (extAddr == pPib->coordExtAddr)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Start disassociation.
 *
 *  \param      pDevAddr      Device address
 *  \param      reason        Disassociation reason
 *  \param      txIndirect    Whether to send indirect or not
 *  \param      toCoord       TRUE if directed at coordinator
 *
 *  \return     Result code
 */
/*************************************************************************************************/
uint8_t Mac154AssocDisassocStart(Mac154Addr_t *pDevAddr, uint8_t reason, uint8_t txIndirect, bool_t toCoord)
{
  PalBb154TxBufDesc_t *pTxDesc;

  if (txIndirect && !toCoord)
  {
    /*
     * Do not start any operation. The frame is simply built and queued
     * in the indirect queue.
     */
    if ((pTxDesc = Bb154BuildDisassocNtf(pDevAddr, reason)) == NULL)
    {
      return MAC_154_ERROR;
    }
    pTxDesc->handle = 0;
    Bb154QueueTxIndirectBuf(pTxDesc);
  }
  else
  {
    Bb154Data_t *p154;
    Bb154Disassoc_t *pDisassoc;

    /* Allocate storage for data transmit BOD's 15.4 specific data */
    if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) != NULL)
    {
      memset(p154, 0, sizeof(Bb154Data_t));
    }
    else
    {
      return MAC_154_ERROR;
    }

    /* Build disassociation notification to Tx */
    if ((pTxDesc = Bb154BuildDisassocNtf(pDevAddr, reason)) == NULL)
    {
      WsfBufFree(p154);
      return MAC_154_ERROR;
    }

    /* Store the general association parameters */
    pDisassoc = &p154->op.disassoc;
    pDisassoc->assoc.cmd = MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF;
    pDisassoc->assoc.pTxDesc = pTxDesc;
    /* Store the specific disassociation parameters */
    pDisassoc->deviceAddr = *pDevAddr;

    /* Start the baseband operation */
    return mac154AssocStartOp(p154);
  }
  return MAC_154_SUCCESS;
}

#endif /* MAC_154_OPT_DISASSOC */

/*************************************************************************************************/
/*!
 *  \brief      Initialize MAC association
 *
 *  \param      None.
 *
 *  \return     None.
 *
 *  Initializes MAC scan control block
 */
/*************************************************************************************************/
void Mac154AssocInit(void)
{
  /* Nothing to do - yet */
}

