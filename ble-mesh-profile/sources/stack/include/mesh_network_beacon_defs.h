/*************************************************************************************************/
/*!
 *  \file   mesh_nwk_beacon_defs.h
 *
 *  \brief  Secure Network Beacon definitions.
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

#ifndef MESH_NWK_BEACON_DEFS_H
#define MESH_NWK_BEACON_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Secure Network Beacon authentication value size in bytes. */
#define MESH_NWK_BEACON_AUTH_NUM_BYTES       8

/*! Mesh Secure Network Beacon Flags byte position. */
#define MESH_NWK_BEACON_FLAGS_BYTE_POS       1

/*! Mesh Secure Network Beacon Key Refresh Bit shift */
#define MESH_NWK_BEACON_KEY_REF_FLAG_SHIFT   0

/*! Mesh Secure Network Beacon Key Refresh Bit shift */
#define MESH_NWK_BEACON_IV_UPDT_FLAG_SHIFT   1

/*! Mesh Secure Network Beacon Network ID start byte. */
#define MESH_NWK_BEACON_NWK_ID_START_BYTE    2

/*! Mesh Secure Network Beacon IV start byte. */
#define MESH_NWK_BEACON_IV_START_BYTE        10

/*! Maximum allowed difference between received IV index and local IV index */
#define MESH_NWK_BEACON_MAX_IV_DIFF          42

#ifdef __cplusplus
}
#endif

#endif /* MESH_NWK_BEACON_DEFS_H */
