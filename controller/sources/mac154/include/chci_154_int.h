/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI interface file.
 *
 *  Copyright (c) 2013-2018 Arm Ltd.
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

#ifndef CHCI_154_INT_H
#define CHCI_154_INT_H

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "mac_154_cfg.h"
#include "mac_154_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      Message header length. */
#define CHCI_154_MSG_HDR_LEN    3

/*! \brief      Command handlers. */
#ifndef CHCI_154_CMD_HANDLER_NUM
#define CHCI_154_CMD_HANDLER_NUM                8
#endif

/*! \brief      Command defines. */
#define CHCI_154_CMD_TEST_GET_BB_STATS_REQ      0xF1
#define CHCI_154_CMD_TEST_GET_BB_STATS_CNF      0xF2
#define CHCI_154_CMD_TEST_CONTINOUS_STOP        0xF3
#define CHCI_154_CMD_TEST_CONTINOUS_TX          0xF4
#define CHCI_154_CMD_TEST_CONTINOUS_RX          0xF5
#define CHCI_154_CMD_TEST_GET_PKT_STATS_REQ     0xF6
#define CHCI_154_CMD_TEST_GET_PKT_STATS_CNF     0xF7
#define CHCI_154_CMD_TEST_GET_LAST_RSSI_REQ     0xF8
#define CHCI_154_CMD_TEST_GET_LAST_RSSI_CNF     0xF9
#define CHCI_154_CMD_TEST_SET_NET_PARAMS_REQ    0xFA
#define CHCI_154_CMD_TEST_SET_NET_PARAMS_CNF    0xFB
#define CHCI_154_CMD_TEST_TX                    0xFC
#define CHCI_154_CMD_TEST_RX                    0xFD
#define CHCI_154_CMD_TEST_END                   0xFE
#define CHCI_154_CMD_TEST_END_IND               0xFF

/*! \brief      Command lengths. */
#define CHCI_154_LEN_TEST_GET_BB_STATS          0
#define CHCI_154_LEN_TEST_GET_BB_STATS_CNF      60 /* Vendor specific? */
#define CHCI_154_LEN_TEST_CONTINUOUS_STOP       0
#define CHCI_154_LEN_TEST_CONTINUOUS_TX         4
#define CHCI_154_LEN_TEST_CONTINUOUS_RX         2
#define CHCI_154_LEN_TEST_SET_NET_PARAMS_MIN    12
#define CHCI_154_LEN_TEST_SET_NET_PARAMS_CNF    1
#define CHCI_154_LEN_TEST_GET_PKT_STATS_CNF     16
#define CHCI_154_LEN_TEST_GET_LAST_RSSI_CNF     5
#define CHCI_154_LEN_TEST_TX                    22
#define CHCI_154_LEN_TEST_RX                    5
#define CHCI_154_LEN_TEST_END                   0
#define CHCI_154_LEN_TEST_END_IND               16

/*! brief       Command CHCI host to controller */
#define CHCI_154_CMD_MLME_ASSOC_REQ             0x01
#define CHCI_154_CMD_MLME_ASSOC_RSP             0x02
#define CHCI_154_CMD_MLME_DISASSOC_REQ          0x03
#define CHCI_154_CMD_MLME_GET_REQ               0x04
#define CHCI_154_CMD_MLME_ORPHAN_RSP            0x05
#define CHCI_154_CMD_MLME_RESET_REQ             0x06
#define CHCI_154_CMD_MLME_RX_ENABLE_REQ         0x07
#define CHCI_154_CMD_MLME_SCAN_REQ              0x08
#define CHCI_154_CMD_MLME_SET_REQ               0x09
#define CHCI_154_CMD_MLME_START_REQ             0x0A
#define CHCI_154_CMD_MLME_POLL_REQ              0x0B
#define CHCI_154_CMD_MCPS_PURGE_REQ             0x0C
#define CHCI_154_CMD_VS_DIAG_CFG_REQ            0xA1

/*! brief       Event CHCI controller to host */
#define CHCI_154_EVT_MLME_ASSOC_CFM             0x81
#define CHCI_154_EVT_MLME_ASSOC_IND             0x82
#define CHCI_154_EVT_MLME_DISASSOC_CFM          0x83
#define CHCI_154_EVT_MLME_DISASSOC_IND          0x84
#define CHCI_154_EVT_MLME_BEACON_NOTIFY_IND     0x85
#define CHCI_154_EVT_MLME_GET_CFM               0x86
#define CHCI_154_EVT_MLME_ORPHAN_IND            0x87
#define CHCI_154_EVT_MLME_RESET_CFM             0x88
#define CHCI_154_EVT_MLME_RX_ENABLE_CFM         0x89
#define CHCI_154_EVT_MLME_SCAN_CFM              0x8A
#define CHCI_154_EVT_MLME_COMM_STATUS_IND       0x8B
#define CHCI_154_EVT_MLME_SET_CFM               0x8C
#define CHCI_154_EVT_MLME_START_CFM             0x8D
#define CHCI_154_EVT_MLME_POLL_CFM              0x8E
#define CHCI_154_EVT_MCPS_PURGE_CFM             0x8F
#define CHCI_154_EVT_MCPS_DATA_CFM              0x90
#define CHCI_154_EVT_MLME_POLL_IND              0x91
#define CHCI_154_EVT_VS_DIAG_IND                0xA0

/*! brief       Data CHCI host to controller */
#define CHCI_154_DATA_MCPS_DATA_REQ             0x01

/*! brief       Data CHCI controller to host */
#define CHCI_154_DATA_MCPS_DATA_IND             0x81

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      HCI header */
typedef struct
{
  uint8_t  code;                    /*!< Code field. */
  uint16_t len;                     /*!< Parameter length. */
} Chci154Hdr_t;

/*! \brief      Command handler call signature. */
typedef bool_t (*Chci154CmdHandler_t)(Chci154Hdr_t *pHdr, uint8_t *pBuf);

/*! \brief      Data handler call signature. */
typedef void (*Chci154DataHandler_t)(Chci154Hdr_t *pHdr, uint8_t *pBuf);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize 802.15.4 controller HCI handler.
 *
 *  \param      None.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154HandlerInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Register a command handler.
 *
 *  \param      cmdHandler  Command handler.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154RegisterCmdHandler(Chci154CmdHandler_t cmdHandler);

/*************************************************************************************************/
/*!
 *  \brief      Invoke a command handler.
 *
 *  \param      pHdr  Command header.
 *  \param      pBuf  Command buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
bool_t Chci154InvokeCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf);

/*************************************************************************************************/
/*!
 *  \brief      Register data handler.
 *
 *  \param      dataHandler  Data handler.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154RegisterDataHandler(Chci154DataHandler_t dataHandler);

/*************************************************************************************************/
/*!
 *  \brief      Send an event and service the event queue.
 *
 *  \param      pBuf        Buffer containing event to send or NULL to service the queue.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154SendEvent(uint8_t *pBuf);

/*************************************************************************************************/
/*!
 *  \brief      Send data and service the data queue.
 *
 *  \param      pBuf        Buffer containing data to send or NULL to service the queue.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154SendData(uint8_t *pBuf);

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for test operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154TestInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Send test end indication.
 *
 *  \param      pktCount    Packet count.
 *  \param      pktErrCount Packet error count.
 *  \param      ackCount    ACK count.
 *  \param      ackErrCount ACK error count.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154TestSendTestEndInd(uint32_t pktCount,
                               uint32_t pktErrCount,
                               uint32_t ackCount,
                               uint32_t ackErrCount);

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for VS operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154VsInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for association operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Send associate confirm.
 *
 *  \param      assocShtAddr  Associated short address
 *  \param      status      Association status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendAssocCfm(uint16_t assocShtAddr, uint8_t status);

/*************************************************************************************************/
/*!
 *  \brief      Send associate indication.
 *
 *  \param      deviceAddr  Device address
 *  \param      capInfo     Capability information
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendAssocInd(uint64a_t deviceAddr, uint8_t capInfo);

#if MAC_154_OPT_DISASSOC

/*************************************************************************************************/
/*!
 *  \brief      Send disassociate indication.
 *
 *  \param      deviceAddr    Address of device disassociating
 *  \param      reason        Disassociation reason
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendDisassocInd(uint64a_t deviceAddr, uint8_t reason);

/*************************************************************************************************/
/*!
 *  \brief      Send disassociate confirm.
 *
 *  \param      deviceAddr  Address disassoc. notification was sent to
 *  \param      status      Disassociation status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendDisassocCfm(Mac154Addr_t *pDeviceAddr, uint8_t status);

#endif /* MAC_154_OPT_DISASSOC */

#if MAC_154_OPT_ORPHAN

/*************************************************************************************************/
/*!
 *  \brief      Send orphan indication.
 *
 *  \param      orphanAddr    Address of orphaned device
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendOrphanInd(uint64a_t orphanAddr);

#endif /* MAC_154_OPT_ORPHAN */

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for scan operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154ScanInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Send scan confirm.
 *
 *  \param      channels    Unscanned channels.
 *  \param      type        Scan type.
 *  \param      listSize    Results list size.
 *  \param      pScanList   Pointer to scan list.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154ScanSendCfm(uint32_t channels,
                        uint8_t type,
                        uint8_t listSize,
                        Mac154ScanResults_t *pScanResults,
                        uint8_t statusOverride);

/*************************************************************************************************/
/*!
 *  \brief      Send scan beacon notify indication.
 *
 *  \param      bsn         Beacon sequence number.
 *  \param      pPanDescr   Pointer to PAN descriptor.
 *  \param      sduLength   SDU (beacon payload) length
 *  \param      pSdu        pointer to SDU (beacon payload)
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154ScanSendBeaconNotifyInd(uint8_t bsn,
                                    Mac154PanDescr_t *pPanDescr,
                                    uint8_t sduLength,
                                    uint8_t *pSdu);

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for miscellaneous operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Send reset confirm.
 *
 *  \param      status  Reset status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscSendResetCfm(const uint8_t status);

/*************************************************************************************************/
/*!
 *  \brief      Send rx enable confirm.
 *
 *  \param      status  Rx enable status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscSendRxEnableCfm(const uint8_t status);

/*************************************************************************************************/
/*!
 *  \brief      Send start confirm.
 *
 *  \param      status  Start status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscSendStartCfm(const uint8_t status);

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for data transmit operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Send data transmit confirm.
 *
 *  \param      msduHandle    Handle of corresponding MCPS-DATA.req
 *  \param      status        Purge status
 *  \param      timestamp     Timestamp of MSDU
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataTxSendCfm(const uint8_t msduHandle, const uint8_t status, const uint32_t timestamp);

/*************************************************************************************************/
/*!
 *  \brief      Send data poll confirm.
 *
 *  \param      status    Poll status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataSendPollCfm(const uint8_t status);

/*************************************************************************************************/
/*!
 *  \brief      Send data poll indication.
 *
 *  \param      srcAddr         Source address
 *  \param      dataFrameSent   Poll status
 *
 *  \return     None.
 *
 *  \note       This is a vendor-specific extension
 */
/*************************************************************************************************/
void Chci154DataSendPollInd(const Mac154Addr_t *srcAddr, const uint8_t dataFrameSent);

/*************************************************************************************************/
/*!
 *  \brief      Send data purge confirm.
 *
 *  \param      msduHandle    Handle of corresponding MCPS-DATA.req
 *  \param      status        Purge status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataSendPurgeCfm(const uint8_t msduHandle, const uint8_t status);

/*************************************************************************************************/
/*!
 *  \brief      Send data indication.
 *
 *  \param      srcAddr           Source address
 *  \param      dstAddr           Destination address
 *  \param      mpduLinkQuality   Link quality
 *  \param      dsn               Data sequence number
 *  \param      timestamp         Timestamp
 *  \param      msduLength        MAC SDU length
 *  \param      pMsdu             Pointer to MAC SDU
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataRxSendInd(const Mac154Addr_t *srcAddr,
                          const Mac154Addr_t *dstAddr,
                          const uint8_t mpduLinkQuality,
                          const uint8_t dsn,
                          const uint32_t timestamp,
                          const uint8_t msduLength,
                          const uint8_t *pMsdu);

/*************************************************************************************************/
/*!
 *  \brief      Send comm status indication.
 *
 *  \param      srcAddr   Source address
 *  \param      dstAddr   Destination address
 *  \param      status    Status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataSendCommStatusInd(const Mac154Addr_t *srcAddr,
                                  const Mac154Addr_t *dstAddr,
                                  const uint8_t status);

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for PIB operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154PibInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for diagnostic operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DiagInit(void);

#ifdef __cplusplus
};
#endif

#endif /* CHCI_154_API_H */
