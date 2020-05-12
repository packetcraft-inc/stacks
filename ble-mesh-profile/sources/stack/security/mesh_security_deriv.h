/*************************************************************************************************/
/*!
 *  \file   mesh_security_deriv.h
 *
 *  \brief  Security derivation functions.
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

#ifndef MESH_SECURITY_DERIV_H
#define MESH_SECURITY_DERIV_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Security Network Key derivation request */
typedef struct meshSecNetKeyDerivReq_tag
{
  meshSecKeyMaterialDerivCback_t cback;         /*!< User callback */
  void                           *pParam;       /*!< Generic parameter provided in request */
  uint16_t                       netKeyListIdx; /*!< Pointer to processed ::meshSecNetKeyInfo_t or
                                                 *   NULL if request is not in progress
                                                 */
  bool_t                         isUpdate;      /*!< TRUE if request type is update */
  uint16_t                       friendUpdtIdx; /*!< Current index of the updated friendship
                                                 *   material array
                                                 */
} meshSecNetKeyDerivReq_t;

/*! Security application key derivation request */
typedef struct meshSecAppKeyDerivReq_tag
{
  meshSecKeyMaterialDerivCback_t cback;         /*!< User callback */
  void                           *pParam;       /*!< Generic parameter provided in request */
  uint16_t                       appKeyListIdx; /*!< Pointer to processed ::meshSecAppKeyInfo_t or
                                                 *   NULL if request is not in progress
                                                 */
  bool_t                         isUpdate;      /*!< TRUE if request type is update */
} meshSecAppKeyDerivReq_t;

/*! Security Network Key derivation request with friendship credentials */
typedef struct meshSecFriendDerivReq_tag
{
  meshSecFriendCredDerivCback_t  cback;         /*!< User callback */
  void                           *pParam;       /*!< Generic parameter provided in request */
  uint16_t                       friendListIdx; /*!< Pointer to processed ::meshSecFriendMat_t or
                                                 *   NULL if request is not in progress
                                                 */
  uint8_t                        *pK2PBuff;     /*!< Pointer to K2 P buffer */
  uint16_t                       netKeyListIdx; /*!< Pointer to associated Network Key
                                                 *   derivation information
                                                 */
  uint16_t                       netKeyIndex;   /*!< Store user request NetKey Index in case the key
                                                 *   gets removed
                                                 */
  bool_t                         doUpdate;      /*!< TRUE if updated material should be generated */
} meshSecFriendDerivReq_t;

/*! Request sources for key derivation procedures. */
typedef struct meshSecKeyDerivRequests_tag
{
  meshSecAppKeyDerivReq_t appKeyDerivReq;     /*! Identification data for an Application Key based
                                               *  derivation procedure
                                               */
  meshSecNetKeyDerivReq_t netKeyDerivReq;     /*! Identification data for a Network Key
                                               *  based derivation procedure
                                               */
  meshSecFriendDerivReq_t friendMatDerivReq;  /*! Identification data for a Network Key based
                                               *  derivation procedure with friendship
                                               *  credentials
                                               */
} meshSecKeyDerivRequests_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

extern meshSecKeyDerivRequests_t secKeyDerivReq;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Gets address of the friendship material based on the friend or LPN address and
 *              associated Network Key information.
 *
 *  \param[in]  pKeyInfo        Pointer to key information.
 *  \param[in]  entryId         Key material entry identifier corresponding to either old or new
 *                              Network Key.
 *  \param[in]  searchAddr      Address that needs to match the remote device in the friendship
 *                              material (either friend or LPN).
 *  \param[out] ppOutFriendMat  Pointer to memory where the address of the friendship material is
 *                              stored on success.
 *
 *  \return     Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
meshSecRetVal_t meshSecNetKeyInfoAndAddrToFriendMat(meshSecNetKeyInfo_t *pKeyInfo, uint8_t entryId,
                                                    meshAddress_t searchAddr,
                                                    meshSecFriendMat_t **ppOutFriendMat);

#ifdef __cplusplus
}
#endif

#endif /* MESH_SECURITY_DERIV_H */
