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

#--------------------------------------------------------------------------------------------------
#     Project
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
    $(ROOT_DIR)/ble-mesh-apps/include \
    $(ROOT_DIR)/ble-mesh-apps/sources/common \
    $(ROOT_DIR)/ble-mesh-apps/sources/provisioner

C_FILES += \
	$(sort $(wildcard $(ROOT_DIR)/ble-mesh-apps/sources/common/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/ble-mesh-apps/sources/provisioner/*.c)) \
	$(ROOT_DIR)/ble-mesh-apps/build/provisioner/main.c \
	$(ROOT_DIR)/ble-mesh-apps/build/provisioner/stack_provisioner.c

#--------------------------------------------------------------------------------------------------
#     Mesh Models
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/ble-mesh-model/build/common/gcc/sources_models.mk

#--------------------------------------------------------------------------------------------------
#     Mesh Stack
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/ble-mesh-profile/build/common/gcc/sources_bearer.mk
include $(ROOT_DIR)/ble-mesh-profile/build/common/gcc/sources_ble-profiles.mk
include $(ROOT_DIR)/ble-mesh-profile/build/common/gcc/sources_stack.mk
include $(ROOT_DIR)/ble-mesh-profile/build/common/gcc/sources_provisioning.mk
