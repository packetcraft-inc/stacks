###################################################################################################
#
# Source and include definition
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
	$(ROOT_DIR)/ble-host/sources/hci/exactle

C_FILES += \
	$(ROOT_DIR)/ble-host/sources/hci/common/hci_core.c \
	$(ROOT_DIR)/ble-host/sources/hci/common/hci_tr.c \
	$(ROOT_DIR)/ble-host/sources/hci/exactle/hci_cmd.c \
	$(ROOT_DIR)/ble-host/sources/hci/exactle/hci_cmd_enc.c \
	$(ROOT_DIR)/ble-host/sources/hci/exactle/hci_cmd_master.c \
	$(ROOT_DIR)/ble-host/sources/hci/exactle/hci_core_ps.c \
	$(ROOT_DIR)/ble-host/sources/hci/exactle/hci_evt.c \
	$(ROOT_DIR)/ble-host/sources/hci/exactle/hci_vs.c
