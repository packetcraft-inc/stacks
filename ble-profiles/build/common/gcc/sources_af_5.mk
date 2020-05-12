###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2012-2017 ARM Ltd. All Rights Reserved.
#
# Copyright (c) 2019 Packetcraft, Inc.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###################################################################################################

INC_DIRS += \
	$(ROOT_DIR)/ble-profiles/include \
	$(ROOT_DIR)/ble-profiles/sources/af

C_FILES += \
	$(ROOT_DIR)//ble-profiles/sources/af/app_disc.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_main.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_master.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_master_ae.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_master_leg.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_server.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_slave.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_slave_ae.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_slave_leg.c \
	$(ROOT_DIR)//ble-profiles/sources/af/app_terminal.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/app_db.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/app_hw.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/app_ui.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/ui_console.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/ui_lcd.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/ui_main.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/ui_platform.c \
	$(ROOT_DIR)//ble-profiles/sources/af/common/ui_timer.c
