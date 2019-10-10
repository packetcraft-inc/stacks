/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband: Frame assembly subroutines.
 *
 *  Copyright (c) 2016-2018 Arm Ltd.
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
#include "util/bstream.h"
#include "bb_154_api_op.h"
#include "bb_154_int.h"
#include "mac_154_defs.h"
#include "mac_154_int.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include <string.h>

#define ADDITIONAL_TESTER_FEATURES

static const uint8_t amSizeLUT[4] = {0,0,2,8};

/*************************************************************************************************/
/*!
 *  \brief      Build Beacon frame
 *
 *  \param      None.
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildBeacon(void)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t fctl;
  uint8_t tmp;
  uint8_t srcAddrMode;

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) +
                                                    MAC_154_BCN_FRAME_LEN_HDR +
                                                    pPib->beaconPayloadLength)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  if ((pPib->shortAddr == 0xFFFF) || (pPib->shortAddr == 0xFFFE))
  {
    srcAddrMode = MAC_154_ADDR_MODE_EXTENDED;
  }
  else
  {
    srcAddrMode = MAC_154_ADDR_MODE_SHORT;
  }

  /* Frame control:
        Beacon frame
        Sec. enabled = 0
        Frame pending = 0
        Ack. requested = 0
        PAN ID compression = 0
        Dst Addr mode = 0 (none)
        Frame version = 0
        Src Addr mode
  */
  fctl = MAC_154_FRAME_TYPE_BEACON |
         (srcAddrMode << MAC_154_FC_SRC_ADDR_MODE_SHIFT);

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number put in later */
  pTxFrame++;

  /* Destination PAN ID: None */
  /* Destination address: None */

  /* Source PAN ID: From PIB */
  UINT16_TO_BSTREAM(pTxFrame, pPib->panId);

  /* Source address: From PIB */
  if (srcAddrMode == MAC_154_ADDR_MODE_EXTENDED)
  {
    UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);
  }
  else
  {
    UINT16_TO_BSTREAM(pTxFrame, pPib->shortAddr);
  }

  /* Superframe spec. LSB: */
  UINT8_TO_BSTREAM(pTxFrame, 0xFF); /* TODO: Magic number (BO=15, SO=15) */

  /* Superframe spec. MSB: */
  tmp = 0xF; /* Final CAP slot = 15 */
  if (pPib->associationPermit)
  {
    tmp |= 0x80; /* TODO: magic number */
  }
  if (pPib->deviceType == MAC_154_DEV_TYPE_PAN_COORD)
  {
    tmp |= 0x40; /* TODO: magic number */
  }
  UINT8_TO_BSTREAM(pTxFrame, tmp); /* TODO: Magic number (BO=15, SO=15) */

  /* GTS specification field: Always 0 */
  UINT8_TO_BSTREAM(pTxFrame, 0x00); /* TODO: Magic number */

  /* Pending address field: Always 0 */
  UINT8_TO_BSTREAM(pTxFrame, 0x00); /* TODO: Magic number */

  /* Beacon payload */
  memcpy(pTxFrame, pPib->beaconPayload, pPib->beaconPayloadLength);
  pTxFrame += pPib->beaconPayloadLength;

  pTxDesc->len = pTxFrame - PAL_BB_154_TX_FRAME_PTR(pTxDesc);
  return pTxDesc;
}

/*************************************************************************************************/
/*!
 *  \brief      Build data frame
 *
 *  \param      len           Length
 *  \param      srcAddrMode   Source address mode
 *  \param      pDstAddr      Pointer to destination address
 *  \param      txOptions     Bitmap of transmit options
 *  \param      msduLen       Length of MSDU
 *  \param      pMsdu         Pointer to MSDU
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildDataFrame(uint8_t len,
                                         uint8_t srcAddrMode,
                                         Mac154Addr_t *pDstAddr,
                                         uint8_t txOptions,
                                         uint8_t msduLen,
                                         uint8_t *pMsdu)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t fctl;
  uint16_t dstPanId;

  /* Allocate data for Tx frame */
  if ((pTxDesc = WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) + len)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  BYTES_TO_UINT16(dstPanId, pDstAddr->panId);

  /* Frame control */
#ifdef ADDITIONAL_TESTER_FEATURES
  if (pPib->vsFctlOverride)
  {
    fctl = pPib->vsFctlOverride;
  }
  else
#endif
  {
    fctl = MAC_154_FRAME_TYPE_DATA;

    if ((txOptions & 1) &&
        (!((pDstAddr->addrMode == MAC_154_ADDR_MODE_SHORT) &&
            (pDstAddr->addr[0] == 0xFF) &&
            (pDstAddr->addr[1] == 0xFF))))
    {
      /* Belt 'n' braces check that it's not a broadcast if txOptions is requesting an ack. */
      fctl |= MAC_154_FC_ACK_REQUEST_MASK;
    }

    if ((pDstAddr->addrMode != MAC_154_ADDR_MODE_NONE) &&
        (dstPanId == pPib->panId))
    {
      fctl |= MAC_154_FC_PAN_ID_COMP_MASK;
    }
    fctl |= (pDstAddr->addrMode << MAC_154_FC_DST_ADDR_MODE_SHIFT) |
            (srcAddrMode << MAC_154_FC_SRC_ADDR_MODE_SHIFT);
  }

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number set later */
  pTxFrame++;

  if (pDstAddr->addrMode != MAC_154_ADDR_MODE_NONE)
  {
    uint8_t size = amSizeLUT[pDstAddr->addrMode];

    /* Destination PAN ID */
    UINT16_TO_BSTREAM(pTxFrame, dstPanId);

    /* Destination address */
    memcpy(pTxFrame, pDstAddr->addr, size);
    pTxFrame += size;
  }

  if (srcAddrMode != MAC_154_ADDR_MODE_NONE)
  {
    /* Source PAN ID */
    if ((fctl & MAC_154_FC_PAN_ID_COMP_MASK) == 0)
    {
      UINT16_TO_BSTREAM(pTxFrame, pPib->panId);
    }

    /* Source address */
    if (srcAddrMode == MAC_154_ADDR_MODE_SHORT)
    {
      UINT16_TO_BSTREAM(pTxFrame, pPib->shortAddr);
    }
    else
    {
      UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);
    }
  }

  /* Data */
  uint8_t hdrLen = pTxFrame - PAL_BB_154_TX_FRAME_PTR(pTxDesc);
  if (msduLen > len - hdrLen)
  {
    /* TODO - truncate for now but probably need to return error */
    msduLen = len - hdrLen;
  }

  /* Copy in data. */
  /* TODO: would be better to allocate the data at the point it comes in */
  memcpy(pTxFrame, pMsdu, msduLen);

  /* Set length in descriptor */
  pTxDesc->len = hdrLen + msduLen;

  return pTxDesc;
}

/*************************************************************************************************/
/*!
 *  \brief      Build raw frame
 *
 *  \param      len           Length
 *  \param      msduLen       Length of MPDU
 *  \param      pMsdu         Pointer to MPDU
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildRawFrame(uint8_t len,
                                        uint8_t mpduLen,
                                        uint8_t *pMpdu)
{
  PalBb154TxBufDesc_t *pTxDesc;

  /* Allocate data for Tx frame */
  if ((pTxDesc = WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) + len)) == NULL)
  {
    return NULL;
  }

  /* MPDU */
  memcpy(PAL_BB_154_TX_FRAME_PTR(pTxDesc), pMpdu, mpduLen);

  /* Set length in descriptor */
  pTxDesc->len = mpduLen - MAC_154_FCS_LEN; /* TODO: Have to decrease length by FCS len? */

  return pTxDesc;
}

/*************************************************************************************************/
/*!
 *  \brief      Build Association Request MAC command frame
 *
 *  \param      pCoordAddr    Address of coordinator
 *  \param      capInfo       Capability information
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildAssocReq(Mac154Addr_t *pCoordAddr, uint8_t capInfo)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t  fctl;
  uint8_t  len;
  uint8_t  size;

  len = (pCoordAddr->addrMode == 2) ?
      MAC_154_CMD_FRAME_LEN_ASSOC_REQ_SHT :
      MAC_154_CMD_FRAME_LEN_ASSOC_REQ_EXT;

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) + len)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  /* Frame control:
        MAC command frame
        Sec. enabled = 0: Security not needed for ZigBee join
        Frame pending = 0
        Ack. requested = 1
        PAN ID compression = 0
        Dst Addr mode
        Frame version = 0
        Src Addr mode = 3 (ext.)
  */
  fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
         MAC_154_FC_ACK_REQUEST_MASK |
         (pCoordAddr->addrMode << MAC_154_FC_DST_ADDR_MODE_SHIFT) |
         (MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_SRC_ADDR_MODE_SHIFT);

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number - inserted later */
  pTxFrame++;

  /* Destination PAN ID */
  memcpy(pTxFrame, pCoordAddr->panId, 2);
  pTxFrame += 2;

  /* Destination address */
  size = amSizeLUT[pCoordAddr->addrMode];
  if (size > 0)
  {
    memcpy(pTxFrame, pCoordAddr->addr, size);
    pTxFrame += size;
  }

  /* Source PAN ID: Broadcast PAN ID */
  UINT16_TO_BSTREAM(pTxFrame, MAC_154_BROADCAST_PANID);

  /* Source address: My extended address */
  UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);

  /* MAC command: Association request */
  UINT8_TO_BSTREAM(pTxFrame, MAC_154_CMD_FRAME_TYPE_ASSOC_REQ);

  /* MAC command payload: Capability information */
  UINT8_TO_BSTREAM(pTxFrame, capInfo);

  pTxDesc->len = len;
  return pTxDesc;
}

/*************************************************************************************************/
/*!
 *  \brief      Build Association Response MAC command frame
 *
 *  \param      dstAddr   Destination extended address
 *  \param      addr      Allocated short address
 *  \param      status    Status from coordinator
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildAssocRsp(uint64a_t dstAddr, uint16a_t addr, uint8_t status)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t fctl;

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) +
                                                    MAC_154_CMD_FRAME_LEN_ASSOC_RSP)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  /* Frame control:
        MAC command frame
        Sec. enabled = 0: Security not needed for ZigBee join
        Frame pending = 0
        Ack. requested = 1
        PAN ID compression = 1
        Dst Addr mode = 3 (ext.)
        Frame version = 0
        Src Addr mode = 3 (ext.)
  */
  fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
         MAC_154_FC_ACK_REQUEST_MASK |
         MAC_154_FC_PAN_ID_COMP_MASK |
         (MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_DST_ADDR_MODE_SHIFT) |
         (MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_SRC_ADDR_MODE_SHIFT);

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number - inserted later */
  pTxFrame++;

  /* Destination PAN ID */
  UINT16_TO_BSTREAM(pTxFrame, pPib->panId);

  /* Destination address */
  memcpy(pTxFrame, dstAddr, 8);
  pTxFrame += 8;

  /* Source PAN ID: Omitted */

  /* Source address */
  UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);

  /* MAC command: Association response */
  UINT8_TO_BSTREAM(pTxFrame, MAC_154_CMD_FRAME_TYPE_ASSOC_RSP);

  /* MAC command payload */

  /* Short address */
  memcpy(pTxFrame, addr, 2);
  pTxFrame += 2;

  /* Status */
  UINT8_TO_BSTREAM(pTxFrame, status);

  pTxDesc->len = MAC_154_CMD_FRAME_LEN_ASSOC_RSP;
  return pTxDesc;
}

#if MAC_154_OPT_DISASSOC

/*************************************************************************************************/
/*!
 *  \brief      Build Disassociation Notification MAC command frame
 *
 *  \param      pDstAddr  Pointer to destination address
 *  \param      reason    Disassociation reason
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildDisassocNtf(Mac154Addr_t *pDstAddr, uint8_t reason)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t fctl;
  uint8_t  len;
  uint8_t  size;

  len = (pDstAddr->addrMode == 2) ?
      MAC_154_CMD_FRAME_LEN_DISASSOC_NTF_SHT :
      MAC_154_CMD_FRAME_LEN_DISASSOC_NTF_EXT;

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) + len)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  /* Frame control:
        MAC command frame
        Sec. enabled = 0: Security not needed for ZigBee join
        Frame pending = 0
        Ack. requested = 1
        PAN ID compression = 1
        Dst Addr mode
        Frame version = 0
        Src Addr mode = 3 (ext.)
  */
  fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
         MAC_154_FC_ACK_REQUEST_MASK |
         MAC_154_FC_PAN_ID_COMP_MASK |
         (pDstAddr->addrMode << MAC_154_FC_DST_ADDR_MODE_SHIFT) |
         (MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_SRC_ADDR_MODE_SHIFT);

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number - inserted later */
  pTxFrame++;

  /* Destination PAN ID */
  UINT16_TO_BSTREAM(pTxFrame, pPib->panId);

  /* Destination address */
  size = amSizeLUT[pDstAddr->addrMode];
  if (size > 0)
  {
    memcpy(pTxFrame, pDstAddr->addr, size);
    pTxFrame += size;
  }

  /* Source PAN ID: Omitted */

  /* Source address: My extended address */
  UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);

  /* MAC command: Disassociation notification */
  UINT8_TO_BSTREAM(pTxFrame, MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF);

  /* MAC command payload: Reason */
  UINT8_TO_BSTREAM(pTxFrame, reason);

  pTxDesc->len = len;
  return pTxDesc;
}

#endif /* MAC_154_OPT_DISASSOC */

/*************************************************************************************************/
/*!
 *  \brief      Build Data Request MAC command frame
 *
 *  \param      pCoordAddr        Pointer to coordinator address.
 *  \param      forceSrcExtAddr   Force source address to be extended
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildDataReq(Mac154Addr_t *pCoordAddr, bool_t forceSrcExtAddr)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t fctl;
  uint8_t srcAddrMode;
  uint8_t len;
  static const uint8_t lenLUT[4] = {
    MAC_154_CMD_FRAME_LEN_DATA_REQ_SHT_SHT,
    MAC_154_CMD_FRAME_LEN_DATA_REQ_SHT_EXT,
    MAC_154_CMD_FRAME_LEN_DATA_REQ_EXT_SHT,
    MAC_154_CMD_FRAME_LEN_DATA_REQ_EXT_EXT
  };

  /* Set source address mode based on macShortAddress. 0xffff and 0xfffe should use extended */
  srcAddrMode = forceSrcExtAddr || (pPib->shortAddr >= MAC_154_NO_SHT_ADDR) ? MAC_154_ADDR_MODE_EXTENDED : MAC_154_ADDR_MODE_SHORT;

  /* Work out frame length based on address modes */
  len = lenLUT[(((pCoordAddr->addrMode << 1) + srcAddrMode)-6) & 3]; /* Belt 'n' braces & 3 */

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) + len)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  /* Frame control:
        MAC command frame
        Sec. enabled = 0
        Frame pending = 0
        Ack. requested = 1
        PAN ID compression = 1
        Dst Addr mode
        Frame version = 0
        Src Addr mode
  */
  fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
         MAC_154_FC_ACK_REQUEST_MASK |
         MAC_154_FC_PAN_ID_COMP_MASK |
         (pCoordAddr->addrMode << MAC_154_FC_DST_ADDR_MODE_SHIFT) |
         (srcAddrMode << MAC_154_FC_SRC_ADDR_MODE_SHIFT);

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number set later */
  pTxFrame++;

  if (pCoordAddr->addrMode != MAC_154_ADDR_MODE_NONE)
  {
    uint8_t size = amSizeLUT[pCoordAddr->addrMode];

    /* Destination PAN ID */
    memcpy(pTxFrame, pCoordAddr->panId, 2);
    pTxFrame += 2;

    /* Destination address */
    memcpy(pTxFrame, pCoordAddr->addr, size);
    pTxFrame += size;
  }

  if (srcAddrMode != MAC_154_ADDR_MODE_NONE)
  {
    /* Source address */
    if (srcAddrMode == MAC_154_ADDR_MODE_SHORT)
    {
      UINT16_TO_BSTREAM(pTxFrame, pPib->shortAddr);
    }
    else
    {
      UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);
    }
  }

  /* MAC command: Data request */
  UINT8_TO_BSTREAM(pTxFrame, MAC_154_CMD_FRAME_TYPE_DATA_REQ);

  /* Set length in descriptor */
  pTxDesc->len = len;
  return pTxDesc;
}

/* PAN ID conflict notification: Not used */

#ifdef MAC_154_OPT_ORPHAN

/*************************************************************************************************/
/*!
 *  \brief      Build Orphan Notification Request MAC command frame
 *
 *  \param      None.
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154ScanBuildOrphanNtf(void)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t fctl;

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) +
                                                    MAC_154_CMD_FRAME_LEN_ORPHAN_NTF)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  /* Frame control:
        MAC command frame
        Sec. enabled = 0
        Frame pending = 0
        Ack. requested = 0
        PAN ID compression = 1
        Dst Addr mode = 2 (sht.)
        Frame version = 0
        Src Addr mode = 3 (ext.)
  */
  fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
         MAC_154_FC_PAN_ID_COMP_MASK |
         (MAC_154_ADDR_MODE_SHORT << MAC_154_FC_DST_ADDR_MODE_SHIFT) |
         (MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_SRC_ADDR_MODE_SHIFT);

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number set later */
  pTxFrame++;

  /* Destination PAN ID: Broadcast PAN ID */
  UINT16_TO_BSTREAM(pTxFrame, MAC_154_BROADCAST_PANID);

  /* Destination address: Broadcast address */
  UINT16_TO_BSTREAM(pTxFrame, MAC_154_BROADCAST_ADDR);

  /* No Source PAN ID */

  /* Source address: Extended address */
  UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);

  /* MAC command: Orphan notification */
  UINT8_TO_BSTREAM(pTxFrame, MAC_154_CMD_FRAME_TYPE_ORPHAN_NTF);

  /* Set length */
  pTxDesc->len = MAC_154_CMD_FRAME_LEN_ORPHAN_NTF;

  return pTxDesc;
}

#endif /* MAC_154_OPT_ORPHAN */

/*************************************************************************************************/
/*!
 *  \brief      Build Beacon Request MAC command frame
 *
 *  \param      None.
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154ScanBuildBeaconReq(void)
{
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame;
  uint16_t fctl;

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) +
                                                    MAC_154_CMD_FRAME_LEN_BEACON_REQ)) == NULL)
  {
    return NULL;
  }
  pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  /* Frame control:
        MAC command frame
        Sec. enabled = 0
        Frame pending = 0
        Ack. requested = 0
        PAN ID compression = 0
        Dst Addr mode = 2 (sht.)
        Frame version = 0
        Src Addr mode = 0 (none)
  */
  fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
         (MAC_154_ADDR_MODE_SHORT << MAC_154_FC_DST_ADDR_MODE_SHIFT);

  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number set later */
  pTxFrame++;

  /* Destination PAN ID: Broadcast PAN ID */
  UINT16_TO_BSTREAM(pTxFrame, MAC_154_BROADCAST_PANID);

  /* Destination address: Broadcast address */
  UINT16_TO_BSTREAM(pTxFrame, MAC_154_BROADCAST_ADDR);

  /* No source address */

  /* MAC command: Beacon request */
  UINT8_TO_BSTREAM(pTxFrame, MAC_154_CMD_FRAME_TYPE_BEACON_REQ);

  /* Set length */
  pTxDesc->len = MAC_154_CMD_FRAME_LEN_BEACON_REQ;

  return pTxDesc;
}

#ifdef MAC_154_OPT_ORPHAN

/*************************************************************************************************/
/*!
 *  \brief      Build Coordinator Realignment frame
 *
 *  \param      orphanAddr  Extended address of orphaned device.
 *  \param      panId       New PAN ID
 *  \param      shtAddr     Existing allocated short address
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildCoordRealign(uint64_t orphanAddr, uint16_t panId, uint16_t shtAddr)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();
  PalBb154TxBufDesc_t *pTxDesc;
  uint8_t *pTxFrame, *pTxFrameStart;
  uint8_t len;
  uint16_t fctl;

  len = (shtAddr == MAC_154_BROADCAST_ADDR) ? MAC_154_CMD_FRAME_LEN_COORD_REALIGN_SHT : MAC_154_CMD_FRAME_LEN_COORD_REALIGN_EXT;

  if ((pTxDesc = (PalBb154TxBufDesc_t *)WsfBufAlloc(sizeof(PalBb154TxBufDesc_t) + len)) == NULL)
  {
    return NULL;
  }
  pTxFrameStart = pTxFrame = PAL_BB_154_TX_FRAME_PTR(pTxDesc);

  /* Frame control */
  if (shtAddr == MAC_154_BROADCAST_ADDR)
  {
    /* Frame control:
          MAC command frame
          Sec. enabled = 0
          Frame pending = 0
          Ack. requested = 1
          PAN ID compression = 0
          Dst Addr mode = 2 (sht.)
          Frame version = 0
          Src Addr mode = 3 (ext.)
    */
    fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
           MAC_154_ADDR_MODE_SHORT << MAC_154_FC_DST_ADDR_MODE_SHIFT |
           MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_SRC_ADDR_MODE_SHIFT;
  }
  else
  {
    /* Frame control:
          MAC command frame
          Sec. enabled = 0
          Frame pending = 0
          Ack. requested = 0
          PAN ID compression = 0
          Dst Addr mode = 3 (ext.)
          Frame version = 0
          Src Addr mode = 3 (ext.)
    */
    fctl = MAC_154_FRAME_TYPE_MAC_COMMAND |
           MAC_154_FC_ACK_REQUEST_MASK |
           MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_DST_ADDR_MODE_SHIFT |
           MAC_154_ADDR_MODE_EXTENDED << MAC_154_FC_SRC_ADDR_MODE_SHIFT;
  }
  if (pPib->panId == MAC_154_BROADCAST_PANID) /* This could happen... */
  {
    fctl |= MAC_154_FC_PAN_ID_COMP_MASK;
  }
  UINT16_TO_BSTREAM(pTxFrame, fctl);

  /* Sequence number set later */
  pTxFrame++;

  /* Destination PAN ID: Broadcast */
  UINT16_TO_BSTREAM(pTxFrame, MAC_154_BROADCAST_PANID);

  if (shtAddr == MAC_154_BROADCAST_ADDR)
  {
    /* Broadcast address */
    UINT16_TO_BSTREAM(pTxFrame, shtAddr);
  }
  else
  {
    /* Destination address from parameter */
    UINT64_TO_BSTREAM(pTxFrame, orphanAddr);
  }

  if (pPib->panId != MAC_154_BROADCAST_PANID)
  {
    /* Source PAN ID: From PIB */
    UINT16_TO_BSTREAM(pTxFrame, pPib->panId);
  }

  /* Source address: From PIB */
  UINT64_TO_BSTREAM(pTxFrame, pPib->extAddr);

  /* Command ID */
  UINT8_TO_BSTREAM(pTxFrame, MAC_154_CMD_FRAME_TYPE_COORD_REALIGN);

  /* PAN ID */
  UINT16_TO_BSTREAM(pTxFrame, panId);

  /* Coordinator Short Address: From PIB  */
  UINT16_TO_BSTREAM(pTxFrame, pPib->shortAddr);

  /* Logical channel: From PHY PIB  */
  UINT8_TO_BSTREAM(pTxFrame, pPhyPib->chan);

  /* Short Address  */
  UINT16_TO_BSTREAM(pTxFrame, shtAddr);

  pTxDesc->len = pTxFrame - pTxFrameStart;
  return pTxDesc;
}

#endif /* MAC_154_OPT_ORPHAN */

/* GTS request: Not used */

/*************************************************************************************************/
/*!
 *  \brief      Get addresses from frame.
 *
 *  \param      pFrame        Frame buffer pointing to first octet of address field
 *  \param      fctl          Frame control field
 *  \param      pSrcAddr      Pointer to source address (passed by reference) - may be NULL
 *  \param      pDstAddr      Pointer to destination address (passed by reference) - may be NULL
 *
 *  \return     Buffer pointer advanced past address fields.
 *
 *  Obtains the source and destination addresses from the frame. If either parameter is NULL,
 *  simply skips past the fields
 */
/*************************************************************************************************/
uint8_t *Bb154GetAddrsFromFrame(uint8_t *pFrame, uint16_t fctl, Mac154Addr_t *pSrcAddr, Mac154Addr_t *pDstAddr)
{
  uint8_t dstAddrMode = (uint8_t)MAC_154_FC_DST_ADDR_MODE(fctl);
  uint8_t srcAddrMode = (uint8_t)MAC_154_FC_SRC_ADDR_MODE(fctl);
  uint8_t addrLen;
  uint16a_t dstPanId = {0,0};

  if (dstAddrMode == MAC_154_ADDR_MODE_NONE)
  {
    /* Belt 'n' braces clearing of PAN ID compression bit */
    fctl &= ~MAC_154_FC_PAN_ID_COMP_MASK;
  }
  else
  {
    /* Dst PAN ID always present with dest addr. */
    dstPanId[0] = *pFrame++;
    dstPanId[1] = *pFrame++;
  }

  /* Destination address */
  if (pDstAddr != NULL)
  {
    pDstAddr->addrMode = dstAddrMode;
    if (dstAddrMode != MAC_154_ADDR_MODE_NONE)
    {
      pDstAddr->panId[0] = dstPanId[0];
      pDstAddr->panId[1] = dstPanId[1];
      memset(&pDstAddr->addr[2], 0, 6);
      addrLen = amSizeLUT[dstAddrMode];
      memcpy(pDstAddr->addr, pFrame, addrLen);
      pFrame += addrLen;
    }
  }
  else
  {
    pFrame += amSizeLUT[dstAddrMode];
  }

  /* Source address */
  if (pSrcAddr != NULL)
  {
    pSrcAddr->addrMode = srcAddrMode;
    if (srcAddrMode != MAC_154_ADDR_MODE_NONE)
    {
      if (fctl & MAC_154_FC_PAN_ID_COMP_MASK)
      {
        pSrcAddr->panId[0] = dstPanId[0];
        pSrcAddr->panId[1] = dstPanId[1];
      }
      else
      {
        pSrcAddr->panId[0] = *pFrame++;
        pSrcAddr->panId[1] = *pFrame++;
      }
      memset(&pSrcAddr->addr[2], 0, 6);
      addrLen = amSizeLUT[srcAddrMode];
      memcpy(pSrcAddr->addr, pFrame, addrLen);
      pFrame += addrLen;
    }
  }
  else
  {
    pFrame += amSizeLUT[srcAddrMode];
    if ((fctl & MAC_154_FC_PAN_ID_COMP_MASK) == 0)
    {
      pFrame += 2;
    }
  }
  return pFrame;
}
