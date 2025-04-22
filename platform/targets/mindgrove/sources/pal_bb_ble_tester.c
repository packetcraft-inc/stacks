/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Baseband tester interface file.
 *
 *  Copyright (c) 2013-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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

#include "pal_types.h"
#include "pal_bb.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

extern int8_t bbTxTifsAdj;
extern uint16_t bbModifyTxHdrMask;
extern uint16_t bbModifyTxHdrValue;
extern uint8_t bbTxCrcInitInvalidStep;
extern uint8_t bbTxAccAddrInvalidStep;
extern uint8_t bbRxCrcInitInvalidStep;
extern uint8_t bbRxAccAddrInvalidStep;
extern uint32_t bbTxCrcInitInvalidAdjMask;
extern uint32_t bbTxAccAddrInvalidAdjMask;
extern uint32_t bbRxCrcInitInvalidAdjMask;
extern uint32_t bbRxAccAddrInvalidAdjMask;
extern uint64_t bbTxCrcInitInvalidChanMask;
extern uint64_t bbTxAccAddrInvalidChanMask;
extern uint64_t bbRxCrcInitInvalidChanMask;
extern uint64_t bbRxAccAddrInvalidChanMask;
extern bool_t bbBlePduFiltEnableBypass;
extern bool_t bbRxAccAddrShiftMask;
extern bool_t bbTxAccAddrShiftMask;
extern bool_t bbTxAccAddrShiftInc;
extern bool_t invalidateAccAddrOnceRx;
extern bool_t invalidateAccAddrOnceTx;

#define NSEC_PER_USEC             1000    /* Convert Nanoseconds to Microseconds */
/* adjust prescaler for the clock rate */
#if   (BB_CLK_RATE_HZ == 1000000)
  #define TICKS_PER_USEC    1
#elif (BB_CLK_RATE_HZ == 2000000)
  #define TICKS_PER_USEC    2
#elif (BB_CLK_RATE_HZ == 4000000)
  #define TICKS_PER_USEC    4
#elif (BB_CLK_RATE_HZ == 8000000)
  #define TICKS_PER_USEC    8
#elif (BB_CLK_RATE_HZ == 32768)
  #define TICKS_PER_USEC    1
#else
  #error "Unsupported clock rate."
#endif

/*************************************************************************************************/
/*!
 *  \brief      Adjust Tx TIFS timing value.
 *
 *  \param      adjNs       Adjustment value in nanoseconds.
 *
 *  Adjust the TIFS timing of transmit by the given signed value of timer ticks.
 *  If adjustment value is out of range, maximum allowed value is used.
 */
/*************************************************************************************************/
void PalBbTesterAdjTxTifsNs(int16_t adjNs)
{
  /* set adjustment global variable the is added to TIFS transmit */
  bbTxTifsAdj = adjNs * TICKS_PER_USEC / NSEC_PER_USEC;
}

/*************************************************************************************************/
/*!
 *  \brief      Trigger channel modifications on matching Tx packet header.
 *
 *  \param      hdrMask     Header mask.
 *  \param      hdrValue    Match value.
 *
 *  Modify the transmit channel parameters of a packet only when the Tx packet header matches
 *  the given parameters. This applies to the modification parameter provided by the following
 *  routines:
 *
 *      - PalBbTesterSetInvalidCrcInit()
 *      - PalBbTesterSetInvalidAccessAddress()
 */
/*************************************************************************************************/
void PalBbTesterSetModifyTxPktTrigger(uint16_t hdrMask, uint16_t hdrValue)
{
  bbModifyTxHdrMask = hdrMask;
  bbModifyTxHdrValue = hdrValue;
}

/*************************************************************************************************/
/*!
 *  \brief      Invalidate CRC initialization value for adjCount packets.
 *
 *  \param      chanMask    Adjustment channel mask.
 *  \param      adjMask     Number of adjustments (0 to disable).
 *  \param      forRx       TRUE for Rx, FALSE for Tx.
 *
 *  Force the receiver to receiver a packet with CRC error up to adjCount packets.
 */
/*************************************************************************************************/
void PalBbTesterSetInvalidCrcInit(uint64_t chanMask, uint32_t adjMask, bool_t forRx)
{
  if (forRx)
  {
    bbRxCrcInitInvalidChanMask = chanMask;
    bbRxCrcInitInvalidAdjMask = adjMask;
    bbRxCrcInitInvalidStep = 0;
  }
  else
  {
    bbTxCrcInitInvalidChanMask = chanMask;
    bbTxCrcInitInvalidAdjMask = adjMask;
    bbTxCrcInitInvalidStep = 0;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Invalidate access address value for adjCount packets.
 *
 *  \param      chanMask    Adjustment channel mask.
 *  \param      adjMask     Number of adjustments (0 to disable).
 *  \param      shiftMask   TRUE if corrupting AA by one bit and corrupted bit location is shifted every TX/RX.
 *  \param      forRx       TRUE for Rx, FALSE for Tx.
 *
 *  Force the receiver to receiver a miss a packet up to adjCount packets.
 */
/*************************************************************************************************/
void PalBbTesterSetInvalidAccessAddress(uint64_t chanMask, uint32_t adjMask, bool_t shiftMask, bool_t forRx)
{
  if (forRx)
  {
    bbRxAccAddrInvalidChanMask = chanMask;
    bbRxAccAddrInvalidAdjMask = adjMask;
    bbRxAccAddrInvalidStep = 0;
    bbRxAccAddrShiftMask = shiftMask;
  }
  else
  {
    bbTxAccAddrInvalidChanMask = chanMask;
    bbTxAccAddrInvalidAdjMask = adjMask;
    bbTxAccAddrInvalidStep = 0;
    bbTxAccAddrShiftMask = shiftMask;
    if (bbTxAccAddrShiftMask == TRUE)
    {
      bbTxAccAddrInvalidStep = 1;
      bbTxAccAddrShiftInc = TRUE;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Enable PDU filtering bypass.
 *
 *  \param      enable      Enable bypass.
 *
 *  Enable all PDU to pass through filtering.
 */
/*************************************************************************************************/
void PalBbTesterEnablePduFilterBypass(bool_t enable)
{
  bbBlePduFiltEnableBypass = enable;
}

/*************************************************************************************************/
/*!
 *  \brief      Invalidate next access address.
 *
 *  \param      forRx    For Rx or Tx boolean.
 */
/*************************************************************************************************/
void PalBbTesterInvalidateNextAccAddr(bool_t forRx)
{
  if (forRx)
  {
    invalidateAccAddrOnceRx = TRUE;
  }
  else
  {
    invalidateAccAddrOnceTx = TRUE;
  }
}
