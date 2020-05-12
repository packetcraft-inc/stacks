/*************************************************************************************************/
/*!
 *  \file   mesh_proxy_main.c
 *
 *  \brief  Mesh Proxy module implementation.
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

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_error_codes.h"
#include "mesh_bearer.h"
#include "mesh_security.h"
#include "mesh_network.h"
#include "mesh_local_config.h"
#include "mesh_seq_manager.h"
#include "mesh_utils.h"

#include "mesh_proxy_main.h"

#include <string.h>

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Proxy control block */
typedef struct meshProxyCb_tag
{
  wsfQueue_t                 txSecQueue;                      /*!< Tx security queue */
  wsfQueue_t                 rxSecQueue;                      /*!< Tx security queue */
  bool_t                     encryptInProgress;               /*!< Flag used to signal encrypt is
                                                               *   in progress;
                                                               */
  bool_t                     decryptInProgress;               /*!< Flag used to signal decrypt is
                                                               *   in progress;
                                                               */
  meshBrNwkPduRecvCback_t    decryptedProxyPduCback;
} meshProxyCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Proxy control block */
static meshProxyCb_t proxyCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Configures Network Encryption parameters and sends a request to the Security Module.
 *
 *  \param[in] pNwkPduMeta  Pointer to structure containing network PDU and meta information.
 *  \param[in] secCback     Security module callback to be called at the end of encryption.
 *
 *  \return    TRUE if request is successful, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshNwkEncryptRequest(meshProxyPduMeta_t *pNwkPduMeta,
                                             meshSecNwkEncObfCback_t secCback)
{
  meshSecNwkEncObfParams_t encParams;

  /* Set pointer to PDU at the beginning of the Network PDU. */
  encParams.pNwkPduNoMic = (uint8_t *)(pNwkPduMeta->pdu);

  /* Set size of network PDU excluding NetMic. */
  encParams.nwkPduNoMicSize = pNwkPduMeta->pduLen - MESH_NETMIC_SIZE_PROXY_PDU;

  /* Set NetMic size. */
  encParams.netMicSize = MESH_NETMIC_SIZE_PROXY_PDU;

  /* Set pointer to NetMic at the end of the Network PDU. */
  encParams.pNwkPduNetMic = ((pNwkPduMeta->pdu) + (encParams.nwkPduNoMicSize));

  /* Set pointer to encrypted and obfuscated Network PDU same as input pointer. */
  encParams.pObfEncNwkPduNoMic = (uint8_t *)(pNwkPduMeta->pdu);

  /* Set NetKey Index. */
  encParams.netKeyIndex = pNwkPduMeta->netKeyIndex;

  /* Set friendship credentials address to unassigned. */
  encParams.friendOrLpnAddress = MESH_ADDR_TYPE_UNASSIGNED;

  /* Set IV Index. */
  encParams.ivIndex = pNwkPduMeta->ivIndex;

  /* Call to security to encrypt the PDU. */
  return (MESH_SUCCESS == MeshSecNwkEncObf(TRUE, &encParams, secCback, (void *)pNwkPduMeta));
}

/*************************************************************************************************/
/*! \brief     Mesh Security Network PDU encryption and obfuscation complete callback.
 *
 *  \param[in] isSuccess           TRUE if operation completed successfully.
 *  \param[in] isProxyConfig       TRUE if Network PDU is a Proxy Configuration Message.
 *  \param[in] pObfEncNwkPduNoMic  Pointer to buffer where the encrypted and obfuscated
 *                                 network PDU is stored.
 *  \param[in] nwkPduNoMicSize     Size of the network PDU excluding NetMIC.
 *  \param[in] pNwkPduNetMic       Pointer to buffer where the calculated NetMIC is stored.
 *  \param[in] netMicSize          Size of the calculated NetMIC.
 *  \param[in] pParam              Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkEncObfCompleteCback(bool_t isSuccess, bool_t isProxyConfig,
                                       uint8_t *pObfEncNwkPduNoMic, uint8_t nwkPduNoMicSize,
                                       uint8_t *pNwkPduNetMic, uint8_t  netMicSize, void *pParam)
{
  (void)isProxyConfig;
  (void)pObfEncNwkPduNoMic;
  (void)nwkPduNoMicSize;
  (void)pNwkPduNetMic;
  (void)netMicSize;

  meshProxyPduMeta_t *pProxyPduMeta = (meshProxyPduMeta_t *)pParam;

  if (isSuccess)
  {
    /* Send the PDU to the bearer interface. */
    if(!MeshBrSendCfgPdu(pProxyPduMeta->rcvdBrIfId, pProxyPduMeta->pdu, pProxyPduMeta->pduLen))
    {
      /* Free as sending failed. */
      WsfBufFree(pProxyPduMeta);
    }
  }

  /* Clear encrypt flag. */
  proxyCb.encryptInProgress = FALSE;

  /* Resume encryption if pending PDU's. */
  pProxyPduMeta = WsfQueueDeq(&(proxyCb.txSecQueue));

  while (pProxyPduMeta != NULL)
  {
    /* Set encrypt in progress flag. */
    proxyCb.encryptInProgress = TRUE;

    /* Request encryption. */
    if (!meshNwkEncryptRequest(pProxyPduMeta, meshNwkEncObfCompleteCback))
    {
      /* Free the queue element. */
      WsfBufFree(pProxyPduMeta);

      /* Clear encrypt flag. */
      proxyCb.encryptInProgress = FALSE;
    }
    else
    {
      /* Break loop. */
      break;
    }

    /* Get next element. */
    pProxyPduMeta = WsfQueueDeq(&(proxyCb.txSecQueue));
  }

  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Triggers a network decrypt request.
 *
 *  \param[in] pRecvPduMeta  Pointer to a received Network PDU with meta information.
 *  \param[in] secCback      Security module callback to be invoked at the end of decryption.
 *
 *  \return    TRUE if request is successful, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshNwkDecryptRequest(meshProxyPduMeta_t *pRecvPduMeta, meshSecNwkDeobfDecCback_t secCback)
{
  meshSecNwkDeobfDecParams_t nwkDecParams;

  /* Set deobfuscation and decryption parameters. */

  /* Set pointer to encrypted and obfuscated Network PDU. */
  nwkDecParams.pObfEncAuthNwkPdu = (uint8_t *)(pRecvPduMeta->pdu);

  /* Set length of the PDU. */
  nwkDecParams.nwkPduSize = pRecvPduMeta->pduLen;

  /* Set pointer to destination buffer for the decrypted PDU. */
  nwkDecParams.pNwkPduNoMic = (uint8_t *)(pRecvPduMeta->pdu);

  /* Request Security module to attempt to decrypt. Set pointer to received
   * interface as generic parameter.
   */
  return (MESH_SUCCESS == MeshSecNwkDeobfDec(TRUE, &nwkDecParams, secCback, (void *)pRecvPduMeta));
}

/*************************************************************************************************/
/*!
 *  \brief     Checks required information from a decrypted Proxy packet and processes the Proxy PDU.
 *
 *  \param[in] pProxyPduMeta  Pointer to the Proxy PDU meta data structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshProxyCheckAndProcessPdu(meshProxyPduMeta_t *pProxyPduMeta)
{
  uint8_t ctl, ttl;
  meshAddress_t dstAddr;

  /* Extract CTL. */
  ctl = MESH_UTILS_BF_GET(*(pProxyPduMeta->pdu + MESH_CTL_TTL_POS), MESH_CTL_SHIFT, MESH_CTL_SIZE);

  /* Extract Destination Addresses. */
  dstAddr = (*(pProxyPduMeta->pdu + MESH_DST_ADDR_POS) << 8) |
            (*(pProxyPduMeta->pdu + MESH_DST_ADDR_POS + 1));

  /* Extract TTL. */
  ttl = MESH_UTILS_BF_GET(*(pProxyPduMeta->pdu + MESH_CTL_TTL_POS),
                                         MESH_TTL_SHIFT,
                                         MESH_TTL_SIZE);

  /* Check fields */
  if ((ctl != 1) || (dstAddr != MESH_ADDR_TYPE_UNASSIGNED) || (ttl != 0))
  {
    return;
  }

  /* Process the PDU. */
  proxyCb.decryptedProxyPduCback(pProxyPduMeta->rcvdBrIfId,
                                 pProxyPduMeta->pdu + MESH_DST_ADDR_POS + sizeof(meshAddress_t),
                                 pProxyPduMeta->pduLen - MESH_NETMIC_SIZE_PROXY_PDU -
                                 MESH_NWK_HEADER_LEN);
}

/*************************************************************************************************/
/*! \brief     Mesh Security Network deobfuscation and decryption complete callback implementation.
 *
 *  \param[in] isSuccess        TRUE if operation completed successfully.
 *  \param[in] isProxyConfig    TRUE if Network PDU is a Proxy Configuration Message.
 *  \param[in] pNwkPduNoMic     Pointer to buffer the deobfuscated and decrypted network PDU is
 *                              stored.
 *  \param[in] nwkPduSizeNoMic  Size of the deobfuscated and decrypted network PDU excluding NetMIC.
 *  \param[in] netKeyIndex      Global network key index associated to the key that successfully
 *                              processed the received network PDU.
 *  \param[in] ivIndex          IV index that successfully authenticated the PDU.
 *  \param[in] friendOrLpnAddr  Friend or LPN address if friendship credentials were used.
 *  \param[in] pParam           Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkDeobfDecCompleteCback(bool_t isSuccess, bool_t isProxyConfig,
                                         uint8_t *pNwkPduNoMic, uint8_t nwkPduSizeNoMic,
                                         uint16_t netKeyIndex, uint32_t ivIndex,
                                         meshAddress_t friendOrLpnAddr, void *pParam)
{
  (void)isProxyConfig;
  (void)ivIndex;
  (void)pNwkPduNoMic;
  (void)nwkPduSizeNoMic;
  (void)netKeyIndex;
  (void)friendOrLpnAddr;

  meshProxyPduMeta_t *pProxyPduMeta = (meshProxyPduMeta_t *)pParam;

  /* If decryption failed, free the memory and abort. */

  if ((isSuccess != FALSE) && (pNwkPduNoMic != NULL) && (pProxyPduMeta != NULL))
  {
    /* Check and process the PDU. */
    meshProxyCheckAndProcessPdu(pProxyPduMeta);
  }

  /* The PDU meta is provided in the request and is allocated. */
  WsfBufFree((void *)pProxyPduMeta);

  /* Clear decrypt in progress flag. */
  proxyCb.decryptInProgress = FALSE;

  /* Run maintenance on decryption. */
  pProxyPduMeta = WsfQueueDeq(&(proxyCb.rxSecQueue));

  while (pProxyPduMeta != NULL)
  {
    /* Set decrypt in progress flag. */
    proxyCb.decryptInProgress = TRUE;

    /* Request decryption. */
    if (!meshNwkDecryptRequest(pProxyPduMeta, meshNwkDeobfDecCompleteCback))
    {
      /* Free memory. */
      WsfBufFree(pProxyPduMeta);

      /* Clear decrypt in progress flag. */
      proxyCb.decryptInProgress = FALSE;
    }
    else
    {
      /* Break loop. */
      break;
    }

    /* Dequeue next on previous fail. */
    pProxyPduMeta = WsfQueueDeq(&(proxyCb.rxSecQueue));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming Proxy Configuration PDUs from the bearer.
 *
 *  \param[in] brIfId   Unique identifier of the interface.
 *  \param[in] pPdu     Pointer to the Proxy Configuration message PDU.
 *  \param[in] pduLen   Length of the Proxy Configuration message PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrToProxyPduRecvCback(meshBrInterfaceId_t brIfId, const uint8_t *pPdu, uint8_t pduLen)
{
  meshProxyPduMeta_t *pRecvPduMeta;

  /* Should never happen since bearer validates this. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
  WSF_ASSERT(pPdu != NULL);

  /* Allocate memory for the PDU and meta information. The structure already contains one byte of
   * the PDU, so subtract 1 from length.
   */
  pRecvPduMeta = WsfBufAlloc(sizeof(meshProxyPduMeta_t) + pduLen - 1);

  /* Check if allocation successful and abort on error. */
  if (pRecvPduMeta == NULL)
  {
    return;
  }

  /* Set received interface ID. */
  pRecvPduMeta->rcvdBrIfId = brIfId;

  /* Copy PDU. */
  memcpy(pRecvPduMeta->pdu, (void *)pPdu, pduLen);

  /* Set PDU length. */
  pRecvPduMeta->pduLen = pduLen;

  /* Check if another decryption is in progress. */
  if (proxyCb.decryptInProgress)
  {
    /* Enqueue the request. */
    WsfQueueEnq(&(proxyCb.rxSecQueue), (void *)pRecvPduMeta);
  }
  else
  {
    /* Set decrypt in progress flag. */
    proxyCb.decryptInProgress = TRUE;

    if (!meshNwkDecryptRequest(pRecvPduMeta, meshNwkDeobfDecCompleteCback))
    {
      /* Free memory. */
      WsfBufFree(pRecvPduMeta);

      /* Clear decrypt in progress flag. */
      proxyCb.decryptInProgress = FALSE;
    }
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the GATT bearer and registers callbacks.
 *
 *  \param[in] eventCback    Event notification callback for the upper layer.
 *  \param[in] pduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                           a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshProxyRegister(meshBrEventNotifyCback_t eventCback, meshBrNwkPduRecvCback_t pduRecvCback)
{
  /* Register the event callbacks */
  MeshBrRegisterProxy(eventCback, meshBrToProxyPduRecvCback);
  proxyCb.decryptedProxyPduCback = pduRecvCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Encrypts and sends a Proxy Configuration message.
 *
 *  \param[in] brIfId  Unique identifier of the interface.
 *  \param[in] opcode  Proxy configuration message opcode.
 *  \param[in] pPdu    Pointer to the Proxy Configuration message PDU.
 *  \param[in] pduLen  Length of the Proxy Configuration message PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshProxySendConfigMessage(meshBrInterfaceId_t brIfId, uint8_t opcode, const uint8_t *pPdu,
                                uint8_t pduLen)

{
  meshNwkPduTxInfo_t  nwkPduTxInfo;
  uint8_t *pHdr, *pDataPtr;
  uint16_t  index = 0;
  bool_t ivUpdtInProgress = FALSE;

  /* Populate TX info structure with Proxy Configuration parameters. */
  if (MeshLocalCfgGetAddrFromElementId(0, &nwkPduTxInfo.src) != MESH_SUCCESS)
  {
    return;
  }

  /* Set Sequence number in TX info. */
  if (MeshSeqGetNumber(nwkPduTxInfo.src, &(nwkPduTxInfo.seqNo), TRUE) != MESH_SUCCESS)
  {
    return;
  }

  /* Set primary subnet key index in Tx info. */
  if (MeshLocalCfgGetNextNetKeyIndex(&nwkPduTxInfo.netKeyIndex, &index) != MESH_SUCCESS)
  {
    return;
  }

  /* Set DST, CTL and TTL */
  nwkPduTxInfo.dst = MESH_ADDR_TYPE_UNASSIGNED;
  nwkPduTxInfo.ctl = 1;
  nwkPduTxInfo.ttl = 0;

  /* Compute NWK PDU len = pdu length + opcode length + header length + NetMic size. */
  uint8_t nwkPduLen = pduLen + sizeof(uint8_t) + MESH_NWK_HEADER_LEN + MESH_NETMIC_SIZE_PROXY_PDU;

  /* Allocate memory. PDU meta already contains first byte of the Network PDU. Subtract 1. */
  meshProxyPduMeta_t *pProxyPduMeta = WsfBufAlloc(sizeof(meshProxyPduMeta_t) - 1 + nwkPduLen);

  /* If no memory is available, return proper error code. */
  if (pProxyPduMeta == NULL)
  {
    return;
  }

  /* Set PDU length. */
  pProxyPduMeta->pduLen = nwkPduLen;

  /* Set pointer to header of network PDU. */
  pHdr = (uint8_t *)(pProxyPduMeta->pdu);

  /* Pack Network PDU header with 0 for IVI and NID since security will set those fields. */
  MeshNwkPackHeader(&nwkPduTxInfo, pHdr, 0, 0);

  /* Set data pointer after Network PDU Header. */
  pDataPtr = (uint8_t *)(pProxyPduMeta->pdu) + MESH_NWK_HEADER_LEN;

  /* Add in opcode. */
  UINT8_TO_BSTREAM(pDataPtr, opcode);

  /* Copy Lower Transport PDU header. */
  memcpy((void *)(pDataPtr), pPdu, pduLen);

  /* Set NetKey index. */
  pProxyPduMeta->netKeyIndex = nwkPduTxInfo.netKeyIndex;

  /* Set bearer interface id. */
  pProxyPduMeta->rcvdBrIfId = brIfId;

  /* Read IV index. */
  pProxyPduMeta->ivIndex = MeshLocalCfgGetIvIndex(&ivUpdtInProgress);

  if (ivUpdtInProgress)
  {
    /* For IV update in progress procedures, the IV index must be decremented by 1.
     * Make sure IV index is not 0.
     */
    WSF_ASSERT(pProxyPduMeta->ivIndex != 0);
    if (pProxyPduMeta->ivIndex != 0)
    {
      --(pProxyPduMeta->ivIndex);
    }
  }

  /* Check if another encryption is in progress. */
  if (proxyCb.encryptInProgress)
  {
    /* Enqueue the PDU in the TX input queue. */
    WsfQueueEnq(&(proxyCb.txSecQueue), (void *)pProxyPduMeta);
  }
  else
  {
    /* Set encrypt in progress flag. */
    proxyCb.encryptInProgress = TRUE;

    /* Prepare encrypt request. */
    if (!meshNwkEncryptRequest(pProxyPduMeta, meshNwkEncObfCompleteCback))
    {
      /* Free memory. */
      WsfBufFree(pProxyPduMeta);

      /* Clear encrypt in progress flag. */
      proxyCb.encryptInProgress = FALSE;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Empty message handler.
 *
 *  \param[in] pMsg  Pointer to API WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshProxyProcessMsgEmpty(wsfMsgHdr_t *pMsg)
{
  (void)pMsg;
}
