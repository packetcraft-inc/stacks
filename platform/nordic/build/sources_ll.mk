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

#--------------------------------------------------------------------------------------------------
#   Startup files
#--------------------------------------------------------------------------------------------------

INC_DIRS  += \
	$(BSP_DIR)/components/boards \
	$(BSP_DIR)/components/device \
	$(BSP_DIR)/components/drivers_nrf/hal \
	$(BSP_DIR)/components/drivers_nrf/nrf_soc_nosd \
	$(BSP_DIR)/components/libraries/bsp \
	$(BSP_DIR)/components/libraries/util \
	$(BSP_DIR)/components/toolchain \
	$(BSP_DIR)/components/toolchain/CMSIS/Include \
	$(ROOT_DIR)/platform/nordic/include \
	$(ROOT_DIR)/platform/include

#--------------------------------------------------------------------------------------------------
#	LL files
#--------------------------------------------------------------------------------------------------

INC_DIRS  += \
	$(ROOT_DIR)/thirdparty/uecc

C_FILES   += \
	$(ROOT_DIR)/thirdparty/uecc/uECC_ll.c

#--------------------------------------------------------------------------------------------------
#	BB files
#--------------------------------------------------------------------------------------------------

C_FILES   += \
	$(ROOT_DIR)/platform/nordic/sources/pal_bb_ble.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_bb_ble_rf.c \
	$(ROOT_DIR)/platform/nordic/sources/pal_bb_ble_tester.c
