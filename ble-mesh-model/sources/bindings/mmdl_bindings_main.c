/*************************************************************************************************/
/*!
 *  \file   mmdl_bindings_main.c
 *
 *  \brief  Implementation of the Model Bindings Resolver module.
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

#include "mesh_defs.h"
#include "mesh_types.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_bindings.h"
#include "mmdl_bindings_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Model bind resolver control block type definition */
typedef struct mmdlBindCb_tag
{
  mmdlBind_t  bindings[MMDL_BINDINGS_MAX];  /*!< Bindings table */
  uint8_t     bindingsCount;                /*!< Number of registered state bindings */
} mmdlBindCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Model bind resolver control block */
static mmdlBindCb_t bindCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Adds a bind to the binding table.
 *
 *  \param[in] srcState           Bound state that has changed.
 *  \param[in] tgtState           Bound state that needs to be changed.
 *  \param[in] srcElementId       Element that contains the state that has changed.
 *  \param[in] tgtElementId       Element that contains the state that needs to be changed.
 *  \param[in] pBindResolverFunc  Resolver function. See ::mmdlBindResolver_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlAddBind(mmdlBoundState_t srcState, mmdlBoundState_t tgtState,
                 meshElementId_t srcElementId, meshElementId_t tgtElementId,
                 mmdlBindResolver_t pBindResolverFunc)
{
  uint8_t bindIdx;

  /* Find a matching binding, */
  for (bindIdx = 0; bindIdx < bindCb.bindingsCount; bindIdx++)
  {
    if ((bindCb.bindings[bindIdx].srcBoundState == srcState) &&
        (bindCb.bindings[bindIdx].tgtBoundState == tgtState) &&
        (bindCb.bindings[bindIdx].srcElementId == srcElementId) &&
        (bindCb.bindings[bindIdx].tgtElementId == tgtElementId))
    {
      /* Binding exists. */
      return;
    }
  }

  /* New binding must be added*/
  WSF_ASSERT(bindCb.bindingsCount < MMDL_BINDINGS_MAX);

  if (bindCb.bindingsCount < MMDL_BINDINGS_MAX)
  {
    /* Add binding to the next entry. */
    bindCb.bindings[bindCb.bindingsCount].srcBoundState = srcState;
    bindCb.bindings[bindCb.bindingsCount].tgtBoundState = tgtState;
    bindCb.bindings[bindCb.bindingsCount].srcElementId = srcElementId;
    bindCb.bindings[bindCb.bindingsCount].tgtElementId = tgtElementId;
    bindCb.bindings[bindCb.bindingsCount].pBindResolverFunc = pBindResolverFunc;

    /* Increase bindings count */
    bindCb.bindingsCount++;
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the model bindings resolver module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlBindingsInit(void)
{
  bindCb.bindingsCount = 0;
  memset(bindCb.bindings, 0, sizeof(mmdlBind_t) * MMDL_BINDINGS_MAX);
}

/*************************************************************************************************/
/*!
 *  \brief     Checks if one of the states of the model instance has a bind with another state and
 *             calls the function to resolve it.
 *
 *  \param[in] srcElementId   Identifier for the element that contains the model instance state
 *                            that has changed.
 *  \param[in] srcBoundState  Identifier for the bound state that has changed.
 *
 *  \param[in] pStateValue    Pointer to the new state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlBindResolve(meshElementId_t srcElementId, mmdlBoundState_t srcBoundState,
                     void *pStateValue)
{
  uint8_t bindIdx;

  /* Find a matching binding, */
  for (bindIdx = 0; bindIdx < bindCb.bindingsCount; bindIdx++)
  {
    if ((bindCb.bindings[bindIdx].srcBoundState == srcBoundState) &&
        (bindCb.bindings[bindIdx].srcElementId == srcElementId))
    {
      /* Call bind resolver */
      bindCb.bindings[bindIdx].pBindResolverFunc(bindCb.bindings[bindIdx].tgtElementId,
                                                 pStateValue);
    }
  }
}
