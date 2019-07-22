###################################################################################################
#
# Makefile for building the Host and Profiles library
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

# GCC ARM cross toolchain
CROSS_COMPILE   := arm-none-eabi-

# Toolchain
CC              := $(CROSS_COMPILE)gcc
AR              := $(CROSS_COMPILE)gcc-ar
LD              := $(CROSS_COMPILE)gcc
DEP             := $(CROSS_COMPILE)gcc
OBJDUMP         := $(CROSS_COMPILE)objdump
OBJCOPY         := $(CROSS_COMPILE)objcopy
SIZE            := $(CROSS_COMPILE)size

# Input
ROOT_DIR        := ../../..
HOST_ROOT       := $(ROOT_DIR)/ble-host
PROFILES_ROOT   := $(ROOT_DIR)/ble-profiles
CONTROLLER_ROOT := $(ROOT_DIR)/controller
WSF_ROOT        := $(ROOT_DIR)/wsf
PLATFORM        ?= nordic

ifeq ($(PLATFORM),nordic)
    ifeq ($(BOARD),PCA10040)
        CPU     := cortex-m4
        CHIP    := NRF52832_XXAA
    else
        CPU     := cortex-m4
        CHIP    := NRF52840_XXAA
    endif
else ifeq ($(PLATFORM),qemu)
    CPU         := cortex-m3
    CHIP        := LM3S6965
else
    CPU         := cortex-m0plus
endif

# Output
LIB_FILE        := libhost.a
INT_DIR         := obj
LIB_DIR         := library
LIB             := $(HOST_ROOT)/$(LIB_DIR)/$(LIB_FILE)

# Options
DEBUG           := 0
TRACE           := 1
TOKEN           := 0

ifeq ($(DEBUG),0)
USE_LTO         := 1
else
USE_LTO         := 0
endif

ifneq ($(findstring USE_EXACTLE=1, $(CFG_DEV)),)
USE_EXACTLE     := 1
endif
ifneq ($(findstring BT_VER=8, $(CFG_DEV)),)
BT_VER          := 8
endif
ifneq ($(findstring BT_VER=9, $(CFG_DEV)),)
BT_VER          := 9
endif
ifneq ($(findstring BT_VER=10, $(CFG_DEV)),)
BT_VER          := 10
endif
ifneq ($(findstring BT_VER=11, $(CFG_DEV)),)
BT_VER          := 11
endif

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

# Set in pan/platform/$(PLATFORM)/build/build.mk and inherited here

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

include $(HOST_ROOT)/build/sources_lib.mk

# Object files
OBJ_FILES       := $(C_FILES:.c=.o)
OBJ_FILES       := $(subst $(PROFILES_ROOT)/,$(INT_DIR)/,$(OBJ_FILES))
OBJ_FILES       := $(subst $(HOST_ROOT)/,$(INT_DIR)/,$(OBJ_FILES))
DEP_FILES       := $(OBJ_FILES:.o=.d)

#--------------------------------------------------------------------------------------------------
#     Compilation flags
#--------------------------------------------------------------------------------------------------

# Compiler flags
C_FLAGS         += -std=c99
C_FLAGS         += -Wall -Wextra -Wno-unused-parameter -Wshadow -Wundef
C_FLAGS         += -mcpu=$(CPU) -mthumb -mlittle-endian
C_FLAGS         += -fno-common -fomit-frame-pointer
C_FLAGS         += -ffunction-sections -fdata-sections
ifeq ($(DEBUG),2)
C_FLAGS         += -O0 -g
else
C_FLAGS         += -Os -g
endif
ifneq ($(DEBUG),0)
C_FLAGS         += -DDEBUG
endif
C_FLAGS         += $(addprefix -I,$(INC_DIRS))
C_FLAGS         += $(addprefix -D,$(CFG_DEV))
ifeq ($(USE_LTO),1)
C_FLAGS         += -flto
endif

# Archiver flags
AR_FLAGS        := rcs

# Dependency flags
DEP_FLAGS       := $(C_FLAGS) -MM -MF

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

all: $(LIB)

$(LIB): $(OBJ_FILES)
	@echo "+++ Build options"
	@echo "    CPU              = $(CPU)"
	@echo "    DEBUG            = $(DEBUG)"
	@echo "    TRACE            = $(TRACE)"
	@echo "    HOST_CFG_DEV     = $(strip $(CFG_DEV))"
	@echo "+++ Archiving: $@"
	@mkdir -p $(dir $(LIB))
	@$(AR) $(AR_FLAGS) -o $(LIB) $(OBJ_FILES)

$(INT_DIR)/%.o: $(HOST_ROOT)/%.c
	@echo "+++ Compiling: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) -DMODULE_ID=$(call FILE_HASH,$<) -c -o $@ $<
	@$(if $(DEP),$(DEP) $(DEP_FLAGS) $(subst .o,.d,$@) -MP -MT $@ $<,)

$(INT_DIR)/%.o: $(PROFILES_ROOT)/%.c
	@echo "+++ Compiling: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) -DMODULE_ID=$(call FILE_HASH,$<) -c -o $@ $<
	@$(if $(DEP),$(DEP) $(DEP_FLAGS) $(subst .o,.d,$@) -MP -MT $@ $<,)

clean.all: clean

clean: token.clean
	@rm -rf $(INT_DIR)
	@rm -rf $(dir $(LIB))

-include $(WSF_ROOT)/build/token.mk
-include $(DEP_FILES)

.PHONY: all clean clean.all
