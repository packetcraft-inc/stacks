###################################################################################################
#
# Build make targets
#
# Copyright (c) 2013-2019 ARM Ltd.
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

# Output
INT_DIR         := obj
LIB_DIR         := lib
LIB             := $(LIB_DIR)/$(LIB_FILE)

# Options
BAUD            := 1000000
UART_HWFLOW          := 1
BB_CLK_RATE_HZ  := 1000000
#ifneq ($(DEBUG),0)
USE_LTO         := 0
#else
#USE_LTO         := 1
#endif

# Board
BOARD           := PCA10056
#BOARD           := PCA10040
BSP_DIR         := $(ROOT_DIR)/thirdparty/nordic-bsp

# Chip
ifeq ($(BOARD),PCA10056)
CPU             := cortex-m4
CHIP            := NRF52840_XXAA
else
CPU             := cortex-m4
CHIP            := NRF52832_XXAA
endif

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

# Host
CFG_DEV         += BT_VER=$(BT_VER)
ifeq ($(USE_EXACTLE),1)
CFG_DEV         += USE_EXACTLE=1
CFG_DEV         += HCI_TX_DATA_TAILROOM=4
endif

# RTOS
CFG_DEV         += WSF_BUF_STATS=1
CFG_DEV         += WSF_CS_STATS=1
ifneq ($(DEBUG),0)
CFG_DEV         += WSF_ASSERT_ENABLED=1
endif
ifeq ($(TOKEN),1)
CFG_DEV         += WSF_TOKEN_ENABLED=1
endif
ifeq ($(TRACE),1)
CFG_DEV         += WSF_TRACE_ENABLED=1
endif

# Platform
CFG_DEV         += $(CHIP)
CFG_DEV         += BOARD_$(BOARD)
CFG_DEV         += __LINT__=0
ifeq ($(CHIP),NRF51422)
CFG_DEV         += __CORTEX_SC=0
CFG_DEV         += NRF51
endif
CFG_DEV         += UART_BAUD=$(BAUD)
CFG_DEV         += UART_DEFAULT_CONFIG_HWFC=$(UART_HWFLOW)

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

include sources*.mk

# Object files
OBJ_FILES       := $(C_FILES:.c=.o)
OBJ_FILES       := $(subst $(ROOT_DIR)/,$(INT_DIR)/,$(OBJ_FILES))
OBJ_FILES       := $(subst $(BSP_DIR)/,$(INT_DIR)/,$(OBJ_FILES))
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

TARGET_DEP      := $(LIB)
ifeq ($(TOKEN),1)
TARGET_DEP      += token
endif

all: $(TARGET_DEP)

$(LIB): $(OBJ_FILES)
	@echo "+++ Build options"
	@echo "    CPU              = $(CPU)"
	@echo "    CHIP             = $(CHIP)"
	@echo "    BOARD            = $(BOARD)"
	@echo "    BSP_DIR          = $(BSP_DIR)"
	@echo "    BAUD             = $(BAUD)"
	@echo "    UART_HWFLOW      = $(UART_HWFLOW)"
	@echo "    BB_CLK_RATE_HZ   = $(BB_CLK_RATE_HZ)"
	@echo "    DEBUG            = $(DEBUG)"
	@echo "    TOKEN            = $(TOKEN)"
	@echo "    USE_LIB          = $(USE_LIB)"
	@echo "    USE_LTO          = $(USE_LTO)"
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

clean: token.clean
	@rm -rf $(INT_DIR)
	@rm -rf $(BIN_DIR)

include $(ROOT_DIR)/wsf/build/token.mk
-include $(DEP_FILES)

.PHONY: all clean
