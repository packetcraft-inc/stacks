/*************************************************************************************************/
/*!
 *  \file   mesh_seq_manager.c
 *
 *  \brief  SEQ manager implementation.
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

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mesh_error_codes.h"
#include "mesh_seq_manager.h"

#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Invalid value for the sequence number */
#define MESH_SEQ_NUMBER_INVALID_VALUE        (MESH_SEQ_MAX_VAL + 1)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Structure containing information required by this module */
typedef struct meshSeqManagerLocals_tag
{
  meshSeqThreshCback_t  threshCback;     /*!< SEQ number threshold exceeded callback callback */
  uint32_t              lowThresh;       /*!< Lower threshold to trigger notification */
  uint32_t              highThresh;      /*!< Higher threshold to trigger notification */
  bool_t                lowThreshNotif;  /*!< TRUE if lower threshold notification is triggered */
  bool_t                highThreshNotif; /*!< TRUE if higher threshold notification is triggered */
} meshSeqManagerCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Sequence Number Manager control block */
static meshSeqManagerCb_t seqCb =
{
  .lowThreshNotif = FALSE,
  .highThreshNotif = FALSE,
  .lowThresh = MESH_SEQ_MAX_VAL,
  .highThresh = MESH_SEQ_MAX_VAL,
  .threshCback = NULL
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Mesh SEQ Manager SEQ number threshold exceeded empty callback.
 *
 *  \param[in] lowThresh   TRUE if lower threshold exceeded
 *  \param[in] highThresh  TRUE if higher threshold exceeded
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSeqExhaustEmptyCback(bool_t lowThresh, bool_t highThresh)
{
  (void)lowThresh;
  (void)highThresh;
  MESH_TRACE_ERR0("MESH SEQ: Exhaust callback not initialized!");
  return;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh SEQ Manager.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSeqInit(void)
{
  seqCb.threshCback = meshSeqExhaustEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh SEQ Manager exhaust callback registration.
 *
 *  \param[in] seqThreshCback  Threshold exceeded callback.
 *  \param[in] lowThresh       Lower threshold for which a notification is triggered.
 *  \param[in] highThresh      Higher threshold for which a notification is triggered.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSeqRegister(meshSeqThreshCback_t seqThreshCback, uint32_t lowThresh, uint32_t highThresh)
{
  /* Validate init parameter. */
  if ((seqThreshCback == NULL) || (lowThresh > highThresh) || (lowThresh > MESH_SEQ_MAX_VAL))
  {
    return;
  }

  /* Configure threshold parameters and callback. */
  seqCb.threshCback = seqThreshCback;
  seqCb.lowThresh = lowThresh;
  seqCb.highThresh = highThresh;
  seqCb.lowThreshNotif = FALSE;
  seqCb.highThreshNotif = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the current SEQ number for a source address.
 *
 *  \param[in]  srcAddr    Source address for which to get the SEQ number.
 *  \param[out] pOutSeqNo  Pointer to store the current SEQ number for the input source address.
 *  \param[in]  autoInc    Auto increment the SEQ number after get.
 *
 *  \return     Success or error reason. \see meshSeqRetVal_t
 */
/*************************************************************************************************/
meshSeqRetVal_t MeshSeqGetNumber(meshAddress_t srcAddr, meshSeqNumber_t *pOutSeqNo, bool_t autoInc)
{
  meshSeqNumber_t      seqNumber = MESH_SEQ_NUMBER_INVALID_VALUE;
  meshElementId_t      elemId    = 0;

  /* Validate output parameter. */
  if (pOutSeqNo == NULL)
  {
    return MESH_SEQ_INVALID_PARAMS;
  }

  /* Validate source address type. */
  if (!MESH_IS_ADDR_UNICAST(srcAddr))
  {
    return MESH_SEQ_INVALID_ADDRESS;
  }

  /* Validate address value by reading element identifier from local config module. */
  if(MeshLocalCfgGetElementIdFromAddr(srcAddr, &elemId) != MESH_SUCCESS)
  {
    return MESH_SEQ_INVALID_ADDRESS;
  }

  /* Read sequence number. */
  if (MeshLocalCfgGetSeqNumber(elemId, &seqNumber) != MESH_SUCCESS)
  {
    WSF_ASSERT(MeshLocalCfgGetSeqNumber(elemId, &seqNumber) == MESH_SUCCESS);
  }

  /* Validate sequence number range. */
  if ((seqNumber > MESH_SEQ_MAX_VAL) ||
      (seqNumber == MESH_SEQ_MAX_VAL && autoInc))
  {
    return MESH_SEQ_EXHAUSTED;
  }

  /* Check if autoincrement is required. */
  if (autoInc)
  {
    /* Set sequence number as read sequence number + 1. */
    if (MeshLocalCfgSetSeqNumber(elemId, seqNumber + 1) != MESH_SUCCESS)
    {
      /* Trap if it ever happens. */
      WSF_ASSERT(MeshLocalCfgSetSeqNumber(elemId, seqNumber + 1) == MESH_SUCCESS)
    }

    /* Verify if thresholds are exceeded. */
    if ((seqNumber >= seqCb.lowThresh) && (!seqCb.lowThreshNotif))
    {
      /* Invoke callback. */
      seqCb.threshCback(TRUE, FALSE);
      /* Set flag to TRUE. */
      seqCb.lowThreshNotif = TRUE;
    }
    if ((seqNumber >= seqCb.highThresh) && (!seqCb.highThreshNotif))
    {
      /* Invoke callback. */
      seqCb.threshCback(FALSE, TRUE);
      /* Set flag to TRUE. */
      seqCb.highThreshNotif = TRUE;
    }
  }

  /* Set sequence number to output parameter. */
  *pOutSeqNo = seqNumber;

  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Increments the current SEQ number for a source address.
 *
 *  \param[in] srcAddr  Source address for which to increment the SEQ number.
 *
 *  \return    Success or error reason. \see meshSeqRetVal_t
 */
/*************************************************************************************************/
meshSeqRetVal_t MeshSeqIncNumber(meshAddress_t srcAddr)
{
  meshSeqNumber_t dummySeqNumber = 0;

  /* Since read-modify-write sequence is needed,
   * call get sequence number with a dummy sequence number.
   */
  return MeshSeqGetNumber(srcAddr, &dummySeqNumber, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief  Resets all SEQ numbers for all addresses.
 *
 *  \return Success or error reason. \see meshSeqRetVal_t
 */
/*************************************************************************************************/
void MeshSeqReset(void)
{
  meshElementId_t      elemId;
  meshLocalCfgRetVal_t retVal;

  /* Reset all sequence numbers for increasing element identifiers until an error occurs. */
  for(elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Call local config to set 0 as the new sequence number. */
    retVal = MeshLocalCfgSetSeqNumber(elemId, 0);

    WSF_ASSERT(retVal == MESH_SUCCESS);
  }

  /* Set threshold notification flag to FALSE. */
  seqCb.lowThreshNotif = FALSE;
  seqCb.highThreshNotif = FALSE;

  (void)retVal;
}
