###################################################################################################
#
# Source and include definition
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
#   Includes
#--------------------------------------------------------------------------------------------------

INC_DIRS  += \
	$(ROOT_DIR)/platform/targets/nordic/include \
	$(ROOT_DIR)/thirdparty/uecc \
	$(BSP_DIR)/components/boards \
	$(BSP_DIR)/components/drivers_nrf/nrf_soc_nosd \
	$(BSP_DIR)/components/libraries/bsp \
	$(BSP_DIR)/components/libraries/crypto \
	$(BSP_DIR)/components/libraries/util \
	$(BSP_DIR)/components/toolchain \
	$(BSP_DIR)/components/toolchain/cmsis/include \
	$(BSP_DIR)/external/nrf_cc310/include \
	$(BSP_DIR)/modules/nrfx \
	$(BSP_DIR)/modules/nrfx/drivers/include \
	$(BSP_DIR)/modules/nrfx/hal \
	$(BSP_DIR)/modules/nrfx/mdk \
	$(BSP_DIR)/modules/nrfx/templates

#--------------------------------------------------------------------------------------------------
#	BSP
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(ROOT_DIR)/platform/targets/nordic/build/retarget_gcc.c \
	$(BSP_DIR)/modules/nrfx/hal/nrf_ecb.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_gpiote.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_i2s.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_spim.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_twim.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_uarte.c \
	$(BSP_DIR)/modules/nrfx/drivers/src/prs/nrfx_prs.c

ifeq ($(CHIP),NRF52832_XXAA)

INC_DIRS  += \
	$(BSP_DIR)/config/nrf52832/config \
	$(BSP_DIR)/modules/nrfx/templates/nRF52832

C_FILES   += \
	$(BSP_DIR)/modules/nrfx/mdk/system_nrf52.c \
	$(ROOT_DIR)/platform/targets/nordic/build/startup_gcc_nrf52832.c

else # Default is NRF52840_XXAA

INC_DIRS  += \
	$(BSP_DIR)/config/nrf52840/config \
	$(BSP_DIR)/modules/nrfx/templates/nRF52840

C_FILES   += \
	$(BSP_DIR)/modules/nrfx/drivers/src/nrfx_qspi.c \
	$(BSP_DIR)/modules/nrfx/mdk/system_nrf52840.c \
	$(ROOT_DIR)/platform/targets/nordic/build/startup_gcc_nrf52840.c

LIBS  += \
	$(BSP_DIR)/external/nrf_cc310/lib/libnrf_cc310_softfp_0.9.10.a

endif

#--------------------------------------------------------------------------------------------------
#	Platform Abstraction Layer (PAL)
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_btn.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_cfg.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_codec.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_crypto.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_led.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_i2s.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_rtc.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_spi.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_sys.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_twi.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_uart.c

ifneq ($(USE_EXACTLE),0)
C_FILES   += \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_timer.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_bb.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_bb_ble.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_bb_ble_rf.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_bb_ble_tester.c
endif

ifeq ($(CHIP),NRF52840_XXAA)
C_FILES   += \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_flash.c \
	$(ROOT_DIR)/platform/targets/nordic/sources/pal_bb_154.c
endif

#--------------------------------------------------------------------------------------------------
#	Audio
#--------------------------------------------------------------------------------------------------

ifeq ($(AUDIO_CAPE),1)

# Board Support

C_FILES   += \
	$(ROOT_DIR)/platform/external/sources/io_exp/pca9557.c \
	$(ROOT_DIR)/platform/external/sources/codec/max9867.c

ifeq ($(CODEC),FRAUNHOFER)

INC_DIRS += \
	$(ROOT_DIR)/thirdparty/fraunhofer/ \
	$(ROOT_DIR)/thirdparty/fraunhofer/basic_op \
	$(ROOT_DIR)/thirdparty/fraunhofer/arm_inc

C_FILES   += \
	$(sort $(wildcard $(ROOT_DIR)/thirdparty/fraunhofer/*.c))

endif

ifeq ($(CODEC),BLUEDROID)

INC_DIRS += \
	$(ROOT_DIR)/thirdparty/bluedroid/encoder/include \
	$(ROOT_DIR)/thirdparty/bluedroid/decoder/include

C_FILES   += \
	$(sort $(wildcard $(ROOT_DIR)/thirdparty/bluedroid/encoder/srce/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/thirdparty/bluedroid/decoder/srce/*.c))

endif

endif

#--------------------------------------------------------------------------------------------------
#	Third Party: Security
#--------------------------------------------------------------------------------------------------

INC_DIRS  += \
	$(ROOT_DIR)/thirdparty/uecc

C_FILES   += \
	$(ROOT_DIR)/thirdparty/uecc/uECC_ll.c
