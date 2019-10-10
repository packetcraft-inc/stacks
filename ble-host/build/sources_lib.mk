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

ifeq ($(BT_VER),8)
APP_FILTER_OUT := \
$(wildcard $(ROOT_DIR)/ble-profiles/sources/apps/app/*ae*.c)
else
APP_FILTER_OUT :=
endif

ifeq ($(BT_VER),8)
    include $(HOST_ROOT)/build/sources_stack_4.mk
    include $(PROFILES_ROOT)/build/sources_profiles_4.mk
else
    include $(HOST_ROOT)/build/sources_stack_5.mk
    include $(PROFILES_ROOT)/build/sources_profiles_5.mk
endif

ifeq ($(USE_EXACTLE),1)
include $(HOST_ROOT)/build/sources_hci_exactle.mk
else
include $(HOST_ROOT)/build/sources_hci_dual_chip.mk
endif
include $(PROFILES_ROOT)/build/sources_services.mk

#--------------------------------------------------------------------------------------------------
# 	Include files
#--------------------------------------------------------------------------------------------------

INC_DIRS  += \
    $(PROFILES_ROOT)/include/app \
    $(PROFILES_ROOT)/sources/apps/app \
    $(CONTROLLER_ROOT)/include/ble \
    $(CONTROLLER_ROOT)/include/common \
    $(WSF_ROOT)/include

C_FILES   += \
    $(sort $(filter-out $(APP_FILTER_OUT),$(wildcard $(PROFILES_ROOT)/sources/apps/app/*.c))) \
    $(sort $(wildcard $(PROFILES_ROOT)/sources/apps/app/common/*.c))
