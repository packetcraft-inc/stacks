###################################################################################################
#
# Makefile for building the meds sample application
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
BIN_FILE        := meds.elf

# Options
DEBUG           := 0
TRACE           := 1
BT_VER          := 8
USE_EXACTLE     := 1
INIT_PERIPHERAL := 1
INIT_ENCRYPTED  := 1

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

CFG_DEV         := BT_VER=$(BT_VER)

ifeq ($(USE_EXACTLE), 1)
include $(ROOT_DIR)/controller/build/common/gcc/config.mk
endif

include $(ROOT_DIR)/ble-apps/build/common/gcc/config.mk
include $(ROOT_DIR)/platform/build/common/gcc/config.mk

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

include sources.mk
include $(ROOT_DIR)/ble-apps/build/common/gcc/sources.mk

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/ble-apps/build/common/gcc/build.mk
include $(ROOT_DIR)/platform/build/common/gcc/install.mk
