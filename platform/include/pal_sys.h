/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  System hooks.
 *
 *  Copyright (c) 2016-2019 Arm Ltd.
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

#ifndef PAL_SYS_H
#define PAL_SYS_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup PAL_SYS
 *  \{ */

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
void PalSysAssertTrap(void);
void PalSysSetTrap(bool_t enable);
uint32_t PalSysGetAssertCount(void);
uint32_t PalSysGetStackUsage(void);
void PalSysSleep(void);
bool_t PalSysIsBusy(void);
void PalSysSetBusy(void);
void PalSysSetIdle(void);
void PalSysInit(void);

/*! \} */    /* PAL_SYS */

#ifdef __cplusplus
};
#endif

#endif  /* PAL_SYS_H */
