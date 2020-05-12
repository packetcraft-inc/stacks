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
BOARD           := PCA10056
endif

BSP_DIR         := $(ROOT_DIR)/thirdparty/nordic-bsp
AUDIO_CAPE      := 0
MESH_CAPE       := 0

# Chip
ifeq ($(BOARD),PCA10056)
CPU             := cortex-m4
CHIP            := NRF52840_XXAA
endif
ifeq ($(BOARD),PCA10040)
CPU             := cortex-m4
CHIP            := NRF52832_XXAA
endif
ifeq ($(BOARD),NRF6832)
CPU             := cortex-m4
CHIP            := NRF52832_XXAA
endif

# Options
ifeq ($(BAUD),)
BAUD            := 1000000
endif
ifeq ($(HWFC),)
HWFC            := 1
endif
ifeq ($(CODEC),)
CODEC			:= BLUEDROID
endif

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------

# Core
CFG_DEV         += $(CHIP)
ifeq ($(CHIP),NRF51422)
CFG_DEV         += NRF51
endif

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
CFG_DEV         += AUDIO_CAPE=$(AUDIO_CAPE)
CFG_DEV         += MESH_CAPE=$(MESH_CAPE)

# Audio
CFG_DEV         += CODEC_$(CODEC)=1

#--------------------------------------------------------------------------------------------------
#     Sources
#--------------------------------------------------------------------------------------------------

# Linker file
ifeq ($(CHIP),NRF52840_XXAA)
LD_FILE         := $(ROOT_DIR)/platform/targets/nordic/build/nrf52840.ld
endif
ifeq ($(CHIP),NRF52832_XXAA)
LD_FILE         := $(ROOT_DIR)/platform/targets/nordic/build/nrf52832.ld
endif
ifeq ($(CHIP),NRF51422)
LD_FILE         := $(ROOT_DIR)/platform/targets/nordic/build/nrf51822.ld
endif

#--------------------------------------------------------------------------------------------------
#     Compilation flags
#--------------------------------------------------------------------------------------------------

# Compiler flags
C_FLAGS         += -mcpu=$(CPU) -mthumb -mlittle-endian

# Linker flags
LD_FLAGS        += -mthumb -mcpu=$(CPU)
