/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband driver implementation file.
 *
 *  Copyright (c) 2016-2019 Arm Ltd. All Rights Reserved.
 *  ARM confidential and proprietary.
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
#include "boards.h"
#include "nrf.h"
#include "nrf_clock.h"
#include "nrf_radio.h"
#include "nrf_ppi.h"
#include "nrf_timer.h"
#include "nrf_nvic.h"

#include "pal_bb.h"
#include "pal_sys.h"
#include "pal_bb_154.h"
#include "pal_led.h"

#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define ADDITIONAL_TESTER_FEATURES

/*! \brief    BB assert enable. */
#ifndef BB_ASSERT_ENABLED
#define BB_ASSERT_ENABLED  TRUE
#endif

/* \brief    Enable BB assertions. */
#ifdef BB_ENABLE_ASSERT
/* TODO: This doesn't work for two reasons:
 * Only works in functions returning void
 * Silently returns without any error indication, which is not the purpose of an assert fn.
 */
#define BB_ASSERT(x)       { if (!(x)) {  return; } }
#else
#define BB_ASSERT(x)       ((void)(x))
#endif

/*! \brief    BB test enable. */
#ifndef BB_TEST_ENABLE
#define BB_TEST_ENABLE  TRUE
#endif

/*! \brief    Increment statistic. */
#define BB_154_DRV_INC_STAT(s)                  s++

/*! \brief    BB operation setup time (microseconds). */
#define BB_154_DRV_SETUP_TIME                   10

/*! \brief    Transmit end delay. */
#define BB_154_DRV_TX_END_DELAY                 64

/*! \brief    Inter-operation delay. */
#define BB_154_DRV_INTER_OP_DELAY               0

/*! \brief    Reset delay. */
#define BB_154_DRV_RESET_DELAY                  0

/*! \brief    Test whether time is in future. */
#define BB_154_DRV_TIME_IN_FUTURE(t, ref)       ((uint32_t)((t) - (ref)) < 0x80000000)

/*! \brief    Adjust prescaler for the clock rate.
 * f = 16MHz / (2 ^ TIMER_PRESCALER),  for TIMER_PRESCALER of 4, clock has 1us resolution
 * TIMER_PRESCALER of 3, clock is 0.5us resolution
 * TIMER_PRESCALER of 2, clock is 0.25us resolution
 * TIMER_PRESCALER of 1, clock is 0.125us resolution
 */
#if   (BB_CLK_RATE_HZ == 1000000)
  #define TIMER_PRESCALER   NRF_TIMER_FREQ_1MHz
  #define TICKS_PER_USEC    1
#elif (BB_CLK_RATE_HZ == 2000000)
  #define TIMER_PRESCALER   NRF_TIMER_FREQ_2MHz
  #define TICKS_PER_USEC    2
#elif (BB_CLK_RATE_HZ == 4000000)
  #define TIMER_PRESCALER   NRF_TIMER_FREQ_4MHz
  #define TICKS_PER_USEC    4
#elif (BB_CLK_RATE_HZ == 8000000)
  #define TIMER_PRESCALER   NRF_TIMER_FREQ_8MHz
  #define TICKS_PER_USEC    8
#elif (USE_RTC_BB_CLK)
  #define TIMER_PRESCALER   NRF_TIMER_FREQ_1MHz       /* Use 1MHz for HFCLK */
  #define TICKS_PER_USEC    1
#else
  #error "Unsupported clock rate."
#endif

/*! \brief  Nordic CRC (FCS) setup. */
#define BB_154_DRV_CRC_LENGTH             2           /* Length of CRC in 802.15.4 frames [bytes] */
#define BB_154_DRV_CRC_POLYNOMIAL         0x011021    /* Polynomial used for CRC calculation in 802.15.4 frames */

/*! \brief  Frame offsets. */
#define BB_154_DRV_RX_FRAME_CTRL_OFFSET   1           /* Offset of byte containing Frame Control (LSB) (+1 for frame length byte) */
#define BB_154_DRV_RX_PAN_ID_OFFSET       4           /* Offset of byte containing PAN ID (LSB) (+1 for frame length byte) */
#define BB_154_DRV_RX_DEST_ADDR_OFFSET    6           /* Offset of byte containing destination address (LSB) (+1 for frame length byte) */

/*! \brief  ED threshold for good CCA. */
#define BB_154_DRV_CCA_ED_THRESHOLD       20

/*! \brief  Bitcount checking.
 * Delay to sequence number to capture frame control field.
 */
#define BB_154_DRV_BCC_SEQ               (PAL_BB_154_FRAME_CONTROL_LEN * 8)

/*! \brief  Baseband driver resource interrupts. */
#define BB_154_DRV_TIMER_IRQ_PRIORITY     0
#define BB_154_DRV_TIMER                  NRF_TIMER0
#define BB_154_DRV_TIMER_IRQ              TIMER0_IRQn
#define BB_154_DRV_RADIO_IRQ_PRIORITY     0
#define BB_154_DRV_RADIO_IRQ              RADIO_IRQn

/*! \brief  Ack. header values. */
#define ACK_HEADER_WITH_PENDING           0x12        /* ACK frame control field with pending bit */
#define ACK_HEADER_WITHOUT_PENDING        0x02        /* ACK frame control field without pending bit */
#define ACK_LENGTH                        5           /* Length of ACK frame including FCS */

#define MHMU_MASK                         0xff0007ff  /* Mask of known bytes in ACK packet */
#define MHMU_PATTERN                      0x00000205  /* Values of known bytes in ACK packet */

/*
 *  ! \brief  PPI Channel 14 -  "Chan 14: TIMER[0].COMPARE[0] -> TXEN/RXEN"
 *
 *   This channel is used to trigger radio task RXEN (or TXEN) on timer event COMPARE[0].
 *   It is only enabled when needed.
 *
 *   The PPI task parameter is meant to be configured just before the channel is enabled.
 *   This task will be either TXEN or RXEN depending on radio operation. Normally it is
 *   RXEN but TXEN is used when CCA is disabled.
 *
 *   The event is static and never changes. It is configured below.
 *
 *   This is used to delay transmit or receive.
 */
#define BB_154_DRV_PPI_TXRX_DELAY_CHAN    14

/*
 *  ! \brief  PPI Channel 15 -  "Chan 15: FRAMESTART -> CAPTURE[2]"
 *
 *   This channel is used to capture the timer value on every radio FRAMESTART event.
 *   It is configured to trigger timer task CAPTURE[2] which copies the timer to CC[2].
 *   This can then be used to calculate the timestamp of the received packet.
 *
 *   Once this channel is set up and enabled, it requires no further action.
 *   It will stay active until the BB driver is disabled.
 */
#define BB_154_DRV_PPI_TIMESTAMP_CHAN     15

/*! \brief  Convert 16-bit unsigned in frame field to uint16_t. */
#define PAL_BB_154_FRM_TO_UINT16(n, p)    (n = (uint16_t)(p)[0] + \
                                           ((uint16_t)(p)[1] << 8))

/*! \brief  Convert 64-bit unsigned in frame field to uint64_t. */
#define PAL_BB_154_FRM_TO_UINT64(n, p)    (n = (uint64_t)(p)[0] + \
                                           ((uint64_t)(p)[1] << 8) + \
                                           ((uint64_t)(p)[2] << 16) + \
                                           ((uint64_t)(p)[3] << 24) + \
                                           ((uint64_t)(p)[4] << 32) + \
                                           ((uint64_t)(p)[5] << 40) + \
                                           ((uint64_t)(p)[6] << 48) + \
                                           ((uint64_t)(p)[7] << 56))

/**************************************************************************************************
  PHY Defines
**************************************************************************************************/

/*! \brief    Factor to multiply by to get microsecond duration. */
#define BB_154_DRV_BACKOFF_FACTOR_US      (PAL_BB_154_aUnitBackoffPeriod * \
                                           PAL_BB_154_SYMBOL_DURATION)

/*! \brief    Energy detect threshold in dBm. */
#ifdef PAL_BB_154_ED_THRESHOLD
#undef PAL_BB_154_ED_THRESHOLD
#endif
#define PAL_BB_154_ED_THRESHOLD           20

/*! \brief      Lowest RSSI below which energy is considered 0. */
#define BB_154_DRV_MIN_RSSI               -100 /* TODO: TBC */

/*! \brief      ED scaling factor for divide by 65536 (>>16).
 *  \note       Consider truncation to 8 bits and 0xff full scale
 */
#define BB_154_DRV_ED_SCALE_65536         (uint32_t)((65536 * 255) / 100)

/*! \brief Get next queue element. */
#define BB_154_QUEUE_NEXT(p)              (((bb154QueueElem_t *)(p))->pNext)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief     Radio driver latencies. */
typedef struct bb154DrvRadioTiming_tag
{
  uint32_t txOnLatency;             /*!< Latency between radio on signal and transmit. */
  uint32_t rxOnLatency;             /*!< Latency between radio on signal and receive. */
  uint32_t txDataPathLatency;       /*!< Transmit data path latency. */
  uint32_t rxDataPathLatency;       /*!< Receive data path latency. */
} bb154DrvRadioTiming_t;

/*! \brief    Queue element */
typedef struct bb154QueueElem_tag
{
  struct bb154QueueElem_tag *pNext; /*!< Pointer to next element. */
} bb154QueueElem_t;

/*! \brief Queue structure */
typedef struct bb154Queue_tag
{
  void      *pHead;                 /*!< Head of queue. */
  void      *pTail;                 /*!< Tail of queue. */
} bb154Queue_t;

/*! \brief      Address structure. */
typedef struct bb154Addr_tag
{
  uint8_t     addrMode;                           /*!< Address mode (short or extended). */
  uint8_t     panId[PAL_BB_154_SHORT_ADDR_LEN];   /*!< PAN ID. */
  uint8_t     addr[PAL_BB_154_EXTENDED_ADDR_LEN]; /*!< Short or extended address
                                                   *  (short address occupies first two bytes).
                                                   */
} bb154Addr_t;

/*! \brief    BBP states. */
enum bb154DrvState_tag
{
  BB_154_DRV_STATE_UNINITIALIZED = 0,  /* Unin */
  BB_154_DRV_STATE_INITIALIZED   = 1,  /* Init */
  BB_154_DRV_STATE_ENABLED       = 2,  /* Enbl */
  BB_154_DRV_STATE_CHAN_SET      = 3,  /* ChSt */
  BB_154_DRV_STATE_OFF           = 4,  /* ROff */
  BB_154_DRV_STATE_RX_HDR        = 5,  /* RxHd */
  BB_154_DRV_STATE_RX_FRM        = 6,  /* RxFm */
  BB_154_DRV_STATE_RX_ACK        = 7,  /* RxAk */
  BB_154_DRV_STATE_ED            = 8,  /* EnDt */
  BB_154_DRV_STATE_TX            = 9,  /* TxFm */
  BB_154_DRV_STATE_TX_CCA        = 10, /* TxCC */
  BB_154_DRV_STATE_TX_ACK        = 11, /* TxAk */
  BB_154_DRV_NUM_STATES
};

/*! \brief    BBP states. */
enum bb154DrvRxHdrState_tag
{
  BB_154_DRV_HDR_STATE_SEQ = 0,     /*!< Bit counter at sequence number. */
  BB_154_DRV_HDR_STATE_PAYLOAD = 1, /*!< Bit counter at payload. */
  BB_154_DRV_NUM_HDR_STATES
};

/*! \brief    BBP control block. */
typedef struct bb154DrvCb_tag
{
  uint8_t               state;              /*!< BBP state. */
  uint8_t               rxHdrState;         /*!< Rx header state. */

  PalBb154Chan_t        chan;               /*!< Channel parameters. */
  PalBb154OpParam_t     op;                 /*!< Operation parameters. */
  PalBb154Alloc_t       allocCback;         /*!< Allocation callback. */
  PalBb154Free_t        freeCback;          /*!< Free callback. */

  /*! Tx parameters. */
  struct
  {
    uint8_t             nb;                 /*!< NB parameter in CSMA/CA. */
    uint8_t             be;                 /*!< BE parameter in CSMA/CA. */
    uint8_t             retry;              /*!< Retry counter. */
    PalBb154TxBufDesc_t *pDesc;             /*!< Saved buffer descriptor. */
  } tx;

  /*! Rx parameters. */
  struct
  {
    bb154Queue_t        bufQ;               /*!< Buffer queue. */
    uint8_t             *pBuf;              /*!< Current receive buffer. */
    uint8_t             bufCount;           /*!< Number of buffers queued. */
  } rx;

  /*! Rx address match parameters. */
  struct
  {
    uint16_t            panId;              /*!< PAN ID. */
    uint16_t            shortAddr;          /*!< Short address. */
    uint64_t            extAddr;            /*!< Extended address. */
  } rxAddr;

  /*! ACK parameters. */
  uint8_t               ack[6];             /*! ACK packet buffer. */
} bb154DrvCb_t;

/*! \brief    Event definitions. */
#define BB_154_DRV_RADIO_EVT_OFFSET  0
#define BB_154_DRV_MAX_RADIO_EVENTS  7
#define BB_154_DRV_TIMER_EVT_OFFSET  BB_154_DRV_MAX_RADIO_EVENTS
#define BB_154_DRV_MAX_TIMER_EVENTS  1
#define BB_154_DRV_MAX_EVENTS        BB_154_DRV_MAX_RADIO_EVENTS + BB_154_DRV_MAX_TIMER_EVENTS
#define BB_154_DRV_NUM_EVT_HANDLERS  17

/*! \brief    Event hander status. */
enum bb154DrvEvHStat_tag
{
  BB_154_DRV_EVH_STAT_OK = 0,
  BB_154_DRV_EVH_STAT_INSIGNIFICANT = 1,
  BB_154_DRV_EVH_STAT_WARN = 0x10,
  BB_154_DRV_EVH_STAT_ERR = 0x20
};

/*! \brief    Event hander type. */
typedef uint8_t (*bb154DrvEvtHandler_t)(void);

/*! \brief    Receive event hander index type. */
typedef uint8_t bb154DrvREHFnIdx_t;

/*! \brief      Receive buffer descriptor.
 * Note - must be packed so buffer immediately follows length.
 */
typedef struct Bb154DrvRxBufDesc
{
  uint8_t len; /*!< Length of frame precedes actual frame that follows. */
} Bb154DrvRxBufDesc_t;

/*! \brief      Get rx frame pointer from rx buffer pointer. */
#define BB_154_DRV_RX_FRAME_PTR_FROM_BUF(x)     ((uint8_t *)(((Bb154DrvRxBufDesc_t *)(x))+1))

/*! \brief      Get rx buffer pointer from rx frame pointer. */
#define BB_154_DRV_RX_BUF_PTR_FROM_FRAME(x)     ((uint8_t *)(((Bb154DrvRxBufDesc_t *)(x))-1))

/*! \brief        Forward declaration of event handlers. */
static uint8_t bb154DrvEvHGenlIllegal(void);
static uint8_t bb154DrvEvHGenlDontCare(void);
static uint8_t bb154DrvEvHGenlReady(void);
static uint8_t bb154DrvEvHGenlFramestart(void);
static uint8_t bb154DrvEvHRxHdEnd(void);
static uint8_t bb154DrvEvHRxHdBcMatch(void);
static uint8_t bb154DrvEvHRxAkBcMatch(void);
static uint8_t bb154DrvEvHRxFmEnd(void);
static uint8_t bb154DrvEvHRxAkEnd(void);
static uint8_t bb154DrvEvHTxFmEnd(void);
static uint8_t bb154DrvEvHTxAkEnd(void);
static uint8_t bb154DrvEvHEnDtEdEnd(void);
static uint8_t bb154DrvEvHTxCCIdle(void);
static uint8_t bb154DrvEvHTxCCBusy(void);
static uint8_t bb154DrvEvHGenlRxTOTmrExp(void);
static uint8_t bb154DrvEvHRxAkTOTmrExp(void);
static uint8_t bb154DrvEvHROffGenl(void);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief        Event handler table. */
static const bb154DrvEvtHandler_t bb154DrvEvtHandler[BB_154_DRV_NUM_EVT_HANDLERS] =
{
  bb154DrvEvHGenlIllegal,      /*  0: __ */
  bb154DrvEvHGenlDontCare,     /*  1: XX */
  bb154DrvEvHGenlReady,        /*  2 */
  bb154DrvEvHGenlFramestart,   /*  3 */
  bb154DrvEvHRxHdBcMatch,      /*  4 */
  bb154DrvEvHRxAkBcMatch,      /*  5 */
  bb154DrvEvHRxHdEnd,          /*  6 */
  bb154DrvEvHRxFmEnd,          /*  7 */
  bb154DrvEvHRxAkEnd,          /*  8 */
  bb154DrvEvHTxFmEnd,          /*  9 */
  bb154DrvEvHTxAkEnd,          /* 10 */
  bb154DrvEvHEnDtEdEnd,        /* 11 */
  bb154DrvEvHTxCCIdle,         /* 12 */
  bb154DrvEvHTxCCBusy,         /* 13 */
  bb154DrvEvHGenlRxTOTmrExp,   /* 14 */
  bb154DrvEvHRxAkTOTmrExp,     /* 15 */
  bb154DrvEvHROffGenl          /* 16 */
};

/*! \brief        Stock event handler indices. */
#define FN_IDX_ILLEGAL    0
#define FN_IDX_DONT_CARE  1

#define __    FN_IDX_ILLEGAL
#define XX    FN_IDX_DONT_CARE

/*! \brief        State transition table. */
static const bb154DrvREHFnIdx_t bb154DrvStateTable[BB_154_DRV_MAX_EVENTS][BB_154_DRV_NUM_STATES] =
{
  /*
    00  01  02  03  04  05  06  07  08  09  10  11
  ------------------------------------------------
     U   I   E   C   R   R   R   R   E   T   T   T
     n   n   n   h   O   x   x   x   n   x   x   x
     i   i   b   S   f   H   F   A   D   F   C   A
     n   t   l   t   f   d   m   k   t   m   C   k
  */
  { __, __, __, __, 16,  2,  2,  2,  2,  2,  2,  2 }, /* NRF_RADIO_EVENT_READY */
  { __, __, __, __, 16,  3, __,  3, __, XX, XX, XX }, /* NRF_RADIO_EVENT_FRAMESTART */
  { __, __, __, __, 16,  4, __,  5, __, XX, XX, XX }, /* NRF_RADIO_EVENT_BCMATCH */
  { __, __, __, __, 16,  6,  7,  8, __,  9, __, 10 }, /* NRF_RADIO_EVENT_END */
  { __, __, __, __, __, __, __, __, 11, __, __, __ }, /* NRF_RADIO_EVENT_EDEND */
  { __, __, __, __, __, __, __, __, __, __, 12, __ }, /* NRF_RADIO_EVENT_CCAIDLE */
  { __, __, __, __, __, __, __, __, __, __, 13, __ }, /* NRF_RADIO_EVENT_CCABUSY */
  { __, __, __, __, __, 14, 14, 15, __, __, __, __ }  /* NRF_TIMER_EVENT_COMPARE1 */
};

#undef __
#undef XX

/*! \brief    Driver control block. */
static bb154DrvCb_t bb154DrvCb;

/*! \brief    Default latency timing. */
static bb154DrvRadioTiming_t bb154DrvRadioTiming =
{
  .txOnLatency = 74,
  .rxOnLatency = 63,
  .txDataPathLatency = 8,
  .rxDataPathLatency = 8
};

/*! \brief    BB driver statistics. */
static PalBb154DrvStats_t bb154DrvStats;

/*! \brief    Receive last RSSI. */
static uint8_t bb154DrvLastRssi;

/*! \brief      Random number generator control block. */
static struct
{
  uint32_t rngW;
  uint32_t rngX;
  uint32_t rngY;
  uint32_t rngZ;
} bb154PrandCb;

/*! \brief      PIB attributes required in baseband. */
static PalBb154DrvPib_t bb154DrvPib;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*! \brief        Function declarations. */
static void bb154DrvNextRx(bool_t rxAck);
static void bb154DrvTxDataNoCca(uint32_t due, bool_t now);
static void bb154DrvOff(void);
static void bb154DrvMacCca(uint32_t due, bool_t now);
static void bb154DrvDelayFailNoRx(void);

/*! \brief        IRQ callback declarations. */
static void Bb154DrvTimerIRQHandler(void);
static void Bb154DrvRadioIRQHandler(void);

/*************************************************************************************************/
/*!
 *  \brief  Enqueue and element to the tail of a queue.
 *
 *  \param  pQueue    Pointer to queue.
 *  \param  pElem     Pointer to element.
 */
/*************************************************************************************************/
static void bb154QueueEnq(bb154Queue_t *pQueue, void *pElem)
{
  BB_ASSERT(pElem != NULL);

  /* Initialize next pointer. */
  BB_154_QUEUE_NEXT(pElem) = NULL;

  /* Enter critical section. */
  PalEnterCs();

  /* If queue empty */
  if (pQueue->pHead == NULL)
  {
    pQueue->pHead = pElem;
    pQueue->pTail = pElem;
  }
  /* Else enqueue element to the tail of queue. */
  else
  {
    BB_154_QUEUE_NEXT(pQueue->pTail) = pElem;
    pQueue->pTail = pElem;
  }

  /* Exit critical section. */
  PalExitCs();
}

/*************************************************************************************************/
/*!
 *  \brief  Dequeue an element from the head of a queue.
 *
 *  \param  pQueue    Pointer to queue.
 *
 *  \return Pointer to element that has been dequeued or NULL if queue is empty.
 */
/*************************************************************************************************/
static void *bb154QueueDeq(bb154Queue_t *pQueue)
{
  bb154QueueElem_t  *pElem;

  /* Enter critical section. */
  PalEnterCs();

  pElem = pQueue->pHead;

  /* if queue is not empty */
  if (pElem != NULL)
  {
    /* set head to next element in queue */
    pQueue->pHead = BB_154_QUEUE_NEXT(pElem);

    /* check for empty queue */
    if (pQueue->pHead == NULL)
    {
      pQueue->pTail = NULL;
    }
  }

  /* Exit critical section. */
  PalExitCs();

  return pElem;
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize random number generator.
 */
/*************************************************************************************************/
static void bb154PrandInit(void)
{
  /* Seed PRNG. */
  bb154PrandCb.rngW = 88675123;
  bb154PrandCb.rngX = 123456789;
  bb154PrandCb.rngY = 362436069;
  bb154PrandCb.rngZ = 521288629;
}

/*************************************************************************************************/
/*!
 *  \brief      Generate random data.
 *
 *  \param      pBuf    Buffer.
 *  \param      len     Length of buffer in bytes.
 *
 *  \note       Uses a xorshift random number generator. See George Marsaglia (2003),
 *              "Xorshift RNGs", Journal of Statistical Software.
 */
/*************************************************************************************************/
static void bb154PrandGen(uint8_t *pBuf, uint16_t len)
{
  while (len > 0)
  {
    uint32_t t;
    uint32_t randNum;

    /* Advance pseudo random number. */
    t = bb154PrandCb.rngX ^ (bb154PrandCb.rngX << 11);
    bb154PrandCb.rngX = bb154PrandCb.rngY;
    bb154PrandCb.rngY = bb154PrandCb.rngZ;
    bb154PrandCb.rngZ = bb154PrandCb.rngW;
    bb154PrandCb.rngW = bb154PrandCb.rngW ^ (bb154PrandCb.rngW >> 19) ^ (t ^ (t >> 8));

    randNum = bb154PrandCb.rngW;
    if (len >= 4)
    {
      *pBuf++ = (uint8_t)randNum;
      *pBuf++ = (uint8_t)(randNum >> 8);
      *pBuf++ = (uint8_t)(randNum >> 16);
      *pBuf++ = (uint8_t)(randNum >> 24);
      len -= 4;
    }
    else
    {
      while (len > 0)
      {
        *pBuf++ = (uint8_t)randNum;
        len -= 1;
        randNum >>= 8;
      }
    }
  }
}

/**************************************************************************************************
  BB Driver Functions
**************************************************************************************************/

static void bb154DisableTimerCompare1(void)
{
  /* Disable COMPARE1 interrupt */
  nrf_timer_int_disable(BB_154_DRV_TIMER, NRF_TIMER_INT_COMPARE1_MASK);
  NVIC_ClearPendingIRQ(BB_154_DRV_TIMER_IRQ); /* necessary, if interrupt already "fired" this is the only way to clear it */
}

/*************************************************************************************************/
/*!
 *  \brief      Set radio channel
 *
 *  \param      channel   Channel to set
 *
 *  Sets the radio channel in the device
 */
/*************************************************************************************************/
static void bb154DrvChannelSet(uint8_t channel)
{
  nrf_radio_frequency_set(5 + (5 * (channel - 11)));
}

/*************************************************************************************************/
/*!
 *  \brief      Set transmit power
 *
 *  \param      dbm   Transmit power in dBm
 *
 *  Sets the radio transmit power in the device
 */
/*************************************************************************************************/
static void bb154DrvTxPowerSet(int8_t dbm)
{
  const int8_t allowedValues[] = {-40, -20, -16, -12, -8, -4, 0, 2, 3, 4, 5, 6, 7, 8, 9};
  const int8_t highestValue    = allowedValues[(sizeof(allowedValues) / sizeof(allowedValues[0])) - 1];

  if (dbm > highestValue)
  {
    dbm = highestValue;
  }
  else
  {
    for (uint32_t i = 0; i < sizeof(allowedValues) / sizeof(allowedValues[0]); i++)
    {
      if (dbm <= allowedValues[i])
      {
        dbm = allowedValues[i];
        break;
      }
    }
  }

  nrf_radio_txpower_set(dbm);
}

/*************************************************************************************************/
/*!
 *  \brief      Start a radio task, possibly delayed.
 *
 *  \param      now         Start now
 *  \param      due         Time task is due
 *  \param      radioTask   Radio task
 *
 *  Triggers the task immediately or waits for the due time
 */
/*************************************************************************************************/
static bool_t bb154DrvStartRadioTask(uint32_t due, bool_t now, nrf_radio_task_t radioTask)
{
  if (now)
  {
    nrf_radio_task_trigger(radioTask);
  }
  else
  {
    due -= bb154DrvRadioTiming.txOnLatency;
    if (!BB_154_DRV_TIME_IN_FUTURE(due - BB_154_DRV_SETUP_TIME, PalBbGetCurrentTime()))
    {
      /* Missed scheduling time */
      return FALSE;
    }

    /* Configure and enable PPI for delay */
    nrf_ppi_channel_endpoint_setup(BB_154_DRV_PPI_TXRX_DELAY_CHAN,
                                   (uint32_t)nrf_timer_event_address_get(BB_154_DRV_TIMER, NRF_TIMER_EVENT_COMPARE0),
                                   (uint32_t)nrf_radio_task_address_get(radioTask));
    nrf_ppi_channel_enable(BB_154_DRV_PPI_TXRX_DELAY_CHAN);

    /*
     * Set timer capture compare match register to due value.
     * Note timer is free running and already started
     */
    nrf_timer_cc_write(BB_154_DRV_TIMER, 0, due); /* Use compare register 0 */
    /* Clear the compare event. Next event will trigger the radio task when done */
    nrf_timer_event_clear(BB_154_DRV_TIMER, NRF_TIMER_EVENT_COMPARE0);
  }
  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      Clear all radio events.
 *
 *  \param      None.
 *
 *  Calling this routine will clear all radio events in the device.
 */
/*************************************************************************************************/
static void bb154DrvClearAllEvents(void)
{
  nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
  nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
  nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
  nrf_radio_event_clear(NRF_RADIO_EVENT_END);
  nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
  nrf_radio_event_clear(NRF_RADIO_EVENT_EDEND);
  nrf_radio_event_clear(NRF_RADIO_EVENT_CCAIDLE);
  nrf_radio_event_clear(NRF_RADIO_EVENT_CCABUSY);

  /*
   * Workaround
   * nRF52840 can get stuck in TX_DISABLE state if not first briefly
   * put into receive mode. Note needs to be done before Tx, doing it once
   * on init does not seem sufficient.
   */
  nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
  nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
  nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
  while (!nrf_radio_event_check(NRF_RADIO_EVENT_DISABLED)) ;
  while (nrf_radio_state_get() != NRF_RADIO_STATE_DISABLED) ;
}

/*************************************************************************************************/
/*!
 *  \brief      Force disable of radio
 *  One-time initialization of baseband resources. This routine can be used to setup software
 *  driver resources, load RF trim parameters and execute RF calibrations.
 *
 *  This routine should block until the BB hardware is completely initialized.
 */
/*************************************************************************************************/
static void bb154DrvOff(void)
{
  /* Disable COMPARE1 interrupt */
  bb154DisableTimerCompare1();

  bb154DrvClearAllEvents();

 /* Disable all peripheral shortcuts which might be enabled */
  nrf_radio_shorts_disable(NRF_RADIO_SHORT_END_DISABLE_MASK |
                           NRF_RADIO_SHORT_DISABLED_TXEN_MASK |
                           NRF_RADIO_SHORT_READY_START_MASK |
                           NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK |
                           NRF_RADIO_SHORT_RXREADY_CCASTART_MASK |
                           NRF_RADIO_SHORT_READY_EDSTART_MASK);

  /* Disable timer to radio channel used for initial delay */
  nrf_ppi_channel_disable(BB_154_DRV_PPI_TXRX_DELAY_CHAN);

  /* Clear IFS setting */
  nrf_radio_ifs_set(0);

  if (bb154DrvCb.state == BB_154_DRV_STATE_ED)
  {
    /* Stop any ED in process */
    nrf_radio_task_trigger(NRF_RADIO_TASK_EDSTOP);
  }

  bb154DrvCb.state = BB_154_DRV_STATE_OFF;

#if 0
  /* Not needed due to bb154ClearAllEvents() call above */
  /* Disable radio if it's inDrv a state where it can be */
  switch (nrf_radio_state_get())
  {
    case NRF_RADIO_STATE_RX:
    case NRF_RADIO_STATE_RX_RU:
    case NRF_RADIO_STATE_RX_IDLE:
    case NRF_RADIO_STATE_TX_RU:
    case NRF_RADIO_STATE_TX_IDLE:
    case NRF_RADIO_STATE_TX:
      nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED); /* Clear disabled event that was set by short. */
      nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
      break;

    default:
      /*
       * case NRF_RADIO_STATE_RX_DISABLE
       * case NRF_RADIO_STATE_DISABLED
       * case NRF_RADIO_STATE_TX_DISABLE
       */
      break;
  }
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Delay timer failure handling in non-Rx state.
 *
 *  \param  rxAck       TRUE if an ACK is being received.
 */
/*************************************************************************************************/
static void bb154DrvDelayFailNoRx(void)
{
  /* Channel remains set up, but TXR is off. */
  bb154DrvOff();

  /* Signal error. */
  if (bb154DrvCb.op.errCback != NULL)
  {
    bb154DrvCb.op.errCback(BB_STATUS_FAILED);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Set up MAC for Rx.
 *
 *  \param      due         Due time for receive (if \a now is FALSE).
 *  \param      now         TRUE if packet should be received with minimal delay.
 *  \param      timeout     Receive timeout.
 */
/*************************************************************************************************/
static void bb154DrvRxData(uint32_t due, bool_t now, uint32_t timeout)
{
  BB_154_DRV_INC_STAT(bb154DrvStats.rxReq);

  bb154DrvClearAllEvents();

  /* Set bit counter to generate event after frame control field */
  nrf_radio_bcc_set(BB_154_DRV_BCC_SEQ);
  bb154DrvCb.rxHdrState = BB_154_DRV_HDR_STATE_SEQ;

  /*
   * Enable shortcuts:
   * EVENTS_READY -> TASKS_START
   * EVENTS_FRAMESTART -> TASKS_BCSTART
   * EVENTS_END -> TASKS_DISABLE
   */
  nrf_radio_shorts_enable(NRF_RADIO_SHORT_READY_START_MASK |
                          NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK |
                          NRF_RADIO_SHORT_END_DISABLE_MASK );

  /* Enable timeout timer if necessary */
  if (timeout > 0)
  {
    if (now)
    {
      /* We still need to set a time for setting timeout */
      due = PalBbGetCurrentTime();
    }

    /* Enable timeout timer interrupt */
    nrf_timer_int_enable(BB_154_DRV_TIMER, NRF_TIMER_INT_COMPARE1_MASK);

    /* Clear the compare event. */
    nrf_timer_event_clear(BB_154_DRV_TIMER, NRF_TIMER_EVENT_COMPARE1);
    /*
     * Set timer capture compare match register to due value.
     * Note timer is free running and already started
     */
    nrf_timer_cc_write(BB_154_DRV_TIMER, 1, due + timeout); /* Use compare register 1 */
  }

  /* Set the packet pointer */
  if (bb154DrvCb.rx.pBuf == NULL)
  {
    bb154DrvCb.rx.pBuf = bb154QueueDeq(&bb154DrvCb.rx.bufQ);
  }

  /* Set packet pointer to supplied buffer. Length obtained from first field */
  nrf_radio_packetptr_set(bb154DrvCb.rx.pBuf);

  /* Start the task, which may be delayed */
  if (!bb154DrvStartRadioTask(due, now, NRF_RADIO_TASK_RXEN))
  {
    BB_154_DRV_INC_STAT(bb154DrvStats.rxSchMiss);

    /* Perform next receive. */
    bb154DrvNextRx(FALSE);

    /* Signal error. */
    if (bb154DrvCb.op.errCback != NULL)
    {
      bb154DrvCb.op.errCback(BB_STATUS_FAILED);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Perform next receive.
 *
 *  \param  rxAck       TRUE if an ACK is being received.
 */
/*************************************************************************************************/
static void bb154DrvNextRx(bool_t rxAck)
{
  if ((bb154DrvCb.rx.pBuf != NULL) || (bb154DrvCb.rx.bufQ.pHead != NULL))
  {
    uint32_t timeout;
    uint8_t nextState;

    if (rxAck)
    {
      timeout =  PAL_BB_154_SYMB_TO_US(PAL_BB_154_RX_ACK_TIMEOUT_SYMB);
      nextState = BB_154_DRV_STATE_RX_ACK;
    }
    else
    {
      timeout = 0;
      nextState = BB_154_DRV_STATE_RX_HDR;
    }

    /* Restart receive immediately. */
    bb154DrvRxData(0, TRUE, timeout);
    bb154DrvCb.state = nextState;
  }
  else
  {
    /* Channel remains set up, but TXR is off. */
    bb154DrvOff();  /* TODO alert MAC about out-of-buffers condition */
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Transmit data with no CCA.
 *
 *  \param      due         Due time for transmit (if \a now is FALSE).
 *  \param      now         TRUE if packet should be transmitted with minimal delay.
 */
/*************************************************************************************************/
static void bb154DrvTxDataNoCca(uint32_t due, bool_t now)
{
  BB_154_DRV_INC_STAT(bb154DrvStats.txReq);

  /* Interrupts are already enabled; just clear events out */
  bb154DrvClearAllEvents();

  /* TODO: HACK - use temp. EasyDMA buffer until figure out how to do this properly */
  if (bb154DrvCb.state == BB_154_DRV_STATE_TX)
  {
    /* Transmit packet */
    /* Adjust length pointer to include CRC */
    bb154DrvCb.tx.pDesc->len += 2;

    /* Set packet pointer to the length field */
    nrf_radio_packetptr_set(&bb154DrvCb.tx.pDesc->len);
  }
  else
  {
    /* Set packet pointer to the length field of ack packet */
    nrf_radio_packetptr_set(bb154DrvCb.ack);
  }

  /* Enable shortcuts:
   * EVENTS_READY -> TASKS_START
   * EVENTS_END -> TASKS_DISABLE
   */
  nrf_radio_shorts_enable(NRF_RADIO_SHORT_READY_START_MASK |
                          NRF_RADIO_SHORT_END_DISABLE_MASK);

  /* Start the task, which may be delayed */
  if (!bb154DrvStartRadioTask(due, now, NRF_RADIO_TASK_TXEN))
  {
    BB_154_DRV_INC_STAT(bb154DrvStats.txSchMiss);
    bb154DrvDelayFailNoRx();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Set up MAC for CCA
 *
 *  \param      due         Due time for energy detect (if \a now is FALSE).
 *  \param      now         TRUE if energy detect should occur minimal delay.
 */
/*************************************************************************************************/
static void bb154DrvMacCca(uint32_t due, bool_t now)
{
  static const uint8_t pow2[8] = {0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF};
  uint32_t backoff = 0;

  if (bb154DrvCb.tx.be > 0)
  {
    uint8_t backoffSymb;

    bb154PrandGen((uint8_t *)&backoffSymb, sizeof(backoffSymb));
    backoff = (uint32_t)(backoffSymb & pow2[bb154DrvCb.tx.be - 1]) * BB_154_DRV_BACKOFF_FACTOR_US;
  }

  bb154DrvClearAllEvents();

  /* Enable EVENT_READY to TASK_CCASTART */
  nrf_radio_shorts_enable(NRF_RADIO_SHORT_RXREADY_CCASTART_MASK);

  if (now)
  {
    /* We still need to set a time due to potential backoff */
    due = PalBbGetCurrentTime();
  }

  /* TODO: Compensate for ramp-up time */

  /* due and now parameters a bit more complex */
  /* Start by enabling Rx. When ready, EVENT_READY will shortcut to starting CCA */
  if (!bb154DrvStartRadioTask(due + backoff, now && (backoff == 0), NRF_RADIO_TASK_RXEN))
  {
    BB_154_DRV_INC_STAT(bb154DrvStats.ccaSchMiss);
    bb154DrvDelayFailNoRx();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Transmit data, possibly with CCA.
 *
 *  \param      due         Due time for transmit (if \a now is FALSE).
 *  \param      now         TRUE if packet should be transmitted with minimal delay.
 */
/*************************************************************************************************/
static void bb154DrvTxData(uint32_t due, bool_t now)
{
  if (!bb154DrvPib.disableCCA && ((bb154DrvCb.op.flags & PAL_BB_154_FLAG_DIS_CCA) == 0))
  {
    /* Update event state. */
    bb154DrvCb.state = BB_154_DRV_STATE_TX_CCA;
    bb154DrvCb.tx.nb = 0;            /* reset NB */
    bb154DrvCb.tx.be = bb154DrvPib.minBE;  /* reset BE */

    /* Start CCA */
    bb154DrvMacCca(due, now);
  }
  else
  {
    /* Update event state. */
    bb154DrvCb.state = BB_154_DRV_STATE_TX;

    /* Start transmit. */
    bb154DrvTxDataNoCca(due, now);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Go idle
 *
 *  \return     TRUE if idle, FALSE if still receiving.
 *
 *  Driver is going idle. It will either continue to receive or turn off the radio.
 */
/*************************************************************************************************/
static void bb154DrvGoIdle(bool_t ackReqd)
{
  if (ackReqd || bb154DrvPib.rxEnabled || bb154DrvPib.rxOnWhenIdle || bb154DrvPib.promiscuousMode)
  {
    bb154DrvNextRx(ackReqd);
  }
  else
  {
    /* Channel remains set up, but TXR is off. */
    bb154DrvOff();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Get addresses from frame.
 *
 *  \param      pFrame        Frame buffer pointing to first octet of address field
 *  \param      fctl          Frame control field
 *  \param      pSrcAddr      Pointer to source address (passed by reference) - may be NULL
 *  \param      pDstAddr      Pointer to destination address (passed by reference) - may be NULL
 *
 *  \return     Buffer pointer advanced past address fields.
 *
 *  Obtains the source and destination addresses from the frame. If either parameter is NULL,
 *  simply skips past the fields
 */
/*************************************************************************************************/
static uint8_t *bb154GetAddrsFromFrame(uint8_t *pFrame, uint16_t fctl,
                                       bb154Addr_t *pSrcAddr, bb154Addr_t *pDstAddr)
{
  static const uint8_t amSizeLUT[4] = {0,0,2,8};
  uint8_t dstAddrMode = (uint8_t)PAL_BB_154_FC_DST_ADDR_MODE(fctl);
  uint8_t srcAddrMode = (uint8_t)PAL_BB_154_FC_SRC_ADDR_MODE(fctl);
  uint8_t addrLen;
  uint8_t dstPanId[2] = {0,0};

  if (dstAddrMode == PAL_BB_154_ADDR_MODE_NONE)
  {
    /* Belt 'n' braces clearing of PAN ID compression bit */
    fctl &= ~PAL_BB_154_FC_PAN_ID_COMP_MASK;
  }
  else
  {
    /* Dst PAN ID always present with dest addr. */
    dstPanId[0] = *pFrame++;
    dstPanId[1] = *pFrame++;
  }

  /* Destination address */
  if (pDstAddr != NULL)
  {
    pDstAddr->addrMode = dstAddrMode;
    if (dstAddrMode != PAL_BB_154_ADDR_MODE_NONE)
    {
      pDstAddr->panId[0] = dstPanId[0];
      pDstAddr->panId[1] = dstPanId[1];
      memset(&pDstAddr->addr[2], 0, 6);
      addrLen = amSizeLUT[dstAddrMode];
      memcpy(pDstAddr->addr, pFrame, addrLen);
      pFrame += addrLen;
    }
  }
  else
  {
    pFrame += amSizeLUT[dstAddrMode];
  }

  /* Source address */
  if (pSrcAddr != NULL)
  {
    pSrcAddr->addrMode = srcAddrMode;
    if (srcAddrMode != PAL_BB_154_ADDR_MODE_NONE)
    {
      if (fctl & PAL_BB_154_FC_PAN_ID_COMP_MASK)
      {
        pSrcAddr->panId[0] = dstPanId[0];
        pSrcAddr->panId[1] = dstPanId[1];
      }
      else
      {
        pSrcAddr->panId[0] = *pFrame++;
        pSrcAddr->panId[1] = *pFrame++;
      }
      memset(&pSrcAddr->addr[2], 0, 6);
      addrLen = amSizeLUT[srcAddrMode];
      memcpy(pSrcAddr->addr, pFrame, addrLen);
      pFrame += addrLen;
    }
  }
  else
  {
    pFrame += amSizeLUT[srcAddrMode];
    if ((fctl & PAL_BB_154_FC_PAN_ID_COMP_MASK) == 0)
    {
      pFrame += 2;
    }
  }
  return pFrame;
}

/************************/
/**** EVENT HANDLERS ****/
/************************/

/**** GENERAL EVENT HANDLERS ****/

/*
     S  S
     t  t
     0  1
     ----
    {1, 1},| Ev0
    {1, 1} | Ev1
*/

/*************************************************************************************************/
/*!
 *  \brief      Illegal state transition handler
 *
 *  \param      None.
 *
 *  Called when event is illegal in current state.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHGenlIllegal(void)
{
  BB_ASSERT(0);
  return BB_154_DRV_EVH_STAT_ERR;
}

/*************************************************************************************************/
/*!
 *  \brief      Don't care state transition handler
 *
 *  \param      None.
 *
 *  Called when event is silently discarded in current state.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHGenlDontCare(void)
{
  return BB_154_DRV_EVH_STAT_WARN;
}

/**** EVENT-SPECIFIC GENERAL EVENT HANDLERS ****/

/*
     S  S
     t  t
     0  1
     ----
    {1, 1},| Ev0
    {_, _} | Ev1
*/

/*************************************************************************************************/
/*!
 *  \brief      Ready event handler
 *
 *  \param      None.
 *
 *  Called when READY event occurs.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHGenlReady(void)
{
  /* Disconnect timer compare from tx/rx task */
  nrf_ppi_channel_disable(BB_154_DRV_PPI_TXRX_DELAY_CHAN);
  return BB_154_DRV_EVH_STAT_INSIGNIFICANT;
}

/*************************************************************************************************/
/*!
 *  \brief      Frame start event handler
 *
 *  \param      None.
 *
 *  Called when FRAMESTART event occurs.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHGenlFramestart(void)
{
  /* Trigger RSSI measurement */
  nrf_radio_task_trigger(NRF_RADIO_TASK_RSSISTART);
  return BB_154_DRV_EVH_STAT_INSIGNIFICANT;
}

/*************************************************************************************************/
/*!
 *  \brief      Receive timeout expiry event handler
 *
 *  \param      None.
 *
 *  Called when Rx timeout expiry event occurs when not expecting an ack.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHGenlRxTOTmrExp(void)
{
  nrf_radio_state_t radioState;

  /* There should only be timeout if receiver is actually on */
  radioState = nrf_radio_state_get();
  BB_ASSERT((radioState == NRF_RADIO_STATE_RX) || (radioState == NRF_RADIO_STATE_RXRU));

  /* Disable COMPARE1 interrupt */
  bb154DisableTimerCompare1();

  /* Go idle */
  bb154DrvGoIdle(FALSE);

  /* Signal error. */
  if (bb154DrvCb.op.errCback != NULL)
  {
    bb154DrvCb.op.errCback(BB_STATUS_RX_TIMEOUT);
  }
  return BB_154_DRV_EVH_STAT_OK;
}

/**** STATE-SPECIFIC GENERAL EVENT HANDLERS ****/

/*
     S  S
     t  t
     0  1
     ----
    {1, _},| Ev0
    {1, _} | Ev1
*/

/*************************************************************************************************/
/*!
 *  \brief      ROff x General event handler
 *
 *  \param      None.
 *
 *  Called when READY event occurs.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHROffGenl(void)
{
  /* It seems possible to get spurious events in the off state, e.g. radio ramp up can finish
   * late once it is supposed to be off or end event can occur. In this case, just force clear
   * all events and redisable the radio
   */
  bb154DrvClearAllEvents();
  return BB_154_DRV_EVH_STAT_OK;
}


/**** TRANSITION-SPECIFIC EVENT HANDLERS ****/

/*
     S  S
     t  t
     0  1
     ----
    {1, _},| Ev0
    {_, _} | Ev1
*/

/*************************************************************************************************/
/*!
 *  \brief      RxHd x BCMATCH event handler
 *
 *  \param      None.
 *
 *  Called when BCMATCH (bitcount match) event occurs.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHRxHdBcMatch(void)
{
  bool_t restart = FALSE;
  uint8_t *pRxFrame = &bb154DrvCb.rx.pBuf[BB_154_DRV_RX_FRAME_CTRL_OFFSET];

  switch (nrf_radio_state_get())
  {
    case NRF_RADIO_STATE_RX:
    case NRF_RADIO_STATE_RXIDLE:
    case NRF_RADIO_STATE_RXDISABLE: /* A lot of states due to shortcuts */
    case NRF_RADIO_STATE_DISABLED:
    case NRF_RADIO_STATE_TXRU:
      if (bb154DrvPib.promiscuousMode)
      {
        /* Accept everything in promiscuous mode */
        bb154DrvCb.state = BB_154_DRV_STATE_RX_FRM;
      }
      else
      {
        uint16_t fctl;

        /* Get Frame Control field. */
        PAL_BB_154_FRM_TO_UINT16(fctl, pRxFrame);

        switch (bb154DrvCb.rxHdrState)
        {
          case BB_154_DRV_HDR_STATE_SEQ:
            if (PAL_BB_154_FC_SECURITY_ENABLED(fctl))
            {
              /* TODO: Drop any security enabled frame */
              restart = TRUE;
            }
            else
            {
              /* At sequence number. Check Frame Control field first. */
              switch (PAL_BB_154_FC_FRAME_TYPE(fctl))
              {
                case PAL_BB_154_FRAME_TYPE_BEACON:
                  /* Beacon is broadcast frame - carry on */
                  bb154DrvCb.state = BB_154_DRV_STATE_RX_FRM;
                  break;

                case PAL_BB_154_FRAME_TYPE_DATA:
                case PAL_BB_154_FRAME_TYPE_MAC_COMMAND:
                  {
                    uint8_t *pPayload = PalBb154GetPayloadPtr(pRxFrame, fctl);
                    if (pPayload)
                    {
                      /* Set bit count to payload.
                       * At that point, all the header will be captured
                       */
                      nrf_radio_bcc_set((pPayload - pRxFrame) * 8);
                      bb154DrvCb.rxHdrState = BB_154_DRV_HDR_STATE_PAYLOAD;
                    }
                    else
                    {
                      /* Illegal address mode combination */
                      restart = TRUE;
                    }
                  }
                  break;

               /* Ack. is handled in RxAk state */
               default:
                 restart = TRUE;
              }
            }
            break;

          case BB_154_DRV_HDR_STATE_PAYLOAD:
            /* At payload. Check addresses. */
            {
              bb154Addr_t srcAddr;
              bb154Addr_t dstAddr;

              /* Get addresses */
              (void)bb154GetAddrsFromFrame(&pRxFrame[3], fctl, &srcAddr, &dstAddr);

              /* Process destination address */
              restart = TRUE;
              if ((dstAddr.addrMode == PAL_BB_154_ADDR_MODE_NONE) &&
                  (bb154DrvPib.deviceType == PAL_BB_154_DEV_TYPE_PAN_COORD))
              {
                /* Sent to addr mode 0 and we are the PAN Coordinator - carry on */
                restart = FALSE;
              }
              else
              {
                uint16_t panId;

                PAL_BB_154_FRM_TO_UINT16(panId, dstAddr.panId);

                if ((panId == bb154DrvCb.rxAddr.panId) ||
                    (panId == PAL_BB_154_BROADCAST_PANID))
                {
                  /* PAN ID matches or sent to broadcast */
                  switch (dstAddr.addrMode)
                  {
                    case PAL_BB_154_ADDR_MODE_SHORT:
                      {
                        uint16_t shtAddr;

                        PAL_BB_154_FRM_TO_UINT16(shtAddr, dstAddr.addr);

                        if ((shtAddr == bb154DrvCb.rxAddr.shortAddr) ||
                            (shtAddr == PAL_BB_154_BROADCAST_ADDR))
                        {
                          /* Short address matches or sent to broadcast */
                          restart = FALSE;
                        }
                      }
                      break;

                    case PAL_BB_154_ADDR_MODE_EXTENDED:
                      {
                        uint64_t extAddr;

                        PAL_BB_154_FRM_TO_UINT64(extAddr, dstAddr.addr);

                        if (extAddr == bb154DrvCb.rxAddr.extAddr)
                        {
                          /* Extended address matches */
                          restart = FALSE;
                        }
                      }
                      break;

                  default:
                    /* Can't happen */
                    break;
                  }
                }
              }

              if (!restart)
              {
                uint64_t addr;

                PAL_BB_154_FRM_TO_UINT64(addr, srcAddr.addr);

                /* Call frame pending check callback */
                if (((fctl & PAL_BB_154_FC_FRAME_TYPE_FP_TEST) == PAL_BB_154_FC_FRAME_TYPE_FP_TEST) &&
                      (bb154DrvCb.op.fpCback != NULL) &&
                      bb154DrvCb.op.fpCback(srcAddr.addrMode, addr))
                {
                  /* Set frame pending bit in ack. */
                  bb154DrvCb.ack[1] = ACK_HEADER_WITH_PENDING;
                }
                else
                {
                  /* Clear frame pending bit in ack. */
                  bb154DrvCb.ack[1] = ACK_HEADER_WITHOUT_PENDING;
                }
                bb154DrvCb.state = BB_154_DRV_STATE_RX_FRM;
              }
            }
            break;

          default:
            /* Something weird happened */
            BB_ASSERT(0);
        } /* END switch (bb154DrvCb.rxHdrState) */
      }
      break;

    case NRF_RADIO_STATE_TXIDLE:
      /* Something had stopped the CPU too long. */
      /* TODO: Try to recover radio state */
      /* radio_reset(); */
      break;

    default:
      /* Something weird happened */
      BB_ASSERT(0);
  }

  if (restart)
  {
    /* Skip and restart Rx */
    bb154DrvNextRx(FALSE);
    return BB_154_DRV_EVH_STAT_INSIGNIFICANT;
  }

  return BB_154_DRV_EVH_STAT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      RxHd x END event handler
 *
 *  \param      None.
 *
 *  Called when unexpected END event occurs. In this case, simply restart rx.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHRxHdEnd(void)
{
  /* Restart Rx */
  bb154DrvNextRx(FALSE);
  return BB_154_DRV_EVH_STAT_INSIGNIFICANT;
}

/*************************************************************************************************/
/*!
 *  \brief      RxAk x BCMATCH event handler
 *
 *  \param      None.
 *
 *  Called when BCMATCH (bitcount match) event occurs.
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHRxAkBcMatch(void)
{
  switch (nrf_radio_state_get())
  {
    case NRF_RADIO_STATE_RX:
    case NRF_RADIO_STATE_RXIDLE:
    case NRF_RADIO_STATE_RXDISABLE: /* A lot of states due to shortcuts */
    case NRF_RADIO_STATE_DISABLED:
    case NRF_RADIO_STATE_TXRU:
      {
        uint16_t fctlLsb;

        /* Get LSB of Frame Control field. */
        fctlLsb = (uint16_t)bb154DrvCb.rx.pBuf[BB_154_DRV_RX_FRAME_CTRL_OFFSET];

        if (PAL_BB_154_FC_FRAME_TYPE(fctlLsb) != PAL_BB_154_FRAME_TYPE_ACKNOWLEDGMENT)
        {
          /* Skip and restart Rx */
          bb154DrvNextRx(FALSE);
        }
      }
      break;

    case NRF_RADIO_STATE_TXIDLE:
      /* Something had stopped the CPU too long. */
      /* TODO: Try to recover radio state */
      /* radio_reset(); */
      break;

    default:
      /* Something weird happened */
      BB_ASSERT(0);
  }
  return BB_154_DRV_EVH_STAT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      RxFm x END event handler
 *
 *  \param      None.
 *
 *  Called when END event occurs when expecting a non-ack. Rx frame
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHRxFmEnd(void)
{
  /* See if the CRC failed */
  if (nrf_radio_crc_status_check())
  {
    /* Success! */
    uint8_t  *pRxBuf    = bb154DrvCb.rx.pBuf;
    uint8_t  *pRxFrame  = BB_154_DRV_RX_FRAME_PTR_FROM_BUF(pRxBuf);
    uint16_t len        = bb154DrvCb.rx.pBuf[0] - PAL_BB_154_FCS_LEN; /* Remove length of CRC */
    int8_t rssi;
    uint32_t ts         = nrf_timer_cc_read(BB_154_DRV_TIMER, 2); /* TODO: minus preamble and SFD time */
    uint16_t fctl;

    /* Store last RSSI and compute dBm value */
    bb154DrvLastRssi = nrf_radio_rssi_sample_get();
    rssi = -(int8_t)bb154DrvLastRssi;

    /* Disable COMPARE1 interrupt */
    bb154DisableTimerCompare1();

    /* Clear reference to received buffer. */
    bb154DrvCb.rx.pBuf = NULL;
    bb154DrvCb.rx.bufCount--;

    PAL_BB_154_FRM_TO_UINT16(fctl, pRxFrame);

    /* Determine next operation - check for tx ack. */
    if (!bb154DrvPib.promiscuousMode &&
        (bb154DrvCb.op.flags & PAL_BB_154_FLAG_RX_AUTO_TX_ACK) &&
        ((fctl & PAL_BB_154_FC_FRAME_TYPE_ACK_TEST) == PAL_BB_154_FC_FRAME_TYPE_ACK_TEST))
    {
      /* Rx requested an ack. so send it */
      /* uint32_t due = ts +
       *                PAL_BB_154_SYMB_TO_US((((len + MAC_154_FCS_LEN) * PHY_154_SYMBOLS_PER_OCTET) +
       *                PHY_154_aTurnaroundTime));
       */
      uint32_t due = PalBbGetCurrentTime() + PAL_BB_154_SYMB_TO_US(PAL_BB_154_aTurnaroundTime);

      bb154DrvCb.state = BB_154_DRV_STATE_TX_ACK;

      if (!((PAL_BB_154_FC_FRAME_TYPE(fctl) == PAL_BB_154_FRAME_TYPE_MAC_COMMAND) &&
            (*PalBb154GetPayloadPtr(pRxFrame, fctl) == PAL_BB_154_CMD_FRAME_TYPE_DATA_REQ)))
      {
        /* Clear frame pending for any frame other than a data request MAC Command frame */
        bb154DrvCb.ack[1] = ACK_HEADER_WITHOUT_PENDING;
      }

      /* Modify ack. frame with correct DSN and transmit ack.*/
      bb154DrvCb.ack[3] = pRxFrame[2];
      bb154DrvTxDataNoCca(due, FALSE);

      /* Call receive complete */
      if (bb154DrvCb.op.rxCback != NULL)
      {
        /* Ignore can go idle return */
        (void)bb154DrvCb.op.rxCback(pRxFrame, len, rssi, ts, PAL_BB_154_FLAG_TX_ACK_START);
      }
      return BB_154_DRV_EVH_STAT_OK;
    }

    /* Otherwise, go idle */
    bb154DrvGoIdle(FALSE);

    /* Signal receive complete. */
    if (bb154DrvCb.op.rxCback != NULL)
    {
      /* rxFlags not relevant at this stage */
      (void)bb154DrvCb.op.rxCback(pRxFrame, len, rssi, ts, 0);
    }
    return BB_154_DRV_EVH_STAT_OK;
  }
  else
  {
    /* Have to ignore CRC errors as they occur frequently due to bug in nRF52840 eng. sample QIAA-AA0. */
    /* Silently perform next receive. */
    bb154DrvNextRx(FALSE);
    return BB_154_DRV_EVH_STAT_INSIGNIFICANT;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      RxAk x END event handler
 *
 *  \param      None.
 *
 *  Called when END event occurs when expecting an ack. Rx frame
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHRxAkEnd(void)
{
  /* See if the CRC failed */
  if (nrf_radio_crc_status_check())
  {
    /* Success! */
    uint8_t rxFlags   = PAL_BB_154_RX_FLAG_GO_IDLE;
    uint8_t  *pRxBuf  = bb154DrvCb.rx.pBuf;
    uint8_t  *pRxFrame  = BB_154_DRV_RX_FRAME_PTR_FROM_BUF(pRxBuf);
    uint16_t len      = bb154DrvCb.rx.pBuf[0] - PAL_BB_154_FCS_LEN; /* Remove length of CRC */;
    int8_t rssi       = -(int8_t)nrf_radio_rssi_sample_get();
    uint32_t ts       = nrf_timer_cc_read(BB_154_DRV_TIMER, 2); /* TODO: minus preamble and SFD time */

    /* Disable COMPARE1 interrupt */
    bb154DisableTimerCompare1();

    /* No need to check frame type here as we already checked at BCMATCH */

    /* Clear reference to received buffer. */
    bb154DrvCb.rx.pBuf = NULL;
    bb154DrvCb.rx.bufCount--;

    /* Clear retry counter */
    bb154DrvCb.tx.retry = 0;

    /* Go to off state */
    bb154DrvOff();

    /* Signal receive complete. */
    if (bb154DrvCb.op.rxCback != NULL)
    {
      /* Need to check if we can go idle or not. */
       rxFlags = bb154DrvCb.op.rxCback(pRxFrame, len, rssi, ts, PAL_BB_154_FLAG_RX_ACK_CMPL);
    }

    if (rxFlags & PAL_BB_154_RX_FLAG_GO_IDLE)
    {
      /* Go idle if poll not sent */
      bb154DrvGoIdle(FALSE);
    }
    return BB_154_DRV_EVH_STAT_OK;
  }
  else
  {
    /* Have to ignore CRC errors as they occur frequently due to bug in nRF52840 eng. sample QIAA-AA0. */
    /* Silently perform next receive. */
    bb154DrvNextRx(FALSE);
    return BB_154_DRV_EVH_STAT_INSIGNIFICANT;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Receive timeout expiry event handler
 *
 *  \param      None.
 *
 *  Called when Rx timeout expiry event occurs
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHRxAkTOTmrExp(void)
{
  nrf_radio_state_t radioState;

  radioState = nrf_radio_state_get();

  /* There should only be timeout if receiver is actually on */
  BB_ASSERT(radioState == NRF_RADIO_STATE_RX);

  /* Disable COMPARE1 interrupt */
  bb154DisableTimerCompare1();

  if (bb154DrvCb.tx.retry--)
  {
    bb154DrvTxData(0, TRUE);
  }
  else
  {
    /* No more retries, go idle */
    bb154DrvGoIdle(FALSE);
    /* Signal error. */
    if (bb154DrvCb.op.errCback != NULL)
    {
      bb154DrvCb.op.errCback(BB_STATUS_ACK_TIMEOUT);
    }
  }
  return BB_154_DRV_EVH_STAT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      TxFm x END event handler
 *
 *  \param      None.
 *
 *  Called when END event occurs when expecting end of a tx'ed non-ack. frame
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHTxFmEnd(void)
{
  bool_t ackReqd = FALSE;
  uint8_t *pTxFrame;

  /* Disable shortcuts */
  nrf_radio_shorts_disable(NRF_RADIO_SHORT_READY_START_MASK |
                           NRF_RADIO_SHORT_END_DISABLE_MASK);

  /* Adjust length pointer remove CRC */
  bb154DrvCb.tx.pDesc->len -= 2;

  /* Determine next operation. */
  if (bb154DrvCb.op.flags & PAL_BB_154_FLAG_TX_AUTO_RX_ACK)
  {
    pTxFrame = PAL_BB_154_TX_FRAME_PTR(bb154DrvCb.tx.pDesc);
    /* See if ack. is requested */
#ifdef ADDITIONAL_TESTER_FEATURES
    /* Illegal frame type 4 is used is one test */
    ackReqd = (((pTxFrame[0] & PAL_BB_154_FC_ACK_REQUEST_MASK) != 0) &&
               (((pTxFrame[0] & PAL_BB_154_FC_FRAME_TYPE_MASK) == PAL_BB_154_FRAME_TYPE_DATA) ||
                ((pTxFrame[0] & PAL_BB_154_FC_FRAME_TYPE_MASK) == PAL_BB_154_FRAME_TYPE_MAC_COMMAND) ||
                ((pTxFrame[0] & PAL_BB_154_FC_FRAME_TYPE_MASK) == PAL_BB_154_FRAME_TYPE_ILLEGAL4)));
#else
    ackReqd = (((pTxFrame[0] & PAL_BB_154_FC_ACK_REQUEST_MASK) != 0) &&
               (((pTxFrame[0] & PAL_BB_154_FC_FRAME_TYPE_MASK) == PAL_BB_154_FRAME_TYPE_DATA) ||
                ((pTxFrame[0] & PAL_BB_154_FC_FRAME_TYPE_MASK) == PAL_BB_154_FRAME_TYPE_MAC_COMMAND)));
#endif
  }

  /* Go idle */
  bb154DrvGoIdle(ackReqd);

  /* Signal transmit complete. */
  if (bb154DrvCb.op.txCback != NULL)
  {
    bb154DrvCb.op.txCback(ackReqd ? PAL_BB_154_FLAG_RX_ACK_START : 0);
  }
  return BB_154_DRV_EVH_STAT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      TxFm x END event handler
 *
 *  \param      None.
 *
 *  Called when END event occurs when expecting end of a tx'ed ack. frame
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHTxAkEnd(void)
{
  /* Disable shortcuts */
  nrf_radio_shorts_disable(NRF_RADIO_SHORT_READY_START_MASK |
                           NRF_RADIO_SHORT_END_DISABLE_MASK);

  /* Go idle */
  bb154DrvGoIdle(FALSE);

  /* Signal transmit complete. */
  if (bb154DrvCb.op.txCback != NULL)
  {
    bb154DrvCb.op.txCback(PAL_BB_154_FLAG_TX_ACK_CMPL);
  }
  return BB_154_DRV_EVH_STAT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      EnDt x EDEND event handler
 *
 *  \param      None.
 *
 *  Called when EDEND event occurs in EnDt state
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHEnDtEdEnd(void)
{
  uint32_t ed = nrf_radio_ed_sample_get();

  /* Channel remains set up, but TXR is off. */
  bb154DrvOff();

  /* Signal ED completion. */
  if (bb154DrvCb.op.edCback != NULL)
  {
    bb154DrvCb.op.edCback((uint8_t)ed);
  }
  return BB_154_DRV_EVH_STAT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      TxCC x CCAIDLE event handler
 *
 *  \param      None.
 *
 *  Called when CCAIDLE event occurs in TxCC state
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHTxCCIdle(void)
{
  /* Energy below threshold - transmit immediately */
  /* Update event state. */
  bb154DrvCb.state = BB_154_DRV_STATE_TX;

  /* Disable shortcut */
  nrf_radio_shorts_disable(NRF_RADIO_SHORT_RXREADY_CCASTART_MASK);

  /* Start transmit. */
  bb154DrvTxDataNoCca(0, TRUE);
  return BB_154_DRV_EVH_STAT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      TxCC x CCABUSY event handler
 *
 *  \param      None.
 *
 *  Called when CCABUSY event occurs in TxCC state
 */
/*************************************************************************************************/
static uint8_t bb154DrvEvHTxCCBusy(void)
{
  /* Energy above threshold. Increment NB and check */
  if (++bb154DrvCb.tx.nb == bb154DrvPib.maxCSMABackoffs)
  {
    /* Run out of attempts to retry CCA */
    /* Channel remains set up, but TXR is off. */
    bb154DrvOff();

    /* CCA has failed */
    if (bb154DrvCb.op.errCback != NULL)
    {
      bb154DrvCb.op.errCback(BB_STATUS_TX_CCA_FAILED);
    }
  }
  else
  {
    /* Retry CCA */
    if (++bb154DrvCb.tx.be > bb154DrvPib.maxBE)
    {
      /* Bump at maxBE */
      bb154DrvCb.tx.be = bb154DrvPib.maxBE;
    }
    bb154DrvMacCca(0, TRUE);
    /* TODO: Any callback needed? */
  }
  return BB_154_DRV_EVH_STAT_OK;
}

/****************************/
/**** EXPORTED FUNCTIONS ****/
/****************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize the 802.15.4 baseband driver.
 *
 *  One-time initialization of baseband resources. This routine can be used to setup software
 *  driver resources, load RF trim parameters and execute RF calibrations.
 *
 *  This routine should block until the BB hardware is completely initialized.
 */
/*************************************************************************************************/
void PalBb154Init(void)
{
  memset(&bb154DrvCb, 0, sizeof(bb154DrvCb));
  memset(&bb154DrvStats, 0, sizeof(bb154DrvStats));

  BB_ASSERT(bb154DrvCb.state == BB_154_DRV_STATE_UNINITIALIZED); /* Driver should only be initialized once */

  /**** RADIO ****/

  /* Set configuration for nRF52840 - use fast ramp-up */
  /* Not in nrf_radio.h yet */
  NRF_RADIO->MODECNF0 = RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos;

  /* Note at present the following two calls do very little */
  /*
   * RadioDrvGetTiming(&bb154DrvRadioTiming);
   * RadioDrvInit();
   */
  bb154PrandInit();

  /* Set initial ack. packet */
  bb154DrvCb.ack[0] = ACK_LENGTH;
  bb154DrvCb.ack[1] = ACK_HEADER_WITHOUT_PENDING;

  bb154DrvCb.state = BB_154_DRV_STATE_INITIALIZED;

  PalBbRegisterProtIrq(BB_PROT_15P4, Bb154DrvTimerIRQHandler, Bb154DrvRadioIRQHandler);
}

/*************************************************************************************************/
/*!
 *  \brief      Register callbacks for the 802.15.4 baseband driver.
 */
/*************************************************************************************************/
void PalBb154Register(PalBb154Alloc_t allocCback, PalBb154Free_t freeCback)
{
  /* Check no null buffers passed. */

  BB_ASSERT(allocCback != NULL);
  BB_ASSERT(freeCback != NULL);

  bb154DrvCb.allocCback = allocCback;
  bb154DrvCb.freeCback = freeCback;
}

/*************************************************************************************************/
/*!
 *  \brief      Enable the BB hardware.
 *
 *  This routine brings the BB hardware out of low power (enable power and clocks). This routine is
 *  called just before a 802.15.4 BOD is executed.
 */
/*************************************************************************************************/
void PalBb154Enable(void)
{
  nrf_radio_packet_conf_t pktConf;
  uint32_t polynomial = BB_154_DRV_CRC_POLYNOMIAL;

  BB_ASSERT(bb154DrvCb.state == BB_154_DRV_STATE_INITIALIZED);

  bb154DrvCb.state = BB_154_DRV_STATE_ENABLED;

  PalBb154FlushPIB();

  /**** TIMER0 ****/

  /* Check HF clock is running using the crystal */
  BB_ASSERT(nrf_clock_hf_is_running(NRF_CLOCK_HFCLK_HIGH_ACCURACY)); /* HF clock source must be the crystal */

  /* Stop timer in case it was running (timer must be stopped for configuration) */
  nrf_timer_task_trigger(BB_154_DRV_TIMER, NRF_TIMER_TASK_STOP);

  /* Clear timer to zero count */
  nrf_timer_task_trigger(BB_154_DRV_TIMER, NRF_TIMER_TASK_CLEAR);

  /* Configure timer */
  nrf_timer_mode_set(BB_154_DRV_TIMER, NRF_TIMER_MODE_TIMER);
  nrf_timer_bit_width_set(BB_154_DRV_TIMER, NRF_TIMER_BIT_WIDTH_32);
  nrf_timer_frequency_set(BB_154_DRV_TIMER, TIMER_PRESCALER);

  /* Start timer as free running clock */
  nrf_timer_task_trigger(BB_154_DRV_TIMER, NRF_TIMER_TASK_START);

  /* Configure and enable PPI for timestamp */
  nrf_ppi_channel_endpoint_setup(BB_154_DRV_PPI_TIMESTAMP_CHAN,
                                 (uint32_t)nrf_radio_event_address_get(NRF_RADIO_EVENT_FRAMESTART),
                                 (uint32_t)nrf_timer_task_address_get(BB_154_DRV_TIMER, NRF_TIMER_TASK_CAPTURE2));
  nrf_ppi_channel_enable(BB_154_DRV_PPI_TIMESTAMP_CHAN);

  nrf_timer_int_disable(BB_154_DRV_TIMER, 0xFFFFFFFF); /* Blanket disable all timer interrupts at source */

  /* Set IRQ priority, clear pending interrupts and enable timer interrupts at system level */
  NVIC_SetPriority(BB_154_DRV_TIMER_IRQ, BB_154_DRV_TIMER_IRQ_PRIORITY);
  NVIC_ClearPendingIRQ(BB_154_DRV_TIMER_IRQ);
  NVIC_EnableIRQ(BB_154_DRV_TIMER_IRQ);

  /**** RADIO ****/

  /* Mode configuration */
  nrf_radio_mode_set(NRF_RADIO_MODE_IEEE802154_250KBIT);
  memset(&pktConf, 0, sizeof(pktConf));
  pktConf.lflen = 8;
  pktConf.plen = NRF_RADIO_PREAMBLE_LENGTH_32BIT_ZERO;
  pktConf.crcinc = TRUE;
  pktConf.maxlen = PAL_BB_154_aMaxPHYPacketSize;
  nrf_radio_packet_configure(&pktConf);

  /* CRC configuration */
#ifdef ADDITIONAL_TESTER_FEATURES
  if (bb154DrvPib.vsCRCOverride)
  {
    polynomial = bb154DrvPib.vsCRCOverride;
  }
#endif
  nrf_radio_crc_configure(BB_154_DRV_CRC_LENGTH, NRF_RADIO_CRC_ADDR_IEEE802154, polynomial);

  /* CCA configuration */
  nrf_radio_cca_configure(NRF_RADIO_CCA_MODE_ED, BB_154_DRV_CCA_ED_THRESHOLD, 0, 0);

  /* Configure MAC Header Match Unit */
  nrf_radio_mhmu_search_pattern_set(0);
  nrf_radio_mhmu_pattern_mask_set(MHMU_MASK);

  /*
   * Enable all relevant interrupts
   * The policy is to leave all interrupts expected enabled and handle interrupt
   * events appropriately given the baseband driver state.
   * TODO: It may be prudent to consider disabling some interrupts for ED
   */
  nrf_radio_int_enable(NRF_RADIO_INT_READY_MASK      |    /* Tx/Rx ramp-up done, ready to Tx/Rx */
                       NRF_RADIO_INT_FRAMESTART_MASK |    /* Length received */
                       NRF_RADIO_INT_BCMATCH_MASK    |    /* Bit counter match */
                       NRF_RADIO_INT_END_MASK        |    /* Packet tx'ed/rx'ed */
                       NRF_RADIO_INT_EDEND_MASK      |    /* ED ended */
                       NRF_RADIO_INT_CCAIDLE_MASK    |    /* CCA ended, idle */
                       NRF_RADIO_INT_CCABUSY_MASK);       /* CCA ended, busy */

  /* Set IRQ priority, clear pending interrupts and enable radio interrupts at system level */
  NVIC_SetPriority(BB_154_DRV_RADIO_IRQ, BB_154_DRV_RADIO_IRQ_PRIORITY);
  NVIC_ClearPendingIRQ(BB_154_DRV_RADIO_IRQ);
  NVIC_EnableIRQ(BB_154_DRV_RADIO_IRQ);
}

/*************************************************************************************************/
/*!
 *  \brief      Disable the BB hardware.
 *
 *  This routine signals the BB hardware to go into low power (disable power and clocks). This
 *  routine is called after all 802.15.4 operations are disabled.
 */
/*************************************************************************************************/
void PalBb154Disable(void)
{
  BB_ASSERT((bb154DrvCb.state == BB_154_DRV_STATE_ENABLED) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_CHAN_SET) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_OFF));

  bb154DrvCb.state = BB_154_DRV_STATE_INITIALIZED;

  /**** TIMER0 ****/

  /* Disable and clear out TIMER0 interrupts */
  nrf_timer_int_disable(BB_154_DRV_TIMER, 0xFFFFFFFF); /* Disable all interrupts at source */
  NVIC_DisableIRQ(BB_154_DRV_TIMER_IRQ);
  NVIC_ClearPendingIRQ(BB_154_DRV_TIMER_IRQ);

  /* Stop timer */
  nrf_timer_task_trigger(BB_154_DRV_TIMER, NRF_TIMER_TASK_STOP);

  /*
   * Disable PPI channels:
   * Chan 14: COMPARE[0] -> TXEN/RXEN
   * Chan 15: FRAMESTART -> CAPTURE[2] for timestamp
   */
  nrf_ppi_channel_disable(BB_154_DRV_PPI_TXRX_DELAY_CHAN);
  nrf_ppi_channel_disable(BB_154_DRV_PPI_TIMESTAMP_CHAN);

  /**** RADIO ****/

  /* Disable and clear out RADIO interrupts */
  nrf_radio_int_disable(0xFFFFFFFF); /* Disable all interrupts at source */
  NVIC_DisableIRQ(BB_154_DRV_RADIO_IRQ);
  NVIC_ClearPendingIRQ(BB_154_DRV_RADIO_IRQ);
}

/*************************************************************************************************/
/*!
 *  \brief      Set channelization parameters.
 *
 *  \param      pParam        Channelization parameters.
 *
 *  Calling this routine will set parameters for all future transmit, receive, and energy detect
 *  operations until this routine is called again providing new parameters.
 *
 *  \note       \a pParam is not guaranteed to be static and is only valid in the context of the
 *              call to this routine. Therefore parameters requiring persistence should be copied.
 */
/*************************************************************************************************/
void PalBb154SetChannelParam(const PalBb154Chan_t *pParam)
{
  /* Retain channel parameters. */
  bb154DrvCb.chan = *pParam;

  /* Event has been started. */
  bb154DrvCb.state = BB_154_DRV_STATE_CHAN_SET;

  /* Set channel parameters */
  bb154DrvChannelSet(bb154DrvCb.chan.channel);
  bb154DrvTxPowerSet(bb154DrvCb.chan.txPower);
}

/*************************************************************************************************/
/*!
 *  \brief      Reset channelization parameters.
 *
 *  Calling this routine will reset (clear) the channelization parameters.
 */
/*************************************************************************************************/
void PalBb154ResetChannelParam(void)
{
  if (bb154DrvCb.state == BB_154_DRV_STATE_ENABLED)
  {
    return;
  }

  BB_ASSERT((bb154DrvCb.state == BB_154_DRV_STATE_CHAN_SET) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_OFF));

  /* Event has been stopped. */
  bb154DrvCb.state = BB_154_DRV_STATE_ENABLED;

  /* Reset channel parameters - nothing to do */
}

/*************************************************************************************************/
/*!
 *  \brief      Set the operation parameters.
 *
 *  \param      pOpParam    Operations parameters.
 *
 *  Calling this routine will set parameters for all future transmit, receive, ED, and CCA
 *  operations until this routine is called again providing new parameters.
 *
 *  \note       \a pOpParam is not guaranteed to be static and is only valid in the context of the
 *              call to this routine. Therefore parameters requiring persistence should be copied.
 */
/*************************************************************************************************/
void PalBb154SetOpParams(const PalBb154OpParam_t *pOpParam)
{
  BB_ASSERT(bb154DrvCb.state == BB_154_DRV_STATE_CHAN_SET);

  /* Retain operation parameters. */
  bb154DrvCb.op = *pOpParam;

  bb154DrvCb.state = BB_154_DRV_STATE_OFF;
}

/*************************************************************************************************/
/*!
 *  \brief      Get Driver PIB.
 *
 *  \return     Driver PIB pointer.
 *
 *  Calling this routine retun a pointer to the driver PIB.
 */
/*************************************************************************************************/
PalBb154DrvPib_t *PalBb154GetDrvPIB(void)
{
  return &bb154DrvPib;
}

/*************************************************************************************************/
/*!
 *  \brief      Flushes PIB attributes to hardware.
 *
 *  \param      None.
 *
 *  Calling this routine will flush all PIB attributes that have a hardware counterpart to the
 *  respective registers in hardware.
 */
/*************************************************************************************************/
void PalBb154FlushPIB(void)
{
  if (!bb154DrvPib.promiscuousMode)
  {
    /* Program network parameters. */
    bb154DrvCb.rxAddr.panId = bb154DrvPib.panId;
    bb154DrvCb.rxAddr.shortAddr = bb154DrvPib.shortAddr;
    bb154DrvCb.rxAddr.extAddr = bb154DrvPib.extAddr;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Transmit a packet.
 *
 *  \param      descs       Array of transmit buffer descriptors.
 *  \param      cnt         Number of descriptors.
 *  \param      due         Due time for transmit (if \a now is FALSE).
 *  \param      now         TRUE if packet should be transmitted with minimal delay.
 *
 *  \note       Assumes unslotted CSMA/CA.
 */
/*************************************************************************************************/
void PalBb154Tx(PalBb154TxBufDesc_t *pDesc, uint8_t cnt, uint32_t due, bool_t now)
{
  /* ----- clear state ----- */

  BB_ASSERT((bb154DrvCb.state == BB_154_DRV_STATE_RX_HDR) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_RX_FRM) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_OFF));
  BB_ASSERT(cnt == 1);

  /* ----- start CCA ----- */

  /* Hard stop of all radio interrupts. */
  NVIC_DisableIRQ(BB_154_DRV_RADIO_IRQ);

  /* Retain buffer. */
  bb154DrvCb.tx.pDesc = pDesc;

  /* Set retry counter */
  bb154DrvCb.tx.retry = bb154DrvPib.maxFrameRetries;

  /* Transmit the data */
  bb154DrvTxData(due, now);

  /* Schedule IFS operation when the Tx operation fully completes. */
  /* Re-enable radio interrupts. */
  NVIC_EnableIRQ(BB_154_DRV_RADIO_IRQ);
}

/*************************************************************************************************/
/*!
 *  \brief      Clear all received buffers (active and queued).
 *
 *  \param      None.
 *
 *  Calling this routine will clear and free the active receive buffer (if any) and all queued
 *  receive buffers. This should only be called when the operation is terminating.
 */
/*************************************************************************************************/
void PalBb154ClearRxBufs(void)
{
  uint8_t *pRxBuf;

  BB_ASSERT(bb154DrvCb.freeCback != NULL);

  /* If currently primed to receive, free the active receive buffer */
  if (bb154DrvCb.rx.pBuf != NULL)
  {
    bb154DrvCb.freeCback(bb154DrvCb.rx.pBuf);
    bb154DrvCb.rx.pBuf = NULL;
  }

  /* Free any queued receive buffers */
  while ((pRxBuf = bb154QueueDeq(&bb154DrvCb.rx.bufQ)) != NULL)
  {
    bb154DrvCb.freeCback(pRxBuf);
  }
  bb154DrvCb.rx.bufCount = 0;
}

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
uint8_t PalBb154ReclaimRxFrame(uint8_t *pRxFrame)
{
  if (pRxFrame != NULL)
  {
    bb154QueueEnq(&bb154DrvCb.rx.bufQ, BB_154_DRV_RX_BUF_PTR_FROM_FRAME(pRxFrame));
    bb154DrvCb.rx.bufCount++;
  }
  return bb154DrvCb.rx.bufCount;
}

/*************************************************************************************************/
/*!
 *  \brief      Build receive buffer queue
 *
 *  \param      len     Length of each receive buffer.
 *  \param      num     Number of buffers to load into the queue.
 */
/*************************************************************************************************/
void PalBb154BuildRxBufQueue(uint8_t num)
{
  uint8_t *pRxBuf;
  uint8_t bufCount;

  BB_ASSERT(bb154DrvCb.allocCback != NULL);

  for (bufCount = bb154DrvCb.rx.bufCount; bufCount < num; bufCount++)
  {
    if ((pRxBuf = bb154DrvCb.allocCback(PAL_BB_154_aMaxPHYPacketSize)) == NULL)
    {
      break;
    }
    bb154QueueEnq(&bb154DrvCb.rx.bufQ, pRxBuf);
    bb154DrvCb.rx.bufCount++;
  }
}

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
uint8_t *PalBb154GetPayloadPtr(uint8_t *pFrame, uint16_t fctl)
{
  static const uint8_t amLUT[4] = {0,127,4,10};
  uint8_t offset;

  offset = amLUT[PAL_BB_154_FC_DST_ADDR_MODE(fctl)] + amLUT[PAL_BB_154_FC_SRC_ADDR_MODE(fctl)];
  if ((offset == 0) || (offset >= 127))
  {
    /* DAM and SAM cannot both be 0 or either an illegal value */
    return NULL;
  }

  /* Work out where the command byte is - post header */
  if (fctl & PAL_BB_154_FC_PAN_ID_COMP_MASK)
  {
    offset -= 2;
  }
  return pFrame + 3 + offset;
}

/*************************************************************************************************/
/*!
 *  \brief      Receive a packet.
 *
 *  \param      due         Due time for receive (if \a now is FALSE).
 *  \param      now         TRUE if packet should be received with minimal delay.
 */
/*************************************************************************************************/
void PalBb154Rx(uint32_t due, bool_t now, uint32_t timeout)
{
  BB_ASSERT(bb154DrvCb.rx.bufQ.pHead != NULL);

  if (bb154DrvCb.state == BB_154_DRV_STATE_RX_HDR)
  {
    /*
     * We may already be in this state as receive gets re-enabled due
     * to RXWI or timed rx enable. In this case, timeout will be indefinite.
     */
    return;
  }

  /* Hard stop of all radio interrupts. */
  NVIC_DisableIRQ(BB_154_DRV_RADIO_IRQ);

  /* Update event state. */
  bb154DrvCb.state = BB_154_DRV_STATE_RX_HDR;

  /* Start receive. */
  bb154DrvRxData(due, now, timeout);

  /* TODO: Schedule IFS operation when this operation completes. */
  /* Re-enable radio interrupts. */
  NVIC_EnableIRQ(BB_154_DRV_RADIO_IRQ);
}

/*************************************************************************************************/
/*!
 *  \brief      Perform energy detect.
 *
 *  \param      due         Due time for energy detect (if \a now is FALSE).
 *  \param      now         TRUE if energy detect should occur minimal delay.
 *
 *  Perform energy detect and return RSSI to determine channel status.
 */
/*************************************************************************************************/
void PalBb154Ed(uint32_t due, bool_t now)
{
  /* ----- clear state ----- */

  BB_ASSERT(bb154DrvCb.state == BB_154_DRV_STATE_OFF);
  BB_154_DRV_INC_STAT(bb154DrvStats.edReq);

  /* Update event state. */
  bb154DrvCb.state = BB_154_DRV_STATE_ED;

  /* ----- start Ed ----- */

  /* Hard stop of all radio interrupts. */
  NVIC_DisableIRQ(BB_154_DRV_RADIO_IRQ);

  bb154DrvClearAllEvents();

  /*
   * Enable shortcuts:
   * EVENTS_READY -> TASKS_EDSTART
   */
  nrf_radio_shorts_enable(NRF_RADIO_SHORT_READY_EDSTART_MASK);

  /* 60 is half an aBaseSuperframeDuration time. */
  nrf_radio_ed_loop_count_set(60); /* Note: Potential to use this more efficiently in scans. */

  /* Start the task, which may be delayed */
  if (!bb154DrvStartRadioTask(due, now, NRF_RADIO_TASK_RXEN))
  {
    BB_154_DRV_INC_STAT(bb154DrvStats.edSchMiss);
    bb154DrvDelayFailNoRx();
  }

  /* Re-enable radio interrupts. */
  NVIC_EnableIRQ(BB_154_DRV_RADIO_IRQ);
}

/*************************************************************************************************/
/*!
 *  \brief      Cancel any pending operation.
 *
 *  \return     TRUE if pending operation could be cancelled.
 *
 *  Cancel any pending operation.
 */
/*************************************************************************************************/
bool_t PalBb154Off(void)
{
  if ((bb154DrvCb.state == BB_154_DRV_STATE_OFF)      ||
      (bb154DrvCb.state == BB_154_DRV_STATE_CHAN_SET) ||
      (bb154DrvCb.state == BB_154_DRV_STATE_ENABLED))
  {
    return TRUE;
  }

  BB_ASSERT((bb154DrvCb.state == BB_154_DRV_STATE_RX_HDR) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_RX_FRM) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_RX_ACK) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_TX)     ||
            (bb154DrvCb.state == BB_154_DRV_STATE_TX_CCA) ||
            (bb154DrvCb.state == BB_154_DRV_STATE_ED)     ||
            (bb154DrvCb.state == BB_154_DRV_STATE_TX_ACK));

  /* Hard stop of all radio interrupts. */
  NVIC_DisableIRQ(BB_154_DRV_RADIO_IRQ);

  bb154DrvOff();

  /* Re-enable radio interrupts. */
  NVIC_EnableIRQ(BB_154_DRV_RADIO_IRQ);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      Convert RSSI to LQI.
 *
 *  \return     LQI value.
 *
 *  Converts RSSI value into equivalent LQI value from 0 to 0xFF.
 */
/*************************************************************************************************/
uint8_t PalBb154RssiToLqi(int8_t rssi)
{
  uint32_t lqi;

  if (rssi <= BB_154_DRV_MIN_RSSI)
  {
    lqi = 0;
  }
  else
  {
    lqi = (uint32_t)(rssi - BB_154_DRV_MIN_RSSI) * BB_154_DRV_ED_SCALE_65536;
    lqi >>= 16; /* Divide by 65536 */
    if (lqi > 0xff)
    {
      lqi = 0xff;
    }
  }
  return (uint8_t)lqi;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the last received RSSI.
 *
 *  \param  pBuf            Storage for results -- first element is integer dBm, second is fractional
 */
/*************************************************************************************************/
void PalBb154GetLastRssi(uint8_t *pBuf)
{
  pBuf[0] = bb154DrvLastRssi;
  pBuf[1] = 0;
  pBuf[2] = 0;
  pBuf[3] = 0;
}

/****************************/
/**** INTERRUPT HANDLERS ****/
/****************************/

/*************************************************************************************************/
/*!
 *  \brief      TIMER0 interrupt handler "RX Timeout"
 *
 *  \param      None.
 *
 *  This is the "RX Timeout" interrupt. No other functionality is shared on the TIMER0 interrupt.
 *  If the receive operation has not gotten started, it is cancelled.
 *
 */
/*************************************************************************************************/
static void Bb154DrvTimerIRQHandler(void)
{
  /* Can simplify with one event */
  if (nrf_timer_event_check(BB_154_DRV_TIMER, NRF_TIMER_EVENT_COMPARE1))
  {
    /* Clear event */
    nrf_timer_event_clear(BB_154_DRV_TIMER, NRF_TIMER_EVENT_COMPARE1);
    /* Invoke event handler */
    bb154DrvEvtHandler[bb154DrvStateTable[0 + BB_154_DRV_TIMER_EVT_OFFSET][bb154DrvCb.state]]();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Radio interrupt handler.
 *
 *  \param      None.
 *
 *  This the radio interrupt service routine.  It is at the heart of the baseband driver
 *  design. It handles the following interrupts:
 *
 */
/*************************************************************************************************/
static void Bb154DrvRadioIRQHandler(void)
{
  int i;
  static const nrf_radio_event_t eventsToCheck[BB_154_DRV_MAX_RADIO_EVENTS] =
  {
     NRF_RADIO_EVENT_READY,
     NRF_RADIO_EVENT_FRAMESTART,
     NRF_RADIO_EVENT_BCMATCH,
     NRF_RADIO_EVENT_END,
     NRF_RADIO_EVENT_EDEND,
     NRF_RADIO_EVENT_CCAIDLE,
     NRF_RADIO_EVENT_CCABUSY
  };
  nrf_radio_event_t eventToCheck;
  uint8_t stat;

  /* Cycle through events and if asserted, clear event and call state handler */
  for (i = 0; i < BB_154_DRV_MAX_RADIO_EVENTS; i++)
  {
    eventToCheck = eventsToCheck[i];
    if (nrf_radio_event_check(eventToCheck))
    {
      /* Clear event */
      nrf_radio_event_clear(eventToCheck);

      /* Invoke event handler */
      stat = bb154DrvEvtHandler[bb154DrvStateTable[i + BB_154_DRV_RADIO_EVT_OFFSET][bb154DrvCb.state]]();
      if (stat != BB_154_DRV_EVH_STAT_INSIGNIFICANT)
      {
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Get baseband driver statistics.
 *
 *  \param  pStats          Storage for statistics.
 */
/*************************************************************************************************/
void PalBb154DrvGetStats(PalBb154DrvStats_t *pStats)
{
  /* Stub, needs proper implementation */
  memset(pStats, 0, sizeof(*pStats));
}

/*************************************************************************************************/
/*!
 *  \brief  Stop continuous Tx or Rx operation
 *
 *  \param  None.
 *
 */
/*************************************************************************************************/
void PalBb154ContinuousStop(void)
{
  /* Stub, needs proper implementation */
}

/*************************************************************************************************/
/*!
 *  \brief  Start continuous Rx mode.
 *
 *  \param  rfChan          RF channel
 */
/*************************************************************************************************/
void PalBb154ContinuousRx(uint8_t rfChan, uint8_t rxPhy)
{
  /* Stub, needs proper implementation */
  (void)rfChan;
  (void)rxPhy;
}

/*************************************************************************************************/
/*!
 *  \brief  Start continuous Tx mode.
 *
 *  \param  rfChan          RF channel.
 *  \param  mod             Modulation.
 *  \param  txPower         Transmit power
 */
/*************************************************************************************************/
void PalBb154ContinuousTx(uint8_t rfChan, uint8_t modulation, uint8_t txPhy, int8_t txPower)
{
  /* Stub, needs proper implementation */
  (void)rfChan;
  (void)modulation;
  (void)txPhy;
  (void)txPower;
}
