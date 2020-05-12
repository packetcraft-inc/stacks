/*************************************************************************************************/
/*!
 *  \file   wsf_os.c
 *
 *  \brief  Software foundation OS main module.
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

#include <windows.h>
#include "wsf_os.h"

#include "wsf_assert.h"
#include "wsf_os_int.h"
#include "wsf_trace.h"
#include "wsf_types.h"
#include "wsf_timer.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "pal_crypto.h"
#include "sec_api.h"
#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "wdxs_api.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_handler.h"
#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"
#include "mesh_prv_cl_api.h"

#include "mmdl_types.h"
#include "mmdl_vendor_test_cl_api.h"
#include "mesh_ht_sr_api.h"
#include "mesh_ht_cl_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_gen_powerlevel_sr_api.h"
#include "mmdl_gen_powerlevelsetup_sr_api.h"
#include "mmdl_gen_powerlevel_cl_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_gen_battery_sr_api.h"
#include "mmdl_gen_default_trans_cl_api.h"
#include "mmdl_time_cl_api.h"
#include "mmdl_time_sr_api.h"
#include "mmdl_timesetup_sr_api.h"
#include "mmdl_gen_battery_cl_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_scene_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_scheduler_sr_api.h"
#include "mmdl_scheduler_cl_api.h"
#include "mmdl_vendor_test_cl_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#if WSF_OS_DIAG == TRUE
#define WSF_OS_SET_ACTIVE_HANDLER_ID(id)          WsfActiveHandler = id;
#else
#define WSF_OS_SET_ACTIVE_HANDLER_ID(id)
#endif /* WSF_OS_DIAG */

/*! Maximum number of event handlers per task */
#define WSF_MAX_HANDLERS      16

/*! \brief OS serivice function number */
#define WSF_OS_MAX_SERVICE_FUNCTIONS                  3

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Thread state. */
typedef enum
{
  WSF_TASK_STATE_FREE,        /*!< Default task state */
  WSF_TASK_STATE_INIT,        /*!< Task initialized */
  WSF_TASK_STATE_STARTED,     /*!< Task started */
  WSF_TASK_STATE_TERMINATED   /*!< Task termination in progress */
} wsfTaskState_t;

/*! Task structure */
typedef struct
{
  wsfTaskState_t    state;                              /*!< Current state */
  wsfTaskEvent_t    taskEventMask;                      /*!< Task events */
  wsfEventHandler_t handler[WSF_MAX_HANDLERS];          /*!< Handlers callbacks */
  wsfHandlerId_t    numHandler;                         /*!< Number of registered handlers */
  wsfEventMask_t    handlerEventMask[WSF_MAX_HANDLERS]; /*!< Handler event mask */
  wsfQueue_t        msgQueue;                           /*!< Message queue */
} wsfOsTask_t;

/*! OS structure */
typedef struct
{
  wsfOsTask_t      task;            /*!< Task resource */
  CRITICAL_SECTION systemLock;      /*!< System resource lock */

  wsfTaskState_t  timerTaskState;   /*!< Timer task state */
  uint8_t         msPerTick;        /*!< Number of milliseconds per timer tick */
  wsfQueue_t      timerQueue;       /*!< Timer queue */
  WORD            lastMs;           /*!< Last time time */
  HANDLE          hSysTimer;        /*!< System periodic timer */
  HANDLE          hSysTimerQueue;   /*!< Windows timer queue for system periodic timer */
  HANDLE          workPendingEvent; /*!< Work pending event */
  WsfOsIdleCheckFunc_t        sleepCheckFuncs[WSF_OS_MAX_SERVICE_FUNCTIONS];
  uint8_t         numFunc;
} wsfOs_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Active task handler ID. */
wsfHandlerId_t WsfActiveHandler;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

wsfOs_t wsfOs;

/*! App handler and init functions-- for test purposes */
static void wsfDummyHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg) {}
static void wsfDummyInit(wsfHandlerId_t handlerId) {}
static wsfTestHandler_t wsfAppHandler = wsfDummyHandler;
static wsfTestInit_t wsfAppInit = wsfDummyInit;

/*************************************************************************************************/
/*!
 *  \brief  Lock task scheduling.
 */
/*************************************************************************************************/
void WsfTaskLock(void)
{
  EnterCriticalSection(&wsfOs.systemLock);
}

/*************************************************************************************************/
/*!
 *  \brief  Unock task scheduling.
 */
/*************************************************************************************************/
void WsfTaskUnlock(void)
{
  LeaveCriticalSection(&wsfOs.systemLock);
}

/*************************************************************************************************/
/*!
 *  \brief  Set an event for an event handler.
 *
 *  \param  handlerId   Handler ID.
 *  \param  event       Event or events to set.
 */
/*************************************************************************************************/
void WsfSetEvent(wsfHandlerId_t handlerId, wsfEventMask_t event)
{
  WSF_ASSERT(handlerId < WSF_MAX_HANDLERS);

  WSF_TRACE_INFO2("WsfSetEvent handlerId:%u event:%u", handlerId, event);

  EnterCriticalSection(&wsfOs.systemLock);
  wsfOs.task.handlerEventMask[handlerId] |= event;
  LeaveCriticalSection(&wsfOs.systemLock);

  WsfTaskSetReady(handlerId, WSF_HANDLER_EVENT);
}

/*************************************************************************************************/
/*!
 *  \brief  Set the task used by the given handler as ready to run.
 *
 *  \param  handlerId   Event handler ID.
 *  \param  event       Task event mask.
 */
/*************************************************************************************************/
void WsfTaskSetReady(wsfHandlerId_t handlerId, wsfTaskEvent_t event)
{
  WSF_ASSERT(handlerId < WSF_MAX_HANDLERS);

  EnterCriticalSection(&wsfOs.systemLock);
  wsfOs.task.taskEventMask |= event;
  LeaveCriticalSection(&wsfOs.systemLock);

  SetEvent(wsfOs.workPendingEvent);
}

/*************************************************************************************************/
/*!
 *  \brief  Return the message queue used by the given handler.
 *
 *  \param  handlerId   Event handler ID.
 *
 *  \return Task message queue.
 */
/*************************************************************************************************/
wsfQueue_t *WsfTaskMsgQueue(wsfHandlerId_t handlerId)
{
  WSF_ASSERT(handlerId < WSF_MAX_HANDLERS);

  return &(wsfOs.task.msgQueue);
}

/*************************************************************************************************/
/*!
 *  \brief  Set the next WSF handler function in the WSF OS handler array.  This function
 *          should only be called as part of the stack initialization procedure.
 *
 *  \param  handler    WSF handler function.
 *
 *  \return WSF handler ID for this handler.
 */
/*************************************************************************************************/
wsfHandlerId_t WsfOsSetNextHandler(wsfEventHandler_t handler)
{
  wsfHandlerId_t handlerId = wsfOs.task.numHandler++;

  WSF_ASSERT(handlerId < WSF_MAX_HANDLERS);

  wsfOs.task.handler[handlerId] = handler;

  return handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief  Main task loop for Windows implementation.
 *
 *  \param  pParam   Pointer to task structure.
 *
 *  \return Always 0.
 */
/*************************************************************************************************/
static unsigned int __stdcall wsfOsDispatcherTask(void *pParam)
{
  wsfOsTask_t       *pTask = pParam;
  void              *pMsg;
  wsfTimer_t        *pTimer;
  DWORD             status;
  wsfEventMask_t    eventMask;
  wsfTaskEvent_t    taskEventMask;
  wsfHandlerId_t    handlerId;
  uint8_t           i;

  WSF_TRACE_INFO0("wsfOsDispatcherTask enter");

  /* initialization */
  wsfOs.workPendingEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  pTask->state = WSF_TASK_STATE_STARTED;

  /* task main loop */
  for(;;)
  {
    status = WaitForSingleObject(wsfOs.workPendingEvent, INFINITE);

    WSF_ASSERT(status == WAIT_OBJECT_0);

    if (pTask->state == WSF_TASK_STATE_TERMINATED)
    {
      break;
    }

    /* get and then clear task event mask (after updating timers) */
    EnterCriticalSection(&wsfOs.systemLock);
    taskEventMask = pTask->taskEventMask;
    pTask->taskEventMask = 0;
    LeaveCriticalSection(&wsfOs.systemLock);

    if (taskEventMask & WSF_MSG_QUEUE_EVENT)
    {
      /* handle msg queue */
      while ((pMsg = WsfMsgDeq(&pTask->msgQueue, &handlerId)) != NULL)
      {
        WSF_ASSERT(handlerId < WSF_MAX_HANDLERS);
        WSF_OS_SET_ACTIVE_HANDLER_ID(handlerId);
        (*pTask->handler[handlerId])(0, pMsg);
        WsfMsgFree(pMsg);
      }
    }

    if (taskEventMask & WSF_TIMER_EVENT)
    {
      /* service timers */
      while ((pTimer = WsfTimerServiceExpired(0)) != NULL)
      {
        WSF_ASSERT(pTimer->handlerId < WSF_MAX_HANDLERS);
        WSF_OS_SET_ACTIVE_HANDLER_ID(pTimer->handlerId);
        (*pTask->handler[pTimer->handlerId])(0, &pTimer->msg);
      }
    }

    if (taskEventMask & WSF_HANDLER_EVENT)
    {
      /* service handlers */
      for (i = 0; i < WSF_MAX_HANDLERS; i++)
      {
        if (pTask->handlerEventMask[i] != 0 && pTask->handler[i] != NULL)
        {
          EnterCriticalSection(&wsfOs.systemLock);
          eventMask = pTask->handlerEventMask[i];
          pTask->handlerEventMask[i] = 0;
          WSF_OS_SET_ACTIVE_HANDLER_ID(i);
          LeaveCriticalSection(&wsfOs.systemLock);

          (*pTask->handler[i])(eventMask, NULL);
        }
      }
    }
  }

  /* shutdown */
  CloseHandle(&wsfOs.workPendingEvent);
  pTask->state = WSF_TASK_STATE_FREE;

  WSF_TRACE_INFO0("wsfOsDispatcherTask exit");

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief  Timer task loop for servicing the timer queue.
 *
 *  \param  pParam   Unused, always NULL.
 *
 *  \return Always 0.
 */
/*************************************************************************************************/
static unsigned int __stdcall wsfTimerThread(void *pParam)
{
  HANDLE          hTimer;
  LARGE_INTEGER   dueTime;
  WORD            remMs = 0;

  hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
  WSF_ASSERT(hTimer != NULL);

  /* convert to 100 NS, where negative means relative time */
  dueTime.QuadPart = (LONGLONG)(-wsfOs.msPerTick * 10000);
  SetWaitableTimer(hTimer, &dueTime, (LONG)wsfOs.msPerTick, NULL, NULL, 0);

  wsfOs.timerTaskState = WSF_TASK_STATE_STARTED;

  for (;;)
  {
    DWORD           status;
    SYSTEMTIME      curTime;
    WORD            deltaMs;
    wsfTimerTicks_t ticks;

    status = WaitForSingleObject(hTimer, INFINITE);
    WSF_ASSERT(status == WAIT_OBJECT_0);

    if (wsfOs.timerTaskState == WSF_TASK_STATE_TERMINATED)
    {
      break;
    }

    /* get current time */
    GetSystemTime(&curTime);

    /* calculate elapsed ms with wraparound */
    deltaMs = (curTime.wMilliseconds >= wsfOs.lastMs) ?
      (curTime.wMilliseconds - wsfOs.lastMs) :
      (1000 + curTime.wMilliseconds - wsfOs.lastMs);
    wsfOs.lastMs = curTime.wMilliseconds;

    /* calculate elapsed ticks and any remainder */
    ticks = (wsfTimerTicks_t) ((deltaMs + remMs) / wsfOs.msPerTick);
    remMs = (deltaMs + remMs) % wsfOs.msPerTick;

    /* update timers */
    WsfTimerUpdate(ticks);
  }

  CloseHandle(hTimer);

  wsfOs.timerTaskState = WSF_TASK_STATE_FREE;

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief  Windows implementation generic stack initialize.
 */
/*************************************************************************************************/
void wsfOsGenericStackInit()
{
  SecInit();
  SecAesInit();
  SecCmacInit();
  SecEccInit();

  /* init stack */
  HciHandlerInit(WsfOsSetNextHandler(HciHandler));

  DmAdvInit();
  DmDevPrivInit();
  DmScanInit();
  DmConnInit();
  DmConnSlaveInit();
  DmConnMasterInit();
  DmSecInit();
  DmSecLescInit();
  DmPrivInit();
  DmPhyInit();
  DmHandlerInit(WsfOsSetNextHandler(DmHandler));

  L2cSlaveHandlerInit(WsfOsSetNextHandler(L2cSlaveHandler));
  L2cInit();
  L2cSlaveInit();
  L2cMasterInit();

  L2cCocHandlerInit(WsfOsSetNextHandler(L2cCocHandler));
  L2cCocInit();

  AttHandlerInit(WsfOsSetNextHandler(AttHandler));
  AttsInit();
  AttsIndInit();
  AttcInit();
  AttcSignInit();
  AttsSignInit();

  SmpHandlerInit(WsfOsSetNextHandler(SmpHandler));
  SmpiScInit();
  SmprScInit();
  HciSetMaxRxAclLen(251);

  AppHandlerInit(WsfOsSetNextHandler(AppHandler));
  WdxsHandlerInit(WsfOsSetNextHandler(WdxsHandler));

  WSF_TRACE_INFO0("wsfOsGenericStackInit");
}

/*************************************************************************************************/
/*!
 *  \brief  Windows implementation generic extended stack initialize.
 */
/*************************************************************************************************/
void wsfOsGenericExtStackInit()
{
  SecInit();
  SecAesInit();
  SecCmacInit();
  SecEccInit();

  /* init stack */
  HciHandlerInit(WsfOsSetNextHandler(HciHandler));

  DmExtAdvInit();
  DmDevPrivInit();
  DmExtScanInit();
  DmConnInit();
  DmExtConnSlaveInit();
  DmExtConnMasterInit();
  DmSecInit();
  DmSecLescInit();
  DmPrivInit();
  DmPhyInit();
  DmConnCteInit();
  DmPastInit();
  DmCisInit();
  DmCisMasterInit();
  DmCisSlaveInit();
  DmBisMasterInit();
  DmBisSlaveInit();
  DmIsoInit();
  DmHandlerInit(WsfOsSetNextHandler(DmHandler));

  L2cSlaveHandlerInit(WsfOsSetNextHandler(L2cSlaveHandler));
  L2cInit();
  L2cSlaveInit();
  L2cMasterInit();

  L2cCocHandlerInit(WsfOsSetNextHandler(L2cCocHandler));
  L2cCocInit();

  AttHandlerInit(WsfOsSetNextHandler(AttHandler));
  AttsInit();
  AttsIndInit();
  AttcInit();
  AttcSignInit();
  AttsSignInit();
  AttsCsfInit();

  SmpHandlerInit(WsfOsSetNextHandler(SmpHandler));
  SmpiScInit();
  SmprScInit();
  HciSetMaxRxAclLen(251);

  AppHandlerInit(WsfOsSetNextHandler(AppHandler));
  WdxsHandlerInit(WsfOsSetNextHandler(WdxsHandler));

  WSF_TRACE_INFO0("wsfOsGenericStackInit");
}

/*************************************************************************************************/
/*!
*  \brief  Windows implementation generic mesh stack initialize.
*
*  \param  mmdlHandler        Mesh model event handler callback.
*
*  \return None.
*/
/*************************************************************************************************/
void wsfOsGenericMeshStackInit(wsfTestHandler_t mmdlHandler)
{
  wsfHandlerId_t handlerId;

  SecInit();
  SecAesInit();
  SecAesRevInit();
  SecCmacInit();
  SecEccInit();
  SecCcmInit();

  /* Initialize stack handlers. */
  handlerId = WsfOsSetNextHandler(HciHandler);
  HciHandlerInit(handlerId);

  handlerId = WsfOsSetNextHandler(DmHandler);
  DmDevVsInit(0);

#if (LL_VER >= LL_VER_BT_CORE_SPEC_5_0)
  DmExtScanInit();
  DmExtAdvInit();
#else
  DmScanInit();
  DmAdvInit();
#endif

  DmConnInit();
#if (LL_VER >= LL_VER_BT_CORE_SPEC_5_0)
  DmExtConnMasterInit();
  DmExtConnSlaveInit();
#else
  DmConnMasterInit();
  DmConnSlaveInit();
#endif

  DmSecInit();
  DmSecLescInit();
  DmPrivInit();
  DmHandlerInit(handlerId);

  handlerId = WsfOsSetNextHandler(L2cSlaveHandler);
  L2cSlaveHandlerInit(handlerId);
  L2cInit();
  L2cMasterInit();
  L2cSlaveInit();

  handlerId = WsfOsSetNextHandler(AttHandler);
  AttHandlerInit(handlerId);
  AttsInit();
  AttsIndInit();
  AttcInit();

  handlerId = WsfOsSetNextHandler(SmpHandler);
  SmpHandlerInit(handlerId);
  SmpiInit();
  SmprInit();
  SmpiScInit();
  SmprScInit();
  HciSetMaxRxAclLen(100);

  /* Initialize Mesh handlers */
  handlerId = WsfOsSetNextHandler(MeshHandler);
  MeshHandlerInit(handlerId);

  /* Initialize Mesh Security handler. */
  handlerId = WsfOsSetNextHandler(MeshSecurityHandler);
  MeshSecurityHandlerInit(handlerId);

  /* Initialize Mesh Provisioning Server handler. */
  handlerId = WsfOsSetNextHandler(MeshPrvSrHandler);
  MeshPrvSrHandlerInit(handlerId);

  /* Initialize Mesh Provisioning Client handler. */
  handlerId = WsfOsSetNextHandler(MeshPrvClHandler);
  MeshPrvClHandlerInit(handlerId);

  /* Initialize model handler handler. */
  handlerId = WsfOsSetNextHandler(mmdlHandler);

  /* Initialize Health Client and Server model handler. */
  MeshHtSrHandlerInit(handlerId);
  MeshHtClHandlerInit(handlerId);

  /* Initialize Generic On Off Client and Server model handler. */
  MmdlGenOnOffClHandlerInit(handlerId);
  MmdlGenOnOffSrHandlerInit(handlerId);

  /* Initialize Generic Power On Off Client and Server model handler. */
  MmdlGenPowOnOffClHandlerInit(handlerId);
  MmdlGenPowOnOffSrHandlerInit(handlerId);
  MmdlGenPowOnOffSetupSrHandlerInit(handlerId);

  /* Initialize Generic Level Client and Server model handler. */
  MmdlGenLevelClHandlerInit(handlerId);
  MmdlGenLevelSrHandlerInit(handlerId);

  /* Initialize Generic Default Transition Client and Server model handler. */
  MmdlGenDefaultTransClHandlerInit(handlerId);
  MmdlGenDefaultTransSrHandlerInit(handlerId);

  /* Initialize Generic Battery Client and Server model handler. */
  MmdlGenBatteryClHandlerInit(handlerId);
  MmdlGenBatterySrHandlerInit(handlerId);

  /* Initialize Generic Power Level Client and Server model handler. */
  MmdlGenPowerLevelClHandlerInit(handlerId);
  MmdlGenPowerLevelSrHandlerInit(handlerId);
  MmdlGenPowerLevelSetupSrHandlerInit(handlerId);

  /* Initialize Time Client and Server model handler. */
  MmdlTimeClHandlerInit(handlerId);
  MmdlTimeSrHandlerInit(handlerId);
  MmdlTimeSetupSrHandlerInit(handlerId);

  /* Initialize Scene Client and Server model handler. */
  MmdlSceneClHandlerInit(handlerId);
  MmdlSceneSrHandlerInit(handlerId);

  /* Initialize Light Lightness Client and Server model handler. */
  MmdlLightLightnessClHandlerInit(handlerId);
  MmdlLightLightnessSrHandlerInit(handlerId);
  MmdlLightLightnessSetupSrHandlerInit(handlerId);

  /* Initialize Light HSL Client and Server model handler. */
  MmdlLightHslClHandlerInit(handlerId);
  MmdlLightHslSrHandlerInit(handlerId);
  MmdlLightHslHueSrHandlerInit(handlerId);
  MmdlLightHslSatSrHandlerInit(handlerId);

  /* Initialize Scheduler Client and Server model handler. */
  MmdlSchedulerClHandlerInit(handlerId);
  MmdlSchedulerSrHandlerInit(handlerId);

  /* Initialize Vendor Model Client model handler. */
  MmdlVendorTestClHandlerInit(handlerId);
}


/*************************************************************************************************/
/*!
 *  \brief  Windows implementation initialize.
 *
 *  \param  bufMemLen Length of free memory
 *  \param  pBufMem   Free memory buffer for building buffer pools
 *  \param  msPerTick Milliseconds per timer tick.
 */
/*************************************************************************************************/
void wsfOsInit(uint8_t msPerTick, uint16_t bufMemLen, uint8_t *pBufMem,
               uint8_t numPools, wsfBufPoolDesc_t *pDesc)
{
  SYSTEMTIME  startTime;

  /* init OS resources */
  memset(&wsfOs, 0, sizeof(wsfOs));
  InitializeCriticalSection(&wsfOs.systemLock);

  /* init WSF services */
  WsfTimerInit();
  WsfBufInit(numPools, pDesc);

  /* create tasks. */
  CreateThread(0, 0, wsfOsDispatcherTask, &wsfOs.task, 0, NULL);

  /* block until thread starts */
  while (wsfOs.task.state != WSF_TASK_STATE_STARTED)
  {
    /* yeild this task */
    Sleep(1);
  }

  /* init timer thread */
  GetSystemTime(&startTime);
  wsfOs.lastMs = startTime.wMilliseconds;
  wsfOs.msPerTick = msPerTick;

  /* create timer task */
  CreateThread(0, 0, wsfTimerThread, NULL, 0, NULL);

  WSF_TRACE_INFO1("wsfOsInit msPerTick:%u", msPerTick);
}

/*************************************************************************************************/
/*!
 *  \brief  Windows implementation shutdown.
 */
/*************************************************************************************************/
void wsfOsShutdown(void)
{
  /* shutdown timer thread */
  wsfOs.timerTaskState = WSF_TASK_STATE_TERMINATED;

  /* block until task terminates */
  while (wsfOs.timerTaskState != WSF_TASK_STATE_FREE)
  {
    /* yeild this task */
    Sleep(1);
  }

  /* signal task termination */
  wsfOs.task.state = WSF_TASK_STATE_TERMINATED;
  SetEvent(wsfOs.workPendingEvent);

  /* block until thread terminate */
  while (wsfOs.task.state != WSF_TASK_STATE_FREE)
  {
    /* yeild this task */
    Sleep(1);
  }

  /* free synchronization objects */
  DeleteCriticalSection(&wsfOs.systemLock);

  WSF_TRACE_INFO0("wsfOsShutdown");
}

/*************************************************************************************************/
/*!
 *  \brief  Set the App event handler and init function for test purposes.
 *
 *  \param  handler     Event handler.
 *  \param  handlerInit Init function.
 *
 *  \note   This function must be called before wsfOsInit().
 */
/*************************************************************************************************/
void wsfOsSetAppHandler(wsfTestHandler_t handler, wsfTestInit_t handlerInit)
{
  handlerInit(WsfOsSetNextHandler(handler));
}

/*************************************************************************************************/
/*!
*  \brief  Check if WSF is ready to sleep.
*
*  \return Return TRUE if there are no pending WSF task events set, FALSE otherwise.
*/
/*************************************************************************************************/
bool_t wsfOsReadyToSleep(void)
{
  /* Not used */
  return FALSE;
}

/*************************************************************************************************/
/*!
*  \brief  Event dispatched.  Designed to be called repeatedly from infinite loop.
*
*  \return None.
*/
/*************************************************************************************************/
void wsfOsDispatcher(void)
{
  /* Not used */
}

/*************************************************************************************************/
/*!
*  \brief  Initialize OS control structure.
*
*  \return None.
*/
/*************************************************************************************************/
void WsfOsInit(void)
{
  memset(&wsfOs.task, 0, sizeof(wsfOs.task));
}

/*************************************************************************************************/
/*!
 *  \brief  Register service check functions.
 *
 *  \param  func   Service function.
 */
/*************************************************************************************************/
void WsfOsRegisterSleepCheckFunc(WsfOsIdleCheckFunc_t func)
{
  wsfOs.sleepCheckFuncs[wsfOs.numFunc++] = func;
}

/*************************************************************************************************/
/*!
 *  \brief  OS starts main loop
 */
/*************************************************************************************************/
void WsfOsEnterMainLoop(void)
{
  while(TRUE)
  {
    WsfTimerSleepUpdate();
    wsfOsDispatcher();

    bool_t pendingFlag = FALSE;

    for (uint8_t i = 0; i < wsfOs.numFunc; i++)
    {
      if (wsfOs.sleepCheckFuncs[i])
      {
        pendingFlag |= wsfOs.sleepCheckFuncs[i]();
      }
    }

    if (!pendingFlag)
    {
      WsfTimerSleep();
    }
  }
}
