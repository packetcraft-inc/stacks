/*************************************************************************************************/
/*!
 *  \file   mesh_utils.c
 *
 *  \brief  Utility function definitions.
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
#include <stdio.h>

#include "wsf_types.h"
#include "mesh_error_codes.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes the Log Field value from a 2-octet value.
 *
 *  \param[in] val  2-octet value.
 *
 *  \return    Log Field value.
 */
/*************************************************************************************************/
uint8_t MeshUtilsGetLogFieldVal(uint16_t val)
{
  uint16_t i = val;
  uint8_t  n = 0;

  while (i)
  {
    n++;
    i >>= 1;
  }

  return n;
}
