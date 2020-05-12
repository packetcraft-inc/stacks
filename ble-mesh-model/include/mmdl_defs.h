/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Model specification definitions.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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
 *
 */
/*************************************************************************************************/

#ifndef MMDL_DEFS_H
#define MMDL_DEFS_H

#include "mmdl_opcodes.h"
#include "mesh_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Recommended minimum value for the random delay used in sending status responses */
#define MMDL_STATUS_RSP_MIN_SEND_DELAY_MS                MESH_ACC_RSP_MIN_SEND_DELAY_MS

/*! \brief Recommended maximum value for the random delay used in sending status responses */
#define MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(unicast)       MESH_ACC_RSP_MAX_SEND_DELAY_MS(unicast)

/*! \brief Health Server SIG model identifier */
#define MESH_HT_SR_MDL_ID                                0x0002

/*! \brief Health Client SIG model identifier */
#define MESH_HT_CL_MDL_ID                                0x0003

/*! \brief Generic OnOff Server SIG model identifier */
#define MMDL_GEN_ONOFF_SR_MDL_ID                         0x1000

/*! \brief Generic OnOff Client SIG model identifier */
#define MMDL_GEN_ONOFF_CL_MDL_ID                         0x1001

/*! \brief Generic Level Server SIG model identifier */
#define MMDL_GEN_LEVEL_SR_MDL_ID                         0x1002

/*! \brief Generic Level Client SIG model identifier */
#define MMDL_GEN_LEVEL_CL_MDL_ID                         0x1003

/*! \brief Generic Level Server SIG model identifier */
#define MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID                 0x1004

/*! \brief Generic Default Transition Client SIG model identifier */
#define MMDL_GEN_DEFAULT_TRANS_CL_MDL_ID                 0x1005

/*! \brief Generic Power OnOff Server SIG model identifier */
#define MMDL_GEN_POWER_ONOFF_SR_MDL_ID                   0x1006

/*! \brief Generic Power OnOff Setup Server SIG model identifier */
#define MMDL_GEN_POWER_ONOFFSETUP_SR_MDL_ID              0x1007

/*! \brief Generic Power OnOff Client SIG model identifier */
#define MMDL_GEN_POWER_ONOFF_CL_MDL_ID                   0x1008

/*! \brief Generic Power Level Server SIG model identifier */
#define MMDL_GEN_POWER_LEVEL_SR_MDL_ID                   0x1009

/*! \brief Generic Power Level Server Setup SIG model identifier */
#define MMDL_GEN_POWER_LEVELSETUP_SR_MDL_ID              0x100A

/*! \brief Generic Power Level Client SIG model identifier */
#define MMDL_GEN_POWER_LEVEL_CL_MDL_ID                   0x100B

/*! \brief Generic Battery Server SIG model identifier */
#define MMDL_GEN_BATTERY_SR_MDL_ID                       0x100C

/*! \brief Generic Battery Client SIG model identifier */
#define MMDL_GEN_BATTERY_CL_MDL_ID                       0x100D

/*! \brief Light Lightness Server SIG model identifier */
#define MMDL_LIGHT_LIGHTNESS_SR_MDL_ID                   0x1300

/*! \brief Light Lightness Server Setup SIG model identifier */
#define MMDL_LIGHT_LIGHTNESSSETUP_SR_MDL_ID              0x1301

/*! \brief Light Lightness Client SIG model identifier */
#define MMDL_LIGHT_LIGHTNESS_CL_MDL_ID                   0x1302

/*! \brief Time Server SIG model identifier */
#define MMDL_TIME_SR_MDL_ID                              0x1200

/*! \brief Time Setup Server SIG model identifier */
#define MMDL_TIMESETUP_SR_MDL_ID                         0x1201

/*! \brief Time Client SIG model identifier */
#define MMDL_TIME_CL_MDL_ID                              0x1202

/*! \brief Vendor Test Server model identifier */
#define MMDL_VENDOR_TEST_SR_MDL_ID                       0x10001234

/*! \brief Vendor Test Client model identifier */
#define MMDL_VENDOR_TEST_CL_MDL_ID                       0x10011234

/*! \brief Size of the opcodes used by Generic OnOff */
#define MMDL_GEN_ONOFF_OPCODES_SIZE                      2

/*! \brief Number of supported commands on the On Off Server */
#define MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES               3

/*! \brief Number of supported commands on the On Off Client */
#define MMDL_GEN_ONOFF_CL_NUM_RCVD_OPCODES               1

/*! \brief Generic OnOff Status Message Maximum Length */
#define MMDL_GEN_ONOFF_STATUS_MAX_LEN                    3

/*! \brief Generic OnOff Status Message Minimum Length */
#define MMDL_GEN_ONOFF_STATUS_MIN_LEN                    2

/*! \brief Generic OnOff Set Message Maximum Length */
#define MMDL_GEN_ONOFF_SET_MAX_LEN                       4

/*! \brief Generic OnOff Set Message Minimum Length */
#define MMDL_GEN_ONOFF_SET_MIN_LEN                       2

/*! \brief Number of supported commands on the Power On Off Server */
#define MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES         1

/*! \brief Number of supported commands on the Power On Off Setup Server */
#define MMDL_GEN_POWER_ONOFFSETUP_SR_NUM_RCVD_OPCODES    2

/*! \brief Number of supported commands on the Power On Off Client */
#define MMDL_GEN_POWER_ONOFF_CL_NUM_RCVD_OPCODES         1

/*! \brief Size of the opcodes used by Generic Power On Off Client */
#define MMDL_GEN_POWER_ONOFF_OPCODES_SIZE                2

/*! \brief Size of the Generic Power On Off messages */
#define MMDL_GEN_POWER_ONOFF_MSG_LEN                     1

/*! \brief Generic Level Status Message Maximum Length */
#define MMDL_GEN_LEVEL_STATUS_MAX_LEN                    5

/*! \brief Generic Level Status Message Minimum Length */
#define MMDL_GEN_LEVEL_STATUS_MIN_LEN                    2

/*! \brief Generic Level Set Message Maximum Length */
#define MMDL_GEN_LEVEL_SET_MAX_LEN                       5

/*! \brief Generic Level Set Message Minimum Length */
#define MMDL_GEN_LEVEL_SET_MIN_LEN                       3

/*! \brief Generic Level Set Message TID index */
#define MMDL_GEN_LEVEL_SET_TID_IDX                       2

/*! \brief Generic Level Set Message TID index */
#define MMDL_GEN_LEVEL_SET_TRANSITION_IDX                3

/*! \brief Generic Level Set Message TID index */
#define MMDL_GEN_LEVEL_SET_DELAY_IDX                     4

/*! \brief Size of the opcodes used by Generic Level */
#define MMDL_GEN_LEVEL_OPCODES_SIZE                      2

/*! \brief Maximum delta level value. */
#define MMDL_GEN_LEVEL_MAX_SIGNED_LEVEL                  0x7FFF

/*! \brief Minimum delta level value. */
#define MMDL_GEN_LEVEL_MIN_SIGNED_LEVEL                  0x8000

/*! \brief Maximum Generic Level Status message length */
#define MMDL_GEN_LEVEL_MAX_STATUS_LENGTH                 5

/*! \brief Generic Default Transition Message Length */
#define MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH                1

/*! \brief Size of the opcodes used by Generic Default Transition */
#define MMDL_GEN_DEFAULT_TRANS_OPCODES_SIZE              2

/*! \brief Number of supported commands on the Generic Default Transition Server */
#define MMDL_GEN_DEFAULT_TRANS_SR_NUM_RCVD_OPCODES       3

/*! \brief Number of supported commands on the Generic Default Transition Client */
#define MMDL_GEN_DEFAULT_TRANS_CL_NUM_RCVD_OPCODES       2

/*! \brief Generic Level Delta Set Message Maximum Length */
#define MMDL_GEN_LEVEL_DELTA_SET_MAX_LEN                 7

/*! \brief Generic Level Delta Set Message Minimum Length */
#define MMDL_GEN_LEVEL_DELTA_SET_MIN_LEN                 5

/*! \brief Generic Level Delta Set Message TID index */
#define MMDL_GEN_LEVEL_DELTA_SET_TID_IDX                 4

/*! \brief Generic Level Delta Set Message TID index */
#define MMDL_GEN_LEVEL_DELTA_SET_TRANSITION_IDX          5

/*! \brief Generic Level Delta Set Message TID index */
#define MMDL_GEN_LEVEL_DELTA_SET_DELAY_IDX               6

/*! \brief Size of the opcodes used by Generic Level */
#define MMDL_GEN_LEVEL_OPCODES_SIZE                      2

/*! \brief Number of supported commands on the Generic Level Server */
#define MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES               7

/*! \brief Number of supported commands on the Generic Level Client */
#define MMDL_GEN_LEVEL_CL_NUM_RCVD_OPCODES               1

/*! \brief Number of supported commands on the Generic Power Level Client */
#define MMDL_GEN_POWER_LEVEL_CL_NUM_RCVD_OPCODES         4

/*! \brief Number of supported commands on the Generic Power Level Server */
#define MMDL_GEN_POWER_LEVEL_SR_NUM_RCVD_OPCODES         6

/*! \brief Number of supported commands on the Generic Power Level Setup Server */
#define MMDL_GEN_POWER_LEVELSETUP_SR_NUM_RCVD_OPCODES    4

/*! \brief Size of the opcodes used by Generic Power Level */
#define MMDL_GEN_POWER_LEVEL_OPCODES_SIZE                2

/*! \brief Generic Power Level Status Message Minimum Length */
#define MMDL_GEN_POWER_LEVEL_STATUS_MIN_LEN              2

/*! \brief Generic Power Level Status Message Maximum Length */
#define MMDL_GEN_POWER_LEVEL_STATUS_MAX_LEN              5

/*! \brief Generic Power Last Status Message Length */
#define MMDL_GEN_POWERLAST_STATUS_LEN                    2

/*! \brief Generic Power Default Status Message Length */
#define MMDL_GEN_POWERDEFAULT_STATUS_LEN                 MMDL_GEN_POWERLAST_STATUS_LEN

/*! \brief Generic Power Range Status Message Length */
#define MMDL_GEN_POWERRANGE_STATUS_LEN                   5

/*! \brief Generic Power Level Set Message Minimum Length */
#define MMDL_GEN_POWER_LEVEL_SET_MIN_LEN                 3

/*! \brief Generic Power Level Set Message Maximum Length */
#define MMDL_GEN_POWER_LEVEL_SET_MAX_LEN                 MMDL_GEN_POWER_LEVEL_STATUS_MAX_LEN

/*! \brief Generic Power Default Set Message Length */
#define MMDL_GEN_POWERDEFAULT_SET_LEN                    2

/*! \brief Generic Power Range Set Message Length */
#define MMDL_GEN_POWERRANGE_SET_LEN                      4

/*! \brief Generic Power Range Status Message Length */
#define MMDL_GEN_POWERRANGE_STATUS_LEN                   5

/*! \brief Generic Power Range Minimum Length */
#define MMDL_GEN_POWERRANGE_MIN                          1

/*! \brief Generic Power Range Maximum Length */
#define MMDL_GEN_POWERRANGE_MAX                          0xFFFF

/*! \brief Generic Battery Status Message Length */
#define MMDL_GEN_BATTERY_STATUS_LENGTH                   8

/*! \brief Size of the opcodes used by Generic Battery */
#define MMDL_GEN_BATTERY_OPCODES_SIZE                    2

/*! \brief Number of supported commands on the Generic Battery Server */
#define MMDL_GEN_BATTERY_SR_NUM_RCVD_OPCODES             1

/*! \brief Number of supported commands on the Generic Battery Client */
#define MMDL_GEN_BATTERY_CL_NUM_RCVD_OPCODES             1

/*! \brief Time Set Message Length */
#define MMDL_TIME_SET_LENGTH                             10

/*! \brief Time Status Message Max Length */
#define MMDL_TIME_STATUS_MAX_LENGTH                      MMDL_TIME_SET_LENGTH

/*! \brief Time Status Message Min Length */
#define MMDL_TIME_STATUS_MIN_LENGTH                      5

/*! \brief Time Role Set Message Length */
#define MMDL_TIMEROLE_SET_LENGTH                         1

/*! \brief Time Role Status Message Length */
#define MMDL_TIMEROLE_STATUS_LENGTH                      MMDL_TIMEROLE_SET_LENGTH

/*! \brief Time Zone Set Message Length */
#define MMDL_TIMEZONE_SET_LENGTH                         6

/*! \brief Time Zone Status Message Length */
#define MMDL_TIMEZONE_STATUS_LENGTH                      (MMDL_TIMEZONE_SET_LENGTH + 1)

/*! \brief Time TAI-UTC Set Message Length */
#define MMDL_TIMEDELTA_SET_LENGTH                        7

/*! \brief Time TAI-UTC Delta Status Message Length */
#define MMDL_TIMEDELTA_STATUS_LENGTH                     (MMDL_TIMEDELTA_SET_LENGTH + 2)

/*! \brief Size of the opcodes used by Generic Battery */
#define MMDL_TIME_OPCODES_SIZE                           2

/*! \brief Number of supported commands on the Time Server */
#define MMDL_TIME_SR_NUM_RCVD_OPCODES                    4

/*! \brief Number of supported commands on the Time Setup Server */
#define MMDL_TIME_SETUP_SR_NUM_RCVD_OPCODES              5

/*! \brief Number of supported commands on the Time Client */
#define MMDL_TIME_CL_NUM_RCVD_OPCODES                    4

/*! \brief Generic OnOff Status Message Maximum Length */
#define MMDL_GEN_ONOFF_STATUS_MAX_LEN                    3

/*! \brief Generic OnOff Set Message Maximum Length */
#define MMDL_GEN_ONOFF_SET_MAX_LEN                       4

/*! \brief Generic OnOff Set Message Minimum Length */
#define MMDL_GEN_ONOFF_SET_MIN_LEN                       2

/*! \brief Generic OnOff Set Message TID index */
#define MMDL_GEN_ONOFF_SET_TID_IDX                       1

/*! \brief Generic OnOff Set Message TID index */
#define MMDL_GEN_ONOFF_SET_TRANSITION_IDX                2

/*! \brief Generic OnOff Set Message TID index */
#define MMDL_GEN_ONOFF_SET_DELAY_IDX                     3

/*! \brief Generic Power OnOff Status Message Length */
#define MMDL_GEN_POWER_ONOFF_STATUS_LEN                  1

/*! \brief Generic Power OnOff Setup Set Message Length */
#define MMDL_GEN_POWER_ONOFFSETUP_SET_LEN                1

/*! \brief Maximum Transition Time for resolution 0 (100ms)  */
#define MMDL_GEN_TR_MAX_TIME_RES0_MS                     6200

/*! \brief Maximum Transition Time for resolution 1 (1s)  */
#define MMDL_GEN_TR_MAX_TIME_RES1_MS                     (MMDL_GEN_TR_MAX_TIME_RES0_MS * 10)

/*! \brief Maximum Transition Time for resolution 2 (10s)  */
#define MMDL_GEN_TR_MAX_TIME_RES2_MS                     (MMDL_GEN_TR_MAX_TIME_RES1_MS * 10)

/*! \brief Maximum Transition Time for resolution 3 (10min)  */
#define MMDL_GEN_TR_MAX_TIME_RES3_MS                     (MMDL_GEN_TR_MAX_TIME_RES2_MS * 60)

/*! \brief Transition Time resolution 0 (100ms)  */
#define MMDL_GEN_TR_TIME_RES0_MS                         100

/*! \brief Transition Time resolution 1 (1s)  */
#define MMDL_GEN_TR_TIME_RES1_MS                         (MMDL_GEN_TR_TIME_RES0_MS * 10)

/*! \brief Transition Time resolution 2 (10s)  */
#define MMDL_GEN_TR_TIME_RES2_MS                         (MMDL_GEN_TR_TIME_RES1_MS * 10)

/*! \brief Transition Time resolution 3 (10min)  */
#define MMDL_GEN_TR_TIME_RES3_MS                         (MMDL_GEN_TR_TIME_RES2_MS * 60)

/*! \brief Scenes Server SIG model identifier */
#define MMDL_SCENE_SR_MDL_ID                             0x1203

/*! \brief Scenes Setup Server SIG model identifier */
#define MMDL_SCENE_SETUP_SR_MDL_ID                       0x1204

/*! \brief Scenes Client SIG model identifier */
#define MMDL_SCENE_CL_MDL_ID                             0x1205

/*! \brief Number of supported commands on the Scenes Server */
#define MMDL_SCENE_SR_NUM_RCVD_OPCODES                   4

/*! \brief Number of supported commands on the Scenes Setup Server */
#define MMDL_SCENE_SETUP_SR_NUM_RCVD_OPCODES             4

/*! \brief Number of supported commands on the Scenes Client */
#define MMDL_SCENE_CL_NUM_RCVD_OPCODES                   2

/*! \brief Scene Recall Message Maximum Length */
#define MMDL_SCENE_RECALL_MAX_LEN                        5

/*! \brief Scene Recall Message Minimum Length */
#define MMDL_SCENE_RECALL_MIN_LEN                        3

/*! \brief Scene Status Message Maximum Length */
#define MMDL_SCENE_STATUS_MAX_LEN                        6

/*! \brief Scene Status Message Minimum Length */
#define MMDL_SCENE_STATUS_MIN_LEN                        3

/*! \brief Number of scenes that the device supports */
#define MMDL_NUM_OF_SCENES                               16

/*! \brief Size of the opcodes used by Scene models */
#define MMDL_SCENE_OPCODES_SIZE                          2

/*! \brief Scene Register Status Message Minimum Length */
#define MMDL_SCENE_REG_STATUS_MIN_LEN                    3

/*! \brief Scene Register Status Message Maximum Length */
#define MMDL_SCENE_REG_STATUS_MAX_LEN                    (3 + MMDL_NUM_OF_SCENES * 2)

/*! \brief Scene Store Message Length */
#define MMDL_SCENE_STORE_LEN                             2

/*! \brief Scene Delete Message Length */
#define MMDL_SCENE_DELETE_LEN                            2

/*! \brief Prohibited value for scene number */
#define MMDL_SCENE_NUM_PROHIBITED                        0

/*! \brief Scene Recall Message TID index */
#define MMDL_SCENE_RECALL_TID_IDX                        2

/*! \brief Scene Recall Message Transition index */
#define MMDL_SCENE_RECALL_TRANSITION_IDX                 3

/*! \brief Scene Recall Message Delay index */
#define MMDL_SCENE_RECALL_DELAY_IDX                      4

/*! \brief Size of the opcodes used by Light Lightness */
#define MMDL_LIGHT_LIGHTNESS_OPCODES_SIZE                2

/*! \brief Light Lightness Default Set Message Length */
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_LEN             2

/*! \brief Light Lightness Range Set Message Length */
#define MMDL_LIGHT_LIGHTNESS_RANGE_SET_LEN               4

/*! \brief Light Lightness Set Message Minimum Length */
#define MMDL_LIGHT_LIGHTNESS_SET_MIN_LEN                 3

/*! \brief Light Lightness Set Message Maximum Length */
#define MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN                 5

/*! \brief Light Lightness Set Message Minimum Length */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MIN_LEN          3

/*! \brief Light Lightness Set Message Maximum Length */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN          5

/*! \brief Light Lightness Status Message Minimum Length */
#define MMDL_LIGHT_LIGHTNESS_STATUS_MIN_LEN              2

/*! \brief Light Lightness Status Message Maximum Length */
#define MMDL_LIGHT_LIGHTNESS_STATUS_MAX_LEN              5

/*! \brief Light Lightness Status Message Minimum Length */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_MIN_LEN       2

/*! \brief Light Lightness Status Message Maximum Length */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_MAX_LEN       5

/*! \brief Light Lightness Last Status Message Length */
#define MMDL_LIGHT_LIGHTNESS_LAST_STATUS_LEN             2

/*! \brief Light Lightness Default Status Message Length */
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_STATUS_LEN          2

/*! \brief GLight Lightness Range Status Message Length */
#define MMDL_LIGHT_LIGHTNESS_RANGE_STATUS_LEN            5

/*! \brief Number of supported commands on the Light Lightness Client */
#define MMDL_LIGHT_LIGHTNESS_CL_NUM_RCVD_OPCODES         5

/*! \brief Number of supported commands on the Light Lightness Server */
#define MMDL_LIGHT_LIGHTNESS_SR_NUM_RCVD_OPCODES         9

/*! \brief Number of supported commands on the Light Lightness Setup Server */
#define MMDL_LIGHT_LIGHTNESSSETUP_SR_NUM_RCVD_OPCODES    4

/*! \brief Light CTL Server SIG model identifier */
#define MMDL_LIGHT_CTL_SR_MDL_ID                        0x1303

/*! \brief Light CTL Setup Server SIG model identifier */
#define MMDL_LIGHT_CTL_SETUP_SR_MDL_ID                  0x1304

/*! \brief Light CTL Client SIG model identifier */
#define MMDL_LIGHT_CTL_CL_MDL_ID                        0x1305

/*! \brief Light CTL Temperature Server SIG model identifier */
#define MMDL_LIGHT_CTL_TEMP_SR_MDL_ID                   0x1306

/*! \brief Number of supported commands on the Light CTL Client */
#define MMDL_LIGHT_CTL_CL_NUM_RCVD_OPCODES               4

/*! \brief Number of supported commands on the Light CTL Setup Server */
#define MMDL_LIGHT_CTL_SETUP_SR_NUM_RCVD_OPCODES         4

/*! \brief Number of supported commands on the Light CTL Server */
#define MMDL_LIGHT_CTL_SR_NUM_RCVD_OPCODES               5

/*! \brief Number of supported commands on the Light CTL Temperature Server */
#define MMDL_LIGHT_CTL_TEMP_SR_NUM_RCVD_OPCODES          3

/*! \brief Light CTL Status Message Minimum Length */
#define MMDL_LIGHT_CTL_STATUS_MIN_LEN                    4

/*! \brief Light CTL Status Message Maximum Length */
#define MMDL_LIGHT_CTL_STATUS_MAX_LEN                    9

/*! \brief Light CTL Set Message Minimum Length */
#define MMDL_LIGHT_CTL_SET_MIN_LEN                       7

/*! \brief Light CTL Set Message Maximum Length */
#define MMDL_LIGHT_CTL_SET_MAX_LEN                       9

/*! \brief Light CTL Temperature Set Message Minimum Length */
#define MMDL_LIGHT_CTL_TEMP_SET_MIN_LEN                  5

/*! \brief Light CTL Temperature Set Message Maximum Length */
#define MMDL_LIGHT_CTL_TEMP_SET_MAX_LEN                  7

/*! \brief Light CTL Temperature Status Message Minimum Length */
#define MMDL_LIGHT_CTL_TEMP_STATUS_MIN_LEN               4

/*! \brief Light CTL Temperature Status Message Maximum Length */
#define MMDL_LIGHT_CTL_TEMP_STATUS_MAX_LEN               9

/*! \brief Light CTL Temperature Range Set Message Length */
#define MMDL_LIGHT_CTL_TEMP_RANGE_SET_LEN                4

/*! \brief Light CTL Temperature Range Status Message Length */
#define MMDL_LIGHT_CTL_TEMP_RANGE_STATUS_LEN             5

/*! \brief Light CTL Default Set Message Length */
#define MMDL_LIGHT_CTL_DEFAULT_SET_LEN                   6

/*! \brief Light CTL Default Status Message Length */
#define MMDL_LIGHT_CTL_DEFAULT_STATUS_LEN                6

/*! \brief Light CTL Temperature State Minimum */
#define MMDL_LIGHT_CTL_TEMP_MIN                          0x0320

/*! \brief Light CTL Temperature State Maximum */
#define MMDL_LIGHT_CTL_TEMP_MAX                          0x4E20

/*! \brief Light HSL Server SIG model identifier */
#define MMDL_LIGHT_HSL_SR_MDL_ID                         0x1307

/*! \brief Light HSL Setup Server SIG model identifier */
#define MMDL_LIGHT_HSL_SETUP_SR_MDL_ID                   0x1308

/*! \brief Light HSL Client SIG model identifier */
#define MMDL_LIGHT_HSL_CL_MDL_ID                         0x1309

/*! \brief Light HSL Hue Server SIG model identifier */
#define MMDL_LIGHT_HSL_HUE_SR_MDL_ID                     0x130A

/*! \brief Light HSL Saturation Server SIG model identifier */
#define MMDL_LIGHT_HSL_SAT_SR_MDL_ID                     0x130B

/*! \brief Number of supported commands on the Light HSL Client */
#define MMDL_LIGHT_HSL_CL_NUM_RCVD_OPCODES               6

/*! \brief Number of supported commands on the Light HSL Server */
#define MMDL_LIGHT_HSL_SR_NUM_RCVD_OPCODES               6

/*! \brief Number of supported commands on the Light HSL HueServer */
#define MMDL_LIGHT_HSL_HUE_SR_NUM_RCVD_OPCODES           3

/*! \brief Number of supported commands on the Light HSL Saturation Server */
#define MMDL_LIGHT_HSL_SAT_SR_NUM_RCVD_OPCODES           3

/*! \brief Number of supported commands on the Light HSL Setup Server */
#define MMDL_LIGHT_HSL_SETUP_SR_NUM_RCVD_OPCODES         4

/*! \brief Light HSL Set Message Maximum Length */
#define MMDL_LIGHT_HSL_SET_MAX_LEN                       9

/*! \brief Light HSL Set Message Minimum Length */
#define MMDL_LIGHT_HSL_SET_MIN_LEN                       7

/*! \brief Light HSL Status Message Maximum Length */
#define MMDL_LIGHT_HSL_STATUS_MAX_LEN                    7

/*! \brief Light HSL Status Message Minimum Length */
#define MMDL_LIGHT_HSL_STATUS_MIN_LEN                    6

/*! \brief Light HSL Hue Set Message Maximum Length */
#define MMDL_LIGHT_HSL_HUE_SET_MAX_LEN                   5

/*! \brief Light HSL Hue Set Message Minimum Length */
#define MMDL_LIGHT_HSL_HUE_SET_MIN_LEN                   3

/*! \brief Light HSL Hue Status Message Maximum Length */
#define MMDL_LIGHT_HSL_HUE_STATUS_MAX_LEN                5

/*! \brief Light HSL Hue Status Message Minimum Length */
#define MMDL_LIGHT_HSL_HUE_STATUS_MIN_LEN                2

/*! \brief Light HSL Saturation Set Message Maximum Length */
#define MMDL_LIGHT_HSL_SAT_SET_MAX_LEN                   5

/*! \brief Light HSL Saturation Set Message Minimum Length */
#define MMDL_LIGHT_HSL_SAT_SET_MIN_LEN                   3

/*! \brief Light HSL Saturation Status Message Maximum Length */
#define MMDL_LIGHT_HSL_SAT_STATUS_MAX_LEN                5

/*! \brief Light HSL Saturation Status Message Minimum Length */
#define MMDL_LIGHT_HSL_SAT_STATUS_MIN_LEN                2

/*! \brief Light HSL Default Set Message Length */
#define MMDL_LIGHT_HSL_DEF_SET_LEN                       6

/*! \brief Light HSL Default Status Message Length */
#define MMDL_LIGHT_HSL_DEF_STATUS_LEN                    6

/*! \brief Light HSL Range Set Message Length */
#define MMDL_LIGHT_HSL_RANGE_SET_LEN                     8

/*! \brief Light HSL Range Status Message Length */
#define MMDL_LIGHT_HSL_RANGE_STATUS_LEN                  9

/*! \brief Scheduler Server SIG model identifier */
#define MMDL_SCHEDULER_SR_MDL_ID                         0x1206

/*! \brief Scheduler Setup Server SIG model identifier */
#define MMDL_SCHEDULER_SETUP_SR_MDL_ID                   0x1207

/*! \brief Scheduler Client SIG model identifier */
#define MMDL_SCHEDULER_CL_MDL_ID                         0x1208

/*! \brief Number of supported commands on the Scheduler Server */
#define MMDL_SCHEDULER_SR_NUM_RCVD_OPCODES               2

/*! \brief Number of supported commands on the Scheduler Setup Server */
#define MMDL_SCHEDULER_SETUP_SR_NUM_RCVD_OPCODES         2

/*! \brief Number of supported commands on the Scheduler Client */
#define MMDL_SCHEDULER_CL_NUM_RCVD_OPCODES               2

/*! \brief Scheduler Action Get Message Parameters Length */
#define MMDL_SCHEDULER_ACTION_GET_LEN                    1

/*! \brief Scheduler Action Set Message Parameters Length */
#define MMDL_SCHEDULER_ACTION_SET_LEN                    10

/*! \brief Scheduler Action Set Unacknowledged Message Parameters Length */
#define MMDL_SCHEDULER_ACTION_SET_NO_ACK_LEN             10

/*! \brief Scheduler Status Message Parameters Length */
#define MMDL_SCHEDULER_STATUS_LEN                        2

/*! \brief Scheduler Action Status Parameters Message Length */
#define MMDL_SCHEDULER_ACTION_STATUS_LEN                 10

/*! \brief Scheduler Register year value if event occurs every year */
#define MMDL_SCHEDULER_REGISTER_YEAR_ALL                 0x64

/*! \brief Checks if Scheduler Register Action is RFU */
#define MMDL_SCHEDULER_ACTION_IS_RFU(act)                (((act) > MMDL_SCHEDULER_ACTION_SCENE_RECALL) &&\
                                                          ((act) < MMDL_SCHEDULER_ACTION_NONE))

/*! \brief Maximum entry value for the Schedule Register State entry */
#define MMDL_SCHEDULER_REGISTER_ENTRY_MAX                0x0F

/** \name Scheduler action message bit length
 * Bit size for each field of the Action message parameters
 */
/**@{*/
#define MMDL_SCHEDULER_REGISTER_FIELD_INDEX_SIZE         4
#define MMDL_SCHEDULER_REGISTER_FIELD_YEAR_SIZE          7
#define MMDL_SCHEDULER_REGISTER_FIELD_MONTH_SIZE         12
#define MMDL_SCHEDULER_REGISTER_FIELD_DAY_SIZE           5
#define MMDL_SCHEDULER_REGISTER_FIELD_HOUR_SIZE          5
#define MMDL_SCHEDULER_REGISTER_FIELD_MINUTE_SIZE        6
#define MMDL_SCHEDULER_REGISTER_FIELD_SECOND_SIZE        6
#define MMDL_SCHEDULER_REGISTER_FIELD_DAYOFWEEK_SIZE     7
#define MMDL_SCHEDULER_REGISTER_FIELD_ACTION_SIZE        4
#define MMDL_SCHEDULER_REGISTER_FIELD_TRANS_TIME_SIZE    8
#define MMDL_SCHEDULER_REGISTER_FIELD_SCENE_NUM_SIZE     16
/**@}*/

/*! \brief Minimum Time To Discharge state. */
#define MMDL_GEN_BAT_MIN_TIME_TO_DISCHARGE               0x000000

/*! \brief Max Time To Discharge state. */
#define MMDL_GEN_BAT_MAX_TIME_TO_DISCHARGE               0xFFFFFE

/*! \brief Unknown Time To Discharge state. */
#define MMDL_GEN_BAT_UNKOWN_TIME_TO_DISCHARGE            0xFFFFFF

/*! \brief The model transition state update interval in milliseconds */
#ifndef MMDL_TRANSITION_STATE_UPDATE_INTERVAL
#define MMDL_TRANSITION_STATE_UPDATE_INTERVAL            1000
#endif

/*! \brief Model Generic OnOff states enumeration */
enum mmdlGenOnOffStates
{
  MMDL_GEN_ONOFF_STATE_OFF        = 0x00,  /*!< Generic OnOff - Off State */
  MMDL_GEN_ONOFF_STATE_ON         = 0x01,  /*!< Generic OnOff - On State */
  MMDL_GEN_ONOFF_STATE_PROHIBITED = 0x02   /*!< Generic OnOff - Start of Prohibited values */
};

/*! \brief Transition Number of Steps enumeration */
enum mmdlTransitionNumberOfSteps
{
  MMDL_GEN_TR_IMMEDIATE        = 0x00,  /*!< Transition Time is immediate */
  MMDL_GEN_TR_UNKNOWN          = 0x3F   /*!< Transition Time is unknown */
};

/*! \brief Transition Step Resolution enumeration */
enum mmdlTransitionStepResolution
{
  MMDL_GEN_TR_RES100MS       = 0x00,  /*!< Transition Step Resolution is 100 milliseconds */
  MMDL_GEN_TR_RES1SEC        = 0x01,  /*!< Transition Step Resolution is 1 second */
  MMDL_GEN_TR_RES10SEC       = 0x02,  /*!< Transition Step Resolution is 10 seconds */
  MMDL_GEN_TR_RES10MIN       = 0x03   /*!< Transition Step Resolution is 10 minutes */
};

/*! \brief Model Generic OnPowerUp states enumeration */
enum mmdlGenOnPowerUpStates
{
  MMDL_GEN_ONPOWERUP_STATE_OFF        = 0x00,  /*!< Generic OnPowerUp - Off State */
  MMDL_GEN_ONPOWERUP_STATE_DEFAULT    = 0x01,  /*!< Generic OnPowerUp - Default State */
  MMDL_GEN_ONPOWERUP_STATE_RESTORE    = 0x02,  /*!< Generic OnPowerUp - Restore State */
  MMDL_GEN_ONPOWERUP_STATE_PROHIBITED = 0x03   /*!< Generic OnPowerUp - Start of Prohibited values*/
};

/*! \brief Range Set status enumeration */
enum mmdlRangeStatus
{
  MMDL_RANGE_SUCCESS         = 0x00,  /*!< Success */
  MMDL_RANGE_CANNOT_SET_MIN  = 0x01,  /*!< The provided value for Range Min cannot be set */
  MMDL_RANGE_CANNOT_SET_MAX  = 0x02,  /*!< The provided value for Range Max cannot be set */
  MMDL_RANGE_PROHIBITED      = 0x03   /*!< Reserved for Future Use */
};

/*! \brief Model Time Role states enumeration */
enum mmdlTimeRoleStates
{
  MMDL_TIME_ROLE_STATE_NONE       = 0x00, /*!< Time Role - None */
  MMDL_TIME_ROLE_STATE_AUTHORITY  = 0x01, /*!< Time Role - Mesh Time Authority */
  MMDL_TIME_ROLE_STATE_RELAY      = 0x02, /*!< Time Role - Mesh Time Relay */
  MMDL_TIME_ROLE_STATE_CLIENT     = 0x03, /*!< Time Role - Mesh Time Client */
  MMDL_TIME_ROLE_STATE_PROHIBITED = 0x04  /*!< Time Role - Start of Prohibited values */
};

/*! \brief Model Scene status enumeration */
enum mmdlSceneStatus
{
  MMDL_SCENE_SUCCESS         = 0x00,  /*!< Success */
  MMDL_SCENE_REGISTER_FULL   = 0x01,  /*!< Scene register full */
  MMDL_SCENE_NOT_FOUND       = 0x02,  /*!< Scene not found */
  MMDL_SCENE_PROHIBITED      = 0x03   /*!< Reserved for Future Use */
};

/*! \brief Model Light Lightness states enumeration */
enum mmdlLightLightnessStates
{
  MMDL_LIGHT_LIGHTNESS_STATE_PROHIBITED  = 0x0000, /*!< Prohibited value */
  MMDL_LIGHT_LIGHTNESS_STATE_HIGHEST     = 0xFFFF, /*!< Highest perceived lightness value */
};

/*! \brief Model Scheduler Register State bit-field representation for the months of the year */
enum mmdlSchedulerRegisterMonthBitfieldValues
{
  MMDL_SCHEDULER_SCHED_IN_JANUARY            = 1 << 0,  /*!< Scheduled in January */
  MMDL_SCHEDULER_SCHED_IN_FEBRUARY           = 1 << 1,  /*!< Scheduled in February */
  MMDL_SCHEDULER_SCHED_IN_MARCH              = 1 << 2,  /*!< Scheduled in March */
  MMDL_SCHEDULER_SCHED_IN_APRIL              = 1 << 3,  /*!< Scheduled in April */
  MMDL_SCHEDULER_SCHED_IN_MAY                = 1 << 4,  /*!< Scheduled in May */
  MMDL_SCHEDULER_SCHED_IN_JUNE               = 1 << 5,  /*!< Scheduled in June */
  MMDL_SCHEDULER_SCHED_IN_JULY               = 1 << 6,  /*!< Scheduled in July */
  MMDL_SCHEDULER_SCHED_IN_AUGUST             = 1 << 7,  /*!< Scheduled in August */
  MMDL_SCHEDULER_SCHED_IN_SEPTEMBER          = 1 << 8,  /*!< Scheduled in September */
  MMDL_SCHEDULER_SCHED_IN_OCTOBER            = 1 << 9,  /*!< Scheduled in October */
  MMDL_SCHEDULER_SCHED_IN_NOVEMBER           = 1 << 10, /*!< Scheduled in November */
  MMDL_SCHEDULER_SCHED_IN_DECEMBER           = 1 << 11, /*!< Scheduled in December */
  MMDL_SCHEDULER_SCHED_IN_PROHIBITED_START   = 1 << 12, /*!< Scheduled in December */
};

/*! \brief Model Scheduler Register State day of the month enumeration */
enum mmdlSchedulerRegisterDayValues
{
  MMDL_SCHEDULER_DAY_ANY   = 0x00, /*!< Any day */
  MMDL_SCHEDULER_DAY_FIRST = 0x01, /*!< First day of the month */
  MMDL_SCHEDULER_DAY_LAST  = 0x1F, /*!< Last day of the month. If month has less than 31, event
                                    *   is scheduled in the last day of the month
                                    */
};

/*! \brief Model Scheduler Register State hour of the day enumeration */
enum mmdlSchedulerRegisterHourValues
{
  MMDL_SCHEDULER_HOUR_FIRST            = 0x00, /*!< First hour of the day (00) */
  MMDL_SCHEDULER_HOUR_LAST             = 0x17, /*!< Last hour of the day (23) */
  MMDL_SCHEDULER_HOUR_ANY              = 0x18, /*!< Any hour of the day */
  MMDL_SCHEDULER_HOUR_RAND             = 0x19, /*!< Once a day, random hour */
  MMDL_SCHEDULER_HOUR_PROHIBITED_START = 0x20, /*!< First prohibited value */
};

/*! \brief Model Scheduler Register State minute field enumeration */
enum mmdlSchedulerRegisterMinuteValues
{
  MMDL_SCHEDULER_MINUTE_FIRST            = 0x00, /*!< First minute of the hour */
  MMDL_SCHEDULER_MINUTE_LAST             = 0x3B, /*!< Last minute of the hour */
  MMDL_SCHEDULER_MINUTE_ANY              = 0x3C, /*!< Any minute of the hour */
  MMDL_SCHEDULER_MINUTE_MOD15            = 0x3D, /*!< Minute value modulo 15 is 0 (0, 15, 30, 45) */
  MMDL_SCHEDULER_MINUTE_MOD20            = 0x3E, /*!< Minute value modulo 20 is 0 (0, 20, 40) */
  MMDL_SCHEDULER_MINUTE_RAND             = 0x3F, /*!< Once an hour, random minute */
  MMDL_SCHEDULER_MINUTE_PROHIBITED_START = 0x40, /*!< First prohibited value */
};

/*! \brief Model Scheduler Register State seconds field enumeration */
enum mmdlSchedulerRegisterSecondValues
{
  MMDL_SCHEDULER_SECOND_FIRST            = 0x00, /*!< First second of the minute */
  MMDL_SCHEDULER_SECOND_LAST             = 0x3B, /*!< Last second of the minute */
  MMDL_SCHEDULER_SECOND_ANY              = 0x3C, /*!< Any second of the minute */
  MMDL_SCHEDULER_SECOND_MOD15            = 0x3D, /*!< Second value modulo 15 is 0 (0, 15, 30, 45) */
  MMDL_SCHEDULER_SECOND_MOD20            = 0x3E, /*!< Second value modulo 20 is 0 (0, 20, 40) */
  MMDL_SCHEDULER_SECOND_RAND             = 0x3F, /*!< Once a minute, random second */
  MMDL_SCHEDULER_SECOND_PROHIBITED_START = 0x40, /*!< First prohibited value */
};

/*! \brief Model Scheduler Register State bit-field representation for Days of Week  */
enum mmdlSchedulerRegisterDayOfWeekBitfieldValues
{
  MMDL_SCHEDULER_SCHED_ON_MONDAYS            = 1 << 0,  /*!< Scheduled on Mondays */
  MMDL_SCHEDULER_SCHED_ON_TUESDAYS           = 1 << 1,  /*!< Scheduled on Tuesdays */
  MMDL_SCHEDULER_SCHED_ON_WEDNESDAYS         = 1 << 2,  /*!< Scheduled on Wednesdays */
  MMDL_SCHEDULER_SCHED_ON_THURSDAYS          = 1 << 3,  /*!< Scheduled on Thursdays */
  MMDL_SCHEDULER_SCHED_ON_FRIDAYS            = 1 << 4,  /*!< Scheduled on Fridays */
  MMDL_SCHEDULER_SCHED_ON_SATURDAYS          = 1 << 5,  /*!< Scheduled on Saturdays */
  MMDL_SCHEDULER_SCHED_ON_SUNDAYS            = 1 << 6,  /*!< Scheduled on Sundays */
  MMDL_SCHEDULER_SCHED_ON_PROHIBITED_START   = 1 << 6,  /*!< First prohibited value */
};

/*! \brief Model Scheduler Register State action field enumeration */
enum mmdlSchedulerRegisterActionValues
{
  MMDL_SCHEDULER_ACTION_TURN_OFF     = 0x00, /*!< Turn Off */
  MMDL_SCHEDULER_ACTION_TURN_ON      = 0x01, /*!< Turn On */
  MMDL_SCHEDULER_ACTION_SCENE_RECALL = 0x02, /*!< Scene Recall */
  MMDL_SCHEDULER_ACTION_NONE         = 0x0F, /*!< No action */
};

#ifdef __cplusplus
}
#endif

#endif /* MMDL_DEFS_H */
