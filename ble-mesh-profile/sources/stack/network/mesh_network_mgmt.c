/*************************************************************************************************/
/*!
 *  \file   mesh_network_mgmt.c
 *
 *  \brief  Network management module implementation.
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
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "wsf_cs.h"
#include "wsf_timer.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_security.h"
#include "mesh_seq_manager.h"
#include "mesh_network_beacon.h"

#include "mesh_lower_transport.h"
#include "mesh_sar_tx.h"
#include "mesh_sar_rx_history.h"

#include "mesh_network_mgmt.h"
#include "mesh_network_mgmt_main.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Primary sub-net NetKey Index */
#define MESH_NWK_MGMT_PRIMARY_SUBNET_KEY_INDEX  0x0000

/*! Invalid NetKey Index */
#define MESH_NWK_MGMT_INVALID_SUBNET_KEY_INDEX  0xFFFF

/*! Lower threshold to start IV update */
#define MESH_NWK_MGMT_LOW_SEQ_THRESH            0x700000

/*! Higher threshold to resume IV normal operation */
#define MESH_NWK_MGMT_HIGH_SEQ_THRESH           0xC00000

/*! 96 Hour limit in seconds */
#define MESH_NWK_MGMT_96H_LIMIT_TO_SEC          (96 * 3600)

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Network Management control block. */
static struct meshNwkMgmtCb_tag
{
  meshNwkMgmtFriendshipSecChgCback_t friendshipCback; /*!< Friendship notification callback */
  wsfTimer_t                         ivUpdtTmr;       /*!< IV Update guard timer */
  wsfTimer_t                         ivRecoverTmr;    /*!< IV Recovery guard timer */
  bool_t                             postponeIvUpdt;  /*!< IV transition postponed */
  bool_t                             ivTransPending;  /*!< IV transition pending */
  bool_t                             ivTestMode;      /*!< IV Test Mode */
} meshNwkMgmtCb;

/*! Action table for transitioning between Key Refresh Phases.
 *  Format is A[old][new] and skips phase 3 since it's seamless transition to not started.
 */
static const keyRefreshTransAct_t
 actTable[MESH_KEY_REFRESH_THIRD_PHASE][MESH_KEY_REFRESH_THIRD_PHASE] =
{
                       /* not started,              first phase,            second phase */
  /* not started */  { meshNwkMgmtTransNone,      meshNwkMgmTransJustSet, meshNwkMgmtTransNone   },
  /* first phase */  { meshNwkMgmtTransRevokeOld, meshNwkMgmtTransNone,   meshNwkMgmTransJustSet },
  /* second phase */ { meshNwkMgmtTransRevokeOld, meshNwkMgmtTransNone,   meshNwkMgmtTransNone   }
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Empty callback implementation for friendship security updates.
 *
 *  \param[in] ivChg        IV update state changed.
 *  \param[in] keyChg       Key refresh state changed for the NetKey specified by netKeyIndex param.
 *  \param[in] netKeyIndex  NetKey Index. Valid if keyChg is TRUE.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkMgmtFriendshipSecChgCbackEmpty(bool_t ivChg, bool_t keyChg, uint16_t netKeyIndex)
{
  (void)ivChg;
  (void)keyChg;
  (void)netKeyIndex;
}

/*************************************************************************************************/
/*!
 *  \brief     Takes no action on a new Key Refresh State of a NetKey.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] oldState     Old Key Refresh Phase State.
 *  \param[in] newState     New Key Refresh Phase State.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkMgmtTransNone(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                 meshKeyRefreshStates_t newState)
{
  (void)netKeyIndex;
  (void)oldState;
  (void)newState;
}

/*************************************************************************************************/
/*!
 *  \brief     Just sets a new Key Refresh State of a NetKey in Local Config.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] oldState     Old Key Refresh Phase State.
 *  \param[in] newState     New Key Refresh Phase State.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkMgmTransJustSet(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                   meshKeyRefreshStates_t newState)
{
  /* Set into Local Config. */
  MeshLocalCfgSetKeyRefreshState(netKeyIndex, newState);

  /* Send beacon for sub-net. Optionally inform Friend module. */
  if ((newState == MESH_KEY_REFRESH_SECOND_PHASE) || (newState == MESH_KEY_REFRESH_NOT_ACTIVE))
  {
    MeshNwkBeaconTriggerSend(netKeyIndex);
    meshNwkMgmtCb.friendshipCback(FALSE, TRUE, netKeyIndex);

    return;
  }

  (void)oldState;
}

/*************************************************************************************************/
/*!
 *  \brief     Manages transition to normal operation .
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] oldState     Old Key Refresh Phase State.
 *  \param[in] newState     New Key Refresh Phase State.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkMgmtTransRevokeOld(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                      meshKeyRefreshStates_t newState)
{
  meshLocalCfgRetVal_t retVal;
  uint16_t appKeyIndex;
  uint16_t indexer = 0;

  /* Iterate through all the bound AppKeys. */
  while(MeshLocalCfgGetNextBoundAppKey(netKeyIndex, &appKeyIndex, &indexer)
        == MESH_SUCCESS)
  {
    /* Replace key material. */
    MeshSecRemoveKeyMaterial(MESH_SEC_KEY_TYPE_APP, appKeyIndex, TRUE);

    /* Replace key. */
    MeshLocalCfgRemoveAppKey(appKeyIndex, TRUE);
  }

  /* Replace NetKey material. */
  retVal = (meshLocalCfgRetVal_t)MeshSecRemoveKeyMaterial(MESH_SEC_KEY_TYPE_NWK,
                                                          netKeyIndex, TRUE);
  WSF_ASSERT(retVal == MESH_SUCCESS);

  /* Replace NetKey. */
  retVal = MeshLocalCfgRemoveNetKey(netKeyIndex, TRUE);
  WSF_ASSERT(retVal == MESH_SUCCESS);

  /* Handle setting in Local Config. */
  meshNwkMgmTransJustSet(netKeyIndex, oldState, newState);

  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief  Transitions to Normal operation.
 *
 *  \return None.
 */
/*************************************************************************************************/
static inline void meshNwkMgmtNormalIvResume(void)
{
  /* Clear IV update flag. */
  MeshLocalCfgSetIvUpdateInProgress(FALSE);

  /* Reset sequence numbers. */
  MeshSeqReset();

  /* Clear SAR RX History. */
  MeshSarRxHistoryIviCleanup(MeshLocalCfgGetIvIndex(NULL));

  /* Trigger beacon send on for all NetKeys. */
  MeshNwkBeaconTriggerSend(MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS);

  /* Inform Friend module (if any). */
  meshNwkMgmtCb.friendshipCback(TRUE, FALSE, MESH_NWK_MGMT_INVALID_SUBNET_KEY_INDEX);
}

/*************************************************************************************************/
/*!
 *  \brief  Handles request to transitions to Normal operation.
 *
 *  \return None.
 */
/*************************************************************************************************/
static inline void meshNwkMgmtHandleNormalIvResume(void)
{
  bool_t guardTimerOn = FALSE;

  WSF_CS_INIT(cs);
  WSF_CS_ENTER(cs);
  guardTimerOn = meshNwkMgmtCb.ivUpdtTmr.isStarted;
  WSF_CS_EXIT(cs);

  if(!guardTimerOn)
  {
    /* Check if SAR Tx allows normal IV resume. */
    if(!meshNwkMgmtCb.postponeIvUpdt)
    {
      /* Resume normal operation. */
      meshNwkMgmtNormalIvResume();

      /* Clear pending transition. */
      meshNwkMgmtCb.ivTransPending = FALSE;

      /* Allow SAR Tx again to accept new transactions (in case of faults). */
      MeshSarTxAcceptIncoming();

      if(!meshNwkMgmtCb.ivTestMode)
      {
        /* Restart guard timer. */
        WsfTimerStartSec(&(meshNwkMgmtCb.ivUpdtTmr), MESH_NWK_MGMT_96H_LIMIT_TO_SEC);
      }

      return;
    }

    /* Prevent SAR Tx from starting new transactions and wait to finish existing. */
    MeshSarTxRejectIncoming();
  }

  /* Set pending transition. */
  meshNwkMgmtCb.ivTransPending = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Transitions to IV update.
 *
 *  \return None.
 */
/*************************************************************************************************/
static inline void meshNwkMgmtIvUpdate(void)
{
  /* Increment IV index. */
  MeshLocalCfgSetIvIndex(MeshLocalCfgGetIvIndex(NULL) + 1);

  /* Set IV update flag. */
  MeshLocalCfgSetIvUpdateInProgress(TRUE);

  /* Trigger beacon send on for all NetKeys. */
  MeshNwkBeaconTriggerSend(MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS);

  /* Inform Friend module (if any). */
  meshNwkMgmtCb.friendshipCback(TRUE, FALSE, MESH_NWK_MGMT_INVALID_SUBNET_KEY_INDEX);
}

/*************************************************************************************************/
/*!
 *  \brief  Handles request to transitions to IV Update operation.
 *
 *  \return None.
 */
/*************************************************************************************************/
static inline void meshNwkMgmtHandleIvUpdate(void)
{
  bool_t guardTimerOn = FALSE;
  uint8_t elemId = 0;
  meshSeqNumber_t seqNo = 0;

  WSF_CS_INIT(cs);
  WSF_CS_ENTER(cs);
  guardTimerOn = meshNwkMgmtCb.ivUpdtTmr.isStarted;
  WSF_CS_EXIT(cs);

  if(!guardTimerOn)
  {
    /* Enter IV Update state. */
    meshNwkMgmtIvUpdate();

    /* Clear pending transition only if second threshold has not been exceeded by any sequence
     * numbers.
     */
    meshNwkMgmtCb.ivTransPending = FALSE;
    for(elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
    {
      /* Read most recent sequence number. */
      MeshLocalCfgGetSeqNumber(0, &seqNo);
      /* Check if second threshold is exceeded. */
      if(seqNo > MESH_NWK_MGMT_HIGH_SEQ_THRESH)
      {
        meshNwkMgmtCb.ivTransPending = TRUE;
      }
    }

    if(!meshNwkMgmtCb.ivTestMode)
    {
      /* Restart guard timer. */
      WsfTimerStartSec(&(meshNwkMgmtCb.ivUpdtTmr), MESH_NWK_MGMT_96H_LIMIT_TO_SEC);
    }

    return;
  }

  /* Set pending transition. */
  meshNwkMgmtCb.ivTransPending = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Manages IV recovery.
 *
 *  \param[in] newIv        Received IV index.
 *  \param[in] newIvUpdate  Received IV Update in progress flag.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static inline void meshNwkMgmtIvRecover(uint32_t newIv, bool_t newIvUpdate)
{
  /* Set new IV. */
  MeshLocalCfgSetIvIndex(newIv);

  /* Set IV Update in progress flag. */
  MeshLocalCfgSetIvUpdateInProgress(newIvUpdate);

  /* Reset sequence numbers. */
  MeshSeqReset();

  /* Clear SAR RX History. */
  MeshSarRxHistoryIviCleanup(MeshLocalCfgGetIvIndex(NULL));

  /* Trigger beacon send on for all NetKeys. */
  MeshNwkBeaconTriggerSend(MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS);

  /* Inform Friend module (if any). */
  meshNwkMgmtCb.friendshipCback(TRUE, FALSE, MESH_NWK_MGMT_INVALID_SUBNET_KEY_INDEX);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles request to transitions to IV Recovery operation.
 *
 *  \param[in] newIv        Received IV index.
 *  \param[in] newIvUpdate  Received IV in progress flag.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static inline void meshNwkMgmtHandleIvRecover(uint32_t newIv, bool_t newIvUpdate)
{
  bool_t guardTimerOn = FALSE;

  WSF_CS_INIT(cs);
  WSF_CS_ENTER(cs);
  guardTimerOn = meshNwkMgmtCb.ivRecoverTmr.isStarted;
  WSF_CS_EXIT(cs);

  if(!guardTimerOn)
  {
    meshNwkMgmtIvRecover(newIv, newIvUpdate);

    /* Stop any pending transition. */
    meshNwkMgmtCb.ivTransPending = FALSE;
    WsfTimerStop(&(meshNwkMgmtCb.ivUpdtTmr));

    if(!meshNwkMgmtCb.ivTestMode)
    {
      /* Start guard timer for future recovery. */
      WsfTimerStartSec(&(meshNwkMgmtCb.ivRecoverTmr), 2 * MESH_NWK_MGMT_96H_LIMIT_TO_SEC);
    }

    /* Allow SAR Tx again to accept new transactions (in case of faults). */
    MeshSarTxAcceptIncoming();
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Manages IV information obtained from a Secure Network Beacon for a sub-net.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] ivIndex      Received IV index.
 *  \param[in] ivUpdate     TRUE if IV Update flag is set, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkMgmtHandleRxIv(uint16_t netKeyIndex, uint32_t ivIndex, bool_t ivUpdate)
{
  uint32_t localIv;
  bool_t localIvUpdate;

  /* Read local IV. */
  localIv = MeshLocalCfgGetIvIndex(&localIvUpdate);

  /* Check if indexes are equal. */
  if (ivIndex == localIv)
  {
    /* Check if local node is in IV Update but remote isn't. */
    if ((localIvUpdate) && (!ivUpdate))
    {
      /* Transition to normal operation. */
      meshNwkMgmtHandleNormalIvResume();
    }
  }
  /* Check if received is greater. */
  else if (ivIndex > localIv)
  {
    /* Filter IV information after authentication. */
    if (netKeyIndex != MESH_NWK_MGMT_PRIMARY_SUBNET_KEY_INDEX)
    {
      /* Check if node is also a member of the primary subnet. */
      if (MeshLocalCfgGetKeyRefreshPhaseState(MESH_NWK_MGMT_PRIMARY_SUBNET_KEY_INDEX)
          != MESH_KEY_REFRESH_PROHIBITED_START)
      {
        /* Primary subnet IV shall not be influenced by sub-net IV's. */
        return;
      }
    }

    /* Check if delta is 1. */
    if ((ivIndex - localIv) == 1)
    {
      /* Check if local node is not in IV update but remote is. */
      if ((!localIvUpdate) && (ivUpdate))
      {
        /* Start IV update*/
        meshNwkMgmtHandleIvUpdate();
        return;
      }
    }

    /* Only nodes in normal operation can do IV recovery. */
    if(!localIvUpdate)
    {
      /* Recover IV. */
      meshNwkMgmtHandleIvRecover(ivIndex, ivUpdate);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Sequence Manager threshold callback implementation.
 *
 *  \param[in] lowThreshExceeded   Lower threshold exceeded.
 *  \param[in] highThreshExceeded  Higher threshold exceeded.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkSeqThreshCback(bool_t lowThreshExceeded, bool_t highThreshExceeded)
{
  bool_t localIvUpdate;

  (void)MeshLocalCfgGetIvIndex(&localIvUpdate);

  /* Check if Threshold for starting IV update is reached. */
  if (lowThreshExceeded)
  {
    if (MeshLocalCfgGetKeyRefreshPhaseState(MESH_NWK_MGMT_PRIMARY_SUBNET_KEY_INDEX) ==
        MESH_KEY_REFRESH_PROHIBITED_START)
    {
      return;
    }

    /* Start IV update if not already started. */
    if (!localIvUpdate)
    {
      /* Start IV update because node is on the primary sub-net. */
      meshNwkMgmtHandleIvUpdate();
    }

    return;
  }

  /* Check if node should transition to normal operation. */
  if (highThreshExceeded)
  {
    if (localIvUpdate)
    {
      meshNwkMgmtHandleNormalIvResume();
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler callback for Network Management.
 *
 *  \param[in] pMsg  WSF message callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkMgmtWsfMsgCback(wsfMsgHdr_t *pMsg)
{
  bool_t ivUpdtInProgress;

  switch(pMsg->event)
  {
    case MESH_NWK_MGMT_MSG_IV_UPDT_ALLOWED:
      meshNwkMgmtCb.postponeIvUpdt = FALSE;
      /* Check if transition to normal operation is pending. */
      if(meshNwkMgmtCb.ivTransPending)
      {
        /* Get in progress flag. */
        (void)MeshLocalCfgGetIvIndex(&ivUpdtInProgress);

        if(ivUpdtInProgress)
        {
          /* Handle transition. */
          meshNwkMgmtHandleNormalIvResume();
        }
      }
      break;
    case MESH_NWK_MGMT_MSG_IV_UPDT_DISALLOWED:
      /* Set flag to postpone transition to normal operation if needed. */
      meshNwkMgmtCb.postponeIvUpdt = TRUE;
      break;
    case MESH_NWK_MGMT_MSG_IV_UPDT_TMR:
      /* Get in progress flag. */
      (void)MeshLocalCfgGetIvIndex(&ivUpdtInProgress);

      /* Check if there is any pending transition. */
      if(meshNwkMgmtCb.ivTransPending)
      {
        /* Handle transition. */
        if(ivUpdtInProgress)
        {
          meshNwkMgmtHandleNormalIvResume();
        }
        else
        {
          meshNwkMgmtHandleIvUpdate();
        }
      }

      break;
      /* Nothing to do */
    case MESH_NWK_MGMT_MSG_IV_RECOVER_TMR:
      break;
    /* Start guard timers after provisioning complete. */
    case MESH_NWK_MGMT_MSG_PRV_COMPLETE:
      WsfTimerStartSec(&(meshNwkMgmtCb.ivUpdtTmr), MESH_NWK_MGMT_96H_LIMIT_TO_SEC);
      break;
    default:
      break;
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief   Initializes the Network Management module.
 *
 *  \return  None.
 *
 *  \remarks This function must be called after initializing the Sequence Manager module
 */
/*************************************************************************************************/
void MeshNwkMgmtInit(void)
{
  /* Empty callback for friendship security parameters changed. */
  meshNwkMgmtCb.friendshipCback = meshNwkMgmtFriendshipSecChgCbackEmpty;

  /* Register sequence manager threshold callback. */
  MeshSeqRegister(meshNwkSeqThreshCback, MESH_NWK_MGMT_LOW_SEQ_THRESH, MESH_NWK_MGMT_HIGH_SEQ_THRESH);

  /* Register WSF message handler. */
  meshCb.nwkMgmtMsgCback = meshNwkMgmtWsfMsgCback;

  /* Configure timers. */
  meshNwkMgmtCb.ivUpdtTmr.msg.event = MESH_NWK_MGMT_MSG_IV_UPDT_TMR;
  meshNwkMgmtCb.ivUpdtTmr.handlerId = meshCb.handlerId;

  meshNwkMgmtCb.ivRecoverTmr.msg.event = MESH_NWK_MGMT_MSG_IV_RECOVER_TMR;
  meshNwkMgmtCb.ivRecoverTmr.handlerId = meshCb.handlerId;

  /* Reset flags. */
  meshNwkMgmtCb.ivTransPending = FALSE;
  meshNwkMgmtCb.postponeIvUpdt = FALSE;
  meshNwkMgmtCb.ivTestMode = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers friendship notification callback.
 *
 *  \param[in] secChgCback  Security parameters changed notification callback.
 *
 *  \return    None.
 *
 *  \remarks   This function must be called after initializing the Sequence Manager module
 */
/*************************************************************************************************/
void MeshNwkMgmtRegisterFriendship(meshNwkMgmtFriendshipSecChgCback_t secChgCback)
{
  if(secChgCback != NULL)
  {
    meshNwkMgmtCb.friendshipCback = secChgCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Manages a new Key Refresh State of a NetKey.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] oldState     Old Key Refresh Phase State.
 *  \param[in] newState     New Key Refresh Phase State.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkMgmtHandleKeyRefreshTrans(uint16_t netKeyIndex, meshKeyRefreshStates_t oldState,
                                      meshKeyRefreshStates_t newState)
{
  /* Create automatic transition to not active. */
  if (newState == MESH_KEY_REFRESH_THIRD_PHASE)
  {
    newState = MESH_KEY_REFRESH_NOT_ACTIVE;
  }

  /* Handle transition. */
  (actTable[oldState][newState])(netKeyIndex, oldState, newState);

  MESH_TRACE_INFO3("NWK MGMT: Key refresh for %d - transition from %d to %d", netKeyIndex, oldState,
                   newState);
}

/*************************************************************************************************/
/*!
 *  \brief     Manages key and IV information obtained from a Secure Network Beacon for a subnet.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] newKeyUsed   TRUE if the new key was used to authenticate.
 *  \param[in] ivIndex      Received IV index.
 *  \param[in] keyRefresh   TRUE if Key Refresh flag is set, FALSE otherwise.
 *  \param[in] ivUpdate     TRUE if IV Update flag is set, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkMgmtHandleBeaconData(uint16_t netKeyIndex, bool_t newKeyUsed, uint32_t ivIndex,
                                 bool_t keyRefresh, bool_t ivUpdate)
{
  meshKeyRefreshStates_t newState;
  meshKeyRefreshStates_t oldState = MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex);

  if (oldState >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    MESH_TRACE_ERR1("NWK MGMT: NetKey %d not found", netKeyIndex);
    return;
  }

  /* Should never happen, but handle this anyway. */
  if (oldState == MESH_KEY_REFRESH_THIRD_PHASE)
  {
    oldState = MESH_KEY_REFRESH_NOT_ACTIVE;
  }

  /* Handle Key Refresh when new key is detected. Ignore Key Refresh Flag for old key. */
  if (newKeyUsed)
  {
    newState = keyRefresh ? MESH_KEY_REFRESH_SECOND_PHASE : MESH_KEY_REFRESH_THIRD_PHASE;

    /* Handle Key Refresh. Optimize for same states. */
    if (oldState != newState)
    {
      MeshNwkMgmtHandleKeyRefreshTrans(netKeyIndex, oldState, newState);
    }
  }

  /* Handle IV. */
  meshNwkMgmtHandleRxIv(netKeyIndex, ivIndex, ivUpdate);
}

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"

/*************************************************************************************************/
/*!
 *  \brief      Configures IV Test Mode.
 *
 *  \param[in]  disableTmr     TRUE to enable test mode and disable guard timers,
 *                             FALSE to disable it.
 *  \param[in]  signalTrans    TRUE if a transition is also intended.
 *  \param[in]  transToUpdate  Valid if signalTrans is TRUE. If TRUE transition is to IV Update,
 *                             otherwise transition is to normal operation.
 *  \param[out] pOutIv         Pointer to memory where to store IV index value.
 *  \param[out] pOutIvUpdate   Pointer to memory where to store current IV Update Flag value.
 *
 *  remarks     Output parameters can be NULL.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshTestIvConfigTestMode(bool_t disableTmr, bool_t signalTrans, bool_t transToUpdate,
                              uint32_t *pOutIv, bool_t *pOutIvUpdate)
{
  uint32_t ivIndex;
  bool_t ivUpdate;

  meshNwkMgmtCb.ivTestMode = disableTmr;

  if(disableTmr)
  {
    /* Stop timers in case they are already running. */
    WsfTimerStop(&(meshNwkMgmtCb.ivUpdtTmr));
    WsfTimerStop(&(meshNwkMgmtCb.ivRecoverTmr));
  }

  if(signalTrans)
  {
    /* Read ivUpdate flag. */
     (void)MeshLocalCfgGetIvIndex(&ivUpdate);

     /* Check if already in target state. */
     if(ivUpdate != transToUpdate)
     {
       /* Transition to new state. */
       if(transToUpdate)
       {
         meshNwkMgmtHandleIvUpdate();
       }
       else
       {
         meshNwkMgmtHandleNormalIvResume();
       }
     }
  }

  /* Read after updates. */
  ivIndex = MeshLocalCfgGetIvIndex(&ivUpdate);

  if(pOutIv != NULL)
  {
    *pOutIv = ivIndex;
  }

  if(pOutIvUpdate != NULL)
  {
    *pOutIvUpdate = ivUpdate;
  }
}

#endif
