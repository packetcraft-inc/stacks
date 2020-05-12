###################################################################################################
#
# Source and include definition
#
# Copyright (c) 2012-2018 Arm Ltd. All Rights Reserved.
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
	$(ROOT_DIR)/ble-profiles/sources/profiles/include \
	$(ROOT_DIR)/ble-profiles/sources/profiles

C_FILES += \
	$(ROOT_DIR)/ble-profiles/sources/profiles/anpc/anpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/atpc/atpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/atps/atps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/bas/bas_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/blpc/blpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/blps/blps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/cpp/cpps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/cscp/cscps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/dis/dis_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/fmpl/fmpl_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/gap/gap_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/gatt/gatt_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/glpc/glpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/glps/glps_db.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/glps/glps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/hid/hid_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/hrpc/hrpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/hrps/hrps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/htpc/htpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/htps/htps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/paspc/paspc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/plxpc/plxpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/plxps/plxps_db.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/plxps/plxps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/rscp/rscps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/scpps/scpps_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/sensor/gyro_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/sensor/temp_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/tipc/tipc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/udsc/udsc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/uribeacon/uricfg_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxc/wdxc_ft.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxc/wdxc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxc/wdxc_stream.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxs/wdxs_au.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxs/wdxs_dc.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxs/wdxs_ft.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxs/wdxs_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxs/wdxs_phy.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wdxs/wdxs_stream.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wpc/wpc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wspc/wspc_main.c \
	$(ROOT_DIR)/ble-profiles/sources/profiles/wsps/wsps_main.c
