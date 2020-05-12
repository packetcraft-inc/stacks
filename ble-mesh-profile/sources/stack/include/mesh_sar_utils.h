/*************************************************************************************************/
/*!
 *  \file   mesh_sar_utils.h
 *
 *  \brief  SAR utility functions.
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

#ifndef MESH_SAR_UTILS_H
#define MESH_SAR_UTILS_H

#include "mesh_lower_transport.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Defines the common 4-octet header type for segmented PDUs (both access and control) */
typedef struct meshSarSegHdr_tag
{
  uint8_t bytes[MESH_LTR_SEG_HDR_LEN];
} meshSarSegHdr_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Packs static segmentation info into a segmentation header for an Access PDU.
 *
 *  \param[out] pOutHdr  Location of the segmentation header (octet array).
 *  \param[in]  akf      Application key flag.
 *  \param[in]  aid      AID value.
 *  \param[in]  szmic    Size of TransMIC.
 *  \param[in]  seqZero  SeqZero value.
 *  \param[in]  segN     SegN value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void meshSarInitSegHdrForAcc(meshSarSegHdr_t *pOutHdr,
                                           uint8_t akf,
                                           uint8_t aid,
                                           uint8_t szmic,
                                           uint16_t seqZero,
                                           uint8_t segN)
{
  /* First octet: SEG=1 (1b) | AKF (1b) | AID (6b) */
  pOutHdr->bytes[0] = (1 << 7) | ((akf & 0x01) << 6) | (aid & 0x3f);
  /* Second octet: SZMIC (1b) | SeqZero[12-6] (7b) */
  pOutHdr->bytes[1] = ((szmic & 0x01) << 7) | ((seqZero >> 6) & 0x7f);
  /* Third octet: SeqZero[5-0] (6b) | SegO=0[4-3] (2b) */
  pOutHdr->bytes[2] = ((seqZero & 0x3f) << 2) | 0x00;
  /* Fourth octet: SegO=0[2-0] (3b) | SegN (5b) */
  pOutHdr->bytes[3] = 0x00 | (segN & 0x1f);
}

/*************************************************************************************************/
/*!
 *  \brief      Packs static segmentation info into a segmentation header for a Control PDU.
 *
 *  \param[out] pOutHdr  Location of the segmentation header (octet array).
 *  \param[in]  opcode   Transport Control Opcode value.
 *  \param[in]  seqZero  SeqZero value.
 *  \param[in]  segN     SegN value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void meshSarInitSegHdrForCtl(meshSarSegHdr_t *pOutHdr,
                                           uint8_t opcode,
                                           uint16_t seqZero,
                                           uint8_t segN)
{
  /* First octet: SEG=1 (1b) | Opcode (7b) */
  pOutHdr->bytes[0] = (1 << 7) | (opcode & 0x7f);
  /* Second octet: RFU=0 (1b) | SeqZero[12-6] (7b) */
  pOutHdr->bytes[1] = 0x00 | ((seqZero >> 6) & 0x7f);
  /* Third octet: SeqZero[5-0] (6b) | SegO=0[4-3] (2b) */
  pOutHdr->bytes[2] = ((seqZero & 0x3f) << 2) | 0x00;
  /* Fourth octet: SegO=0[2-0] (3b) | SegN (5b) */
  pOutHdr->bytes[3] = 0x00 | (segN & 0x1f);
}

/*************************************************************************************************/
/*!
 *  \brief      Packs a SegO value into a prefilled segmentation header.
 *
 *  \param[out] pOutHdr  Location of the segmentation header (octet array).
 *  \param[in]  segO     SegO value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void meshSarSetSegHdrSegO(meshSarSegHdr_t *pOutHdr, uint8_t segO)
{
  /* Third octet: SeqZero[5-0] (6b) | SegO=0[4-3] (2b) */
  pOutHdr->bytes[2] &= 0xfc;
  pOutHdr->bytes[2] |= ((segO >> 3) & 0x03);
  /* Fourth octet: SegO=0[2-0] (3b) | SegN (5b) */
  pOutHdr->bytes[3] &= 0x1f;
  pOutHdr->bytes[3] |= ((segO & 0x07) << 5);
}

/*************************************************************************************************/
/*!
 *  \brief      Computes segment count and last segment length for a transaction.
 *
 *  \param[in]  pduSize         Size of the PDU.
 *  \param[in]  segmentSize     Max size of each segment.
 *  \param[out] pOutSegCount    Pointer to segment count.
 *  \param[out] pOutLastLength  Pointer to last segment length.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void meshSarComputeSegmentCountAndLastLength(uint16_t pduSize,
                                                           uint8_t segmentSize,
                                                           uint8_t *pOutSegCount,
                                                           uint8_t *pOutLastLength)
{
  *pOutSegCount = (uint8_t) (pduSize / segmentSize);
  *pOutLastLength = (uint8_t) (pduSize - *pOutSegCount * segmentSize);
  if (0 == *pOutLastLength)
    /* Length is multiple of segment size */
  {
    *pOutLastLength = segmentSize;
  }
  else
    /* Length is not multiple of segment size */
  {
    *pOutSegCount = 1 + *pOutSegCount;
  }
}

#ifdef __cplusplus
}
#endif

#endif /* MESH_SAR_UTILS_H */
