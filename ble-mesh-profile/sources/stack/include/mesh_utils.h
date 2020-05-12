/*************************************************************************************************/
/*!
 *  \file   mesh_utils.h
 *
 *  \brief  Utility function declarations.
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

/*************************************************************************************************/
/*!
 *  \addtogroup MESH_UTILS Mesh Helper functions API
 *  @{
 */
/*************************************************************************************************/

#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Sets the bit on position "pos" */
#define MESH_UTILS_BIT_SET(var, pos)         ((var) |= (1 << (pos)))
/*! Clears the bit on position 'pos" */
#define MESH_UTILS_BIT_CLR(var, pos)         ((var) &= ~(1 << (pos)))
/*! Flips the bit on position "pos" */
#define MESH_UTILS_BIT_FLP(var, pos)         ((var) ^= (1 << (pos)))
/*! Checks the bit value on position "pos" */
#define MESH_UTILS_BIT_CHK(var, pos)         ((var) & (1 << (pos)))

/*! Sets the bitmask "mask" */
#define MESH_UTILS_BITMASK_SET(var, mask)    ((var) |= (mask))
/*! Clears the bitmask "mask" */
#define MESH_UTILS_BITMASK_CLR(var, mask)    ((var) &= (~(mask)))
/*! Flips the bitmask "mask"*/
#define MESH_UTILS_BITMASK_FLP(var, mask)    ((var) ^= (mask))
/*! Checks the bitmask "mask" */
#define MESH_UTILS_BITMASK_CHK(var, mask)    (((var) & (mask)) == (mask))
/*! Checks that only the bits in bitmask are set */
#define MESH_UTILS_BITMASK_XCL(var, mask)    (((var) & ~(mask)) == 0)
/*! Creates a bitmask of given length */
#define MESH_UTILS_BTMASK_MAKE(len)          ((1 << (len)) - 1)

/*! Creates a bitfield mask of given length at given start bit */
#define MESH_UTILS_BFMASK_MAKE(start, len)         (MESH_UTILS_BTMASK_MAKE(len) << (start))
/*! Prepares a bitmask for insertion or combining */
#define MESH_UTILS_BFMASK_PREP(val, start, len)    (((val) & MESH_UTILS_BTMASK_MAKE(len)) \
                                                   << (start))
/*! Extracts a bitfiled of length "len" starting at bit "start" from a value "val" */
#define MESH_UTILS_BF_GET(val, start, len)         (((val) >> (start)) & \
                                                   MESH_UTILS_BTMASK_MAKE(len))
/*! Inserts a bitfield value "val2" into "val1" value */
#define MESH_UTILS_BF_SET(val1, val2, start, len)  ((val1) = ((val1) & \
                                                   ~ MESH_UTILS_BFMASK_MAKE(start, len)) | \
                                                   MESH_UTILS_BFMASK_PREP(val2, start, len))

/*! Aligns a value to the architecture instruction set size */
#define MESH_UTILS_ALIGN(value)                    (((value) + (sizeof(void *) - 1)) &\
                                                   ~(sizeof(void *) - 1))

/*! Validates if a value is aligned to the architecture instruction set size */
#define MESH_UTILS_IS_ALIGNED(value)               (((uint32_t)(value) &\
                                                    (sizeof(void *) - 1)) == 0)

/*! Transforms a Log Field value into a 4-octet value */
#define MESH_UTILS_GET_4OCTET_VALUE(value)         ((uint32_t)(1) << ((value) - 1))

/**************************************************************************************************
  Function Declarations
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
uint8_t MeshUtilsGetLogFieldVal(uint16_t val);

#ifdef __cplusplus
}
#endif

#endif /* MESH_UTILS_H */

/*************************************************************************************************/
/*!
 *  @}
 */
/*************************************************************************************************/
