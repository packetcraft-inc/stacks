###################################################################################################
#
# Project configuration for the BLE v5.1 controller.
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
#     Project
#--------------------------------------------------------------------------------------------------

ROOT_DIR        := ../../../..

# Output
BIN_FILE        := ble5-ctr.elf

# Options
DEBUG           := 0
TOKEN           := 0
BB_LED_ENA      := 1
INIT_BROADCASTER:= 1
INIT_PERIPHERAL := 1
INIT_OBSERVER   := 1
INIT_CENTRAL    := 1
INIT_ENCRYPTED  := 1
BT_VER          := 11

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

CFG_DEV         := BT_VER=$(BT_VER)
ifeq ($(INIT_CENTRAL),1)
CFG_DEV         += INIT_BROADCASTER INIT_PERIPHERAL INIT_OBSERVER INIT_CENTRAL
else ifeq ($(INIT_PERIPHERAL),1)
CFG_DEV         += INIT_BROADCASTER INIT_PERIPHERAL
else ifeq ($(INIT_BROADCASTER),1)
CFG_DEV         += INIT_BROADCASTER
endif
ifeq ($(INIT_ENCRYPTED),1)
CFG_DEV         += INIT_ENCRYPTED
endif
ifeq ($(TOKEN),1)
CFG_DEV         += LL_TRACE_ENABLED=1
endif
CFG_DEV         += SCH_CHECK_LIST_INTEGRITY=1
CFG_DEV         += BB_LED_ENA=$(BB_LED_ENA)

include $(ROOT_DIR)/controller/build/common/gcc/config.mk
include $(ROOT_DIR)/platform/build/common/gcc/config.mk

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

include sources.mk
include $(ROOT_DIR)/controller/build/common/gcc/sources_ll_5.mk
include $(ROOT_DIR)/wsf/build/sources.mk
include $(ROOT_DIR)/platform/targets/$(PLATFORM)/build/sources.mk

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/controller/build/common/gcc/build.mk
include $(ROOT_DIR)/wsf/build/token.mk
include $(ROOT_DIR)/platform/build/common/gcc/install.mk
