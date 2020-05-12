/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Health model common definitions.
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

/*! ***********************************************************************************************
 * \addtogroup MESH_HEALTH_MODELS Mesh Health Models definitions and types
 * @{
 *************************************************************************************************/

#ifndef MESH_HT_MDL_H
#define MESH_HT_MDL_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Health Attention Get Opcode */
#define MESH_HT_ATTENTION_GET_OPCODE       {0x80, 0x04}
/*! \brief Health Attention Set Opcode */
#define MESH_HT_ATTENTION_SET_OPCODE       {0x80, 0x05}
/*! \brief Health Attention Set Unacknowledged Opcode */
#define MESH_HT_ATTENTION_SET_UNACK_OPCODE {0x80, 0x06}
/*! \brief Health Attention Status Opcode */
#define MESH_HT_ATTENTION_STATUS_OPCODE    {0x80, 0x07}

/*! \brief Health Current Status Opcode */
#define MESH_HT_CRT_STATUS_OPCODE          {0x04}

/*! \brief Health Fault Clear Opcode */
#define MESH_HT_FAULT_CLEAR_OPCODE         {0x80, 0x2F}
/*! \brief Health Fault Clear Unacknowledged Opcode */
#define MESH_HT_FAULT_CLEAR_UNACK_OPCODE   {0x80, 0x30}
/*! \brief Health Fault Get Opcode */
#define MESH_HT_FAULT_GET_OPCODE           {0x80, 0x31}
/*! \brief Health Fault Test Opcode */
#define MESH_HT_FAULT_TEST_OPCODE          {0x80, 0x32}
/*! \brief Health Fault Test Unacknowledged Opcode */
#define MESH_HT_FAULT_TEST_UNACK_OPCODE    {0x80, 0x33}
/*! \brief Health Fault Status Opcode */
#define MESH_HT_FAULT_STATUS_OPCODE        {0x05}

/*! \brief Health Period Get Opcode */
#define MESH_HT_PERIOD_GET_OPCODE          {0x80, 0x34}
/*! \brief Health Period Set Opcode */
#define MESH_HT_PERIOD_SET_OPCODE          {0x80, 0x35}
/*! \brief Health Period Set Unacknowledged Opcode */
#define MESH_HT_PERIOD_SET_UNACK_OPCODE    {0x80, 0x36}
/*! \brief Health Period Status Opcode */
#define MESH_HT_PERIOD_STATUS_OPCODE       {0x80, 0x37}

/*! \brief Maximum allowed value for the Fast Period Divizor State */
#define MESH_HT_PERIOD_MAX_VALUE           15

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Heath model fault identifier. See ::meshHtModelFaultValues */
typedef uint8_t meshHtFaultId_t;

/*! \brief Heath model test identifier. See ::meshHtModelTestIdValues */
typedef uint8_t meshHtMdlTestId_t;

/*! Health Period state. It signifies the divider for the Publish Period for this model
 *
 *  \remarks Divider is exponential. A value of 2 means that Publish Period is divided by pow(2,2)
 */
typedef uint8_t meshHtPeriod_t;

/*! Attention Timer state. Zero means attention timer is disabled, non-zero is remaining time
 *  in seconds
 */
typedef uint8_t meshHtAttTimer_t;

#ifdef __cplusplus
}
#endif

#endif /* MESH_HT_MDL_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
