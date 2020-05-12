/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI Isochronous (ISO) data path command module.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
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
#include "pal_codec.h"

/*************************************************************************************************/
/*!
*  \brief      HCI LE enable ISO Tx test.
*
*  \param      handle      CIS or BIS handle.
*  \param      pldType     Payload type.
*
*  \return     None.
*/
/*************************************************************************************************/
void HciLeIsoTxTest(uint16_t handle, uint8_t pldType)
{
  uint8_t status;

  status = LlIsoTxTest(handle, pldType);

  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
*  \brief      HCI LE enable ISO Rx test.
*
*  \param      handle      CIS or BIS handle.
*  \param      pldType     Payload type.
*
*  \return     None.
*/
/*************************************************************************************************/
void HciLeIsoRxTest(uint16_t handle, uint8_t pldType)
{
  uint8_t status;

  status = LlIsoRxTest(handle, pldType);

  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
*  \brief  HCI LE read ISO test counter.
*
*  \param      handle      CIS or BIS handle.
*
*  \return     None.
*/
/*************************************************************************************************/
void HciLeIsoReadTestCounters(uint16_t handle)
{
  /* unused */
}

/*************************************************************************************************/
/*!
*  \brief      HCI LE ISO test end.
*
*  \param      handle      CIS or BIS handle.
*
*  \return     None.
*/
/*************************************************************************************************/
void HciLeIsoTestEnd(uint16_t handle)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE setup ISO data path command.
 *
 *  \param      pDataPathParam  Parameters for setting up ISO data path.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetupIsoDataPathCmd(HciIsoSetupDataPath_t *pDataPathParam)
{
  hciLeSetupIsoDataPathCmdCmplEvt_t evt;

  evt.hdr.param = pDataPathParam->handle;
  evt.hdr.status = LlSetupIsoDataPath((LlIsoSetupDataPath_t *) pDataPathParam);
  evt.hdr.event = HCI_LE_SETUP_ISO_DATA_PATH_CMD_CMPL_CBACK_EVT;

  evt.status = evt.hdr.status;
  evt.handle = pDataPathParam->handle;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE remove ISO data path command.
 *
 *  \param      handle          Connection handle of the CIS or BIS.
 *  \param      directionBits   Data path direction bits.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeRemoveIsoDataPathCmd(uint16_t handle, uint8_t directionBits)
{
  hciLeRemoveIsoDataPathCmdCmplEvt_t evt;

  evt.hdr.param = handle;
  evt.hdr.status = LlRemoveIsoDataPath(handle, directionBits);
  evt.hdr.event = HCI_LE_REMOVE_ISO_DATA_PATH_CMD_CMPL_CBACK_EVT;

  evt.status = evt.hdr.status;
  evt.handle = handle;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI configure data path command.
 *
 *  \param      pDataPathParam  Parameters for configuring data path.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciConfigDataPathCmd(HciConfigDataPath_t *pDataPathParam)
{
  hciConfigDataPathCmdCmplEvt_t evt;

  if (PalCodecConfigureDataPath(pDataPathParam->dpDir, pDataPathParam->dpId))
  {
    evt.hdr.status = evt.status = HCI_SUCCESS;
  }
  else
  {
    evt.hdr.status = evt.status =  HCI_ERR_INVALID_PARAM;
  }

  evt.hdr.event = HCI_CONFIG_DATA_PATH_CMD_CMPL_CBACK_EVT;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI read local supported codecs command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciReadLocalSupCodecsCmd(void)
{
  hciReadLocalSupCodecsCmdCmplEvt_t evt;

  evt.numStdCodecs = evt.numVsCodecs = HCI_MAX_CODEC;
  PalCodecReadLocalSupportedCodecs(&evt.numStdCodecs, (AudioStdCodecInfo_t *) evt.stdCodecs,
                                   &evt.numVsCodecs, (AudioVsCodecInfo_t *) evt.vsCodecs);

  for (uint8_t i = 0; i < evt.numStdCodecs; i++)
  {
    evt.stdCodecTrans[i] = HCI_CODEC_TRANS_CIS_BIT | HCI_CODEC_TRANS_BIS_BIT;
  }

  for (uint8_t i = 0; i < evt.numVsCodecs; i++)
  {
    evt.vsCodecTrans[i] = HCI_CODEC_TRANS_CIS_BIT | HCI_CODEC_TRANS_BIS_BIT;
  }

  evt.hdr.status = evt.status = HCI_SUCCESS;
  evt.hdr.event = HCI_READ_LOCAL_SUP_CODECS_CMD_CMPL_CBACK_EVT;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI read local supported codec capabilities command.
 *
 *  \param      pCodecParam     Parameters to read codec capablilties.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciReadLocalSupCodecCapCmd(HciReadLocalSupCodecCaps_t *pCodecParam)
{
  hciReadLocalSupCodecCapCmdCmplEvt_t evt;

  if (PalCodecReadLocalSupportedCodecCapabilities(pCodecParam->codingFmt, pCodecParam->compId,
                                                  pCodecParam->vsCodecId, pCodecParam->direction))
  {
    if ((pCodecParam->transType & (HCI_CODEC_TRANS_CIS_BIT | HCI_CODEC_TRANS_BIS_BIT)) == 0)
    {
      evt.hdr.status = evt.status = HCI_ERR_INVALID_PARAM;
    }
    else
    {
      evt.hdr.status = evt.status = HCI_SUCCESS;
    }
  }
  else
  {
    evt.hdr.status = evt.status = HCI_ERR_INVALID_PARAM;
  }

  evt.hdr.event = HCI_READ_LOCAL_SUP_CODEC_CAP_CMD_CMPL_CBACK_EVT;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI read local supported controller delay command.
 *
 *  \param      pDelayParam     Parameters to read controller delay.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciReadLocalSupControllerDlyCmd(HciReadLocalSupControllerDly_t *pDelayParam)
{
  hciReadLocalSupCtrDlyCmdCmplEvt_t evt;

  evt.minDly = evt.maxDly = 0;

  if (PalCodecReadLocalSupportedControllerDelay(pDelayParam->codingFmt, pDelayParam->compId,
                                                pDelayParam->vsCodecId, pDelayParam->direction,
                                                &evt.minDly, &evt.maxDly))
  {
    if ((pDelayParam->transType & (HCI_CODEC_TRANS_CIS_BIT | HCI_CODEC_TRANS_BIS_BIT)) == 0)
    {
      evt.hdr.status = evt.status = HCI_ERR_INVALID_PARAM;
    }
    else
    {
      evt.hdr.status = evt.status = HCI_SUCCESS;
    }
  }
  else
  {
    evt.hdr.status = evt.status = HCI_ERR_INVALID_PARAM;
  }

  evt.hdr.event = HCI_READ_LOCAL_SUP_CTR_DLY_CMD_CMPL_CBACK_EVT;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}
