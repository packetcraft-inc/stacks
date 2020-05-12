/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Model opcodes definitions.
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

#ifndef MMDL_OPCODES_H
#define MMDL_OPCODES_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Generic OnOff SIG defined Get Opcode */
#define MMDL_GEN_ONOFF_GET_OPCODE                      0x8201

/*! \brief Generic OnOff SIG defined Set Opcode */
#define MMDL_GEN_ONOFF_SET_OPCODE                      0x8202

/*! \brief Generic OnOff SIG defined Set Unacknowledged Opcode */
#define MMDL_GEN_ONOFF_SET_NO_ACK_OPCODE               0x8203

/*! \brief Generic OnOff SIG defined Status Opcode */
#define MMDL_GEN_ONOFF_STATUS_OPCODE                   0x8204

/*! \brief Generic Level SIG defined Get Opcode */
#define MMDL_GEN_LEVEL_GET_OPCODE                      0x8205

/*! \brief Generic Level SIG defined Set Opcode */
#define MMDL_GEN_LEVEL_SET_OPCODE                      0x8206

/*! \brief Generic Level defined Set Unacknowledged Opcode */
#define MMDL_GEN_LEVEL_SET_NO_ACK_OPCODE               0x8207

/*! \brief Generic Level defined Status Opcode */
#define MMDL_GEN_LEVEL_STATUS_OPCODE                   0x8208

/*! \brief Generic Level SIG defined Delta Set Opcode */
#define MMDL_GEN_LEVEL_DELTA_SET_OPCODE                0x8209

/*! \brief Generic Level defined Delta Set Unacknowledged Opcode */
#define MMDL_GEN_LEVEL_DELTA_SET_NO_ACK_OPCODE         0x820A

/*! \brief Generic Level SIG defined Move Set Opcode */
#define MMDL_GEN_LEVEL_MOVE_SET_OPCODE                 0x820B

/*! \brief Generic Level defined Move Set Unacknowledged Opcode */
#define MMDL_GEN_LEVEL_MOVE_SET_NO_ACK_OPCODE          0x820C

/*! \brief Generic Default Transition SIG defined Get Opcode */
#define MMDL_GEN_DEFAULT_TRANS_GET_OPCODE              0x820D

/*! \brief Generic Default Transition SIG defined Set Opcode */
#define MMDL_GEN_DEFAULT_TRANS_SET_OPCODE              0x820E

/*! \brief Generic Default Transition defined Set Unacknowledged Opcode */
#define MMDL_GEN_DEFAULT_TRANS_SET_NO_ACK_OPCODE       0x820F

/*! \brief Generic Default Transition defined Status Opcode */
#define MMDL_GEN_DEFAULT_TRANS_STATUS_OPCODE           0x8210

/*! \brief Generic OnPowerUp SIG defined Get Opcode */
#define  MMDL_GEN_ONPOWERUP_GET_OPCODE                 0x8211

/*! \brief Generic OnPowerUp SIG defined Status Opcode */
#define  MMDL_GEN_ONPOWERUP_STATUS_OPCODE              0x8212

/*! \brief Generic OnPowerUp SIG defined Set Opcode */
#define  MMDL_GEN_ONPOWERUP_SET_OPCODE                 0x8213

/*! \brief Generic OnPowerUp SIG defined Set Unacknowledged Opcode */
#define  MMDL_GEN_ONPOWERUP_SET_NO_ACK_OPCODE          0x8214

/*! \brief Generic Power Level SIG defined Get Opcode */
#define MMDL_GEN_POWER_LEVEL_GET_OPCODE                0x8215

/*! \brief Generic Power Level SIG defined Set Opcode */
#define MMDL_GEN_POWER_LEVEL_SET_OPCODE                0x8216

/*! \brief Generic Power Level defined Set Unacknowledged Opcode */
#define MMDL_GEN_POWER_LEVEL_SET_NO_ACK_OPCODE         0x8217

/*! \brief Generic Power Level SIG defined Get Opcode */
#define MMDL_GEN_POWERLAST_GET_OPCODE                  0x8219

/*! \brief Generic Power Default SIG defined Get Opcode */
#define MMDL_GEN_POWERDEFAULT_GET_OPCODE               0x821B

/*! \brief Generic Power Range SIG defined Get Opcode */
#define MMDL_GEN_POWERRANGE_GET_OPCODE                 0x821D

/*! \brief Generic Power Default SIG defined Set Opcode */
#define MMDL_GEN_POWERDEFAULT_SET_OPCODE               0x821F

/*! \brief Generic Power Default defined Set Unacknowledged Opcode */
#define MMDL_GEN_POWERDEFAULT_SET_NO_ACK_OPCODE        0x8220

/*! \brief Generic Power Range SIG defined Set Opcode */
#define MMDL_GEN_POWERRANGE_SET_OPCODE                 0x8221

/*! \brief Generic Power Range SIG defined Set Unacknowledged Opcode */
#define MMDL_GEN_POWERRANGE_SET_NO_ACK_OPCODE          0x8222

/*! \brief Generic Battery SIG defined Get Opcode */
#define MMDL_GEN_BATTERY_GET_OPCODE                    0x8223

/*! \brief Generic Battery defined Status Opcode */
#define MMDL_GEN_BATTERY_STATUS_OPCODE                 0x8224

/*! \brief Generic Power Level defined Status Opcode */
#define MMDL_GEN_POWER_LEVEL_STATUS_OPCODE             0x8218

/*! \brief Generic Power Level defined Status Opcode */
#define MMDL_GEN_POWERLAST_STATUS_OPCODE               0x821A

/*! \brief Generic Power Default defined Status Opcode */
#define MMDL_GEN_POWERDEFAULT_STATUS_OPCODE            0x821C

/*! \brief Generic Power Range defined Status Opcode */
#define MMDL_GEN_POWERRANGE_STATUS_OPCODE              0x821E

/*! \brief Scene Get Opcode */
#define MMDL_SCENE_GET_OPCODE                          0x8241

/*! \brief Scene Recall Opcode */
#define MMDL_SCENE_RECALL_OPCODE                       0x8242

/*! \brief Scene Recall Unacknowledged Opcode */
#define MMDL_SCENE_RECALL_NO_ACK_OPCODE                0x8243

/*! \brief Scene Status Opcode */
#define MMDL_SCENE_STATUS_OPCODE                       0x5E

/*! \brief Scene Register Get Opcode */
#define MMDL_SCENE_REGISTER_GET_OPCODE                 0x8244

/*! \brief Scene Register Status Opcode */
#define MMDL_SCENE_REGISTER_STATUS_OPCODE              0x8245

/*! \brief Scene Store Opcode */
#define MMDL_SCENE_STORE_OPCODE                        0x8246

/*! \brief Scene Store Unacknowledged Opcode */
#define MMDL_SCENE_STORE_NO_ACK_OPCODE                 0x8247

/*! \brief Scene Delete Opcode */
#define MMDL_SCENE_DELETE_OPCODE                       0x829E

/*! \brief Scene Delete Unacknowledged Opcode */
#define MMDL_SCENE_DELETE_NO_ACK_OPCODE                0x829F

/*! \brief Light CTL Get Opcode */
#define MMDL_LIGHT_CTL_GET_OPCODE                      0x825D

/*! \brief Light CTL Set Opcode */
#define MMDL_LIGHT_CTL_SET_OPCODE                      0x825E

/*! \brief Light CTL Set Unacknowledged Opcode */
#define MMDL_LIGHT_CTL_SET_NO_ACK_OPCODE               0x825F

/*! \brief Light CTL Status Opcode */
#define MMDL_LIGHT_CTL_STATUS_OPCODE                   0x8260

/*! \brief Light CTL Temperature Get Opcode */
#define MMDL_LIGHT_CTL_TEMP_GET_OPCODE                 0x8261

/*! \brief Light CTL Temperature Range Get Opcode */
#define MMDL_LIGHT_CTL_TEMP_RANGE_GET_OPCODE           0x8262

/*! \brief Light CTL Temperature Range Status Opcode */
#define MMDL_LIGHT_CTL_RANGE_STATUS_OPCODE             0x8263

/*! \brief Light CTL Temperature Set Opcode */
#define MMDL_LIGHT_CTL_TEMP_SET_OPCODE                 0x8264

/*! \brief Light CTL Temperature Set Unacknowledged Opcode */
#define MMDL_LIGHT_CTL_TEMP_SET_NO_ACK_OPCODE          0x8265

/*! \brief Light CTL Temperature Status Opcode */
#define MMDL_LIGHT_CTL_TEMP_STATUS_OPCODE              0x8266

/*! \brief Light CTL Default Get Opcode */
#define MMDL_LIGHT_CTL_DEFAULT_GET_OPCODE              0x8267

/*! \brief Light CTL Default Status Opcode */
#define MMDL_LIGHT_CTL_DEFAULT_STATUS_OPCODE           0x8268

/*! \brief Light CTL Default Set Opcode */
#define MMDL_LIGHT_CTL_DEFAULT_SET_OPCODE              0x8269

/*! \brief Light CTL Default Set Unacknowledged Opcode */
#define MMDL_LIGHT_CTL_DEFAULT_SET_NO_ACK_OPCODE       0x826A

/*! \brief Light CTL Temperature Range Set Opcode */
#define MMDL_LIGHT_CTL_TEMP_RANGE_SET_OPCODE           0x826B

/*! \brief Light CTL Temperature Range Set Unacknowledged Opcode */
#define MMDL_LIGHT_CTL_TEMP_RANGE_SET_NO_ACK_OPCODE    0x826C

/*! \brief Light HSL Get Opcode */
#define MMDL_LIGHT_HSL_GET_OPCODE                      0x826D

/*! \brief Light HSL Hue Get Opcode */
#define MMDL_LIGHT_HSL_HUE_GET_OPCODE                  0x826E

/*! \brief Light HSL Hue Set Opcode */
#define MMDL_LIGHT_HSL_HUE_SET_OPCODE                  0x826F

/*! \brief Light HSL Hue Set Unacknowledged Opcode */
#define MMDL_LIGHT_HSL_HUE_SET_NO_ACK_OPCODE           0x8270

/*! \brief Light HSL Hue Set Opcode */
#define MMDL_LIGHT_HSL_HUE_STATUS_OPCODE               0x8271

/*! \brief Light HSL Saturation Get Opcode */
#define MMDL_LIGHT_HSL_SAT_GET_OPCODE                  0x8272

/*! \brief Light HSL Saturation Set Opcode */
#define MMDL_LIGHT_HSL_SAT_SET_OPCODE                  0x8273

/*! \brief Light HSL Saturation Set Unacknowledged Opcode */
#define MMDL_LIGHT_HSL_SAT_SET_NO_ACK_OPCODE           0x8274

/*! \brief Light HSL Saturation Set Opcode */
#define MMDL_LIGHT_HSL_SAT_STATUS_OPCODE               0x8275

/*! \brief Light HSL Set Opcode */
#define MMDL_LIGHT_HSL_SET_OPCODE                      0x8276

/*! \brief Light HSL Set Unacknowledged Opcode */
#define MMDL_LIGHT_HSL_SET_NO_ACK_OPCODE               0x8277

/*! \brief Light HSL Set Opcode */
#define MMDL_LIGHT_HSL_STATUS_OPCODE                   0x8278

/*! \brief Light HSL Target Get Opcode */
#define MMDL_LIGHT_HSL_TARGET_GET_OPCODE               0x8279

/*! \brief Light HSL Target Status Opcode */
#define MMDL_LIGHT_HSL_TARGET_STATUS_OPCODE            0x827A

/*! \brief Light HSL Default Get Opcode */
#define MMDL_LIGHT_HSL_DEFAULT_GET_OPCODE              0x827B

/*! \brief Light HSL Default Status Opcode */
#define MMDL_LIGHT_HSL_DEFAULT_STATUS_OPCODE           0x827C

/*! \brief Light HSL Range Get Opcode */
#define MMDL_LIGHT_HSL_RANGE_GET_OPCODE                0x827D

/*! \brief Light HSL Range Status Opcode */
#define MMDL_LIGHT_HSL_RANGE_STATUS_OPCODE             0x827E

/*! \brief Light HSL Default Set Opcode */
#define MMDL_LIGHT_HSL_DEFAULT_SET_OPCODE              0x827F

/*! \brief Light HSL Default Set Unacknowledged Opcode */
#define MMDL_LIGHT_HSL_DEFAULT_SET_NO_ACK_OPCODE       0x8280

/*! \brief Light HSL Range Set Opcode */
#define MMDL_LIGHT_HSL_RANGE_SET_OPCODE                0x8281

/*! \brief Light HSL Range Set Unacknowledged Opcode */
#define MMDL_LIGHT_HSL_RANGE_SET_NO_ACK_OPCODE         0x8282

/*! \brief Light Lightness defined Get Opcode */
#define MMDL_LIGHT_LIGHTNESS_GET_OPCODE                0x824B

/*! \brief Light Lightness defined Set Opcode */
#define MMDL_LIGHT_LIGHTNESS_SET_OPCODE                0x824C

/*! \brief Light Lightness defined Set Unacknowledged Opcode */
#define MMDL_LIGHT_LIGHTNESS_SET_NO_ACK_OPCODE         0x824D

/*! \brief Light Lightness defined Status Opcode */
#define MMDL_LIGHT_LIGHTNESS_STATUS_OPCODE             0x824E

/*! \brief Light Lightness Linear defined Get Opcode */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_GET_OPCODE         0x824F

/*! \brief Light Lightness Linear defined Set Opcode */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_SET_OPCODE         0x8250

/*! \brief Light Lightness Linear defined Set Unacknowledged Opcode */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_SET_NO_ACK_OPCODE  0x8251

/*! \brief Light Lightness Linear defined Status Opcode */
#define MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_OPCODE      0x8252

/*! \brief Light Lightness Last defined Get Opcode */
#define MMDL_LIGHT_LIGHTNESS_LAST_GET_OPCODE           0x8253

/*! \brief Light Lightness Last defined Status Opcode */
#define MMDL_LIGHT_LIGHTNESS_LAST_STATUS_OPCODE        0x8254

/*! \brief Light Lightness Default defined Get Opcode */
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_GET_OPCODE        0x8255

/*! \brief Light Lightness Default defined Status Opcode */
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_STATUS_OPCODE     0x8256

/*! \brief Light Lightness Range defined Get Opcode */
#define MMDL_LIGHT_LIGHTNESS_RANGE_GET_OPCODE          0x8257

/*! \brief Light Lightness Range defined Status Opcode */
#define MMDL_LIGHT_LIGHTNESS_RANGE_STATUS_OPCODE       0x8258

/*! \brief Light Lightness Default defined Set Opcode */
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_OPCODE        0x8259

/*! \brief Light Lightness Default defined Set Unacknowledged Opcode */
#define MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_NO_ACK_OPCODE 0x825A

/*! \brief Light Lightness Range defined Set Opcode */
#define MMDL_LIGHT_LIGHTNESS_RANGE_SET_OPCODE          0x825B

/*! \brief Light Lightness Range defined Set Unacknowledged Opcode */
#define MMDL_LIGHT_LIGHTNESS_RANGE_SET_NO_ACK_OPCODE   0x825C

/*! \brief Time SIG defined Get Opcode */
#define MMDL_TIME_GET_OPCODE                           0x8237

/*! \brief Time SIG defined Set Opcode */
#define MMDL_TIME_SET_OPCODE                           0x5C

/*! \brief Time SIG defined Status Opcode */
#define MMDL_TIME_STATUS_OPCODE                        0x5D

/*! \brief Time Role SIG defined Get Opcode */
#define MMDL_TIMEROLE_GET_OPCODE                       0x8238

/*! \brief Time Role SIG defined Set Opcode */
#define MMDL_TIMEROLE_SET_OPCODE                       0x8239

/*! \brief Time Role SIG defined Status Opcode */
#define MMDL_TIMEROLE_STATUS_OPCODE                    0x823A

/*! \brief Time Zone SIG defined Get Opcode */
#define MMDL_TIMEZONE_GET_OPCODE                       0x823B

/*! \brief Time Zone SIG defined Set Opcode */
#define MMDL_TIMEZONE_SET_OPCODE                       0x823C

/*! \brief Time Zone SIG defined Status Opcode */
#define MMDL_TIMEZONE_STATUS_OPCODE                    0x823D

/*! \brief Time TAI-UTC Delta SIG defined Get Opcode */
#define MMDL_TIMEDELTA_GET_OPCODE                      0x823E

/*! \brief Time TAI-UTC Delta SIG defined Set Opcode */
#define MMDL_TIMEDELTA_SET_OPCODE                      0x823F

/*! \brief Time TAI-UTC Delta SIG defined Status Opcode */
#define MMDL_TIMEDELTA_STATUS_OPCODE                   0x8240

/*! \brief Scheduler Action Get Opcode */
#define MMDL_SCHEDULER_ACTION_GET_OPCODE               0x8248

/*! \brief Scheduler Get Opcode */
#define MMDL_SCHEDULER_GET_OPCODE                      0x8249

/*! \brief Scheduler Status Opcode */
#define MMDL_SCHEDULER_STATUS_OPCODE                   0x824A

/*! \brief Scheduler Action Status Opcode */
/*! \brief Scheduler Action Status Opcode */
#define MMDL_SCHEDULER_ACTION_STATUS_OPCODE            0x5F

/*! \brief Scheduler Action Set Unacknowledged Opcode */
#define MMDL_SCHEDULER_ACTION_SET_OPCODE               0x60

/*! \brief Scheduler Action Set Opcode*/
#define MMDL_SCHEDULER_ACTION_SET_NO_ACK_OPCODE        0x61

/*! \brief Vendor Test Get Opcode */
#define MMDL_VENDOR_TEST_GET_OPCODE                    0xF01234

/*! \brief Vendor Test Set Opcode */
#define MMDL_VENDOR_TEST_SET_OPCODE                    0xF11234

/*! \brief Vendor Test Set Unacknowledged Opcode */
#define MMDL_VENDOR_TEST_SET_NO_ACK_OPCODE             0xF21234

/*! \brief Vendor Test Status Opcode */
#define MMDL_VENDOR_TEST_STATUS_OPCODE                 0xF31234

#ifdef __cplusplus
}
#endif

#endif /* MMDL_OPCODES_H */
