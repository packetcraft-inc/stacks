/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_cl_pg0_bstream.h
 *
 *  \brief  Configuration Client Composition Page 0 stream parser macros.
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

/*! ***********************************************************************************************
 *
 * @addtogroup MESH_CONFIGURATION_MODELS
 * @{
 *************************************************************************************************/

#ifndef MESH_CFG_MDL_CL_PG0_BSTREAM_H
#define MESH_CFG_MDL_CL_PG0_BSTREAM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_types.h"
#include "mesh_types.h"
#include "util/bstream.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Configuration Client Composition Page 0 stream parser macros for page header.
 *
 *  \note pBuf is Page 0 raw byte array, len is the initial length.
 *   cid = company ID (2 bytes);
 *   pid = product ID (2 bytes);
 *   vid = version ID (2 bytes);
 *   crpl = minimum number of replay protection list entries in a device (2 bytes);
 *   feat = features (see ::meshFeatures_t);
 */
#define BSTREAM_TO_CFG_CL_COMP_PG0_HDR(pBuf, len, cid, pid, vid, crpl, feat) \
                                       do                                    \
                                       {                                     \
                                         BSTREAM_TO_UINT16((cid), (pBuf));   \
                                         (len) -= 2;                         \
                                         BSTREAM_TO_UINT16((pid), (pBuf));   \
                                         (len) -= 2;                         \
                                         BSTREAM_TO_UINT16((vid), (pBuf));   \
                                         (len) -= 2;                         \
                                         BSTREAM_TO_UINT16((crpl), (pBuf));  \
                                         (len) -= 2;                         \
                                         BSTREAM_TO_UINT16((feat), (pBuf));  \
                                         (len) -= 2;                         \
                                       } while (0)

/*! Mesh Configuration Client Composition Page 0 stream parser macros for element header.
 *  \note pBuf is remaining of Page 0 raw byte array, len is the remaining length.
 *   loc  = location descriptor (2 bytes);
 *   numS = number of SIG models on this element (1 byte);
 *   numV = number of Vendor models on this element (1 byte);
 */
#define BSTREAM_TO_CFG_CL_COMP_PG0_ELEM_HDR(pBuf, len, loc, numS, numV)      \
                                       do                                    \
                                       {                                     \
                                         if((len) < 4)                       \
                                         {                                   \
                                           (len) = 0;  (loc) = 0;            \
                                           (numS) = 0; (numV)=0;             \
                                         }                                   \
                                         else                                \
                                         {                                   \
                                            BSTREAM_TO_UINT16((loc), (pBuf));\
                                            BSTREAM_TO_UINT8((numS), (pBuf));\
                                            BSTREAM_TO_UINT8((numV), (pBuf));\
                                            (len) -= 4;                      \
                                         }                                   \
                                       } while (0)

/*! Mesh Configuration Client Composition Page 0 stream parser macros for SIG models.
 *  \note pBuf is remaining of Page 0 raw byte array, len is the remaining length.
 *   sigId = SIG model ID (2 bytes);
 */
#define BSTREAM_TO_CFG_CL_COMP_PG0_SIG_MODEL_ID(pBuf, len, sigId)              \
                                       do                                      \
                                       {                                       \
                                         if((len) < sizeof(meshSigModelId_t))  \
                                         {                                     \
                                           (len) = 0;                          \
                                           (sigId) = 0x0000;                   \
                                         }                                     \
                                         else                                  \
                                         {                                     \
                                            BSTREAM_TO_UINT16((sigId), (pBuf));\
                                            (len) -= sizeof(meshSigModelId_t); \
                                         }                                     \
                                       } while (0)

/*! Mesh Configuration Client Composition Page 0 stream parser macros for Vendor models.
 *  \note pBuf is remaining of Page 0 raw byte array, len is the remaining length.
 *        vendId = Vendor model ID (4 bytes);
 */
#define BSTREAM_TO_CFG_CL_COMP_PG0_VENDOR_MODEL_ID(pBuf, len, vendId)             \
                                       do                                         \
                                       {                                          \
                                         if((len) < sizeof(meshVendorModelId_t))  \
                                         {                                        \
                                           (len) = 0;                             \
                                           (vendId) = 0x0000;                     \
                                         }                                        \
                                         else                                     \
                                         {                                        \
                                            BSTREAM_TO_UINT32((vendId), (pBuf));  \
                                            (len) -= sizeof(meshVendorModelId_t); \
                                         }                                        \
                                       } while (0)

/*!<
 * \remarks PAGE 0 structure: Page Header | [Elem X HDR | Elem X SIG IDs | Elem X Vendor IDs]...
 * \remarks Stream macros for Composition Data Page 0 must be used in a specific order. See code.
 * \code
 *  uint16_t cid, pid, vid, crpl, locDescr;
 *  uint8_t numS, numV;
 *  meshFeatures_t feat;
 *  meshSigModelId_t sigModelId;
 *  meshVendorModelId_t vendModelId;
 *
 *  // Extract header.
 *  BSTREAM_TO_CFG_CL_COMP_PG0_HDR(pPg0, len, cid, pid, vid, crpl, feat);
 *
 *  // Print header.
 *  PRINT_NUM("COMPANY ID", cid);
 *  PRINT_NUM("PRODUCT ID", pid);
 *  PRINT_NUM("VERSION ID", vid);
 *  PRINT_NUM("Replay protection number of entries", crpl);
 *
 *  if (feat & MESH_FEAT_RELAY)
 *  {
 *    PRINT("Relay supported");
 *  }
 *  if (feat & MESH_FEAT_PROXY)
 *  {
 *    PRINT("Proxy supported");
 *  }
 *  if (feat & MESH_FEAT_FRIEND)
 *  {
 *    PRINT("Friend supported");
 *  }
 *  if (feat & MESH_FEAT_LOW_POWER)
 *  {
 *    PRINT("LPN supported");
 *  }
 *
 *  uint8_t i = 0;
 *  while (len > 0)
 *  {
 *    // First get element header.
 *    BSTREAM_TO_CFG_CL_COMP_PG0_ELEM_HDR(pPg0, len, locDescr, numS, numV);
 *
 *    PRINT_NUM("Element", i);
 *    i++;
 *
 *    PRINT("Element information: ");
 *    PRINT_NUM("Location descriptor", locDescr);
 *    // Then get all SIG models for this element
 *    while ((len != 0) && numS != 0)
 *    {
 *      // Get SIG model ID.
 *      BSTREAM_TO_CFG_CL_COMP_PG0_SIG_MODEL_ID(pPg0, len, sigModelId);
 *
 *      PRINT_NUM("SIG MODEL ID", sigModelId);
 *      numS--;
 *    }
 *    // Finally get all Vendor models for this element
 *    while ((len != 0) && numV != 0)
 *    {
 *      // Get Vendor model ID.
 *      BSTREAM_TO_CFG_CL_COMP_PG0_VENDOR_MODEL_ID(pPg0, len, vendModelId);
 *
 *      PRINT_NUM("VENDOR MODEL ID", vendModelId);
 *      numV--;
 *    }
 *  }
 * \endcode
 */

#ifdef __cplusplus
}
#endif

#endif /* MESH_CFG_MDL_CL_PG0_BSTREAM_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
