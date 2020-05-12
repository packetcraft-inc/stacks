###################################################################################################
#
# Common GCC build configuration.
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

# Platform targets
PLATFORM_LIST   := $(sort $(notdir $(wildcard $(ROOT_DIR)/platform/targets/*)))
RTOS_LIST       := $(sort $(notdir $(wildcard $(ROOT_DIR)/wsf/sources/targets/*)))

# Inputs
ifeq ($(PLATFORM),)
PLATFORM        := $(firstword $(PLATFORM_LIST))
endif
ifeq ($(RTOS),)
RTOS            := $(firstword $(RTOS_LIST))
endif

# Output
INT_DIR         := obj
BIN_DIR         := bin
BIN             := $(BIN_DIR)/$(BIN_FILE)

ifeq ($(BB_CLK_RATE_HZ),)
BB_CLK_RATE_HZ  := 1000000
endif

USE_LTO         := 0

# Suggest UART transport for controller; platform may select alternate
CHCI_TR			:= CHCI_TR_UART=1

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

# Controller
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

# RTOS
ifeq ($(DEBUG),0)
CFG_DEV         += WSF_BUF_FREE_CHECK_ASSERT=0
CFG_DEV         += WSF_BUF_STATS=0
CFG_DEV         += WSF_CS_STATS=0
CFG_DEV         += WSF_ASSERT_ENABLED=0
else
CFG_DEV         += WSF_BUF_FREE_CHECK_ASSERT=1
CFG_DEV         += WSF_BUF_STATS=1
CFG_DEV         += WSF_CS_STATS=1
CFG_DEV         += WSF_ASSERT_ENABLED=1
endif
ifeq ($(TOKEN),1)
CFG_DEV         += WSF_TOKEN_ENABLED=1
endif
ifeq ($(TRACE),1)
CFG_DEV         += WSF_TRACE_ENABLED=1
endif

#--------------------------------------------------------------------------------------------------
#     Compilation flags
#--------------------------------------------------------------------------------------------------

# Compiler flags
C_FLAGS         += -std=c99
C_FLAGS         += -Wall -Wextra -Wno-unused-parameter -Wshadow -Wno-expansion-to-defined #-Wundef
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
ifeq ($(USE_LTO),1)
C_FLAGS         += -flto
endif

# Archiver flags
AR_FLAGS        := rcs

# Linker flags
LD_FLAGS        := -Wl,-Map=$(BIN:.elf=.map)
LD_FLAGS        += -Wl,--gc-sections
LD_FLAGS        += --specs=nano.specs
LD_FLAGS        += --specs=nosys.specs
