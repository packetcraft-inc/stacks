###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2015-2018 Arm Ltd.
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

#--------------------------------------------------------------------------------------------------
# 	Application - Mesh Side
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(ROOT_DIR)/ble-mesh-model/include \
	$(ROOT_DIR)/ble-mesh-model/include/app \
	$(ROOT_DIR)/ble-mesh-model/sources/apps \
	$(ROOT_DIR)/ble-mesh-model/sources/apps/app

#--------------------------------------------------------------------------------------------------
# 	Application - GATT Side
#--------------------------------------------------------------------------------------------------

#ifeq ($(BT_VER),8)
#APP_FILTER_OUT := \
#	$(wildcard $(ROOT_DIR)/ble-profiles/sources/apps/app/*ae*.c)
#else
#APP_FILTER_OUT :=
#endif

INC_DIRS += \
	$(ROOT_DIR)/ble-profiles/include/app \
	$(ROOT_DIR)/ble-profiles/sources/apps \
	$(ROOT_DIR)/ble-profiles/sources/apps/app

#--------------------------------------------------------------------------------------------------
# 	Mesh Models
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/ble-mesh-model/build/sources_models.mk

#--------------------------------------------------------------------------------------------------
# 	Mesh Stack
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/ble-mesh-profile/build/sources_bearer.mk
include $(ROOT_DIR)/ble-mesh-profile/build/sources_ble-profiles.mk
include $(ROOT_DIR)/ble-mesh-profile/build/sources_stack.mk
include $(ROOT_DIR)/ble-mesh-profile/build/sources_provisioning.mk

ifeq ($(MESH_ENABLE_TEST),1)
include $(ROOT_DIR)/ble-mesh-profile/build/sources_test.mk
endif

#--------------------------------------------------------------------------------------------------
#   Host Stack
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(ROOT_DIR)/ble-profiles/sources/services \
	$(ROOT_DIR)/ble-profiles/sources/profiles/include \
	$(ROOT_DIR)/ble-profiles/sources/profiles

INC_DIRS += \
	$(ROOT_DIR)/ble-host/include \
	$(ROOT_DIR)/ble-host/sources/sec/common \
	$(ROOT_DIR)/ble-host/sources/stack/att \
	$(ROOT_DIR)/ble-host/sources/stack/cfg \
	$(ROOT_DIR)/ble-host/sources/stack/dm \
	$(ROOT_DIR)/ble-host/sources/stack/hci \
	$(ROOT_DIR)/ble-host/sources/stack/l2c \
	$(ROOT_DIR)/ble-host/sources/stack/smp \
	$(ROOT_DIR)/thirdparty/uecc


ifeq ($(USE_EXACTLE),1)
INC_DIRS += \
	$(ROOT_DIR)/ble-host/sources/hci/exactle
else
INC_DIRS += \
	$(ROOT_DIR)/ble-host/sources/hci/dual_chip
endif

#--------------------------------------------------------------------------------------------------
# 	Controller
#--------------------------------------------------------------------------------------------------

ifeq ($(USE_EXACTLE),1)
CFG_DEV         += uECC_ASM=2
CFG_DEV         += SCH_CHECK_LIST_INTEGRITY=1
ifneq ($(BB_CLK_RATE_HZ),)
CFG_DEV         += BB_CLK_RATE_HZ=$(BB_CLK_RATE_HZ)
endif

INC_DIRS += \
	$(ROOT_DIR)/controller/include/common \
	$(ROOT_DIR)/controller/include/ble \
	$(ROOT_DIR)/controller/sources/ble/include
endif

#--------------------------------------------------------------------------------------------------
# 	Platform
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(WSF_ROOT)/include \
	$(ROOT_DIR)/platform/include \
	$(ROOT_DIR)/platform/$(PLATFORM)/include

#ifeq ($(USE_EXACTLE),1)
#include $(ROOT_DIR)/platform/$(PLATFORM)/build/sources_ll.mk
#endif
