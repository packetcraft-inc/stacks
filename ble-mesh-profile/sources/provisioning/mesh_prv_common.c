/*************************************************************************************************/
/*!
 *  \file   mesh_prv_common.c
 *
 *  \brief  Mesh Provisioning common module implementation.
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
#include "wsf_msg.h"
#include "util/bstream.h"
#include "sec_api.h"

#include "mesh_prv.h"
#include "mesh_prv_defs.h"
#include "mesh_prv_common.h"

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Generates a random string of alphanumeric characters.
 *
 *  \param[in] pOutArray  Pointer to the output buffer.
 *  \param[in] size       Size of the output buffer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvGenerateRandomAlphanumeric(uint8_t* pOutArray, uint8_t size)
{
  uint8_t alphaCount = 'Z' - 'A' + 1;
  uint8_t numCount = '9' - '0' + 1;
  uint8_t index;
  uint32_t random;

  for (uint8_t j = 0; j < size; j++)
  {
    /* Generate random integer between 0 and 4.294.967.295 */
    SecRand((uint8_t*)&random, sizeof(uint32_t));

    /* Reduce the large integer modulo number of symbols.
       The loss of entropy is, on average, negligible. */
    index = (uint8_t)(random % (alphaCount + numCount));

    /* Map the index to the corresponding symbol */
    if (index < alphaCount)
    {
      pOutArray[j] = 'A' + index;
    }
    else
    {
      index -= alphaCount;
      pOutArray[j] = '0' + index;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Generates a random number on a given number of digits.
 *
 *  \param[in] digits  Number of digits, range 1-8.
 *
 *  \return    The 4-octet random number.
 */
/*************************************************************************************************/
uint32_t meshPrvGenerateRandomNumeric(uint8_t digits)
{
  uint32_t random;
  uint32_t max;

  switch (digits)
  {
    case 1: max = 9; break;
    case 2: max = 99; break;
    case 3: max = 999; break;
    case 4: max = 9999; break;
    case 5: max = 99999; break;
    case 6: max = 999999; break;
    case 7: max = 9999999; break;
    case 8: max = 99999999; break;

    default: return 0;
  }

  /* Generate random integer between 0 and 4.294.967.295 */
  SecRand((uint8_t*)&random, sizeof(uint32_t));

  /* Reduce the large integer modulo the maximum representable on the given number of digits.
     The loss of entropy is, on average, negligible. */
  random = random % (max + 1);

  return random;
}

/*************************************************************************************************/
/*!
 *  \brief     Checks that an array contains only alphanumeric characters.
 *
 *  \param[in] pArray  Array of characters.
 *  \param[in] size    Array size.
 *
 *  \return    TRUE if all characters are alphanumeric, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshPrvIsAlphanumericArray(uint8_t *pArray, uint8_t size)
{
  bool_t isUppercase;
  bool_t isDigit;

  for (uint8_t j = 0; j < size; j++)
  {
    isUppercase = (pArray[j] >= 'A' && pArray[j] <= 'Z');
    isDigit = (pArray[j] >= '0' && pArray[j] <= '9');

    if (!isUppercase && !isDigit)
    {
      return FALSE;
    }
  }

  /* No illegal character was found */
  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Packs OOB data into the AuthValue array.
 *
 *  \param[in] pOutOobArray16B  Authentication value array of 16 octets.
 *  \param[in] oobData          OOB data obtained from the application.
 *  \param[in] oobSize          Size of alphanumeric OOB data, or 0 if OOB data is numeric.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshPrvPackInOutOobToAuthArray(uint8_t* pOutOobArray16B,
                                    meshPrvInOutOobData_t oobData,
                                    uint8_t oobSize)
{
  if (oobSize > 0)
  {
    /* OOB data is alphanumeric - copy as array right-padded with zeros */
    memcpy(pOutOobArray16B, oobData.alphanumericOob, oobSize);
    memset(pOutOobArray16B + oobSize, 0x00, MESH_PRV_AUTH_VALUE_SIZE - oobSize);
  }
  else
  {
    /* OOB data is numeric - copy as big-endian 4-octet number, left-padded with zeros */
    memset(pOutOobArray16B, 0x00, MESH_PRV_AUTH_VALUE_SIZE - MESH_PRV_NUMERIC_OOB_SIZE_OCTETS);
    UINT32_TO_BE_BUF((pOutOobArray16B + MESH_PRV_AUTH_VALUE_SIZE - MESH_PRV_NUMERIC_OOB_SIZE_OCTETS),
                     oobData.numericOob);
  }
}
