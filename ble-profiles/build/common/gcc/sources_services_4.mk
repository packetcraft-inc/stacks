###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2012-2017 ARM Ltd. All Rights Reserved.
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

INC_DIRS += \
	$(ROOT_DIR)/ble-profiles/sources/services

C_FILES += \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_alert.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_batt.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_bps.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_core.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_cps.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_cscs.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_dis.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_gls.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_gyro.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_hid.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_hrs.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_hts.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_ipss.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_plxs.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_px.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_rscs.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_scpss.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_temp.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_time.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_uricfg.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_wdxs.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_wp.c \
	$(ROOT_DIR)/ble-profiles/sources/services/svc_wss.c
