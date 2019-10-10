###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2015-2019 Arm Ltd.
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
# 	Application
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(ROOT_DIR)/ble-mesh-model/include \
	$(ROOT_DIR)/ble-mesh-model/include/app \
	$(ROOT_DIR)/ble-mesh-model/sources/apps \
	$(ROOT_DIR)/ble-mesh-model/sources/apps/app \
	$(ROOT_DIR)/ble-mesh-model/sources/apps/provisioner

C_FILES += \
	$(sort $(wildcard $(ROOT_DIR)/ble-mesh-model/sources/apps/app/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/ble-mesh-model/sources/apps/provisioner/*.c)) \
	$(ROOT_DIR)/projects/ble-mesh-apps/provisioner/main.c \
	$(ROOT_DIR)/projects/ble-mesh-apps/provisioner/stack_provisioner.c

#--------------------------------------------------------------------------------------------------
# 	Application - GATT Side
#--------------------------------------------------------------------------------------------------

ifeq ($(BT_VER),8)
APP_FILTER_OUT := \
	$(wildcard $(ROOT_DIR)/ble-profiles/sources/apps/app/*ae*.c)
else
APP_FILTER_OUT :=
endif

INC_DIRS += \
	$(ROOT_DIR)/ble-profiles/include/app \
	$(ROOT_DIR)/ble-profiles/sources/apps \
	$(ROOT_DIR)/ble-profiles/sources/apps/app

C_FILES += \
	$(sort $(filter-out $(APP_FILTER_OUT),$(wildcard $(ROOT_DIR)/ble-profiles/sources/apps/app/*.c))) \
	$(sort $(wildcard $(ROOT_DIR)/ble-profiles/sources/apps/app/common/*.c)) \

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

#--------------------------------------------------------------------------------------------------
#   Host Stack
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/ble-profiles/build/sources_services.mk

ifeq ($(BT_VER),9)
include $(ROOT_DIR)/ble-profiles/build/sources_profiles_5.mk
else
include $(ROOT_DIR)/ble-profiles/build/sources_profiles_4.mk
endif

ifeq ($(BT_VER),9)
include $(ROOT_DIR)/ble-host/build/sources_stack_mesh_5.mk
else
include $(ROOT_DIR)/ble-host/build/sources_stack_mesh_4.mk
endif

ifeq ($(USE_EXACTLE),1)
include $(ROOT_DIR)/ble-host/build/sources_hci_exactle.mk
else
include $(ROOT_DIR)/ble-host/build/sources_hci_dual_chip.mk
endif

#--------------------------------------------------------------------------------------------------
# 	Third party
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(ROOT_DIR)/thirdparty/uecc/uECC.c

#--------------------------------------------------------------------------------------------------
# 	Controller
#--------------------------------------------------------------------------------------------------

ifeq ($(USE_EXACTLE),1)
ifeq ($(USE_CTR_LIB),1)
INC_DIRS += \
	$(ROOT_DIR)/controller/include/common \
	$(ROOT_DIR)/controller/include/ble

LIBS += $(ROOT_DIR)/controller/library/libcontroller.a
else
include $(ROOT_DIR)/controller/build/sources_ll_5.mk
endif
endif

#--------------------------------------------------------------------------------------------------
# 	Platform
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(WSF_ROOT)/include

include $(ROOT_DIR)/platform/$(PLATFORM)/build/sources.mk

ifeq ($(USE_EXACTLE),1)
include $(ROOT_DIR)/platform/$(PLATFORM)/build/sources_ll.mk
endif
