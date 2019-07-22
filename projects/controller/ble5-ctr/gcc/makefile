###################################################################################################
#
# Makefile for building the BLE controller
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
BT_VER          := 10

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

CFG_DEV         :=
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
CFG_DEV         += LL_TRACE_ENABLED=TRUE
endif
CFG_DEV         += SCH_CHECK_LIST_INTEGRITY=1
CFG_DEV         += uECC_ASM=2
CFG_DEV         += BB_LED_ENA=$(BB_LED_ENA)

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

include $(ROOT_DIR)/platform/$(PLATFORM)/build/build.mk
include $(ROOT_DIR)/platform/$(PLATFORM)/build/install.mk

all: $(LIBS)

$(LIB_WSF):
	$(MAKE) -j -C $(dir $(LIB_WSF_MAKE))

$(LIB_CONTROLLER):
	$(MAKE) -j -C $(dir $(LIB_CONTROLLER_MAKE))

build.broadcaster:
	$(MAKE) INIT_BROADCASTER=1 INIT_PERIPHERAL=0 INIT_OBSERVER=0 INIT_CENTRAL=0 INIT_ENCRYPTED=0

build.peripheral:
	$(MAKE) INIT_BROADCASTER=1 INIT_PERIPHERAL=1 INIT_OBSERVER=0 INIT_CENTRAL=0 INIT_ENCRYPTED=0

build.encrypted.peripheral:
	$(MAKE) INIT_BROADCASTER=1 INIT_PERIPHERAL=1 INIT_OBSERVER=0 INIT_CENTRAL=0 INIT_ENCRYPTED=1

build.central:
	$(MAKE) INIT_BROADCASTER=1 INIT_PERIPHERAL=1 INIT_OBSERVER=1 INIT_CENTRAL=1 INIT_ENCRYPTED=0

build.encrypted.central:
	$(MAKE) INIT_BROADCASTER=1 INIT_PERIPHERAL=1 INIT_OBSERVER=1 INIT_CENTRAL=1 INIT_ENCRYPTED=1

show.platform:
	@echo $(PLATFORM_LIST)

show.rtos:
	@echo $(RTOS_LIST)

-include $(WSF_ROOT)/build/token.mk

ifneq ($(wildcard $(LIB_WSF_MAKE)),)
.PHONY: $(LIB_WSF)
endif
ifneq ($(wildcard $(LIB_CONTROLLER_MAKE)),)
.PHONY: $(LIB_CONTROLLER)
endif
