###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2017-2019 Arm Ltd. All Rights Reserved.
#
# Copyright (c) 2019 Packetcraft, Inc.
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
#	Configuration
#--------------------------------------------------------------------------------------------------

CFG_SYS         += BB_CLK_RATE_HZ=$(BB_CLK_RATE_HZ)

include $(ROOT_DIR)/platform/$(PLATFORM)/build/sources.mk

#--------------------------------------------------------------------------------------------------
# 	Startup files
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(ROOT_DIR)/platform/include\
	$(ROOT_DIR)/thirdparty/unity.framework/extras/fixture/src \
	$(ROOT_DIR)/thirdparty/unity.framework/src

C_FILES += \
	$(ROOT_DIR)/thirdparty/unity.framework/extras/fixture/src/unity_fixture.c \
	$(ROOT_DIR)/thirdparty/unity.framework/src/unity.c \
	$(sort $(wildcard $(ROOT_DIR)/platform/test/*.c)) \
	$(ROOT_DIR)/projects/platform/unittest/main.c
