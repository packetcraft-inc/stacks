###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2016-2019 Arm Ltd.
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

include $(ROOT_DIR)/platform/$(PLATFORM)/build/sources.mk
ifeq ($(USE_EXACTLE),1)
INC_DIRS += \
	$(HOST_ROOT)/sources/hci/exactle
include $(ROOT_DIR)/platform/$(PLATFORM)/build/sources_ll.mk
else
INC_DIRS += \
	$(HOST_ROOT)/sources/hci/dual_chip
endif

INC_DIRS += \
	$(HOST_ROOT)/include \
	$(HOST_ROOT)/sources/stack/cfg \
	$(PROFILES_ROOT)/include/app \
	$(PROFILES_ROOT)/sources/apps \
	$(PROFILES_ROOT)/sources/apps/app \
	$(PROFILES_ROOT)/sources/profiles \
	$(PROFILES_ROOT)/sources/services \
	$(CONTROLLER_ROOT)/include/ble \
	$(CONTROLLER_ROOT)/include/common \
	$(WSF_ROOT)/include

C_FILES += \
	$(PROFILES_ROOT)/sources/apps/hidapp/hidapp_main.c \
	$(ROOT_DIR)/projects/ble-apps/hidapp/main.c \
	$(ROOT_DIR)/projects/ble-apps/hidapp/stack_hidapp.c
