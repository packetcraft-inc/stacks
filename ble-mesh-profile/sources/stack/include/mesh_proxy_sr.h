/*************************************************************************************************/
/*!
 *  \file   mesh_proxy_sr.h
 *
 *  \brief  Proxy Server internal module interface.
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

#ifndef MESH_PROXY_SERVER_H
#define MESH_PROXY_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Disables the Proxy Server functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshProxySrDisable(void);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PROXY_SERVER_H */
