/*************************************************************************************************/
/*!
 *  \file   mesh_prv_common.h
 *
 *  \brief  Mesh Provisioning common internal module interface.
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

#ifndef MESH_PRV_COMMON_H
#define MESH_PRV_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void meshPrvGenerateRandomAlphanumeric(uint8_t* pOutArray, uint8_t size);
uint32_t meshPrvGenerateRandomNumeric(uint8_t digits);
bool_t meshPrvIsAlphanumericArray(uint8_t *pArray, uint8_t size);
void meshPrvPackInOutOobToAuthArray(uint8_t* pOutOobArray16B,
                                    meshPrvInOutOobData_t oobData,
                                    uint8_t oobSize);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_COMMON_H */
