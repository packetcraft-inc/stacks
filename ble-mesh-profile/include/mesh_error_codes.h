/*************************************************************************************************/
/*!
 *  \file   mesh_error_codes.h
 *
 *  \brief  Error code definitions.
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

/*! ***********************************************************************************************
 * @addtogroup MESH_CORE_API
 * @{
 *************************************************************************************************/

#ifndef MESH_ERROR_CODES_H
#define MESH_ERROR_CODES_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Gets module base from error code. */
#define MESH_ERR_CODE_TO_MODULE(errCode)   ((uint8_t)((errCode) >> 8))

/*! \brief Gets base code from error code. */
#define MESH_ERR_CODE_TO_BASE_ERR(errCode) ((uint8_t)((errCode) & 0xFF))

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Mesh stack return value (status) enumeration */
enum meshReturnValues
{
/*! Mask that can be used to determine the module that triggered the error */
  MESH_MODULE_BASE_MASK              = 0xFF00,
/*! Mask that can be used to determine the error code */
  MESH_RETURN_VALUE_MASK             = 0x00FF,

/* Mesh Stack module base values */

/*! General return value base */
  MESH_RETVAL_BASE                   = 0x0000,
/*! Mesh Provisioning return value base */
  MESH_PRV_RETVAL_BASE               = 0x0100,
/*! Mesh Bearer return value base */
  MESH_BR_RETVAL_BASE                = 0x0200,
/*! Mesh Advertising bearer return value base */
  MESH_ADV_BR_RETVAL_BASE            = 0x0300,
/*! Mesh GATT bearer return value base */
  MESH_GATT_BR_RETVAL_BASE           = 0x0400,
/*! Mesh Network return value base */
  MESH_NWK_RETVAL_BASE               = 0x0500,
/*! Mesh Network cache return value base */
  MESH_NWK_CACHE_RETVAL_BASE         = 0x0600,
/*! Mesh Lower Transport return value base */
  MESH_LTR_RETVAL_BASE               = 0x0700,
/*! Mesh SAR RX return value base */
  MESH_SAR_RX_RETVAL_BASE            = 0x0800,
/*! Mesh SAR TX return value base */
  MESH_SAR_TX_RETVAL_BASE            = 0x0900,
/*! Mesh Upper Transport return value base */
  MESH_UTR_RETVAL_BASE               = 0x0A00,
/*! Mesh Access return value base */
  MESH_ACC_RETVAL_BASE               = 0x0B00,
/*! Mesh Sequence Manager return value base */
  MESH_SEQ_RETVAL_BASE               = 0x0C00,
/*! Mesh Local Configuration return value base */
  MESH_LOCAL_CFG_RETVAL_BASE         = 0x0D00,
/*! Mesh Security Toolbox return value base */
  MESH_SEC_TOOL_RETVAL_BASE          = 0x0E00,
/*! Mesh Security return value base */
  MESH_SEC_RETVAL_BASE               = 0x0F00,
/*! Mesh Timer return value base */
  MESH_TMR_RETVAL_BASE               = 0x1000,

/* Mesh Stack generic return values */

/*! Operation completed without errors */
  MESH_SUCCESS                       = 0x0000,
/*! A general failure.  A more specific reason may be defined within event. */
  MESH_FAILURE                       = 0x0001,
/*! Memory allocation failed */
  MESH_OUT_OF_MEMORY                 = 0x0002,
/*! Invalid parameter(s) in request */
  MESH_INVALID_PARAM                 = 0x0003,
/*! Invalid configuration (params could be valid) */
  MESH_INVALID_CONFIG                = 0x0004,
/*! No resources available to complete request */
  MESH_NO_RESOURCES                  = 0x0005,
/*! An OS error has occurred */
  MESH_OS_ERROR                      = 0x0006,
/*! An error not covered by other codes occurred */
  MESH_UNKNOWN_ERROR                 = 0x0007,
/*! Init function already called */
  MESH_ALREADY_INITIALIZED           = 0x0008,
/*! Init function not called */
  MESH_UNINITIALIZED                 = 0x0009,
/*! Insufficient memory allocation */
  MESH_INSUFFICIENT_MEMORY           = 0x000A,
/*! Duplicate entry found. */
  MESH_ALREADY_EXISTS                = 0x000B,
/*! Invalid interface. */
  MESH_INVALID_INTERFACE             = 0x000C,
/*! Last member of the common return values */
  MESH_LAST_COMMON_RETVAL            = 0x000F,

/* Mesh API return values */

/*! An OS error occurred during init */
  MESH_INIT_OS_ERROR                 = MESH_RETVAL_BASE | MESH_OS_ERROR,
/*! At least one of the configuration parameters is invalid */
  MESH_INIT_INVALID_CONFIG           = MESH_RETVAL_BASE | MESH_INVALID_CONFIG,
/*! An error not covered by other codes occurred */
  MESH_INIT_UNKNOWN_ERROR            = MESH_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Factory reset procedure encountered an error */
  MESH_RESET_FAILED                  = MESH_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 1),
/*! No interface available to send a message */
  MESH_MSG_TX_NO_INTERFACE           = MESH_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 2),

/* Mesh Provisioning API return values */

/*! Unable to allocate more memory */
  MESH_PRV_OUT_OF_MEMORY             = MESH_PRV_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! The function parameter has invalid value */
  MESH_PRV_INVALID_PARAM             = MESH_PRV_RETVAL_BASE | MESH_INVALID_PARAM,
/*! Invalid static key received */
  MESH_PRV_INVALID_STATIC_KEY        = MESH_PRV_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 1),
/*! Requested provisioning capabilities are not supported by the stack */
  MESH_PRV_UNSUPPORTED_CAPABILITIES  = MESH_PRV_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 2),
/*! Error occurred during advertising */
  MESH_PRV_ADV_ERROR                 = MESH_PRV_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 3),

/* Mesh bearer return values */

/*! Memory allocation failed */
  MESH_BR_OUT_OF_MEMORY              = MESH_BR_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! At least one of the parameters is invalid */
  MESH_BR_INVALID_PARAMS             = MESH_BR_RETVAL_BASE | MESH_INVALID_PARAM,
/*! Invalid configuration (params could be valid) */
  MESH_BR_INVALID_CONFIG             = MESH_BR_RETVAL_BASE | MESH_INVALID_CONFIG,
/*! An error not covered by other codes occurred */
  MESH_BR_UNKNOWN_ERROR              = MESH_BR_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Bearer is already initialized */
  MESH_BR_ALREADY_INITIALIZED        = MESH_BR_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! An invalid bearer interface was used */
  MESH_BR_INVALID_INTERFACE          = MESH_BR_RETVAL_BASE | (MESH_INVALID_INTERFACE),

/* Mesh advertising bearer return values */

/*! Unable to allocate more memory */
  MESH_ADV_OUT_OF_MEMORY             = MESH_ADV_BR_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! At least one of the parameters is invalid */
  MESH_ADV_INVALID_PARAMS            = MESH_ADV_BR_RETVAL_BASE | MESH_INVALID_PARAM,
/*! Invalid configuration (params could be valid) */
  MESH_ADV_INVALID_CONFIG            = MESH_ADV_BR_RETVAL_BASE | MESH_INVALID_CONFIG,
/*! An error not covered by other codes occurred */
  MESH_ADV_UNKNOWN_ERROR             = MESH_ADV_BR_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Advertising bearer is already initialized */
  MESH_ADV_ALREADY_INITIALIZED       = MESH_ADV_BR_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! No more interfaces available */
  MESH_ADV_NO_RESOURCES              = MESH_ADV_BR_RETVAL_BASE | MESH_NO_RESOURCES,
/*! Interface already exists */
  MESH_ADV_ALREADY_EXISTS            = MESH_ADV_BR_RETVAL_BASE | MESH_ALREADY_EXISTS,
/*! An invalid advertising interface was used */
  MESH_ADV_INVALID_INTERFACE         = MESH_ADV_BR_RETVAL_BASE | (MESH_INVALID_INTERFACE),
/*! No more entries available in the TX queue */
  MESH_ADV_QUEUE_FULL                = MESH_ADV_BR_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 2),
/*! TX queue is empty */
  MESH_ADV_QUEUE_EMPTY               = MESH_ADV_BR_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 3),

/* Mesh GATT bearer return values */

/*! Unable to allocate more memory */
  MESH_GATT_OUT_OF_MEMORY             = MESH_GATT_BR_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! At least one of the parameters is invalid */
  MESH_GATT_INVALID_PARAMS            = MESH_GATT_BR_RETVAL_BASE | MESH_INVALID_PARAM,
/*! Invalid configuration (params could be valid) */
  MESH_GATT_INVALID_CONFIG            = MESH_GATT_BR_RETVAL_BASE | MESH_INVALID_CONFIG,
/*! An error not covered by other codes occurred */
  MESH_GATT_UNKNOWN_ERROR             = MESH_GATT_BR_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Advertising bearer is already initialized */
  MESH_GATT_ALREADY_INITIALIZED       = MESH_GATT_BR_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! No more interfaces available */
  MESH_GATT_NO_RESOURCES              = MESH_GATT_BR_RETVAL_BASE | MESH_NO_RESOURCES,
/*! Interface already exists */
  MESH_GATT_ALREADY_EXISTS            = MESH_GATT_BR_RETVAL_BASE | MESH_ALREADY_EXISTS,
/*! An invalid advertising interface was used */
  MESH_GATT_INVALID_INTERFACE         = MESH_GATT_BR_RETVAL_BASE | (MESH_INVALID_INTERFACE),

/* Mesh network return values */

/*! Unable to allocate more memory */
  MESH_NWK_OUT_OF_MEMORY             = MESH_NWK_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! The function parameter has invalid value */
  MESH_NWK_INVALID_PARAMS            = MESH_NWK_RETVAL_BASE | MESH_INVALID_PARAM,
/*! An error not covered by other codes occured */
  MESH_NWK_UNKNOWN_ERROR             = MESH_NWK_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Network is already initialized */
  MESH_NWK_ALREADY_INITIALIZED       = MESH_NWK_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! Error occured during transport PDU send */
  MESH_NWK_TRANSPORT_ERROR           = MESH_NWK_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 1),
/*! Error occurred during network initialization */
  MESH_NWK_INIT_ERROR                = MESH_NWK_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 2),
/*! Interface ID does not exist */
  MESH_NWK_INVALID_INTERFACE_ID      = MESH_NWK_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 3),
/*! DST address is unassigned address */
  MESH_NWK_INVALID_DST               = MESH_NWK_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 4),
/*! SRC address is not a unicast address */
  MESH_NWK_INVALID_SRC               = MESH_NWK_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 5),
/*! Invalid TTL value */
  MESH_NWK_INVALID_TTL               = MESH_NWK_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 6),
/*! NID cannot be matched to any known network keys */
  MESH_NWK_UNKNOWN_NID               = MESH_NWK_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 7),

/* Mesh network cache return values */

/*! The function parameter has invalid value */
  MESH_NWK_CACHE_INVALID_PARAM       = MESH_NWK_CACHE_RETVAL_BASE | MESH_INVALID_PARAM,
/*! Insufficient memory was provided */
  MESH_NWK_CACHE_INSUFFICIENT_MEMORY = MESH_NWK_CACHE_RETVAL_BASE | MESH_INSUFFICIENT_MEMORY,
/*! Network PDU already exists in the Network Cache */
  MESH_NWK_CACHE_ALREADY_EXISTS      = MESH_NWK_CACHE_RETVAL_BASE | MESH_ALREADY_EXISTS,

/* Mesh lower transport return values */

/*! At least one of the parameters is invalid */
  MESH_LTR_INVALID_PARAMS            = MESH_LTR_RETVAL_BASE | MESH_INVALID_PARAM,
/*! An error not covered by other codes occurred */
  MESH_LTR_UNKNOWN_ERROR             = MESH_LTR_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Lower Transport is already initialized */
  MESH_LTR_ALREADY_INITIALIZED       = MESH_LTR_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! Lower Transport not initialized */
  MESH_LTR_UNINITIALIZED             = MESH_LTR_RETVAL_BASE | MESH_UNINITIALIZED,

/* Mesh SAR Rx return values */

/*! No memory to process the request */
  MESH_SAR_RX_OUT_OF_MEMORY          = MESH_SAR_RX_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! Invalid parameters in the request */
  MESH_SAR_RX_INVALID_PARAMS         = MESH_SAR_RX_RETVAL_BASE | MESH_INVALID_PARAM,

/* Mesh SAR Tx return values */

/*! Buffer allocation failed */
  MESH_SAR_TX_OUT_OF_MEMORY          = MESH_SAR_TX_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! At least one of the parameters is invalid */
  MESH_SAR_TX_INVALID_PARAMS         = MESH_SAR_TX_RETVAL_BASE | MESH_INVALID_PARAM,
/*! No SAR Tx transaction could be found */
  MESH_SAR_TX_NO_TRANSACTION         = MESH_SAR_TX_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 1),

/* Mesh upper transport return values */

/*! At least one of the parameters is invalid */
  MESH_UTR_INVALID_PARAMS            = MESH_UTR_RETVAL_BASE | MESH_INVALID_PARAM,
/*! Buffer allocation failed */
  MESH_UTR_OUT_OF_MEMORY             = MESH_UTR_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! An error not covered by other codes occurred */
  MESH_UTR_UNKNOWN_ERROR             = MESH_UTR_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Upper Transport is already initialized */
  MESH_UTR_ALREADY_INITIALIZED       = MESH_UTR_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! Upper Transport not initialized */
  MESH_UTR_UNINITIALIZED             = MESH_UTR_RETVAL_BASE | MESH_UNINITIALIZED,

/* Mesh access return values */

/*! Memory allocation failed */
  MESH_ACC_OUT_OF_MEMORY             = MESH_ACC_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! At least one of the parameters is invalid */
  MESH_ACC_INVALID_PARAMS            = MESH_ACC_RETVAL_BASE | MESH_INVALID_PARAM,
/*! An error occurred but the reason is none of the above */
  MESH_ACC_UNKNOWN_ERROR             = MESH_ACC_RETVAL_BASE | MESH_UNKNOWN_ERROR,

/* Mesh sequence manager return values */

/*! At least one of the parameters is invalid */
  MESH_SEQ_INVALID_PARAMS            = MESH_SEQ_RETVAL_BASE | MESH_INVALID_PARAM,
/*! An error not covered by other codes occured */
  MESH_SEQ_UNKNOWN_ERROR             = MESH_SEQ_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Sequence Manager is already initialized */
  MESH_SEQ_ALREADY_INITIALIZED       = MESH_SEQ_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! Sequence Manager API called prior to initialization */
  MESH_SEQ_INIT_ERROR                = MESH_SEQ_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 1),
/*! The address passed as parameter is an invalid address */
  MESH_SEQ_INVALID_ADDRESS           = MESH_SEQ_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 2),
/*! The sequence number has reached maximum value */
  MESH_SEQ_EXHAUSTED                 = MESH_SEQ_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 3),

/* Mesh local configuration return values */

/*! No resources to service requests */
  MESH_LOCAL_CFG_OUT_OF_MEMORY       = MESH_LOCAL_CFG_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! At least one of the parameters is invalid */
  MESH_LOCAL_CFG_INVALID_PARAMS      = MESH_LOCAL_CFG_RETVAL_BASE | MESH_INVALID_PARAM,
/*! An error occurred but the reason is none of the above */
  MESH_LOCAL_CFG_UNKNOWN_ERROR       = MESH_LOCAL_CFG_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Requested element already exists */
  MESH_LOCAL_CFG_ALREADY_EXIST       = MESH_LOCAL_CFG_RETVAL_BASE | MESH_ALREADY_EXISTS,
/*! Invalid operation was executed */
  MESH_LOCAL_CFG_INVALID_OPERATION   = MESH_LOCAL_CFG_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 1),
/*! Requested element not found */
  MESH_LOCAL_CFG_NOT_FOUND           = MESH_LOCAL_CFG_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 2),

/* Mesh security toolbox return values */

/*! No resources to service requests */
  MESH_SEC_TOOL_OUT_OF_MEMORY        = MESH_SEC_TOOL_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! Invalid parameters in request */
  MESH_SEC_TOOL_INVALID_PARAMS       = MESH_SEC_TOOL_RETVAL_BASE | MESH_INVALID_PARAM,
/*! An error occured but the reason is none of the above */
  MESH_SEC_TOOL_UNKNOWN_ERROR        = MESH_SEC_TOOL_RETVAL_BASE | MESH_UNKNOWN_ERROR,

/* Mesh security return values */

/*! No resources available for the operation. Either the cryptographic framework is busy or the
 *  module is out of memory
 */
  MESH_SEC_OUT_OF_MEMORY             = MESH_SEC_RETVAL_BASE | MESH_OUT_OF_MEMORY,
/*! Invalid parameters */
  MESH_SEC_INVALID_PARAMS            = MESH_SEC_RETVAL_BASE | MESH_INVALID_PARAM,
/*! The requested key is not stored in the module */
  MESH_SEC_KEY_NOT_FOUND             = MESH_SEC_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 1),
/*! The key derivation material is not available for the given key index */
  MESH_SEC_KEY_MATERIAL_NOT_FOUND    = MESH_SEC_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 2),
/*! The key derivation material already exists for the specified key index */
  MESH_SEC_KEY_MATERIAL_EXISTS       = MESH_SEC_RETVAL_BASE | (MESH_LAST_COMMON_RETVAL + 3),

/* Mesh Timer return values */

/*! At least one of the parameters is invalid */
  MESH_TMR_INVALID_PARAMS      = MESH_TMR_RETVAL_BASE | MESH_INVALID_PARAM,
/*! An error not covered by other codes occured */
  MESH_TMR_UNKNOWN_ERROR       = MESH_TMR_RETVAL_BASE | MESH_UNKNOWN_ERROR,
/*! Timer is already initialized */
  MESH_TMR_ALREADY_INITIALIZED = MESH_TMR_RETVAL_BASE | MESH_ALREADY_INITIALIZED,
/*! Timer not initialized */
  MESH_TMR_UNINITIALIZED       = MESH_TMR_RETVAL_BASE | MESH_UNINITIALIZED,
};

#ifdef __cplusplus
}
#endif

#endif /* MESH_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
