/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI Constant Tone Extension (CTE) command module.
 *
 *  Copyright (c) 2018 Arm Ltd. All Rights Reserved.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_queue.h"
#include "wsf_timer.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "hci_cmd.h"
#include "hci_tr.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_core_ps.h"

#include "ll_api.h"

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set connection CTE receive parameters command.
 *
 *  \param      connHandle       Connection handle.
 *  \param      samplingEnable   TRUE to enable Connection IQ sampling, FALSE to disable it.
 *  \param      slotDurations    Switching and sampling slot durations to be used while receiving CTE.
 *  \param      switchPatternLen Number of Antenna IDs in switching pattern.
 *  \param      pAntennaIDs      List of Antenna IDs in switching pattern.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetConnCteRxParamsCmd(uint16_t connHandle, uint8_t samplingEnable, uint8_t slotDurations,
                                uint8_t switchPatternLen, uint8_t *pAntennaIDs)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set connection CTE transmit parameters command.
 *
 *  \param      connHandle       Connection handle.
 *  \param      cteTypeBits      Permitted CTE type bits used for transmitting CTEs requested by peer.
 *  \param      switchPatternLen Number of Antenna IDs in switching pattern.
 *  \param      pAntennaIDs      List of Antenna IDs in switching pattern.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetConnCteTxParamsCmd(uint16_t connHandle, uint8_t cteTypeBits, uint8_t switchPatternLen,
                                uint8_t *pAntennaIDs)
{
  /* unused */
}

/*************************************************************************************************/
/*!
  *  \brief      HCI LE connection CTE request enable command.
  *
  *  \param      connHandle  Connection handle.
  *  \param      enable      TRUE to enable CTE request for connection, FALSE to disable it.
  *  \param      cteReqInt   CTE request interval.
  *  \param      reqCteLen   Minimum length of CTE being requested in 8 us units.
  *  \param      reqCteType  Requested CTE type.
  *
  *  \return     None.
  */
/*************************************************************************************************/
void HciLeConnCteReqEnableCmd(uint16_t connHandle, uint8_t enable, uint16_t cteReqInt,
                              uint8_t reqCteLen, uint8_t reqCteType)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE connection CTE response enable command.
 *
 *  \param      connHandle  Connection handle.
 *  \param      enable      TRUE to enable CTE response for connection, FALSE to disable it.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeConnCteRspEnableCmd(uint16_t connHandle, uint8_t enable)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE read antenna information command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeReadAntennaInfoCmd(void)
{
  /* unused */
}
