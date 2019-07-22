###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2012-2018 ARM Ltd.
#
# Copyright (c) 2019 Packetcraft, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###################################################################################################

DM_FILTER_OUT := \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/dm/*ae*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/dm/*phy*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/dm/*past*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/dm/*cte*.c)

HCI_FILTER_OUT := \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/hci/*ae*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/hci/*phy*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/hci/*past*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/stack/hci/*cte*.c)

INC_DIRS += \
	$(ROOT_DIR)/ble-host/include \
	$(ROOT_DIR)/ble-host/sources/stack/att \
	$(ROOT_DIR)/ble-host/sources/stack/cfg \
	$(ROOT_DIR)/ble-host/sources/stack/dm \
	$(ROOT_DIR)/ble-host/sources/stack/hci \
	$(ROOT_DIR)/ble-host/sources/stack/l2c \
	$(ROOT_DIR)/ble-host/sources/stack/smp \
	$(ROOT_DIR)/platform/include

C_FILES += \
	$(sort $(wildcard $(ROOT_DIR)/ble-host/sources/stack/att/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/ble-host/sources/stack/cfg/*.c)) \
	$(sort $(filter-out $(DM_FILTER_OUT),$(wildcard $(ROOT_DIR)/ble-host/sources/stack/dm/*.c))) \
	$(sort $(filter-out $(HCI_FILTER_OUT),$(wildcard $(ROOT_DIR)/ble-host/sources/stack/hci/*.c))) \
	$(sort $(wildcard $(ROOT_DIR)/ble-host/sources/stack/l2c/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/ble-host/sources/stack/smp/*.c))

C_FILES += \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_aes.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_cmac_hci.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_ecc_hci.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_main.c
