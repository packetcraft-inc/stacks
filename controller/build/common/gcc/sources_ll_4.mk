###################################################################################################
#
# Source file definitions for LL v4.2 features.
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
#     Sources
#--------------------------------------------------------------------------------------------------

INC_DIRS += \
	$(ROOT_DIR)/controller/include/common \
	$(ROOT_DIR)/controller/include/ble \
	$(ROOT_DIR)/controller/sources/ble/include \
	$(ROOT_DIR)/thirdparty/uecc

C_FILES += \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_conn_slave.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_conn.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_main.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_dtm.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_whitelist.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_reslist.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_periodiclist.c \
	$(ROOT_DIR)/controller/sources/ble/bb/bb_ble_pdufilt.c \
	$(ROOT_DIR)/controller/sources/ble/init/init_ctr.c \
	$(ROOT_DIR)/controller/sources/ble/init/init.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_act_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_act_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_act_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_act_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_act_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_act_enc.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_act_init_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_isr_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_isr_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_isr_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_isr_conn_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_isr_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_isr_init_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_conn_data.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_conn_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_init_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_priv.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main_sc.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_main.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_pdu_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_pdu_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_pdu_enc.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_conn_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_init_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_llcp_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_llcp_conn_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_llcp_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_llcp_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/lctr/lctr_sm_llcp_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_adv_priv.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_conn_priv.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_sc.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs_sc.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd_vs.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_cmd.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_conn_priv.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_sc.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt_vs.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_evt.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_adv_priv.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_conn_priv.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init_sc.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_init.c \
	$(ROOT_DIR)/controller/sources/ble/lhci/lhci_main.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_conn_slave.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_init_master.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_priv.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init_sc.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_init.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_adv_master.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_adv_slave.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_conn_master.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_conn_slave.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_conn.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_dtm.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_enc_master.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_enc_slave.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_priv.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main_sc.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_main.c \
	$(ROOT_DIR)/controller/sources/ble/ll/ll_math.c \
	$(ROOT_DIR)/controller/sources/ble/lmgr/lmgr_events.c \
	$(ROOT_DIR)/controller/sources/ble/lmgr/lmgr_main_conn.c \
	$(ROOT_DIR)/controller/sources/ble/lmgr/lmgr_main_master.c \
	$(ROOT_DIR)/controller/sources/ble/lmgr/lmgr_main_priv.c \
	$(ROOT_DIR)/controller/sources/ble/lmgr/lmgr_main_sc.c \
	$(ROOT_DIR)/controller/sources/ble/lmgr/lmgr_main_slave.c \
	$(ROOT_DIR)/controller/sources/ble/lmgr/lmgr_main.c \
	$(ROOT_DIR)/controller/sources/ble/sch/sch_ble.c \
	$(ROOT_DIR)/controller/sources/ble/sch/sch_rm.c \
	$(ROOT_DIR)/controller/sources/ble/sch/sch_tm.c \
	$(ROOT_DIR)/controller/sources/common/bb/bb_main.c \
	$(ROOT_DIR)/controller/sources/common/chci/chci_tr.c \
	$(ROOT_DIR)/controller/sources/common/sch/sch_list.c \
	$(ROOT_DIR)/controller/sources/common/sch/sch_main.c
