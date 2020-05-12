###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2019-2020 Packetcraft, Inc.
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

#--------------------------------------------------------------------------------------------------
#     App common sources
#--------------------------------------------------------------------------------------------------

ifeq ($(BT_VER), 11)  # Core v5.2
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_af_5.mk
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_profiles_5.mk
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_services_5.mk
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_host_5.mk

ifeq ($(USE_EXACTLE), 1)
include $(ROOT_DIR)/controller/build/common/gcc/sources_ll_5.mk
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_hci_exactle_5.mk
else
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_hci_dual_chip_5.mk
endif
endif

ifeq ($(BT_VER), 10)  # Core v5.1
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_af_5.mk
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_profiles_5.mk
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_services_5.mk
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_host_5.mk

ifeq ($(USE_EXACTLE), 1)
include $(ROOT_DIR)/controller/build/common/gcc/sources_ll_5.mk
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_hci_exactle_5.mk
else
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_hci_dual_chip_5.mk
endif
endif

ifeq ($(BT_VER), 8)   # Core v4.2
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_af_4.mk
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_profiles_4.mk
include $(ROOT_DIR)/ble-profiles/build/common/gcc/sources_services_4.mk
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_host_4.mk

ifeq ($(USE_EXACTLE), 1)
include $(ROOT_DIR)/controller/build/common/gcc/sources_ll_4.mk
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_hci_exactle_4.mk
else
include $(ROOT_DIR)/ble-host/build/common/gcc/sources_hci_dual_chip_4.mk
endif
endif

include $(ROOT_DIR)/wsf/build/sources.mk
include $(ROOT_DIR)/platform/build/common/gcc/sources.mk
