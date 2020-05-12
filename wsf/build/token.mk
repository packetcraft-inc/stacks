###################################################################################################
#
# Token generation make targets
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
#     Project
#--------------------------------------------------------------------------------------------------

# Parent makefile must export the following variables
#    CC
#    ROOT_DIR
#    BSP_DIR
#    BIN
#    C_FILES
#    C_FLAGS

# Toolchain
PYTHON          := python
TOK_TO_HDR      := $(PYTHON) $(ROOT_DIR)/wsf/build/token2header.py
MONITOR         := $(PYTHON) $(ROOT_DIR)/wsf/trace_monitor.py

# Output
TOK_DIR         := tok
TOKEN_DEF       := $(BIN:.elf=.tokens)
TOKEN_HDR       := $(BIN:.elf=_tokens.h)

#-------------------------------------------------------------------------------
#     Scripts
#-------------------------------------------------------------------------------

HASH_SCR        := import sys, binascii; \
                   print hex(binascii.crc32(sys.argv[1]) & 0xFFFF)
FILE_HASH       = $(shell $(PYTHON) -c "$(HASH_SCR)" $(notdir $(1)))

#--------------------------------------------------------------------------------------------------
#     Objects
#--------------------------------------------------------------------------------------------------

TOK_FILES       := $(C_FILES:.c=.pp)
TOK_FILES       := $(subst $(ROOT_DIR)/,$(TOK_DIR)/,$(TOK_FILES))
TOK_FILES       := $(filter-out tok/thirdparty/%,$(TOK_FILES))

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

token: $(TOK_FILES)
	@rm -f $(TOKEN_DEF)
	@mkdir -p $(dir $(TOKEN_DEF))
	@find $(TOK_DIR) -name \*.mod -exec cat {} \; >> $(TOKEN_DEF)
	@find $(TOK_DIR) -name \*.pp -exec grep __WSF_TOKEN_DEFINE__ {} \; | cut -d"(" -f2 | cut -d")" -f1 >> $(TOKEN_DEF)
	@$(TOK_TO_HDR) $(TOKEN_DEF) $(TOKEN_HDR)

token.monitor:
	@echo "Detokenize: $(TOKEN_DEF)"
ifneq ($(TOKEN_DEV),)
	@$(MONITOR) -d=$(TOKEN_DEV) -b=$(BAUD) -p=$(TOKEN_FILTER) $(TOKEN_DEF)
else
	@echo "error: missing TOKEN_DEV make enviornment definition"
endif

$(TOK_DIR)/%.pp: $(ROOT_DIR)/%.c
	@echo "+++ Scanning: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) -DTOKEN_GENERATION -DMODULE_ID=$(call FILE_HASH,$<) -E -o $@ $<
	@echo "$(call FILE_HASH,$<), 0, $<, , ," >> $(@:.pp=.mod)

$(TOK_DIR)/%.pp: $(BSP_DIR)/%.c
	@echo "+++ Scanning: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) -DTOKEN_GENERATION -DMODULE_ID=$(call FILE_HASH,$<) -E -o $@ $<
	@echo "$(call FILE_HASH,$<), 0, $<, , ," >> $(@:.pp=.mod)

token.clean:
	@rm -rf $(TOK_DIR)
	@rm -f $(TOKEN_DEF)
	@rm -f $(TOKEN_HDR)

.PHONY: token.clean
