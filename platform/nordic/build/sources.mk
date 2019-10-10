###################################################################################################
#
# Source and include definition
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

INC_DIRS  += \
	$(BSP_DIR)/components/boards \
	$(BSP_DIR)/modules/nrfx/mdk \
	$(BSP_DIR)/modules/nrfx \
	$(BSP_DIR)/modules/nrfx/templates \
	$(BSP_DIR)/modules/nrfx/hal \
	$(BSP_DIR)/modules/nrfx/drivers/include \
	$(BSP_DIR)/modules/nrfx/drivers \
	$(BSP_DIR)/modules/nrfx/soc \
	$(BSP_DIR)/components/drivers_nrf/nrf_soc_nosd \
	$(BSP_DIR)/components/libraries/crypto \
	$(BSP_DIR)/components/libraries/util \
	$(BSP_DIR)/components/toolchain/CMSIS/Include \
	$(BSP_DIR)/external/nrf_cc310/include \
	$(ROOT_DIR)/platform/nordic/include \
	$(ROOT_DIR)/platform/include \
	$(ROOT_DIR)/thirdparty/uecc

ifeq (NRF52840_XXAA,$(findstring NRF52840_XXAA,$(CFG_DEV)))
INC_DIRS  += \
	$(BSP_DIR)/modules/nrfx/templates/nRF52840
else
ifeq (NRF52832_XXAA,$(findstring NRF52832_XXAA,$(CFG_DEV)))
INC_DIRS  += \
	$(BSP_DIR)/modules/nrfx/templates/nRF52832
endif
endif

#--------------------------------------------------------------------------------------------------
#	BSP files
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(ROOT_DIR)/platform/nordic/build/retarget_gcc.c \
	$(BSP_DIR)/modules/nrfx/hal/nrf_ecb.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_twim.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_uarte.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_uart.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/prs/nrfx_prs.c

ifeq (NRF52840_XXAA,$(findstring NRF52840_XXAA,$(CFG_DEV)))
C_FILES   += \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_qspi.c \
	$(ROOT_DIR)/platform/nordic/build/startup_gcc_nrf52840.c

ifeq ($(PLATFORM_MESH),1)
LIBS  += \
	$(BSP_DIR)/external/nrf_cc310/lib/libnrf_cc310_softfp_0.9.10.a
endif

else
ifeq (NRF52832_XXAA,$(findstring NRF52832_XXAA,$(CFG_DEV)))
C_FILES   += \
	$(ROOT_DIR)/platform/nordic/build/startup_gcc_nrf52832.c
endif
endif

#--------------------------------------------------------------------------------------------------
#	Platform files
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(ROOT_DIR)/platform/nordic/sources/pal_sys.c

#--------------------------------------------------------------------------------------------------
#	BB files
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(ROOT_DIR)/platform/nordic/sources/pal_bb.c

#--------------------------------------------------------------------------------------------------
#	Driver files
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(sort $(wildcard $(ROOT_DIR)/platform/external/sources/audio_amp/*.c))

C_FILES   += \
	$(sort $(wildcard $(ROOT_DIR)/platform/external/sources/io_exp/*.c))

C_FILES   += \
	$(ROOT_DIR)/platform/nordic/sources/pal_btn.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_led.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_timer.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_uart.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_rtc.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_cfg.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_twi.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_nvm.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_crypto.c
