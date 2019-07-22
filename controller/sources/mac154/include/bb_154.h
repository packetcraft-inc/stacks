/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      IEEE 802.15.4 MAC Baseband driver interface file.
 *
 *  Copyright (c) 2016-2018 Arm Ltd.
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

#ifndef PAL_BB_154_H
#define PAL_BB_154_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* Define structures as packed if necessary. */
#if defined (__GNUC__)
# define PAL_BB_154_PACKED                __attribute__ ((packed))
#elif defined (__CC_ARM)
# define PAL_BB_154_PACKED                __attribute__ ((packed))
#else
# error "Not supported compiler type."
#endif

/*! \brief      Minimum number of receive buffers. */
#define PAL_BB_154_MIN_RX_BUF_CNT       2         /*!< PAL_BB_154_MIN_RX_BUF_CNT. */
#define PAL_BB_154_RX_BUF_CNT           2         /*!< PAL_BB_154_RX_BUF_CNT. */

/*! \brief      Driver status flags. */
enum
{
  PAL_BB_154_FLAG_RX_ACK_CMPL  = (1 << 0),       /*!< Rx ack. completed. */
  PAL_BB_154_FLAG_TX_ACK_CMPL  = (1 << 1),       /*!< Tx ack. completed. */
  PAL_BB_154_FLAG_RX_ACK_START = (1 << 2),       /*!< Rx ack. started. */
  PAL_BB_154_FLAG_TX_ACK_START = (1 << 3),       /*!< Tx ack. started. */
};

/*! \brief      Operation flags. */
enum
{
  PAL_BB_154_FLAG_TX_AUTO_RX_ACK = (1 << 0),  /*!< Automatically wait for ACK after transmit completes. */
  PAL_BB_154_FLAG_RX_AUTO_TX_ACK = (1 << 1),  /*!< Automatically send ACK after receive completes. */
  PAL_BB_154_FLAG_RX_WHILE_ED    = (1 << 2),  /*!< Receive any packet detected while performing ED. */
  PAL_BB_154_FLAG_DIS_CCA        = (1 << 3),  /*!< Disable CCA before transmit. */
  PAL_BB_154_FLAG_RAW            = (1 << 4)   /*!< Treat as raw frame */
};

/*! \brief      Receive  flags. */
enum
{
  PAL_BB_154_RX_FLAG_GO_IDLE = (1 << 0),    /*!< Can go idle. */
  PAL_BB_154_RX_FLAG_SET_ACK_FP = (1 << 1)  /*!< Set frame pending in ack. */
};

/*! \brief      PAL_BB_154_FLAG_TX_RX_AUTO_ACK. */
#define PAL_BB_154_FLAG_TX_RX_AUTO_ACK (PAL_BB_154_FLAG_TX_AUTO_RX_ACK | PAL_BB_154_FLAG_RX_AUTO_TX_ACK)

/* \brief    Symbols to microseconds for 802.15.4-2006 2.4GHz PHY */
#define PAL_BB_154_SYMB_TO_US(x)                    ((x) * 16)            /*!< PAL_BB_154_SYMB_TO_US. */
#define PAL_BB_154_SYMB_TO_MS(x)                    (((x) * 16) / 1000)   /*!< PAL_BB_154_SYMB_TO_MS. */
#define PAL_BB_154_US_TO_SYMB(x)                    ((x) / 16)            /*!< PAL_BB_154_US_TO_SYMB. */

/* \brief    Transaction persistence time factor */
#define PAL_BB_154_TPT_TO_MS(x)                     (((x) * 15723) >> 10) /*!< 15723/1024 approximates to 15.36 */

/*! \brief    Energy detect threshold in dBm. */
#define PAL_BB_154_ED_THRESHOLD                     -75                   /*!< 10 dBm above 802.15.4 specified -85 dBm */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      802.15.4 channelization parameters. */
typedef struct
{
  uint8_t               channel;            /*!< Channel. */
  int8_t                txPower;            /*!< Transmit power, units dBm. */
} PalBb154Chan_t;

/*! \brief      Transmit complete ISR callback signature. */
typedef void (*PalBb154TxIsr_t)(uint8_t flags);

/*! \brief      Frame pending check callback. */
typedef bool_t (*PalBb154FPIsr_t)(uint8_t srcAddrMode, uint64_t srcAddr);

/*! \brief      Receive complete ISR callback signature. */
typedef uint8_t (*PalBb154RxIsr_t)(uint8_t *pBuf, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags);

/*! \brief      CCA or energy detect complete ISR callback signature. */
typedef void (*PalBb154EdIsr_t)(int8_t rssi);

/*! \brief      Driver error callback signature. */
typedef void (*PalBb154Err_t)(uint8_t status);

/*! \brief      Operation parameters. */
typedef struct
{
  uint8_t         flags;          /*!< Baseband driver operation flags. */
  uint8_t         psduMaxLength;  /*!< Maximum length of PSDU. */
  PalBb154TxIsr_t txCback;        /*!< Transmit complete ISR callback. */
  PalBb154FPIsr_t fpCback;        /*!< Frame pending check callback. */
  PalBb154RxIsr_t rxCback;        /*!< Receive complete ISR callback. */
  PalBb154EdIsr_t edCback;        /*!< ED complete ISR callback. */
  PalBb154Err_t   errCback;       /*!< Error callback. */
} PalBb154OpParam_t;

/*! \brief      Transmit buffer descriptor. */ /* Note - must be packed so buffer immediately follows length */
typedef struct PAL_BB_154_PACKED PalBb154TxBufDesc
{
  uint8_t pad[2]; /*!< Padding to make structure uint32 aligned */
  uint8_t handle; /*!< Handle used for data frames only */
  uint8_t len;    /*!< Length of frame, which is concatenated to this header */
} PalBb154TxBufDesc_t;

/*! \brief      PAL_BB_154_TX_FRAME_PTR */
#define PAL_BB_154_TX_FRAME_PTR(x)           ((uint8_t *)(((PalBb154TxBufDesc_t *)(x))+1))

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize the 802.15.4 baseband driver.
 *
 *  \return     None.
 *
 *  One-time initialization of baseband resources. This routine can be used to setup software
 *  driver resources, load RF trim parameters and execute RF calibrations.
 *
 *  This routine should block until the BB hardware is completely initialized.
 */
/*************************************************************************************************/
void PalBb154Init(void);

/*************************************************************************************************/
/*!
 *  \brief      Enable the BB hardware.
 *
 *  \return     None.
 *
 *  This routine brings the BB hardware out of low power (enable power and clocks). This routine is
 *  called just before a 802.15.4 BOD is executed.
 */
/*************************************************************************************************/
void PalBb154Enable(void);

/*************************************************************************************************/
/*!
 *  \brief      Disable the BB hardware.
 *
 *  \return     None.
 *
 *  This routine signals the BB hardware to go into low power (disable power and clocks). This
 *  routine is called after all 802.15.4 operations are disabled.
 */
/*************************************************************************************************/
void PalBb154Disable(void);

/*************************************************************************************************/
/*!
 *  \brief      Set channelization parameters.
 *
 *  \param      pParam        Channelization parameters.
 *
 *  \return     None.
 *
 *  Calling this routine will set parameters for all future transmit, receive, and energy detect
 *  operations until this routine is called again providing new parameters.
 *
 *  \note       \a pParam is not guaranteed to be static and is only valid in the context of the
 *              call to this routine. Therefore parameters requiring persistence should be copied.
 */
/*************************************************************************************************/
void PalBb154SetChannelParam(const PalBb154Chan_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief      Reset channelization parameters.
 *
 *  \return     None.
 *
 *  Calling this routine will reset (clear) the channelization parameters.
 */
/*************************************************************************************************/
void PalBb154ResetChannelParam(void);

/*************************************************************************************************/
/*!
 *  \brief      Set the operation parameters.
 *
 *  \param      pOpParam    Operations parameters.
 *
 *  \return     None.
 *
 *  Calling this routine will set parameters for all future transmit, receive, ED, and CCA
 *  operations until this routine is called again providing new parameters.
 *
 *  \note       \a pOpParam is not guaranteed to be static and is only valid in the context of the
 *              call to this routine. Therefore parameters requiring persistence should be copied.
 */
/*************************************************************************************************/
void PalBb154SetOpParams(const PalBb154OpParam_t *pOpParam);

/*************************************************************************************************/
/*!
 *  \brief      Flushes PIB attributes to hardware.
 *
 *  \return     None.
 *
 *  Calling this routine will flush all PIB attributes that have a hardware counterpart to the
 *  respective registers in hardware.
 */
/*************************************************************************************************/
void PalBb154FlushPIB(void);

/*************************************************************************************************/
/*!
 *  \brief      Clear all received buffers (active and queued).
 *
 *  \return     None.
 *
 *  Calling this routine will clear and free the active receive buffer (if any) and all queued
 *  receive buffers. This should only be called when the operation is terminating.
 */
/*************************************************************************************************/
void PalBb154ClearRxBufs(void);

/*************************************************************************************************/
/*!
 *  \brief      Reclaim the buffer associated with the received frame.
 *
 *  \param      pRxFrame  Pointer to the received frame.
 *
 *  \return     Total number of receive buffers queued.
 *
 *  Calling this routine will put the buffer associated with the received frame back onto the
 *  receive queue. Note the actual buffer pointer may not be the same as the frame pointer
 *  dependent on driver implementation. If the queue is empty when the driver expects to
 *  transition to the receive state, the driver will instead move into the off state.
 */
/*************************************************************************************************/
uint8_t PalBb154ReclaimRxFrame(uint8_t *pRxFrame);

/*************************************************************************************************/
/*!
 *  \brief      Build receive buffer queue
 *
 *  \param      len     Length of each receive buffer.
 *  \param      num     Number of buffers to load into the queue.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalBb154BuildRxBufQueue(uint16_t len, uint8_t num);

/*************************************************************************************************/
/*!
 *  \brief      Get payload pointer.
 *
 *  \param      pFrame        Frame buffer pointing to first octet of frame
 *  \param      fctl          Frame control field
 *
 *  \return     Pointer to frame payload or NULL if illegal addr mode combo.
 *
 *  Obtains the source and destination addresses from the frame. If either parameter is NULL,
 *  simply skips past the fields
 */
/*************************************************************************************************/
uint8_t *PalBb154GetPayloadPtr(uint8_t *pFrame, uint16_t fctl);

/*************************************************************************************************/
/*!
 *  \brief      Transmit a packet.
 *
 *  \param      pDesc       Chain of transmit buffer descriptors.
 *  \param      cnt         Number of descriptors.
 *  \param      due         Due time for transmit (if \a now is FALSE).
 *  \param      now         TRUE if packet should be transmitted with minimal delay.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalBb154Tx(PalBb154TxBufDesc_t *pDesc, uint8_t cnt, uint32_t due, bool_t now);

/*************************************************************************************************/
/*!
 *  \brief      Receive a packet.
 *
 *  \param      due         Due time for receive (if \a now is FALSE).
 *  \param      now         TRUE if packet should be received with minimal delay.
 *  \param      timeout     Timeout.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalBb154Rx(uint32_t due, bool_t now, uint32_t timeout);

/*************************************************************************************************/
/*!
 *  \brief      Perform energy detect.
 *
 *  \param      due         Due time for energy detect (if \a now is FALSE).
 *  \param      now         TRUE if energy detect should occur minimal delay.
 *
 *  \return     None.
 *
 *  Perform energy detect and return energy level to assess channel status.
 */
/*************************************************************************************************/
void PalBb154Ed(uint32_t due, bool_t now);

/*************************************************************************************************/
/*!
 *  \brief      Cancel any pending operation.
 *
 *  \return     TRUE if pending operation could be cancelled.
 *
 *  Cancel any pending operation.
 */
/*************************************************************************************************/
bool_t PalBb154Off(void);

/*************************************************************************************************/
/*!
 *  \brief      Convert RSSI to LQI.
 *
 *  \return     LQI value.
 *
 *  Converts RSSI value into equivalent LQI value from 0 to 0xFF.
 */
/*************************************************************************************************/
uint8_t PalBb154RssiToLqi(int8_t rssi);

/*************************************************************************************************/
/*!
 *  \brief  Return the last received RSSI.
 *
 *  \param  pBuf            Storage for results -- first element is integer dBm, second is fractional
 *
 *  \return None.
 */
/*************************************************************************************************/
void PalBb154GetLastRssi(uint8_t *pBuf);

/*************************************************************************************************/
/*!
 *  \brief  Enter Continuous Transmit mode.
 *
 *  \param  rfChan           Physical channel number
 *  \param  modulation       Modulation type (0 disables)
 *  \param  txPhy            PHY type
 *  \param  power            Pransmit power (dBm)
 *
 *  \return None.
 */
/*************************************************************************************************/
void PalBb154ContinuousTx(uint8_t rfChan, uint8_t modulation, uint8_t txPhy, int8_t power);

/*************************************************************************************************/
/*!
 *  \brief  Enter Continuous Receive mode.
 *
 *  \param  rfChan           Physical channel number
 *  \param  rxPhy            PHY type
 *
 *  \return None.
 */
/*************************************************************************************************/
void PalBb154ContinuousRx(uint8_t rfChan, uint8_t rxPhy);

/*************************************************************************************************/
/*!
 *  \brief  Stop Continous Transmit or Receive mode.
 *
 *  \return None.
 */
/*************************************************************************************************/
void PalBb154ContinuousStop(void);

#ifdef __cplusplus
};
#endif

#endif /* PAL_BB_154_H */
