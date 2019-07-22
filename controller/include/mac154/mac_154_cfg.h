/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 constant definitions.
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

#ifndef MAC_154_CFG_H
#define MAC_154_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  MAC options
**************************************************************************************************/

#define MAC_154_OPT_DISASSOC      1   /* Enable only if disassociation support required */
#define MAC_154_OPT_ORPHAN        1   /* Enable only if orphan scan/realignment support required */
#define MAC_154_PPL_TEST          1   /* Enable PPL testing */

#ifdef __cplusplus
};
#endif

#endif /* MAC_154_CFG_H */
