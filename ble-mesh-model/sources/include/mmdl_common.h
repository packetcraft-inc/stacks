/*************************************************************************************************/
/*!
 *  \file   mmdl_common.h
 *
 *  \brief  Interface of the common model utilities.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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

#ifndef MMDL_COMMON_H
#define MMDL_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

/*! Transform an opcode on 1 byte to a 3-byte array */
#define UINT8_OPCODE_TO_BYTES(oc)   (oc), 0x00, 0x00

/*! Transform an opcode on 2 bytes to a 3-byte array */
#define UINT16_OPCODE_TO_BYTES(oc)   UINT16_TO_BE_BYTES(oc), 0x00

/*! Transform an opcode on 3 bytes to a 3-byte array */
#define UINT24_OPCODE_TO_BYTES(oc)   UINT24_TO_BE_BYTES(oc)

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Generic empty event callback, used by all models.
 *
 *  \param[in] pEvent  Pointer to model event
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlEmptyCback(const wsfMsgHdr_t *pEvent);

/*************************************************************************************************/
/*!
*  \brief     Return size of a Mesh Model callback event.
*
*  \param[in] pMeshCfgEvt  Mesh Model callback event.
*
*  \return    Size of Mesh Model callback event.
*/
/*************************************************************************************************/
uint16_t MmdlSizeOfEvt(wsfMsgHdr_t *pEvt);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_COMMON_H */
