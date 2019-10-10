/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Vendor specific baseband 802.15.4 MAC definitions.
 *
 *  Copyright (c) 2016-2018 ARM Ltd.
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
 */
/*************************************************************************************************/

#ifndef BB_154_DRV_VS_H
#define BB_154_DRV_VS_H

#include "wsf_types.h"
#include "hci_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

/*! \brief      Baseband driver statistics. */
typedef struct
{
  uint32_t txSchMiss;               /*!< Number of missed Rx. */
  uint32_t rxSchMiss;               /*!< Number of missed Tx. */
  uint32_t txPkt;                   /*!< Number of successful Tx. */
  uint32_t txDmaFail;               /*!< Number of Tx DMA failures. */
  uint32_t rxPkt;                   /*!< Number of successful Rx. */
  uint32_t rxPktTimeout;            /*!< Number of Rx timeouts. */
  uint32_t rxFilterFail;            /*!< Number of Rx filter failures. */
  uint32_t rxCrcFail;               /*!< Number of CRC failures. */
  uint32_t rxDmaFail;               /*!< Number of Rx DMA failures. */
  uint32_t edReq;                   /*!< Number of ED requests. */
  uint32_t ed;                      /*!< Number of successful ED. */
  uint32_t edSchMiss;               /*!< Number of missed ED. */
  uint32_t ccaSchMiss;              /*!< Number of missed CCA. */
  uint32_t txReq;                   /*!< Number of Tx requests. */
  uint32_t rxReq;                   /*!< Number of Rx requests. */
} Bb154DrvStats_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void Bb154DrvGetStats(Bb154DrvStats_t *pStats);

#ifdef __cplusplus
};
#endif

#endif /* BB_154_DRV_VS_H */
