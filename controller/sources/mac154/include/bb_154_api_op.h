/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband interface file.
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

#ifndef BB_154_API_OP_H
#define BB_154_API_OP_H

#include "bb_api.h"
#include "bb_154.h"
#include "mac_154_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup BB_API_154
 *  \{ */

/**************************************************************************************************
  Constants
**************************************************************************************************/

#define BB_154_SCAN_MIN_RX_BUF_CNT              2
#define BB_154_DATA_TX_MIN_RX_BUF_CNT           2
#define BB_154_DATA_RX_MIN_RX_BUF_CNT           2
#define BB_154_DATA_IND_BUF_CNT                 5
#define BB_154_ASSOC_MIN_RX_BUF_CNT             2

#define BB_154_BASE_SUPERFRAME_DURATION_SYMB    960
#define BB_154_ED_DURATION_SYMB                 8

/*! \brief      Operation types. */
enum
{
  BB_154_OP_TEST_TX,                    /*!< Continuous Tx test mode. */
  BB_154_OP_TEST_RX,                    /*!< Continuous Rx test mode. */
  BB_154_OP_SCAN,                       /*!< Scan mode. */
  BB_154_OP_ASSOC,                      /*!< Association mode. */
  BB_154_OP_DATA_TX,                    /*!< Single frame data transmit mode. */
  BB_154_OP_DATA_RX,                    /*!< Data receive mode. */
  BB_154_OP_DATA_POLL,                  /*!< Data poll mode. */
  BB_154_OP_NUM
};

/*! \brief      ED/CCA test modes. */
enum
{
  BB_154_ED_SCAN_TEST_MODE_NONE = 0,    /*!< ED scan no test mode. */
  BB_154_ED_SCAN_TEST_MODE_ED = 1,      /*!< ED scan ED test mode. */
  BB_154_ED_SCAN_TEST_MODE_CCA = 2,     /*!< ED scan CCA test mode. */
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/* Forward reference */
typedef struct Bb154Data_tag Bb154Data_t;

/*! \brief      Execution operation function. */
typedef void (*bb154ExecOpFn_t)(BbOpDesc_t *pBod, Bb154Data_t *p154);

/*
 * TODO: It's not clear we need a separate entry for each operation
 * type as they are mutually exclusive at present. This might be needed
 * for nested operations.
 */

/*! \brief      BB control block. */
typedef struct
{
  struct
  {
    bb154ExecOpFn_t execOpCback;        /*!< Execute operation handler. */
  } opCbacks[BB_154_OP_NUM];            /*!< Operation handlers. */
} bb154CtrlBlk_t;

/*! \brief      Test completion callback signature. */
typedef bool_t (*Bb154TestComp_t)(BbOpDesc_t *pBod, bool_t ack, bool_t success);

/*! \brief      Test transmit operation data. */
typedef struct
{
  Bb154TestComp_t         testCback;          /*!< Test callback. */
  PalBb154TxBufDesc_t     *pTxDesc;           /*!< Transmit data buffer descriptor. */
  uint16_t                rxLen;              /*!< Receive data buffer length. */
  uint32_t                pktInterUsec;       /*!< Transmit packet interval. */
  bool_t                  rxAck;              /*!< TRUE if ACK should be received. */
} Bb154TestTx_t;

/*! \brief      Test receive operation data. */
typedef struct
{
  Bb154TestComp_t         testCback;          /*!< Test callback. */
  uint8_t                 *pRxFrame;          /*!< Received frame */
  uint16_t                rxLen;              /*!< Received frame length */
  bool_t                  txAck;              /*!< TRUE if ACK should be transmitted. */
} Bb154TestRx_t;

/*! \brief      Scan operation data. */
typedef struct
{
  PalBb154TxBufDesc_t     *pTxDesc;           /*!< Beacon request/orphan notification buffer */
  uint32_t                snapshot;           /*!< BB timer snapshot */
  uint32_t                elapsed;            /*!< BB timer elapsed time */
  uint32_t                duration;           /*!< Scan Duration */
  uint32_t                remaining;          /*!< Scan Remaining time */
  uint32_t                channels;           /*!< Channel bitmap to scan */
  uint8_t                 channel;            /*!< Intermediate channel value */
  uint8_t                 type;               /*!< Type of Scan */
  bool_t                  terminate;          /*!< Flag to force termination */
  uint8_t                 testMode;           /*!< ED/CCA scan test mode - 10 consecutive ED/CCA scans on one channel */
  uint8_t                 listSize;           /*!< List size */
  Mac154ScanResults_t     results;            /*!< Scan results */
} Bb154Scan_t;

/*! \brief      Association request operation data. */
typedef struct
{
  PalBb154TxBufDesc_t     *pTxDesc;           /*!< Associated transmit frame buffer */
  uint8_t                 *pRxFrame;          /*!< Pointer to received frame */
  uint16_t                rxLen;              /*!< Received frame length */
  uint8_t                 cmd;                /*|< MAC command frame (assoc req, assoc rsp, disassoc) */
  uint8_t                 status;             /*|< Confirm status */
} Bb154Assoc_t;

/*! \brief      Association request operation data. */
typedef struct
{
  Bb154Assoc_t            assoc;              /*!< Common association data */
  Mac154Addr_t            coordAddr;          /*|< Coordinator address */
  uint16_t                addr;               /*!< Association allocated short address */
  uint8_t                 status;             /*!< Status */
} Bb154AssocReq_t;

/*! \brief      Association response operation data. */
typedef struct
{
  Bb154Assoc_t            assoc;              /*!< Common association data */
} Bb154AssocRsp_t;

/*! \brief      Disassociation operation data. */
typedef struct
{
  Bb154Assoc_t            assoc;              /*!< Common association data */
  uint8_t                 txIndirect;         /*!< Sending indirect */
  Mac154Addr_t            deviceAddr;         /*|< Device address */
  uint8_t                 reason;             /*!< Reason */
} Bb154Disassoc_t;

/*! \brief      Start operation data. */
typedef struct
{
  PalBb154TxBufDesc_t     *pTxDesc;           /*!< Associated frame buffer */
  uint32_t                snapshot;           /*!< BB timer snapshot */
  uint32_t                timestamp;          /*!< When frame was transmitted */
  uint8_t                 status;             /*!< Status */
  uint16_t                panId;              /*!< PAN ID to start */
  uint8_t                 panCoord;           /*!< Start as PAN coordinator */
  uint8_t                 logChan;            /*!< Channel to start on */
  uint8_t                 txPower;            /*!< Tx power to start on */
} Bb154Start_t;

/*! \brief      Data transmit operation data. */
typedef struct
{
  PalBb154TxBufDesc_t     *pTxDesc;           /*!< Associated frame buffer */
  uint32_t                snapshot;           /*!< BB timer snapshot */
  uint32_t                timestamp;          /*!< When frame was transmitted */
  uint8_t                 status;             /*!< Status */
} Bb154DataTx_t;

/*! \brief      Data poll operation data. */
typedef struct
{
  PalBb154TxBufDesc_t     *pTxDesc;           /*!< Associated frame buffer */
  uint32_t                snapshot;           /*!< BB timer snapshot */
  uint32_t                timestamp;          /*!< When frame was transmitted */
  uint8_t                 status;             /*!< Status */
  uint8_t                 linkQuality;        /*!< Link quality */
  uint8_t                 *pRxFrame;          /*!< Pointer to received frame */
  uint16_t                rxLen;              /*!< Received frame length */
} Bb154Poll_t;

/*! \brief      Data receive operation data. */
typedef struct
{
  PalBb154TxBufDesc_t     *pTxDesc;           /*!< Frame buffer which may be transmitted. */
  uint8_t                 *pRxFrame;          /*!< Pointer to received frame */
  uint16_t                rxLen;              /*!< Received frame length */
  uint8_t                 msduHandle;         /*!< MSDU handle. */
} Bb154DataRx_t;

/*! \brief      802.15.4 protocol specific operation parameters. */
struct Bb154Data_tag
{
  Mac154ParamTimer_t      guardTimer;         /*!< Guard timer */
  uint8_t                 opType;             /*!< Operation type. */
  PalBb154Chan_t          chan;               /*!< Channel. */
  PalBb154OpParam_t       opParam;            /*!< Operation parameters. */

  union
  {
    Bb154TestTx_t         testTx;             /*!< Transmit test data. */
    Bb154TestRx_t         testRx;             /*!< Receive test data. */
    Bb154Assoc_t          assoc;              /*!< Common association data. */
    Bb154AssocReq_t       assocReq;           /*!< Association request data. */
    Bb154AssocRsp_t       assocRsp;           /*!< Association response data. */
    Bb154Disassoc_t       disassoc;           /*!< Disassociation notification data. */
    Bb154Scan_t           scan;               /*!< Scan data. */
    Bb154Start_t          start;              /*!< Start data. */
    Bb154DataTx_t         dataTx;             /*!< Single frame tx data. */
    Bb154DataRx_t         dataRx;             /*!< Rx data. */
    Bb154Poll_t           poll;               /*!< Poll. */
  } op;                                       /*!< Operation specific data. */
};

/*! \} */    /* BB_API_154 */

/*************************************************************************************************/
/*!
 *  \brief      Register operation handlers.
 *
 *  \param      opType          Operation type.
 *  \param      execOpCback     Execute operation callback.
 *  \param      cancelOpCback   Cancel operation callback.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void bb154RegisterOp(uint8_t opType, bb154ExecOpFn_t execOpCback);

#ifdef __cplusplus
};
#endif

#endif /* BB_154_API_OP_H */
