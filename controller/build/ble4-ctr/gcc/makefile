###################################################################################################
#
# Project configuration for the BLE v4.2 controller.
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

ROOT_DIR        := ../../../..

# Output
BIN_FILE        := ble4-ctr.elf

# Options
DEBUG           := 0
TOKEN           := 0
BB_LED_ENA      := 1
INIT_BROADCASTER:= 1
INIT_PERIPHERAL := 1
INIT_OBSERVER   := 1
INIT_CENTRAL    := 1
INIT_ENCRYPTED  := 1
BT_VER          := 8

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

CFG_DEV         := BT_VER=$(BT_VER)
ifeq ($(TOKEN),1)
CFG_DEV         += LL_TRACE_ENABLED=1
endif
CFG_DEV         += SCH_CHECK_LIST_INTEGRITY=1
CFG_DEV         += uECC_ASM=2
CFG_DEV         += BB_LED_ENA=$(BB_LED_ENA)

include $(ROOT_DIR)/controller/build/common/gcc/config.mk
include $(ROOT_DIR)/platform/build/common/gcc/config.mk

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

include sources.mk
include $(ROOT_DIR)/controller/build/common/gcc/sources_ll_4.mk
include $(ROOT_DIR)/wsf/build/sources.mk
include $(ROOT_DIR)/platform/build/common/gcc/sources.mk

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/controller/build/common/gcc/build.mk
include $(ROOT_DIR)/wsf/build/token.mk
include $(ROOT_DIR)/platform/build/common/gcc/install.mk
