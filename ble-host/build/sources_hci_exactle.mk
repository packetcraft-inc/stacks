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

ifeq ($(BT_VER),8)
HCI_FILTER_OUT := \
	$(wildcard $(ROOT_DIR)/ble-host/sources/hci/exactle/*ae*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/hci/exactle/*phy*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/hci/exactle/*past*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/hci/exactle/*cte*.c)
else ifeq ($(BT_VER),9)
HCI_FILTER_OUT := \
	$(wildcard $(ROOT_DIR)/ble-host/sources/hci/exactle/*cte*.c) \
	$(wildcard $(ROOT_DIR)/ble-host/sources/hci/exactle/*past*.c)
endif

INC_DIRS += \
	$(ROOT_DIR)/ble-host/sources/hci/exactle

C_FILES += \
	$(sort $(wildcard $(ROOT_DIR)/ble-host/sources/hci/common/*.c)) \
	$(sort $(filter-out $(HCI_FILTER_OUT),$(wildcard $(ROOT_DIR)/ble-host/sources/hci/exactle/*.c)))
