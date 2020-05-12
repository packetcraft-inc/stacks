/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Trace message interface.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
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
#ifndef WSF_TRACE_H
#define WSF_TRACE_H

#include "wsf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define WSF_TRACE(type, subsys, msg, ...)           WsfTrace(WSF_TRACE_TYPE_##type, WSF_TRACE_SUBSYS_##subsys, msg, __VA_ARGS__)

#define WSF_TRACE_INFO0(msg)                        WSF_TRACE(INFO, WSF, msg)
#define WSF_TRACE_INFO1(msg, var1)                  WSF_TRACE(INFO, WSF, msg, var1)
#define WSF_TRACE_INFO2(msg, var1, var2)            WSF_TRACE(INFO, WSF, msg, var1, var2)
#define WSF_TRACE_INFO3(msg, var1, var2, var3)      WSF_TRACE(INFO, WSF, msg, var1, var2, var3)
#define WSF_TRACE_WARN0(msg)                        WSF_TRACE(WARN, WSF, msg)
#define WSF_TRACE_WARN1(msg, var1)                  WSF_TRACE(WARN, WSF, msg, var1)
#define WSF_TRACE_WARN2(msg, var1, var2)            WSF_TRACE(WARN, WSF, msg, var1, var2)
#define WSF_TRACE_WARN3(msg, var1, var2, var3)      WSF_TRACE(WARN, WSF, msg, var1, var2, var3)
#define WSF_TRACE_ERR0(msg)                         WSF_TRACE(ERR, WSF, msg)
#define WSF_TRACE_ERR1(msg, var1)                   WSF_TRACE(ERR, WSF, msg, var1)
#define WSF_TRACE_ERR2(msg, var1, var2)             WSF_TRACE(ERR, WSF, msg, var1, var2)
#define WSF_TRACE_ERR3(msg, var1, var2, var3)       WSF_TRACE(ERR, WSF, msg, var1, var2, var3)
#define WSF_TRACE_ALLOC0(msg)                       WSF_TRACE(ALLOC, WSF, msg)
#define WSF_TRACE_ALLOC1(msg, var1)                 WSF_TRACE(ALLOC, WSF, msg, var1)
#define WSF_TRACE_ALLOC2(msg, var1, var2)           WSF_TRACE(ALLOC, WSF, msg, var1, var2)
#define WSF_TRACE_ALLOC3(msg, var1, var2, var3)     WSF_TRACE(ALLOC, WSF, msg, var1, var2, var3)
#define WSF_TRACE_FREE0(msg)                        WSF_TRACE(FREE, WSF, msg)
#define WSF_TRACE_FREE1(msg, var1)                  WSF_TRACE(FREE, WSF, msg, var1)
#define WSF_TRACE_FREE2(msg, var1, var2)            WSF_TRACE(FREE, WSF, msg, var1, var2)
#define WSF_TRACE_FREE3(msg, var1, var2, var3)      WSF_TRACE(FREE, WSF, msg, var1, var2, var3)
#define WSF_TRACE_MSG0(msg)                         WSF_TRACE(MSG, WSF, msg)
#define WSF_TRACE_MSG1(msg, var1)                   WSF_TRACE(MSG, WSF, msg, var1)
#define WSF_TRACE_MSG2(msg, var1, var2)             WSF_TRACE(MSG, WSF, msg, var1, var2)
#define WSF_TRACE_MSG3(msg, var1, var2, var3)       WSF_TRACE(MSG, WSF, msg, var1, var2, var3)

#define HCI_TRACE_INFO0(msg)                        WSF_TRACE(INFO, HCI, msg)
#define HCI_TRACE_INFO1(msg, var1)                  WSF_TRACE(INFO, HCI, msg, var1)
#define HCI_TRACE_INFO2(msg, var1, var2)            WSF_TRACE(INFO, HCI, msg, var1, var2)
#define HCI_TRACE_INFO3(msg, var1, var2, var3)      WSF_TRACE(INFO, HCI, msg, var1, var2, var3)
#define HCI_TRACE_WARN0(msg)                        WSF_TRACE(WARN, HCI, msg)
#define HCI_TRACE_WARN1(msg, var1)                  WSF_TRACE(WARN, HCI, msg, var1)
#define HCI_TRACE_WARN2(msg, var1, var2)            WSF_TRACE(WARN, HCI, msg, var1, var2)
#define HCI_TRACE_WARN3(msg, var1, var2, var3)      WSF_TRACE(WARN, HCI, msg, var1, var2, var3)
#define HCI_TRACE_ERR0(msg)                         WSF_TRACE(ERR, HCI, msg)
#define HCI_TRACE_ERR1(msg, var1)                   WSF_TRACE(ERR, HCI, msg, var1)
#define HCI_TRACE_ERR2(msg, var1, var2)             WSF_TRACE(ERR, HCI, msg, var1, var2)
#define HCI_TRACE_ERR3(msg, var1, var2, var3)       WSF_TRACE(ERR, HCI, msg, var1, var2, var3)

#define HCI_PDUMP_CMD(len, pBuf)                    WsfPDump(WSF_PDUMP_TYPE_HCI_CMD, len, pBuf)
#define HCI_PDUMP_EVT(len, pBuf)                    WsfPDump(WSF_PDUMP_TYPE_HCI_EVT, len, pBuf)
#define HCI_PDUMP_TX_ACL(len, pBuf)                 WsfPDump(WSF_PDUMP_TYPE_HCI_TX_ACL, len, pBuf)
#define HCI_PDUMP_RX_ACL(len, pBuf)                 WsfPDump(WSF_PDUMP_TYPE_HCI_RX_ACL, len, pBuf)
#define HCI_PDUMP_TX_ISO(len, pBuf)                 WsfPDump(WSF_PDUMP_TYPE_HCI_TX_ISO, len, pBuf)
#define HCI_PDUMP_RX_ISO(len, pBuf)                 WsfPDump(WSF_PDUMP_TYPE_HCI_RX_ISO, len, pBuf)

#define DM_TRACE_INFO0(msg)                         WSF_TRACE(INFO, DM, msg)
#define DM_TRACE_INFO1(msg, var1)                   WSF_TRACE(INFO, DM, msg, var1)
#define DM_TRACE_INFO2(msg, var1, var2)             WSF_TRACE(INFO, DM, msg, var1, var2)
#define DM_TRACE_INFO3(msg, var1, var2, var3)       WSF_TRACE(INFO, DM, msg, var1, var2, var3)
#define DM_TRACE_WARN0(msg)                         WSF_TRACE(WARN, DM, msg)
#define DM_TRACE_WARN1(msg, var1)                   WSF_TRACE(WARN, DM, msg, var1)
#define DM_TRACE_WARN2(msg, var1, var2)             WSF_TRACE(WARN, DM, msg, var1, var2)
#define DM_TRACE_WARN3(msg, var1, var2, var3)       WSF_TRACE(WARN, DM, msg, var1, var2, var3)
#define DM_TRACE_ERR0(msg)                          WSF_TRACE(ERR, DM, msg)
#define DM_TRACE_ERR1(msg, var1)                    WSF_TRACE(ERR, DM, msg, var1)
#define DM_TRACE_ERR2(msg, var1, var2)              WSF_TRACE(ERR, DM, msg, var1, var2)
#define DM_TRACE_ERR3(msg, var1, var2, var3)        WSF_TRACE(ERR, DM, msg, var1, var2, var3)
#define DM_TRACE_ALLOC0(msg)                        WSF_TRACE(ALLOC, DM, msg)
#define DM_TRACE_ALLOC1(msg, var1)                  WSF_TRACE(ALLOC, DM, msg, var1)
#define DM_TRACE_ALLOC2(msg, var1, var2)            WSF_TRACE(ALLOC, DM, msg, var1, var2)
#define DM_TRACE_ALLOC3(msg, var1, var2, var3)      WSF_TRACE(ALLOC, DM, msg, var1, var2, var3)
#define DM_TRACE_FREE0(msg)                         WSF_TRACE(FREE, DM, msg)
#define DM_TRACE_FREE1(msg, var1)                   WSF_TRACE(FREE, DM, msg, var1)
#define DM_TRACE_FREE2(msg, var1, var2)             WSF_TRACE(FREE, DM, msg, var1, var2)
#define DM_TRACE_FREE3(msg, var1, var2, var3)       WSF_TRACE(FREE, DM, msg, var1, var2, var3)

#define L2C_TRACE_INFO0(msg)                        WSF_TRACE(INFO, L2C, msg)
#define L2C_TRACE_INFO1(msg, var1)                  WSF_TRACE(INFO, L2C, msg, var1)
#define L2C_TRACE_INFO2(msg, var1, var2)            WSF_TRACE(INFO, L2C, msg, var1, var2)
#define L2C_TRACE_INFO3(msg, var1, var2, var3)      WSF_TRACE(INFO, L2C, msg, var1, var2, var3)
#define L2C_TRACE_WARN0(msg)                        WSF_TRACE(WARN, L2C, msg)
#define L2C_TRACE_WARN1(msg, var1)                  WSF_TRACE(WARN, L2C, msg, var1)
#define L2C_TRACE_WARN2(msg, var1, var2)            WSF_TRACE(WARN, L2C, msg, var1, var2)
#define L2C_TRACE_WARN3(msg, var1, var2, var3)      WSF_TRACE(WARN, L2C, msg, var1, var2, var3)
#define L2C_TRACE_ERR0(msg)                         WSF_TRACE(ERR, L2C, msg)
#define L2C_TRACE_ERR1(msg, var1)                   WSF_TRACE(ERR, L2C, msg, var1)
#define L2C_TRACE_ERR2(msg, var1, var2)             WSF_TRACE(ERR, L2C, msg, var1, var2)
#define L2C_TRACE_ERR3(msg, var1, var2, var3)       WSF_TRACE(ERR, L2C, msg, var1, var2, var3)

#define ATT_TRACE_INFO0(msg)                        WSF_TRACE(INFO, ATT, msg)
#define ATT_TRACE_INFO1(msg, var1)                  WSF_TRACE(INFO, ATT, msg, var1)
#define ATT_TRACE_INFO2(msg, var1, var2)            WSF_TRACE(INFO, ATT, msg, var1, var2)
#define ATT_TRACE_INFO3(msg, var1, var2, var3)      WSF_TRACE(INFO, ATT, msg, var1, var2, var3)
#define ATT_TRACE_WARN0(msg)                        WSF_TRACE(WARN, ATT, msg)
#define ATT_TRACE_WARN1(msg, var1)                  WSF_TRACE(WARN, ATT, msg, var1)
#define ATT_TRACE_WARN2(msg, var1, var2)            WSF_TRACE(WARN, ATT, msg, var1, var2)
#define ATT_TRACE_WARN3(msg, var1, var2, var3)      WSF_TRACE(WARN, ATT, msg, var1, var2, var3)
#define ATT_TRACE_ERR0(msg)                         WSF_TRACE(ERR, ATT, msg)
#define ATT_TRACE_ERR1(msg, var1)                   WSF_TRACE(ERR, ATT, msg, var1)
#define ATT_TRACE_ERR2(msg, var1, var2)             WSF_TRACE(ERR, ATT, msg, var1, var2)
#define ATT_TRACE_ERR3(msg, var1, var2, var3)       WSF_TRACE(ERR, ATT, msg, var1, var2, var3)

#define EATT_TRACE_INFO0(msg)                       WSF_TRACE(INFO, EATT, msg)
#define EATT_TRACE_INFO1(msg, var1)                 WSF_TRACE(INFO, EATT, msg, var1)
#define EATT_TRACE_INFO2(msg, var1, var2)           WSF_TRACE(INFO, EATT, msg, var1, var2)
#define EATT_TRACE_INFO3(msg, var1, var2, var3)     WSF_TRACE(INFO, EATT, msg, var1, var2, var3)
#define EATT_TRACE_WARN0(msg)                       WSF_TRACE(WARN, EATT, msg)
#define EATT_TRACE_WARN1(msg, var1)                 WSF_TRACE(WARN, EATT, msg, var1)
#define EATT_TRACE_WARN2(msg, var1, var2)           WSF_TRACE(WARN, EATT, msg, var1, var2)
#define EATT_TRACE_WARN3(msg, var1, var2, var3)     WSF_TRACE(WARN, EATT, msg, var1, var2, var3)
#define EATT_TRACE_ERR0(msg)                        WSF_TRACE(ERR, EATT, msg)
#define EATT_TRACE_ERR1(msg, var1)                  WSF_TRACE(ERR, EATT, msg, var1)
#define EATT_TRACE_ERR2(msg, var1, var2)            WSF_TRACE(ERR, EATT, msg, var1, var2)
#define EATT_TRACE_ERR3(msg, var1, var2, var3)      WSF_TRACE(ERR, EATT, msg, var1, var2, var3)

#define SMP_TRACE_INFO0(msg)                        WSF_TRACE(INFO, SMP, msg)
#define SMP_TRACE_INFO1(msg, var1)                  WSF_TRACE(INFO, SMP, msg, var1)
#define SMP_TRACE_INFO2(msg, var1, var2)            WSF_TRACE(INFO, SMP, msg, var1, var2)
#define SMP_TRACE_INFO3(msg, var1, var2, var3)      WSF_TRACE(INFO, SMP, msg, var1, var2, var3)
#define SMP_TRACE_WARN0(msg)                        WSF_TRACE(WARN, SMP, msg)
#define SMP_TRACE_WARN1(msg, var1)                  WSF_TRACE(WARN, SMP, msg, var1)
#define SMP_TRACE_WARN2(msg, var1, var2)            WSF_TRACE(WARN, SMP, msg, var1, var2)
#define SMP_TRACE_WARN3(msg, var1, var2, var3)      WSF_TRACE(WARN, SMP, msg, var1, var2, var3)
#define SMP_TRACE_ERR0(msg)                         WSF_TRACE(ERR, SMP, msg)
#define SMP_TRACE_ERR1(msg, var1)                   WSF_TRACE(ERR, SMP, msg, var1)
#define SMP_TRACE_ERR2(msg, var1, var2)             WSF_TRACE(ERR, SMP, msg, var1, var2)
#define SMP_TRACE_ERR3(msg, var1, var2, var3)       WSF_TRACE(ERR, SMP, msg, var1, var2, var3)

#define APP_TRACE_INFO0(msg)                        WSF_TRACE(INFO, APP, msg)
#define APP_TRACE_INFO1(msg, var1)                  WSF_TRACE(INFO, APP, msg, var1)
#define APP_TRACE_INFO2(msg, var1, var2)            WSF_TRACE(INFO, APP, msg, var1, var2)
#define APP_TRACE_INFO3(msg, var1, var2, var3)      WSF_TRACE(INFO, APP, msg, var1, var2, var3)
#define APP_TRACE_INFO4(msg, var1, var2, var3, var4) \
                                                    WSF_TRACE(INFO, APP, msg, var1, var2, var3, var4)
#define APP_TRACE_INFO5(msg, var1, var2, var3, var4, var5) \
                                                    WSF_TRACE(INFO, APP, msg, var1, var2, var3, var4, var5)
#define APP_TRACE_INFO6(msg, var1, var2, var3, var4, var5, var6) \
                                                    WSF_TRACE(INFO, APP, msg, var1, var2, var3, var4, var5, var6)
#define APP_TRACE_INFO7(msg, var1, var2, var3, var4, var5, var6, var7) \
                                                    WSF_TRACE(INFO, APP, msg, var1, var2, var3, var4, var5, var6, var7)
#define APP_TRACE_INFO8(msg, var1, var2, var3, var4, var5, var6, var7, var8) \
                                                    WSF_TRACE(INFO, APP, msg, var1, var2, var3, var4, var5, var6, var7, var8)
#define APP_TRACE_INFO9(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9) \
                                                    WSF_TRACE(INFO, APP, msg, var1, var2, var3, var4, var5, var6, var7, var8, var9)
#define APP_TRACE_INFO12(msg, var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12) \
                                                    WSF_TRACE(INFO, APP, msg, var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12)
#define APP_TRACE_WARN0(msg)                        WSF_TRACE(WARN, APP, msg)
#define APP_TRACE_WARN1(msg, var1)                  WSF_TRACE(WARN, APP, msg, var1)
#define APP_TRACE_WARN2(msg, var1, var2)            WSF_TRACE(WARN, APP, msg, var1, var2)
#define APP_TRACE_WARN3(msg, var1, var2, var3)      WSF_TRACE(WARN, APP, msg, var1, var2, var3)
#define APP_TRACE_ERR0(msg)                         WSF_TRACE(ERR, APP, msg)
#define APP_TRACE_ERR1(msg, var1)                   WSF_TRACE(ERR, APP, msg, var1)
#define APP_TRACE_ERR2(msg, var1, var2)             WSF_TRACE(ERR, APP, msg, var1, var2)
#define APP_TRACE_ERR3(msg, var1, var2, var3)       WSF_TRACE(ERR, APP, msg, var1, var2, var3)

#define LL_TRACE_INFO0(msg)                         WSF_TRACE(INFO, LL, msg)
#define LL_TRACE_INFO1(msg, var1)                   WSF_TRACE(INFO, LL, msg, var1)
#define LL_TRACE_INFO2(msg, var1, var2)             WSF_TRACE(INFO, LL, msg, var1, var2)
#define LL_TRACE_INFO3(msg, var1, var2, var3)       WSF_TRACE(INFO, LL, msg, var1, var2, var3)

/*! \brief 0 argument MESH info trace. */
#define MESH_TRACE_INFO0(msg)                       WSF_TRACE(INFO, MESH, msg)
/*! \brief 1 argument MESH info trace. */
#define MESH_TRACE_INFO1(msg, var1)                 WSF_TRACE(INFO, MESH, msg, var1)
/*! \brief 2 argument MESH info trace. */
#define MESH_TRACE_INFO2(msg, var1, var2)           WSF_TRACE(INFO, MESH, msg, var1, var2)
/*! \brief 3 argument MESH info trace. */
#define MESH_TRACE_INFO3(msg, var1, var2, var3)     WSF_TRACE(INFO, MESH, msg, var1, var2, var3)

/*! \brief 0 argument MESH warning trace. */
#define MESH_TRACE_WARN0(msg)                       WSF_TRACE(WARN, MESH, msg)
/*! \brief 1 argument MESH warning trace. */
#define MESH_TRACE_WARN1(msg, var1)                 WSF_TRACE(WARN, MESH, msg, var1)
/*! \brief 2 argument MESH warning trace. */
#define MESH_TRACE_WARN2(msg, var1, var2)           WSF_TRACE(WARN, MESH, msg, var1, var2)
/*! \brief 3 argument MESH warning trace. */
#define MESH_TRACE_WARN3(msg, var1, var2, var3)     WSF_TRACE(WARN, MESH, msg, var1, var2, var3)
/*! \brief 0 argument MESH error trace. */
#define MESH_TRACE_ERR0(msg)                        WSF_TRACE(ERR, MESH,  msg)
/*! \brief 1 argument MESH error trace. */
#define MESH_TRACE_ERR1(msg, var1)                  WSF_TRACE(ERR, MESH,  msg, var1)
/*! \brief 2 argument MESH error trace. */
#define MESH_TRACE_ERR2(msg, var1, var2)            WSF_TRACE(ERR, MESH,  msg, var1, var2)
/*! \brief 3 argument MESH error trace. */
#define MESH_TRACE_ERR3(msg, var1, var2, var3)      WSF_TRACE(ERR, MESH,  msg, var1, var2, var3)

/*! \brief 0 argument MMDL info trace. */
#define MMDL_TRACE_INFO0(msg)                       WSF_TRACE(INFO, MMDL, msg)
/*! \brief 1 argument MMDL info trace. */
#define MMDL_TRACE_INFO1(msg, var1)                 WSF_TRACE(INFO, MMDL, msg, var1)
/*! \brief 2 argument MMDL info trace. */
#define MMDL_TRACE_INFO2(msg, var1, var2)           WSF_TRACE(INFO, MMDL, msg, var1, var2)
/*! \brief 3 argument MMDL info trace. */
#define MMDL_TRACE_INFO3(msg, var1, var2, var3)     WSF_TRACE(INFO, MMDL, msg, var1, var2, var3)
/*! \brief 0 argument MMDL warning trace. */
#define MMDL_TRACE_WARN0(msg)                       WSF_TRACE(WARN, MMDL, msg)
/*! \brief 1 argument MMDL warning trace. */
#define MMDL_TRACE_WARN1(msg, var1)                 WSF_TRACE(WARN, MMDL, msg, var1)
/*! \brief 2 argument MMDL warning trace. */
#define MMDL_TRACE_WARN2(msg, var1, var2)           WSF_TRACE(WARN, MMDL, msg, var1, var2)
/*! \brief 3 argument MMDL warning trace. */
#define MMDL_TRACE_WARN3(msg, var1, var2, var3)     WSF_TRACE(WARN, MMDL, msg, var1, var2, var3)
/*! \brief 0 argument MMDL error trace. */
#define MMDL_TRACE_ERR0(msg)                        WSF_TRACE(ERR, MMDL,  msg)
/*! \brief 1 argument MMDL error trace. */
#define MMDL_TRACE_ERR1(msg, var1)                  WSF_TRACE(ERR, MMDL,  msg, var1)
/*! \brief 2 argument MMDL error trace. */
#define MMDL_TRACE_ERR2(msg, var1, var2)            WSF_TRACE(ERR, MMDL,  msg, var1, var2)
/*! \brief 3 argument MMDL error trace. */
#define MMDL_TRACE_ERR3(msg, var1, var2, var3)      WSF_TRACE(ERR, MMDL,  msg, var1, var2, var3)


/**************************************************************************************************
  Types
**************************************************************************************************/

/*! Trace types */
typedef enum
{
  WSF_TRACE_TYPE_ERR      = (1 << 0),
  WSF_TRACE_TYPE_WARN     = (1 << 1),
  WSF_TRACE_TYPE_INFO     = (1 << 2),
  WSF_TRACE_TYPE_MSG      = (1 << 3),
  WSF_TRACE_TYPE_ALLOC    = (1 << 4),
  WSF_TRACE_TYPE_FREE     = (1 << 5),
  WSF_TRACE_TYPE_NONE     = 0,
  WSF_TRACE_TYPE_ALL      = 0xFFFF
} wsfTraceType_t;

/*! Subsystems */
typedef enum
{
  WSF_TRACE_SUBSYS_WSF,
  WSF_TRACE_SUBSYS_HCI,
  WSF_TRACE_SUBSYS_DM,
  WSF_TRACE_SUBSYS_L2C,
  WSF_TRACE_SUBSYS_ATT,
  WSF_TRACE_SUBSYS_EATT,
  WSF_TRACE_SUBSYS_SMP,
  WSF_TRACE_SUBSYS_SCR,
  WSF_TRACE_SUBSYS_APP,
  WSF_TRACE_SUBSYS_LL,
  WSF_TRACE_SUBSYS_MESH,
  WSF_TRACE_SUBSYS_MMDL,
  WSF_TRACE_SUBSYS_MAX,
} wsfTraceSubsys_t;

/*! Protocol types */
typedef enum
{
  WSF_PDUMP_TYPE_HCI_CMD    = (1 << 0),
  WSF_PDUMP_TYPE_HCI_EVT    = (1 << 1),
  WSF_PDUMP_TYPE_HCI_TX_ACL = (1 << 2),
  WSF_PDUMP_TYPE_HCI_RX_ACL = (1 << 3),
  WSF_PDUMP_TYPE_HCI_TX_ISO = (1 << 4),
  WSF_PDUMP_TYPE_HCI_RX_ISO = (1 << 5),
  WSF_PDUMP_TYPE_NONE       = 0,
  WSF_PDUMP_TYPE_ALL        = 0xFFFF
} wsfPDumpType_t;

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

void WsfSetTraceTypeFilter(wsfTraceSubsys_t subsys, wsfTraceType_t ttypeMask);
void WsfSetPDumpTypeFilter(wsfPDumpType_t ttypeMask);
void WsfTrace(wsfTraceType_t ttype, wsfTraceSubsys_t subsys, const char *pMsg, ...);
void WsfPDump(wsfPDumpType_t pdType, uint16_t length, uint8_t *pBuffer);

#ifdef __cplusplus
};
#endif

#endif /* WSF_TRACE_H */
