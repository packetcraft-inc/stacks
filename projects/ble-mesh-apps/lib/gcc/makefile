###################################################################################################
#
# Makefile for building the Mesh library
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
ROOT_DIR        := ../../../..
PLATFORM        := nordic
CPU             := cortex-m0plus
BB_CLK_RATE_HZ  := 1000000
WSF_ROOT        := $(ROOT_DIR)/wsf

# Output
LIB_FILE        := libmesh.a
INT_DIR         := obj
LIB_DIR         := lib
LIB             := $(LIB_DIR)/$(LIB_FILE)

# Options
DEBUG           := 1
BT_VER          := 8
MESH_EN_TEST    := 1

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

CFG_DEV         := INIT_BROADCASTER INIT_PERIPHERAL INIT_OBSERVER INIT_CENTRAL
CFG_DEV         += INIT_ENCRYPTED
CFG_DEV         += MESH_ENABLE_TEST=$(MESH_EN_TEST)
CFG_DEV         += MESH_NVM_USE_STUBS=1
CFG_DEV         += BT_VER=$(BT_VER)
CFG_DEV         += LL_VER=LL_VER_BT_CORE_SPEC_4_2
ifneq ($(DEBUG),0)
CFG_DEV         += WSF_ASSERT_ENABLED=1
endif

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

include sources*.mk

# Object files
OBJ_FILES       := $(C_FILES:.c=.o)
OBJ_FILES       := $(subst $(ROOT_DIR)/,$(INT_DIR)/,$(OBJ_FILES))
OBJ_FILES       := $(subst ./,$(INT_DIR)/,$(OBJ_FILES))
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
	@echo "    BB_CLK_RATE_HZ   = $(BB_CLK_RATE_HZ)"
	@echo "    DEBUG            = $(DEBUG)"
	@echo "    CFG_DEV          = $(strip $(CFG_DEV))"
	@echo "+++ Archiving: $@"
	@mkdir -p $(LIB_DIR)
	@$(AR) $(AR_FLAGS) -o $(LIB) $(OBJ_FILES)

$(INT_DIR)/%.o: $(ROOT_DIR)/%.c
	@echo "+++ Compiling: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) -DMODULE_ID=$(call FILE_HASH,$<) -c -o $@ $<
	@$(if $(DEP),$(DEP) $(DEP_FLAGS) $(subst .o,.d,$@) -MP -MT $@ $<,)

$(INT_DIR)/%.o: $(BSP_DIR)/%.c
	@echo "+++ Compiling: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) -c -o $@ $<
	@$(if $(DEP),$(DEP) $(DEP_FLAGS) $(subst .o,.d,$@) -MP -MT $@ $<,)

clean.all: clean

clean:
	@rm -rf $(INT_DIR)
	@rm -f $(LIB)

-include $(DEP_FILES)

.PHONY: all clean

-include $(WSF_ROOT)/build/token.mk
