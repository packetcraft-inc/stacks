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

INC_DIRS += \
	$(ROOT_DIR)/ble-host/include \
	$(ROOT_DIR)/ble-host/sources/stack/att \
	$(ROOT_DIR)/ble-host/sources/stack/cfg \
	$(ROOT_DIR)/ble-host/sources/stack/dm \
	$(ROOT_DIR)/ble-host/sources/stack/hci \
	$(ROOT_DIR)/ble-host/sources/stack/l2c \
	$(ROOT_DIR)/ble-host/sources/stack/smp \
	$(ROOT_DIR)/platform/include

C_FILES += \
	$(ROOT_DIR)/ble-host/sources/stack/att/att_main.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/att_uuid.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/attc_disc.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/attc_main.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/attc_proc.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/attc_read.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/attc_sign.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/attc_write.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_ccc.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_csf.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_dyn.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_ind.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_main.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_proc.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_read.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_sign.c \
	$(ROOT_DIR)/ble-host/sources/stack/att/atts_write.c \
	$(ROOT_DIR)/ble-host/sources/stack/cfg/cfg_stack.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_adv.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_adv_leg.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_conn.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_conn_master.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_conn_master_leg.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_conn_slave.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_conn_slave_leg.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_conn_sm.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_dev.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_dev_priv.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_main.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_priv.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_scan.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_scan_leg.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_sec.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_sec_lesc.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_sec_master.c \
	$(ROOT_DIR)/ble-host/sources/stack/dm/dm_sec_slave.c \
	$(ROOT_DIR)/ble-host/sources/stack/hci/hci_main.c\
	$(ROOT_DIR)/ble-host/sources/stack/l2c/l2c_coc.c \
	$(ROOT_DIR)/ble-host/sources/stack/l2c/l2c_main.c \
	$(ROOT_DIR)/ble-host/sources/stack/l2c/l2c_master.c \
	$(ROOT_DIR)/ble-host/sources/stack/l2c/l2c_slave.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smp_act.c \
 	$(ROOT_DIR)/ble-host/sources/stack/smp/smp_db.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smp_main.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smp_non.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smp_sc_act.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smp_sc_main.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpi_act.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpi_sc_act.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpi_sc_sm.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpi_sm.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpr_act.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpr_sc_act.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpr_sc_sm.c \
	$(ROOT_DIR)/ble-host/sources/stack/smp/smpr_sm.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_aes.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_aes_rev.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_ccm_hci.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_cmac_hci.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_ecc_debug.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_ecc_hci.c \
	$(ROOT_DIR)/ble-host/sources/sec/common/sec_main.c
