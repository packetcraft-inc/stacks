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
#     Sources
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(ROOT_DIR)/controller/include/common \
	$(ROOT_DIR)/controller/include/ble \
	$(ROOT_DIR)/controller/sources/ble/include \
	$(ROOT_DIR)/thirdparty/uecc

C_FILES += \
	$(sort $(wildcard $(ROOT_DIR)/controller/sources/common/bb/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/controller/sources/common/chci/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/controller/sources/common/sch/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/controller/sources/ble/bb/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/controller/sources/ble/init/*.c)) \
	$(sort $(filter-out $(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_tester.c, $(wildcard $(ROOT_DIR)/controller/sources/ble/lctr/*.c))) \
	$(sort $(filter-out $(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs_tester.c, $(wildcard $(ROOT_DIR)/controller/sources/ble/lhci/*.c))) \
	$(sort $(filter-out $(ROOT_DIR)/controller/sources/ble/ll/ll_main_tester.c, $(wildcard $(ROOT_DIR)/controller/sources/ble/ll/*.c))) \
	$(sort $(wildcard $(ROOT_DIR)/controller/sources/ble/lmgr/*.c)) \
	$(sort $(wildcard $(ROOT_DIR)/controller/sources/ble/sch/*.c))
