###################################################################################################
#
# Makefile for building the mesh provisioner sample application
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
#     Project
#--------------------------------------------------------------------------------------------------

# Inputs
ROOT_DIR        := ../../../..
PLATFORM        := nordic
RTOS            := baremetal

# Input Libraries
LIB_DIR         := library
WSF_ROOT        := $(ROOT_DIR)/wsf
CONTROLLER_ROOT := $(ROOT_DIR)/controller
HOST_ROOT       := $(ROOT_DIR)/ble-host
LIB_WSF         := $(WSF_ROOT)/$(LIB_DIR)/libwsf.a
LIB_WSF_MAKE    := $(WSF_ROOT)/build/gcc/makefile
LIB_CONTROLLER  := $(CONTROLLER_ROOT)/$(LIB_DIR)/libcontroller.a
LIB_CONTROLLER_MAKE := $(CONTROLLER_ROOT)/build/gcc/makefile
LIB_HOST        := $(HOST_ROOT)/$(LIB_DIR)/libhost.a
LIB_HOST_MAKE   := $(HOST_ROOT)/build/gcc/makefile

LIBS := $(LIB_HOST)
LIBS += $(LIB_CONTROLLER)
LIBS += $(LIB_WSF)

# Output
BIN_FILE        := provisioner.elf

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

# Options
DEBUG                    := 1
TOKEN                    := 0
TRACE                    := 0
BT_VER                   := 9
USE_EXACTLE              := 1
USE_UECC                 := 1
USE_CCM_PLATFORM         := 1
PLATFORM_MESH            := 1
TERMINAL_MAX_ARGC        := 17
TERMINAL_MAX_COMMAND_LEN := 180

CFG_DEV         += INIT_PERIPHERAL INIT_CENTRAL INIT_ENCRYPTED
CFG_DEV         += WSF_MAX_HANDLERS=32 WSF_PRINTF_MAX_LEN=256 WSF_MS_PER_TICK=1
CFG_DEV         += TERMINAL_MAX_ARGC=$(TERMINAL_MAX_ARGC)
CFG_DEV         += TERMINAL_MAX_COMMAND_LEN=$(TERMINAL_MAX_COMMAND_LEN)
ifeq ($(BT_VER),9)
CFG_DEV         += LL_VER=LL_VER_BT_CORE_SPEC_5_0
else
CFG_DEV         += LL_VER=LL_VER_BT_CORE_SPEC_4_2
endif
ifeq ($(USE_CCM_PLATFORM), 1)
CFG_DEV         += SEC_CCM_CFG=SEC_CCM_CFG_PLATFORM
else
CFG_DEV         += SEC_CCM_CFG=SEC_CCM_CFG_HCI
endif
ifeq ($(USE_UECC), 1)
CFG_DEV         += SEC_ECC_CFG=SEC_ECC_CFG_UECC
else ifeq ($(USE_UECC), 2)
CFG_DEV         += SEC_ECC_CFG=SEC_ECC_CFG_DEBUG
else
CFG_DEV         += SEC_ECC_CFG=SEC_ECC_CFG_HCI
endif

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

$(LIB_HOST):
	$(MAKE) -j -C $(dir $(LIB_HOST_MAKE))

ifneq ($(wildcard $(LIB_WSF_MAKE)),)
.PHONY: $(LIB_WSF)
endif
ifneq ($(wildcard $(LIB_CONTROLLER_MAKE)),)
.PHONY: $(LIB_CONTROLLER)
endif
ifneq ($(wildcard $(LIB_HOST_MAKE)),)
.PHONY: $(LIB_HOST)
endif

-include $(WSF_ROOT)/build/token.mk
