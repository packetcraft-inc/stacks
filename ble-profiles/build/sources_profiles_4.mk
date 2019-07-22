###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2012-2018 Arm Ltd.
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

PROFILE_FILTER_OUT := \
	$(wildcard $(ROOT_DIR)/ble-profiles/sources/profiles/wdxs/*phy*.c) \
	$(wildcard $(ROOT_DIR)/ble-profiles/sources/profiles/atp*/*)

INC_DIRS += \
	$(ROOT_DIR)/ble-profiles/sources/profiles/include \
	$(ROOT_DIR)/ble-profiles/sources/profiles

C_FILES += \
	$(sort $(filter-out $(PROFILE_FILTER_OUT),$(shell find $(ROOT_DIR)/ble-profiles/sources/profiles -name *.c)))
