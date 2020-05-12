/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Trace message implementation.
 *
 *  Copyright (c) 2009-2019 Arm Ltd. All Rights Reserved.
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

#include "wsf_trace.h"

#include <windows.h>
#include <python.h>
#include "util/bstream.h"
#include "hci_defs.h"
#include "l2c_defs.h"
#include "att_defs.h"
#include "smp_defs.h"
#include "wsf_detoken.h"
#include "cfg_stack.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Maximum size of a single trace log message. */
#define WSF_TRACE_BUFFER_SIZE  1024

/*! Number of bytes to wrap when dumping data. */
#define WSF_PDUMP_WRAP_SIZE     16

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Trace type filter (set bit to allow output) */
wsfTraceType_t wsfTraceFilterMask[WSF_TRACE_SUBSYS_MAX] =
{
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_WSF */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_HCI */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_DM */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_L2C */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_ATT */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_SMP */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_SCR */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_APP */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_LL */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_MESH */
  WSF_TRACE_TYPE_ALL, /* WSF_TRACE_SUBSYS_MMDL */
};

/*! Protocol dump type filter (set bit to allow output) */
wsfPDumpType_t wsfPDumpFilterMask = WSF_PDUMP_TYPE_ALL;

/*! Subsystem string table */
const char *wsfTraceSubsys[] =
{
  "wsf",  /* WSF_TRACE_SUBSYS_WSF */
  "hci",  /* WSF_TRACE_SUBSYS_HCI */
  "dm ",  /* WSF_TRACE_SUBSYS_DM */
  "l2c",  /* WSF_TRACE_SUBSYS_L2C */
  "att",  /* WSF_TRACE_SUBSYS_ATT */
  "eatt", /* WSF_TRACE_SUBSYS_EATT */
  "smp",  /* WSF_TRACE_SUBSYS_SMP */
  "scr",  /* WSF_TRACE_SUBSYS_SCR */
  "app",  /* WSF_TRACE_SUBSYS_APP */
  "ll",   /* WSF_TRACE_SUBSYS_LL */
  "mesh", /* WSF_TRACE_SUBSYS_MESH */
  "mmdl", /* WSF_TRACE_SUBSYS_MMDL */
};

/*************************************************************************************************/
/*!
 *  \fn     wsfPDumpHciAcl
 *
 *  \brief  Decode and dump HCI ACL packets
 */
/*************************************************************************************************/
static wsfPDumpHciAcl(char *pTypeStr, char *pTimeBuf, char *pLineBuf, uint16_t length, uint8_t *pBuffer)
{
  char     *pOpStr;
  uint16_t cid;
  uint16_t len;
  uint8_t  op;
  uint16_t hciHandle;
  uint16_t hciLen;

  /* parse ACL packet header */
  BSTREAM_TO_UINT16(hciHandle, pBuffer);
  BSTREAM_TO_UINT16(hciLen, pBuffer);

  /* if continuation packet */
  if ((hciHandle & HCI_PB_FLAG_MASK) == HCI_PB_CONTINUE)
  {
    snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: HCI continuation packet Handle=%u Length=%u\n",
             pTimeBuf, pTypeStr, (hciHandle & HCI_HANDLE_MASK), hciLen);
  }
  else
  {
    /* get L2CAP CID and length */
    BSTREAM_TO_UINT16(len, pBuffer);
    BSTREAM_TO_UINT16(cid, pBuffer);

    if ((cid == L2C_CID_ATT) || 
        ((EATT_CONN_CHAN_MAX != 0) && (cid != L2C_CID_LE_SIGNALING) && (cid != L2C_CID_SMP)))
    {
      if (cid != L2C_CID_ATT)
      {
        pBuffer += 2;
      }

      BSTREAM_TO_UINT8(op, pBuffer);
      switch (op)
      {
        case ATT_PDU_ERR_RSP:              pOpStr = "ATT ERROR Response"; break;
        case ATT_PDU_MTU_REQ:              pOpStr = "ATT EXCHANGE_MTU Request"; break;
        case ATT_PDU_MTU_RSP:              pOpStr = "ATT EXCHANGE_MTU Response"; break;
        case ATT_PDU_FIND_INFO_REQ:        pOpStr = "ATT FIND_INFORMATION Request"; break;
        case ATT_PDU_FIND_INFO_RSP:        pOpStr = "ATT FIND_INFORMATION Response"; break;
        case ATT_PDU_FIND_TYPE_REQ:        pOpStr = "ATT FIND_BY_TYPE_VALUE Request"; break;
        case ATT_PDU_FIND_TYPE_RSP:        pOpStr = "ATT FIND_BY_TYPE_VALUE Response"; break;
        case ATT_PDU_READ_TYPE_REQ:        pOpStr = "ATT READ_BY_TYPE Request"; break;
        case ATT_PDU_READ_TYPE_RSP:        pOpStr = "ATT READ_BY_TYPE Response"; break;
        case ATT_PDU_READ_REQ:             pOpStr = "ATT READ Request"; break;
        case ATT_PDU_READ_RSP:             pOpStr = "ATT READ Response"; break;
        case ATT_PDU_READ_BLOB_REQ:        pOpStr = "ATT READ_BLOB Request"; break;
        case ATT_PDU_READ_BLOB_RSP:        pOpStr = "ATT READ_BLOB Response"; break;
        case ATT_PDU_READ_MULT_REQ:        pOpStr = "ATT READ_MULTIPLE Request"; break;
        case ATT_PDU_READ_MULT_RSP:        pOpStr = "ATT READ_MULTIPLE Response"; break;
        case ATT_PDU_READ_GROUP_TYPE_REQ:  pOpStr = "ATT READ_BY_GROUP_TYPE Request"; break;
        case ATT_PDU_READ_GROUP_TYPE_RSP:  pOpStr = "ATT READ_BY_GROUP_TYPE Response"; break;
        case ATT_PDU_WRITE_REQ:            pOpStr = "ATT WRITE Request"; break;
        case ATT_PDU_WRITE_RSP:            pOpStr = "ATT WRITE Response"; break;
        case ATT_PDU_WRITE_CMD:            pOpStr = "ATT WRITE Command"; break;
        case ATT_PDU_SIGNED_WRITE_CMD:     pOpStr = "ATT SIGNED WRITE Command"; break;
        case ATT_PDU_PREP_WRITE_REQ:       pOpStr = "ATT PREPARE_WRITE Request"; break;
        case ATT_PDU_PREP_WRITE_RSP:       pOpStr = "ATT PREPARE_WRITE Response"; break;
        case ATT_PDU_EXEC_WRITE_REQ:       pOpStr = "ATT EXECUTE_WRITE Request"; break;
        case ATT_PDU_EXEC_WRITE_RSP:       pOpStr = "ATT EXECUTE_WRITE Response"; break;
        case ATT_PDU_VALUE_NTF:            pOpStr = "ATT HANDLE_VALUE Notification"; break;
        case ATT_PDU_VALUE_IND:            pOpStr = "ATT HANDLE_VALUE Indication"; break;
        case ATT_PDU_VALUE_CNF:            pOpStr = "ATT HANDLE_VALUE Confirm"; break;
        case ATT_PDU_READ_MULT_VAR_REQ:    pOpStr = "ATT READ_MULTIPLE_VARIABLE Request"; break;
        case ATT_PDU_READ_MULT_VAR_RSP:    pOpStr = "ATT READ_MULTIPLE_VARIABLE Response"; break;
        case ATT_PDU_MULT_VALUE_NTF:       pOpStr = "ATT MULT_VALUE Notification"; break;
        default:                           pOpStr = "ATT UNKNOWN PDU"; break;
      }

      snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: %s CID=0x%04x (0x%02x)\n",
               pTimeBuf, pTypeStr, pOpStr, cid, op);
    }
    else if (cid == L2C_CID_LE_SIGNALING)
    {
      BSTREAM_TO_UINT8(op, pBuffer);
      switch (op)
      {
        case L2C_SIG_CMD_REJ:              pOpStr = "L2CAP Command Reject"; break;
        case L2C_SIG_DISCONNECT_REQ:       pOpStr = "L2CAP Disconnect Request"; break;
        case L2C_SIG_DISCONNECT_RSP:       pOpStr = "L2CAP Disconnect Response"; break;
        case L2C_SIG_CONN_UPDATE_REQ:      pOpStr = "L2CAP Connection Param Update Request"; break;
        case L2C_SIG_CONN_UPDATE_RSP:      pOpStr = "L2CAP Connection Param Update Response"; break;
        case L2C_SIG_LE_CONNECT_REQ:       pOpStr = "L2CAP LE Connection Request"; break;
        case L2C_SIG_LE_CONNECT_RSP:       pOpStr = "L2CAP LE Connection Response"; break;
        case L2C_SIG_FLOW_CTRL_CREDIT:     pOpStr = "L2CAP LE Flow Control Credit"; break;
        case L2C_SIG_EN_CONNECT_REQ:       pOpStr = "L2CAP LE Enhanced Connection Request"; break;
        case L2C_SIG_EN_CONNECT_RSP:       pOpStr = "L2CAP LE Enhanced Connection Response"; break;
        case L2C_SIG_EN_RECONFIG_REQ:      pOpStr = "L2CAP LE Reconfiguration Request"; break;
        case L2C_SIG_EN_RECONFIG_RSP:      pOpStr = "L2CAP LE Reconfiguration Response"; break;
        default:                           pOpStr = "L2CAP UNKNOWN PDU"; break;
      }

      snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: %s (0x%02x)\n",
               pTimeBuf, pTypeStr, pOpStr, op);
    }
    else if (cid == L2C_CID_SMP)
    {
      BSTREAM_TO_UINT8(op, pBuffer);
      switch (op)
      {
        case SMP_CMD_PAIR_REQ:             pOpStr = "SMP Pairing Request"; break;
        case SMP_CMD_PAIR_RSP:             pOpStr = "SMP Pairing Response"; break;
        case SMP_CMD_PAIR_CNF:             pOpStr = "SMP Pairing Confirm"; break;
        case SMP_CMD_PAIR_RAND:            pOpStr = "SMP Pairing Random"; break;
        case SMP_CMD_PAIR_FAIL:            pOpStr = "SMP Pairing Failed"; break;
        case SMP_CMD_ENC_INFO:             pOpStr = "SMP Encryption Information"; break;
        case SMP_CMD_MASTER_ID:            pOpStr = "SMP Master Identification"; break;
        case SMP_CMD_ID_INFO:              pOpStr = "SMP Identity Information"; break;
        case SMP_CMD_ID_ADDR_INFO:         pOpStr = "SMP Identity Address Information"; break;
        case SMP_CMD_SIGN_INFO:            pOpStr = "SMP Signing Information"; break;
        case SMP_CMD_SECURITY_REQ:         pOpStr = "SMP Security Request"; break;
        case SMP_CMD_PUBLIC_KEY:           pOpStr = "SMP Public Key"; break;
        case SMP_CMD_DHKEY_CHECK:          pOpStr = "SMP DH Key Check"; break;
        case SMP_CMD_KEYPRESS:             pOpStr = "SMP User Key Press"; break;
        default:                           pOpStr = "SMP UNKNOWN PDU"; break;
      }

      snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: %s (0x%02x)\n",
               pTimeBuf, pTypeStr, pOpStr, op);
    }
    else
    {
      snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: L2CAP data packet CID=0x%04x Length=0x%04x\n",
               pTimeBuf, pTypeStr, cid, len);
    }
  }

  PySys_WriteStdout(pLineBuf);
}

/*************************************************************************************************/
/*!
 *  \fn     wsfPDumpHciIso
 *
 *  \brief  Decode and dump HCI ISO packets
 */
/*************************************************************************************************/
static wsfPDumpHciIso(char *pTypeStr, char *pTimeBuf, char *pLineBuf, uint16_t length, uint8_t *pBuffer)
{
  uint16_t hciHandle;
  uint16_t hciLen;

  /* parse ISO packet header */
  BSTREAM_TO_UINT16(hciHandle, pBuffer);
  BSTREAM_TO_UINT16(hciLen, pBuffer);

  /* if continuation packet */
  if ((hciHandle & HCI_PB_FLAG_MASK) == HCI_PB_CONTINUE)
  {
    snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: HCI continuation packet Handle=%u Length=%u\n",
             pTimeBuf, pTypeStr, (hciHandle & HCI_HANDLE_MASK), hciLen);
  }
  else
  {
    snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: HCI packet Handle=%u Length=%u\n",
             pTimeBuf, pTypeStr, (hciHandle & HCI_HANDLE_MASK), hciLen);
  }

  PySys_WriteStdout(pLineBuf);
}

/*************************************************************************************************/
/*!
 *  \fn     wsfPDumpHciEvt
 *
 *  \brief  Decode and dump HCI events
 */
/*************************************************************************************************/
static void wsfPDumpHciEvt(char *pTypeStr, char *pTimeBuf, char *pLineBuf, uint16_t length, uint8_t *pBuffer)
{
  char     *pEvtStr;
  uint8_t  evt = *pBuffer;
  uint16_t opcode = HCI_OPCODE_NOP;
  uint8_t  status;

  switch (evt)
  {
    case HCI_DISCONNECT_CMPL_EVT:           pEvtStr = "DISCONNECT_CMPL"; break;
    case HCI_ENC_CHANGE_EVT:                pEvtStr = "ENC_CHANGE"; break;
    case HCI_READ_REMOTE_VER_INFO_CMPL_EVT: pEvtStr = "READ_REMOTE_VER_INFO_CMPL"; break;
    case HCI_CMD_CMPL_EVT:                  pEvtStr = "CMD_CMPL";
      status = *(pBuffer + 5);
      BYTES_TO_UINT16(opcode, (pBuffer + 3));
      break;
    case HCI_CMD_STATUS_EVT:                pEvtStr = "CMD_STATUS";
      status = *(pBuffer + 2);
      BYTES_TO_UINT16(opcode, (pBuffer + 4));
      break;
    case HCI_HW_ERROR_EVT:                  pEvtStr = "HW_ERROR"; break;
    case HCI_NUM_CMPL_PKTS_EVT:             pEvtStr = "NUM_CMPL_PKTS"; break;
    case HCI_DATA_BUF_OVERFLOW_EVT:         pEvtStr = "DATA_BUF_OVERFLOW"; break;
    case HCI_ENC_KEY_REFRESH_CMPL_EVT:      pEvtStr = "ENC_KEY_REFRESH_CMPL"; break;
    case HCI_AUTH_PAYLOAD_TIMEOUT_EVT:      pEvtStr = "AUTH_PAYLOAD_TIMEOUT"; break;
    case HCI_VENDOR_SPEC_EVT:               pEvtStr = "VENDOR_SPEC"; break;
    case HCI_LE_META_EVT:
      switch (*(pBuffer + 2))
      {
        case HCI_LE_CONN_CMPL_EVT:                    pEvtStr = "LE_CONN_CMPL"; break;
        case HCI_LE_ADV_REPORT_EVT:                   pEvtStr = "LE_ADV_REPORT"; break;
        case HCI_LE_CONN_UPDATE_CMPL_EVT:             pEvtStr = "LE_CONN_UPDATE_CMPL"; break;
        case HCI_LE_READ_REMOTE_FEAT_CMPL_EVT:        pEvtStr = "LE_READ_REMOTE_FEAT_CMPL"; break;
        case HCI_LE_LTK_REQ_EVT:                      pEvtStr = "LE_LTK_REQ"; break;
        case HCI_LE_REM_CONN_PARAM_REQ_EVT:           pEvtStr = "LE_REM_CONN_PARAM_REQ"; break;
        case HCI_LE_DATA_LEN_CHANGE_EVT:              pEvtStr = "LE_DATA_LEN_CHANGE"; break;
        case HCI_LE_READ_LOCAL_P256_PUB_KEY_CMPL_EVT: pEvtStr = "LE_READ_LOCAL_P256_PUB_KEY_CMPL"; break;
        case HCI_LE_GENERATE_DHKEY_CMPL_EVT:          pEvtStr = "LE_GENERATE_DHKEY_CMPL"; break;
        case HCI_LE_ENHANCED_CONN_CMPL_EVT:           pEvtStr = "LE_ENHANCED_CONN_CMPL"; break;
        case HCI_LE_DIRECT_ADV_REPORT_EVT:            pEvtStr = "LE_DIRECT_ADV_REPORT"; break;
        case HCI_LE_PHY_UPDATE_CMPL_EVT:              pEvtStr = "LE_PHY_UPDATE_CMPL"; break;
        case HCI_LE_EXT_ADV_REPORT_EVT:               pEvtStr = "LE_EXT_ADV_REPORT_EVT"; break;
        case HCI_LE_PER_ADV_SYNC_EST_EVT:             pEvtStr = "LE_PER_ADV_SYNC_EST_EVT"; break;
        case HCI_LE_PER_ADV_REPORT_EVT:               pEvtStr = "LE_PER_ADV_REPORT_EVT"; break;
        case HCI_LE_PER_ADV_SYNC_LOST_EVT:            pEvtStr = "LE_PER_ADV_SYNC_LOST_EVT"; break;
        case HCI_LE_SCAN_TIMEOUT_EVT:                 pEvtStr = "LE_SCAN_TIMEOUT_EVT"; break;
        case HCI_LE_ADV_SET_TERM_EVT:                 pEvtStr = "LE_ADV_SET_TERM_EVT"; break;
        case HCI_LE_SCAN_REQ_RCVD_EVT:                pEvtStr = "LE_SCAN_REQ_RCVD_EVT"; break;
        case HCI_LE_CH_SEL_ALGO_EVT:                  pEvtStr = "LE_CH_SEL_ALGO_EVT"; break;
        case HCI_LE_CONNLESS_IQ_REPORT_EVT:           pEvtStr = "LE_CONNLESS_IQ_REPORT_EVT"; break;
        case HCI_LE_CONN_IQ_REPORT_EVT:               pEvtStr = "LE_CONN_IQ_REPORT_EVT"; break;
        case HCI_LE_CTE_REQ_FAILED_EVT:               pEvtStr = "LE_CTE_REQ_FAILED_EVT"; break;
        case HCI_LE_PER_SYNC_TRSF_RCVD_EVT:           pEvtStr = "LE_PER_SYNC_TRSF_RCVD_EVT"; break;
        case HCI_LE_CIS_EST_EVT:                      pEvtStr = "LE_CIS_EST_EVT"; break;
        case HCI_LE_CIS_REQ_EVT:                      pEvtStr = "LE_CIS_REQ_EVT"; break;
        case HCI_LE_CREATE_BIG_CMPL_EVT:              pEvtStr = "LE_CREATE_BIG_CMPL_EVT"; break;
        case HCI_LE_TERMINATE_BIG_CMPL_EVT:           pEvtStr = "LE_TERMINATE_BIG_CMPL_EVT"; break;
        case HCI_LE_BIG_SYNC_EST_EVT:                 pEvtStr = "LE_BIG_SYNC_EST_EVT"; break;
        case HCI_LE_BIG_SYNC_LOST_EVT:                pEvtStr = "LE_BIG_SYNC_LOST_EVT"; break;
        case HCI_LE_REQ_PEER_SCA_CMPLT_EVT:           pEvtStr = "LE_REQ_PEER_SCA_CMPLT_EVT"; break;
        case HCI_LE_PATH_LOSS_REPORT_EVT:             pEvtStr = "LE_PATH_LOSS_REPORT_EVT"; break;
        case HCI_LE_POWER_REPORT_EVT:                 pEvtStr = "LE_POWER_REPORT_EVT"; break;
        case HCI_LE_BIG_INFO_ADV_REPORT_EVT:          pEvtStr = "LE_BIG_INFO_ADV_REPORT_EVT"; break;
        default:                                      pEvtStr = "UNKNOWN"; break;
      }
      break;
    default:                                pEvtStr = "UNKNOWN"; break;
  }

  if (evt == HCI_CMD_CMPL_EVT || evt == HCI_CMD_STATUS_EVT)
  {
    snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: %s Event (0x%02x) Opcode=0x%04x Status=0x%02x\n",
             pTimeBuf, pTypeStr, pEvtStr, evt, opcode, status);
  }
  else if (evt == HCI_LE_META_EVT)
  {
    snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: %s Event (0x%02x) Meta=0x%02x\n",
             pTimeBuf, pTypeStr, pEvtStr, evt, *(pBuffer + 2));
  }
  else
  {
    {
      snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: %s Event (0x%02x)\n",
               pTimeBuf, pTypeStr, pEvtStr, evt);
    }
  }

  PySys_WriteStdout(pLineBuf);
}


/*************************************************************************************************/
/*!
 *  \fn     wsfPDumpHciCmd
 *
 *  \brief  Decode and dump HCI commands
 */
/*************************************************************************************************/
static wsfPDumpHciCmd(char *pTypeStr, char *pTimeBuf, char *pLineBuf, uint16_t length, uint8_t *pBuffer)
{
  uint16_t opcode;
  char     *pOpStr;

  BYTES_TO_UINT16(opcode, pBuffer);

  switch (opcode)
  {
    case HCI_OPCODE_NOP:                           pOpStr = "NOP"; break;
    case HCI_OPCODE_DISCONNECT:                    pOpStr = "DISCONNECT"; break;
    case HCI_OPCODE_READ_REMOTE_VER_INFO:          pOpStr = "READ_REMOTE_VER_INFO"; break;
    case HCI_OPCODE_SET_EVENT_MASK:                pOpStr = "SET_EVENT_MASK"; break;
    case HCI_OPCODE_RESET:                         pOpStr = "RESET"; break;
    case HCI_OPCODE_READ_TX_PWR_LVL:               pOpStr = "READ_TX_PWR_LVL"; break;
    case HCI_OPCODE_SET_EVENT_MASK_PAGE2:          pOpStr = "SET_EVENT_MASK_PAGE2"; break;
    case HCI_OPCODE_READ_AUTH_PAYLOAD_TO:          pOpStr = "READ_AUTH_PAYLOAD_TO"; break;
    case HCI_OPCODE_WRITE_AUTH_PAYLOAD_TO:         pOpStr = "WRITE_AUTH_PAYLOAD_TO"; break;
    case HCI_OPCODE_CONFIG_DATA_PATH:              pOpStr = "CONFIG_DATA_PATH"; break;
    case HCI_OPCODE_READ_LOCAL_VER_INFO:           pOpStr = "READ_LOCAL_VER_INFO"; break;
    case HCI_OPCODE_READ_LOCAL_SUP_CMDS:           pOpStr = "READ_LOCAL_SUP_CMDS"; break;
    case HCI_OPCODE_READ_LOCAL_SUP_FEAT:           pOpStr = "READ_LOCAL_SUP_FEAT"; break;
    case HCI_OPCODE_READ_BUF_SIZE:                 pOpStr = "READ_BUF_SIZE"; break;
    case HCI_OPCODE_READ_BD_ADDR:                  pOpStr = "READ_BD_ADDR"; break;
    case HCI_OPCODE_READ_LOCAL_SUP_CODECS:         pOpStr = "READ_LOCAL_SUP_CODECS"; break;
    case HCI_OPCODE_READ_LOCAL_SUP_CODEC_CAP:      pOpStr = "READ_LOCAL_SUP_CODEC_CAP"; break;
    case HCI_OPCODE_READ_LOCAL_SUP_CONTROLLER_DLY: pOpStr = "READ_LOCAL_SUP_CONTROLLER_DLY"; break;
    case HCI_OPCODE_READ_RSSI:                     pOpStr = "READ_RSSI"; break;
    case HCI_OPCODE_LE_SET_EVENT_MASK:             pOpStr = "LE_SET_EVENT_MASK"; break;
    case HCI_OPCODE_LE_READ_BUF_SIZE:              pOpStr = "LE_READ_BUF_SIZE"; break;
    case HCI_OPCODE_LE_READ_LOCAL_SUP_FEAT:        pOpStr = "LE_READ_LOCAL_SUP_FEAT"; break;
    case HCI_OPCODE_LE_SET_RAND_ADDR:              pOpStr = "LE_SET_RAND_ADDR"; break;
    case HCI_OPCODE_LE_SET_ADV_PARAM:              pOpStr = "LE_SET_ADV_PARAM"; break;
    case HCI_OPCODE_LE_READ_ADV_TX_POWER:          pOpStr = "LE_READ_ADV_TX_POWER"; break;
    case HCI_OPCODE_LE_SET_ADV_DATA:               pOpStr = "LE_SET_ADV_DATA"; break;
    case HCI_OPCODE_LE_SET_SCAN_RESP_DATA:         pOpStr = "LE_SET_SCAN_RESP_DATA"; break;
    case HCI_OPCODE_LE_SET_ADV_ENABLE:             pOpStr = "LE_SET_ADV_ENABLE"; break;
    case HCI_OPCODE_LE_SET_SCAN_PARAM:             pOpStr = "LE_SET_SCAN_PARAM"; break;
    case HCI_OPCODE_LE_SET_SCAN_ENABLE:            pOpStr = "LE_SET_SCAN_ENABLE"; break;
    case HCI_OPCODE_LE_CREATE_CONN:                pOpStr = "LE_CREATE_CONN"; break;
    case HCI_OPCODE_LE_CREATE_CONN_CANCEL:         pOpStr = "LE_CREATE_CONN_CANCEL"; break;
    case HCI_OPCODE_LE_READ_WHITE_LIST_SIZE:       pOpStr = "LE_READ_WHITE_LIST_SIZE"; break;
    case HCI_OPCODE_LE_CLEAR_WHITE_LIST:           pOpStr = "LE_CLEAR_WHITE_LIST"; break;
    case HCI_OPCODE_LE_ADD_DEV_WHITE_LIST:         pOpStr = "LE_ADD_DEV_WHITE_LIST"; break;
    case HCI_OPCODE_LE_REMOVE_DEV_WHITE_LIST:      pOpStr = "LE_REMOVE_DEV_WHITE_LIST"; break;
    case HCI_OPCODE_LE_CONN_UPDATE:                pOpStr = "LE_CONN_UPDATE"; break;
    case HCI_OPCODE_LE_SET_HOST_CHAN_CLASS:        pOpStr = "LE_SET_HOST_CHAN_CLASS"; break;
    case HCI_OPCODE_LE_READ_CHAN_MAP:              pOpStr = "LE_READ_CHAN_MAP"; break;
    case HCI_OPCODE_LE_READ_REMOTE_FEAT:           pOpStr = "LE_READ_REMOTE_FEAT"; break;
    case HCI_OPCODE_LE_ENCRYPT:                    pOpStr = "LE_ENCRYPT"; break;
    case HCI_OPCODE_LE_RAND:                       pOpStr = "LE_RAND"; break;
    case HCI_OPCODE_LE_START_ENCRYPTION:           pOpStr = "LE_START_ENCRYPTION"; break;
    case HCI_OPCODE_LE_LTK_REQ_REPL:               pOpStr = "LE_LTK_REQ_REPL"; break;
    case HCI_OPCODE_LE_LTK_REQ_NEG_REPL:           pOpStr = "LE_LTK_REQ_NEG_REPL"; break;
    case HCI_OPCODE_LE_READ_SUP_STATES:            pOpStr = "LE_READ_SUP_STATES"; break;
    case HCI_OPCODE_LE_RECEIVER_TEST:              pOpStr = "LE_RECEIVER_TEST"; break;
    case HCI_OPCODE_LE_TRANSMITTER_TEST:           pOpStr = "LE_TRANSMITTER_TEST"; break;
    case HCI_OPCODE_LE_TEST_END:                   pOpStr = "LE_TEST_END"; break;
    case HCI_OPCODE_LE_REM_CONN_PARAM_REP:         pOpStr = "LE_REM_CONN_PARAM_REP"; break;
    case HCI_OPCODE_LE_REM_CONN_PARAM_NEG_REP:     pOpStr = "LE_REM_CONN_PARAM_NEG_REP"; break;
    case HCI_OPCODE_LE_SET_DATA_LEN:               pOpStr = "LE_SET_DATA_LEN"; break;
    case HCI_OPCODE_LE_READ_DEF_DATA_LEN:          pOpStr = "LE_READ_DEF_DATA_LEN"; break;
    case HCI_OPCODE_LE_WRITE_DEF_DATA_LEN:         pOpStr = "LE_WRITE_DEF_DATA_LEN"; break;
    case HCI_OPCODE_LE_READ_LOCAL_P256_PUB_KEY:    pOpStr = "LE_READ_LOCAL_P256_PUB_KEY"; break;
    case HCI_OPCODE_LE_GENERATE_DHKEY:             pOpStr = "LE_GENERATE_DHKEY"; break;
    case HCI_OPCODE_LE_READ_MAX_DATA_LEN:          pOpStr = "LE_READ_MAX_DATA_LEN"; break;
    case HCI_OPCODE_LE_ADD_DEV_RES_LIST:           pOpStr = "LE_ADD_DEV_RES_LIST"; break;
    case HCI_OPCODE_LE_REMOVE_DEV_RES_LIST:        pOpStr = "LE_REMOVE_DEV_RES_LIST"; break;
    case HCI_OPCODE_LE_CLEAR_RES_LIST:             pOpStr = "LE_CLEAR_RES_LIST"; break;
    case HCI_OPCODE_LE_READ_RES_LIST_SIZE:         pOpStr = "LE_READ_RES_LIST_SIZE"; break;
    case HCI_OPCODE_LE_READ_PEER_RES_ADDR:         pOpStr = "LE_READ_PEER_RES_ADDR"; break;
    case HCI_OPCODE_LE_READ_LOCAL_RES_ADDR:        pOpStr = "LE_READ_LOCAL_RES_ADDR"; break;
    case HCI_OPCODE_LE_SET_ADDR_RES_ENABLE:        pOpStr = "LE_SET_ADDR_RES_ENABLE"; break;
    case HCI_OPCODE_LE_SET_RES_PRIV_ADDR_TO:       pOpStr = "LE_SET_RES_PRIV_ADDR_TO"; break;
    case HCI_OPCODE_LE_READ_PHY:                   pOpStr = "LE_READ_PHY"; break;
    case HCI_OPCODE_LE_SET_DEF_PHY:                pOpStr = "LE_SET_DEF_PHY"; break;
    case HCI_OPCODE_LE_SET_PHY:                    pOpStr = "LE_SET_PHY"; break;
    case HCI_OPCODE_LE_ENHANCED_RECEIVER_TEST:     pOpStr = "LE_ENHANCED_RECEIVER_TEST"; break;
    case HCI_OPCODE_LE_ENHANCED_TRANSMITTER_TEST:  pOpStr = "LE_ENHANCED_TRANSMITTER_TEST"; break;
    case HCI_OPCODE_LE_SET_ADV_SET_RAND_ADDR:      pOpStr = "LE_SET_ADV_SET_RAND_ADDR"; break;
    case HCI_OPCODE_LE_SET_EXT_ADV_PARAM:          pOpStr = "LE_SET_EXT_ADV_PARAM"; break;
    case HCI_OPCODE_LE_SET_EXT_ADV_DATA:           pOpStr = "LE_SET_EXT_ADV_DATA"; break;
    case HCI_OPCODE_LE_SET_EXT_SCAN_RESP_DATA:     pOpStr = "LE_SET_EXT_SCAN_RESP_DATA"; break;
    case HCI_OPCODE_LE_SET_EXT_ADV_ENABLE:         pOpStr = "LE_SET_EXT_ADV_ENABLE"; break;
    case HCI_OPCODE_LE_READ_MAX_ADV_DATA_LEN:      pOpStr = "LE_READ_MAX_ADV_DATA_LEN"; break;
    case HCI_OPCODE_LE_READ_NUM_SUP_ADV_SETS:      pOpStr = "LE_READ_NUM_SUP_ADV_SETS"; break;
    case HCI_OPCODE_LE_REMOVE_ADV_SET:             pOpStr = "LE_REMOVE_ADV_SET"; break;
    case HCI_OPCODE_LE_CLEAR_ADV_SETS:             pOpStr = "LE_CLEAR_ADV_SETS"; break;
    case HCI_OPCODE_LE_SET_PER_ADV_PARAM:          pOpStr = "LE_SET_PER_ADV_PARAM"; break;
    case HCI_OPCODE_LE_SET_PER_ADV_DATA:           pOpStr = "LE_SET_PER_ADV_DATA"; break;
    case HCI_OPCODE_LE_SET_PER_ADV_ENABLE:         pOpStr = "LE_SET_PER_ADV_ENABLE"; break;
    case HCI_OPCODE_LE_SET_EXT_SCAN_PARAM:         pOpStr = "LE_SET_EXT_SCAN_PARAM"; break;
    case HCI_OPCODE_LE_SET_EXT_SCAN_ENABLE:        pOpStr = "LE_SET_EXT_SCAN_ENABLE"; break;
    case HCI_OPCODE_LE_EXT_CREATE_CONN:            pOpStr = "LE_EXT_CREATE_CONN"; break;
    case HCI_OPCODE_LE_PER_ADV_CREATE_SYNC:        pOpStr = "LE_PER_ADV_CREATE_SYNC"; break;
    case HCI_OPCODE_LE_PER_ADV_CREATE_SYNC_CANCEL: pOpStr = "LE_PER_ADV_CREATE_SYNC_CANCEL"; break;
    case HCI_OPCODE_LE_PER_ADV_TERMINATE_SYNC:     pOpStr = "LE_PER_ADV_TERMINATE_SYNC"; break;
    case HCI_OPCODE_LE_ADD_DEV_PER_ADV_LIST:       pOpStr = "LE_ADD_DEV_PER_ADV_LIST"; break;
    case HCI_OPCODE_LE_REMOVE_DEV_PER_ADV_LIST:    pOpStr = "LE_REMOVE_DEV_PER_ADV_LIST"; break;
    case HCI_OPCODE_LE_CLEAR_PER_ADV_LIST:         pOpStr = "LE_CLEAR_PER_ADV_LIST"; break;
    case HCI_OPCODE_LE_READ_PER_ADV_LIST_SIZE:     pOpStr = "LE_READ_PER_ADV_LIST_SIZE"; break;
    case HCI_OPCODE_LE_READ_TX_POWER:              pOpStr = "LE_READ_TX_POWER"; break;
    case HCI_OPCODE_LE_WRITE_RF_PATH_COMP:         pOpStr = "LE_WRITE_RF_PATH_COMP"; break;
    case HCI_OPCODE_LE_READ_RF_PATH_COMP:          pOpStr = "LE_READ_RF_PATH_COMP"; break;
    case HCI_OPCODE_LE_SET_PRIVACY_MODE:           pOpStr = "LE_SET_PRIVACY_MODE"; break;
    case HCI_OPCODE_LE_VS_ENABLE_READ_FEAT_ON_CONN:pOpStr = "LE_VS_ENABLE_READ_FEAT_ON_CONN"; break;
    case HCI_OPCODE_LE_SET_CONN_CTE_RX_PARAMS:     pOpStr = "LE_SET_CONN_CTE_RX_PARAMS"; break;
    case HCI_OPCODE_LE_SET_CONN_CTE_TX_PARAMS:     pOpStr = "LE_SET_CONN_CTE_TX_PARAMS"; break;
    case HCI_OPCODE_LE_CONN_CTE_REQ_ENABLE:        pOpStr = "LE_CONN_CTE_REQ_ENABLE"; break;
    case HCI_OPCODE_LE_CONN_CTE_RSP_ENABLE:        pOpStr = "LE_CONN_CTE_RSP_ENABLE"; break;
    case HCI_OPCODE_LE_READ_ANTENNA_INFO:          pOpStr = "LE_READ_ANTENNA_INFO"; break;
    case HCI_OPCODE_LE_SET_PER_ADV_RCV_ENABLE:     pOpStr = "LE_SET_PER_ADV_RCV_ENABLE"; break;
    case HCI_OPCODE_LE_PER_ADV_SYNC_TRANSFER:      pOpStr = "LE_PER_ADV_SYNC_TRANSFER"; break;
    case HCI_OPCODE_LE_PER_ADV_SET_INFO_TRANSFER:  pOpStr = "LE_PER_ADV_SET_INFO_TRANSFER"; break;
    case HCI_OPCODE_LE_SET_PAST_PARAM:             pOpStr = "LE_SET_PAST_PARAM"; break;
    case HCI_OPCODE_LE_SET_DEFAULT_PAST_PARAM:     pOpStr = "LE_SET_DEFAULT_PAST_PARAM"; break;
    case HCI_OPCODE_LE_GENERATE_DHKEY_V2:          pOpStr = "LE_GENERATE_DHKEY_V2"; break;
    case HCI_OPCODE_LE_MODIFY_SLEEP_CLK_ACC:       pOpStr = "LE_MODIFY_SLEEP_CLK_ACC"; break;
    case HCI_OPCODE_LE_READ_BUF_SIZE_V2:           pOpStr = "LE_READ_BUF_SIZE_V2"; break;
    case HCI_OPCODE_LE_READ_ISO_TX_SYNC:           pOpStr = "LE_READ_ISO_TX_SYNC"; break;
    case HCI_OPCODE_LE_SET_CIG_PARAMS:             pOpStr = "LE_SET_CIG_PARAMS"; break;
    case HCI_OPCODE_LE_SET_CIG_PARAMS_TEST:        pOpStr = "LE_SET_CIG_PARAMS_TEST"; break;
    case HCI_OPCODE_LE_CREATE_CIS:                 pOpStr = "LE_CREATE_CIS"; break;
    case HCI_OPCODE_LE_REMOVE_CIG:                 pOpStr = "LE_REMOVE_CIG"; break;
    case HCI_OPCODE_LE_ACCEPT_CIS_REQ:             pOpStr = "LE_ACCEPT_CIS_REQ"; break;
    case HCI_OPCODE_LE_REJECT_CIS_REQ:             pOpStr = "LE_REJECT_CIS_REQ"; break;
    case HCI_OPCODE_LE_CREATE_BIG:                 pOpStr = "LE_CREATE_BIG"; break;
    case HCI_OPCODE_LE_CREATE_BIG_TEST:            pOpStr = "LE_CREATE_BIG_TEST"; break;
    case HCI_OPCODE_LE_TERMINATE_BIG:              pOpStr = "LE_TERMINATE_BIG"; break;
    case HCI_OPCODE_LE_BIG_CREATE_SYNC:            pOpStr = "LE_BIG_CREATE_SYNC"; break;
    case HCI_OPCODE_LE_BIG_TERMINATE_SYNC:         pOpStr = "LE_BIG_TERMINATE_SYNC"; break;
    case HCI_OPCODE_LE_REQUEST_PEER_SCA:           pOpStr = "LE_REQUEST_PEER_SCA"; break;
    case HCI_OPCODE_LE_SETUP_ISO_DATA_PATH:        pOpStr = "LE_SETUP_ISO_DATA_PATH"; break;
    case HCI_OPCODE_LE_REMOVE_ISO_DATA_PATH:       pOpStr = "LE_REMOVE_ISO_DATA_PATH"; break;
    case HCI_OPCODE_LE_ISO_TX_TEST:                pOpStr = "LE_ISO_TX_TEST"; break;
    case HCI_OPCODE_LE_ISO_RX_TEST:                pOpStr = "LE_ISO_RX_TEST"; break;
    case HCI_OPCODE_LE_ISO_READ_TEST_COUNTERS:     pOpStr = "LE_ISO_READ_TEST_COUNTERS"; break;
    case HCI_OPCODE_LE_ISO_TEST_END:               pOpStr = "LE_ISO_TEST_END"; break;
    case HCI_OPCODE_LE_SET_HOST_FEATURE:           pOpStr = "LE_SET_HOST_FEATURE"; break;
    case HCI_OPCODE_LE_READ_ISO_LINK_QUAL:         pOpStr = "_LE_READ_ISO_LINK_QUAL"; break;
    case HCI_OPCODE_LE_READ_ENHANCED_TX_POWER:     pOpStr = "LE_READ_ENHANCED_TX_POWER"; break;
    case HCI_OPCODE_LE_READ_REMOTE_TX_POWER:       pOpStr = "LE_READ_REMOTE_TX_POWER"; break;
    case HCI_OPCODE_LE_SET_PATH_LOSS_REPORTING_PARAMS: pOpStr = "LE_SET_PATH_LOSS_REPORTING_PARAMS"; break;
    case HCI_OPCODE_LE_SET_PATH_LOSS_REPORTING_ENABLE: pOpStr = "LE_SET_PATH_LOSS_REPORTING_ENABLE"; break;
    case HCI_OPCODE_LE_SET_TX_POWER_REPORT_ENABLE: pOpStr = "LE_SET_TX_POWER_REPORT_ENABLE"; break;
    default:                                       pOpStr = "UNKNOWN"; break;
  }

  snprintf(pLineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s: %s Command (0x%04x)\n", pTimeBuf, pTypeStr, pOpStr, opcode);
  PySys_WriteStdout(pLineBuf);
}

/*************************************************************************************************/
/*!
 *  \fn     WsfSetTraceTypeFilter
 *
 *  \brief  Set new trace type filter (set bit to output).
 *
 *  \param  ttypeMask   Trace type to enable.
 */
/*************************************************************************************************/
void WsfSetTraceTypeFilter(wsfTraceSubsys_t subsys, wsfTraceType_t ttypeMask)
{
  wsfTraceFilterMask[subsys] = ttypeMask;
}

/*************************************************************************************************/
/*!
 *  \fn     WsfSetPDumpTypeFilter
 *
 *  \brief  Set new protocol dump type filter (set bit to output).
 *
 *  \param  ttypeMask   Protocol dump type to enable.
 */
/*************************************************************************************************/
void WsfSetPDumpTypeFilter(wsfPDumpType_t ttypeMask)
{
  wsfPDumpFilterMask = ttypeMask;
}

/*************************************************************************************************/
/*!
 *  \fn     WsfTrace
 *
 *  \brief  Print a trace message.
 *
 *  \param  pStr      Message format string
 *  \param  ...       Additional arguments, PySys_WriteStdout-style
 */
/*************************************************************************************************/
void WsfTrace(wsfTraceType_t ttype, wsfTraceSubsys_t subsys, const char *pMsg, ...)
{
  va_list           argv;
  PyGILState_STATE  gstate;
  SYSTEMTIME        curTime;
  char              *pTypeStr;
  char              traceBuf[WSF_TRACE_BUFFER_SIZE + 1];
  char              timeBuf[] = "00:00:00.000";

  GetLocalTime(&curTime);
  GetTimeFormat(LANG_USER_DEFAULT, 0, &curTime, "HH:mm:ss",
    timeBuf, sizeof(timeBuf));
  sprintf(timeBuf + strlen(timeBuf), ".%03u", curTime.wMilliseconds);

  if ((ttype & wsfTraceFilterMask[subsys]) == 0)
  {
    return;
  }

  switch(ttype)
  {
    case WSF_TRACE_TYPE_ERR:
      pTypeStr = "   ERROR";
      break;

    case WSF_TRACE_TYPE_WARN:
      pTypeStr = " WARNING";
      break;

    case WSF_TRACE_TYPE_INFO:
      pTypeStr = "    INFO";
      break;

    case WSF_TRACE_TYPE_MSG:
      pTypeStr = "     MSG";
      break;

    case WSF_TRACE_TYPE_ALLOC:
      pTypeStr = "   ALLOC";
      break;

    case WSF_TRACE_TYPE_FREE:
      pTypeStr = "    FREE";
      break;

    default:
      pTypeStr = "  ??????";
      break;
  };

  snprintf(traceBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %-3s%s: ", timeBuf,
    wsfTraceSubsys[subsys], pTypeStr);

  va_start(argv, pMsg);
  vsnprintf(traceBuf + strlen(traceBuf), WSF_TRACE_BUFFER_SIZE - strlen(traceBuf), pMsg, argv);
  strncat(traceBuf, "\n", WSF_TRACE_BUFFER_SIZE - strlen(traceBuf));
  va_end(argv);

  gstate = PyGILState_Ensure();
  PySys_WriteStdout(traceBuf);
  PyGILState_Release(gstate);
}


/*************************************************************************************************/
/*!
 *  \fn     WsfPDump
 *
 *  \brief  Print a protocol dump message.
 *
 *  \param  pdType    Protocol dump type
 *  \param  length    Length of \a pBuffer
 *  \param  pBuffer   Data buffer to dump
 */
/*************************************************************************************************/
void WsfPDump(wsfPDumpType_t pdType, uint16_t length, uint8_t *pBuffer)
{
  PyGILState_STATE  gstate;
  SYSTEMTIME        curTime;
  char              *pTypeStr;
  char              lineBuf[WSF_TRACE_BUFFER_SIZE + 1];
  char              timeBuf[] = "00:00:00.000";
  uint8_t           offset    = 0;
  uint16_t          vsEvent;

  GetLocalTime(&curTime);
  GetTimeFormat(LANG_USER_DEFAULT, 0, &curTime, "HH:mm:ss",
    timeBuf, sizeof(timeBuf));
  sprintf(timeBuf + strlen(timeBuf), ".%03u", curTime.wMilliseconds);

  if ((pdType & wsfPDumpFilterMask) == 0)
  {
    return;
  }

  gstate = PyGILState_Ensure();

  switch(pdType)
  {
    case WSF_PDUMP_TYPE_HCI_CMD:
      pTypeStr = "    HCI-CMD";
      wsfPDumpHciCmd(pTypeStr, timeBuf, lineBuf, length, pBuffer);
      break;

    case WSF_PDUMP_TYPE_HCI_EVT:
      pTypeStr = "    HCI-EVT";
      wsfPDumpHciEvt(pTypeStr, timeBuf, lineBuf, length, pBuffer);
      break;

    case WSF_PDUMP_TYPE_HCI_TX_ACL:
      pTypeStr = " HCI-TX-ACL";
      wsfPDumpHciAcl(pTypeStr, timeBuf, lineBuf, length, pBuffer);
      break;

    case WSF_PDUMP_TYPE_HCI_RX_ACL:
      pTypeStr = " HCI-RX-ACL";
      wsfPDumpHciAcl(pTypeStr, timeBuf, lineBuf, length, pBuffer);
      break;

    case WSF_PDUMP_TYPE_HCI_TX_ISO:
      pTypeStr = " HCI-TX-ISO";
      wsfPDumpHciIso(pTypeStr, timeBuf, lineBuf, length, pBuffer);
      break;

    case WSF_PDUMP_TYPE_HCI_RX_ISO:
      pTypeStr = " HCI-RX-ISO";
      wsfPDumpHciIso(pTypeStr, timeBuf, lineBuf, length, pBuffer);
      break;

    default:
      pTypeStr = "    ???-???";
      break;
  };

  BYTES_TO_UINT16(vsEvent, pBuffer + 2);

  if (vsEvent != WSF_DETOKEN_VS_EVT_TOKEN)
  {
    while (length-- > 0)
    {
      if (offset++ == 0)
      {
        /* output address (line header) */
        snprintf(lineBuf, WSF_TRACE_BUFFER_SIZE, "[%s] %s:   ", timeBuf, pTypeStr);
      }

      /* output byte */
      snprintf(lineBuf + strlen(lineBuf), WSF_TRACE_BUFFER_SIZE - strlen(lineBuf), "%02x ", *pBuffer++);

      if (offset >= WSF_PDUMP_WRAP_SIZE)
      {
        /* display line */
        strncat(lineBuf, "\n", WSF_TRACE_BUFFER_SIZE - strlen(lineBuf));
        PySys_WriteStdout(lineBuf);

        /* reset line buffer */
        offset = 0;
      }
    }

    /* display row remainder */
    if (offset != 0)
    {
      /* display line */
      strncat(lineBuf, "\n", WSF_TRACE_BUFFER_SIZE - strlen(lineBuf));
      PySys_WriteStdout(lineBuf);
    }
  }

  PyGILState_Release(gstate);
}
