###################################################################################################
#
# Makefile for building the 802.15.4 MAC controller
#
# Copyright (c) 2013-2019 Arm Ltd.
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
#     Project
#--------------------------------------------------------------------------------------------------

ROOT_DIR        := ../../../..
PLATFORM_LIST   := $(sort $(filter-out test docs include external,$(notdir $(wildcard $(ROOT_DIR)/platform/*))))
RTOS_LIST       := $(sort $(notdir $(wildcard $(ROOT_DIR)/wsf/sources/port/*)))

# Inputs
PLATFORM        := $(firstword $(PLATFORM_LIST))
RTOS            := $(firstword $(RTOS_LIST))

# Input Libraries
LIB_DIR         := library
WSF_ROOT        := $(ROOT_DIR)/wsf
CONTROLLER_ROOT := $(ROOT_DIR)/controller
LIB_WSF         := $(WSF_ROOT)/$(LIB_DIR)/libwsf.a
LIB_WSF_MAKE    := $(WSF_ROOT)/build/gcc/makefile
LIB_CONTROLLER  := $(CONTROLLER_ROOT)/$(LIB_DIR)/libcontroller.a
LIB_CONTROLLER_MAKE := $(CONTROLLER_ROOT)/build/gcc/makefile

LIBS := $(LIB_CONTROLLER)
LIBS += $(LIB_WSF)

# Output
BIN_FILE        := mac154-ctr.elf

# Options
DEBUG           := 0
TOKEN           := 0

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

CFG_DEV         += SCH_CHECK_LIST_INTEGRITY=1
CFG_DEV         += uECC_ASM=2

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

ifeq ($(BUILD_LIB),1)
include buildlib.mk
else
include $(ROOT_DIR)/platform/$(PLATFORM)/build/build.mk
endif
include $(ROOT_DIR)/platform/$(PLATFORM)/build/install.mk

all: $(LIBS)

$(LIB_WSF):
	$(MAKE) -j -C $(WSF_ROOT)/build/gcc

$(LIB_CONTROLLER):
	$(MAKE) -j -C $(CONTROLLER_ROOT)/build/gcc

show.platform:
	@echo $(PLATFORM_LIST)

show.rtos:
	@echo $(RTOS_LIST)
