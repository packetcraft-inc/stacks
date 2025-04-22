###################################################################################################
#
# Build make targets
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

# Board
ifeq ($(BOARD),)
BOARD           := SECURE-IOT
endif

BSP_DIR         := $(ROOT_DIR)/thirdparty/seciot-bsp
AUDIO_CAPE      := 0
MESH_CAPE       := 0

# Options
ifeq ($(BAUD),)
BAUD            := 115200
endif
ifeq ($(HWFC),)
HWFC            := 1
endif
#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

# Core
CFG_DEV         += SECIOT

# Peripherals
ifneq ($(BB_CLK_RATE_HZ),)
CFG_DEV         += BB_CLK_RATE_HZ=$(BB_CLK_RATE_HZ)
endif
CFG_DEV         += BB_ENABLE_INLINE_ENC_TX=1 #BB_ENABLE_INLINE_DEC_RX=1
CFG_DEV         += UART_BAUD=$(BAUD)
CFG_DEV         += UART_HWFC=$(HWFC)
CFG_DEV         += $(CHCI_TR)

# Board
CFG_DEV         += BOARD_$(BOARD)=1

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

# Linker file
LD_FILE         := $(ROOT_DIR)/platform/targets/mindgrove/build/flash-link.ld

#--------------------------------------------------------------------------------------------------
#     Compilation flags
#--------------------------------------------------------------------------------------------------

# Compiler flags
C_FLAGS         += -march=rv64imafd_zicsr_zifencei -mabi=lp64d -mcmodel=medany -Os -ffunction-sections -fdata-sections -fno-unwind-tables -fshort-enums -fsingle-precision-constant -Wdouble-promotion -fno-exceptions

# Linker flags
LD_FLAGS        += -lgcov -Wl,--gc-sections -w
