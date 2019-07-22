/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband interface file: Internal functions
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

#ifndef BB_154_INT_H
#define BB_154_INT_H

#include "wsf_types.h"
#include "bb_154.h"
#include "bb_154_api_op.h"
#include "mac_154_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup BB_API_154
 *  \{ */

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize the 802.15.4 BB.
 *
 *  \return     None.
 *
 *  Initialize baseband resources.
 */
/*************************************************************************************************/
void Bb154Init(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize for test operations.
 *
 *  \return     None.
 *
 *  Update the operation table with test operations routines.
 */
/*************************************************************************************************/
void Bb154TestInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize for scan operations.
 *
 *  \return     None.
 *
 *  Update the operation table with scan operations routines according to scan type
 */
/*************************************************************************************************/
void Bb154ScanInit(uint8_t scanType);

/*************************************************************************************************/
/*!
 *  \brief      Initialize for association operations.
 *
 *  \return     None.
 *
 *  Update the operation table with data transmit operations routines
 */
/*************************************************************************************************/
void Bb154AssocInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize for data baseband operations.
 *
 *  \return     None.
 *
 *  Update the operation table with data transmit operations
 */
/*************************************************************************************************/
void Bb154DataInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize for data baseband operations.
 *
 *  \return     None.
 *
 *  Update the operation table with data transmit operations
 */
/*************************************************************************************************/
void Bb154DataDeInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Check if 15.4 Rx is in progress
 *
 *  \return     BbOpDesc_t*   Pointer to BOD if in progress.
 */
/*************************************************************************************************/
BbOpDesc_t *Bb154RxInProgress(void);

/*************************************************************************************************/
/*!
 *  \brief      Cleanup test BOD.
 *
 *  \param      pOp    Pointer to the BOD to execute.
 *  \param      p154    BL802.15.4E operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Bb154GenCleanupOp(BbOpDesc_t *pOp, Bb154Data_t *p154);

/**** Frame operations ****/

/*************************************************************************************************/
/*!
 *  \brief      Build Beacon frame
 *
 *  \param      None.
 *
 *  \return     Pointer to buffer or NULL if failed to allocate.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildBeacon(void);

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
 *  \return     TRUE if canceled, FALSE otherwise.
 */
/*************************************************************************************************/
PalBb154TxBufDesc_t *Bb154BuildDataFrame(uint8_t len,
                                         uint8_t srcAddrMode,
                                         Mac154Addr_t *pDstAddr,
                                         uint8_t txOptions,
                                         uint8_t msduLen,
                                         uint8_t *pMsdu);

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
                                        uint8_t *pMpdu);

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
PalBb154TxBufDesc_t *Bb154BuildAssocReq(Mac154Addr_t *pCoordAddr, uint8_t capInfo);

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
PalBb154TxBufDesc_t *Bb154BuildAssocRsp(uint64a_t dstAddr, uint16a_t addr, uint8_t status);

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
PalBb154TxBufDesc_t *Bb154BuildDisassocNtf(Mac154Addr_t *pDstAddr, uint8_t reason);

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
PalBb154TxBufDesc_t *Bb154BuildDataReq(Mac154Addr_t *pCoordAddr, bool_t forceSrcExtAddr);

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
PalBb154TxBufDesc_t *Bb154ScanBuildOrphanNtf(void);

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
PalBb154TxBufDesc_t *Bb154ScanBuildBeaconReq(void);

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
PalBb154TxBufDesc_t *Bb154BuildCoordRealign(uint64_t orphanAddr, uint16_t panId, uint16_t shtAddr);

#endif /* MAC_154_OPT_ORPHAN */

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
uint8_t *Bb154GetAddrsFromFrame(uint8_t *pBuf, uint16_t fctl, Mac154Addr_t *pSrcAddr, Mac154Addr_t *pDstAddr);

/*************************************************************************************************/
/*!
 *  \brief      Queue the Tx indirect packet.
 *
 *  \param      pTxDesc   Transmit descriptor.
 *
 *  \return     Total number of buffers queued.
 *
 *  Calling this routine will queue a Tx indirect frame to the Tx indirect queue
 */
/*************************************************************************************************/
uint8_t Bb154QueueTxIndirectBuf(PalBb154TxBufDesc_t *pTxDesc);

/*************************************************************************************************/
/*!
 *  \brief      Purge a tx indirect packet.
 *
 *  \param      msduHandle    MSDU handle to purge
 *
 *  \return     TRUE if purged, FALSE otherwise.
 *
 *  Calling this routine will purge a Tx indirect frame from the Tx indirect queue
 */
/*************************************************************************************************/
bool_t Bb154PurgeTxIndirectBuf(uint8_t msduHandle);

/*************************************************************************************************/
/*!
 *  \brief      Handle transaction persistence timer timeout.
 *
 *  \param      pTimer    Associated timer
 *
 *  \return     None.
 *
 *  Calling this routine will queue a Tx indirect frame to the Tx indirect queue
 */
/*************************************************************************************************/
void Bb154HandleTPTTimeout(void *pMsg);

/*! \} */    /* BB_API_154 */

#ifdef __cplusplus
};
#endif

#endif /* BB_154_API_H */
