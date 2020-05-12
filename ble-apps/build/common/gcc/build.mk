###################################################################################################
#
# Common GCC build make targets.
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
#     Toolchain
#--------------------------------------------------------------------------------------------------

# GCC ARM cross toolchain
CROSS_COMPILE   := arm-none-eabi-

# Toolchain
CC              := $(CROSS_COMPILE)gcc
AR              := $(CROSS_COMPILE)gcc-ar
LD              := $(CROSS_COMPILE)gcc
DEP             := $(CROSS_COMPILE)gcc
OBJDUMP         := $(CROSS_COMPILE)objdump
OBJCOPY         := $(CROSS_COMPILE)objcopy
SIZE            := $(CROSS_COMPILE)size

#--------------------------------------------------------------------------------------------------
#     Compiler Directives
#--------------------------------------------------------------------------------------------------

C_FLAGS         += $(addprefix -I,$(INC_DIRS))
C_FLAGS         += $(addprefix -D,$(CFG_DEV))

# Dependency flags
DEP_FLAGS       := $(C_FLAGS) -MM -MF

#--------------------------------------------------------------------------------------------------
#     Object files
#--------------------------------------------------------------------------------------------------

# Object files
OBJ_FILES       := $(C_FILES:.c=.o)
OBJ_FILES       := $(subst $(ROOT_DIR)/,$(INT_DIR)/,$(OBJ_FILES))
OBJ_FILES       := $(subst $(BSP_DIR)/,$(INT_DIR)/,$(OBJ_FILES))
OBJ_FILES       := $(subst ./,$(INT_DIR)/,$(OBJ_FILES))

# Dependency files
DEP_FILES       := $(OBJ_FILES:.o=.d)

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

all: $(BIN) show.options
	@echo "+++ Toolchain"
	@$(CC) --version | head -n 1

$(BIN): $(OBJ_FILES) $(LD_FILE)
	@echo "+++ Linking: $@"
	@mkdir -p $(BIN_DIR)
	@$(LD) -o $(BIN) -T$(LD_FILE) $(LD_FLAGS) $(OBJ_FILES)
	@$(OBJDUMP) -t $(BIN) > $(BIN:.elf=.sym)
	@$(OBJCOPY) -O binary $(BIN) $(BIN:.elf=.bin)
	@$(OBJCOPY) -O ihex $(BIN) $(BIN:.elf=.hex)
	@$(OBJDUMP) -t $(BIN) > $(BIN:.elf=.sym)
	@echo "+++ Binary summary: $(BIN)"
	@-$(SIZE) $(BIN)
	@echo "+++ Section summary: $(BIN:.elf=.map)"
	@grep ^.text $(BIN:.elf=.map) | awk '{print "\t" $$3 "\t" $$1}'
	@grep ^.bss  $(BIN:.elf=.map) | awk '{print "\t" $$3 "\t" $$1}'
	@grep ^.data $(BIN:.elf=.map) | awk '{print "\t" $$3 "\t" $$1}'

$(INT_DIR)/%.o: $(ROOT_DIR)/%.c
	@echo "+++ Compiling: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) -DMODULE_ID=$(call FILE_HASH,$<) -c -o $@ $<
	@$(if $(DEP),$(DEP) $(DEP_FLAGS) $(subst .o,.d,$@) -MP -MT $@ $<,)

$(INT_DIR)/%.o: $(ROOT_DIR)/thirdparty/%.c
	@echo "+++ Compiling: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(filter-out -W%),$(C_FLAGS)) -c -o $@ $<
	@$(if $(DEP),$(DEP) $(DEP_FLAGS) $(subst .o,.d,$@) -MP -MT $@ $<,)

clean:
	@rm -rf $(INT_DIR)
	@rm -rf $(BIN_DIR)

show.options:
	@echo "+++ Build options"
	@echo "    CPU              = $(CPU)"
	@echo "    CHIP             = $(CHIP)"
	@echo "    PLATFORM         = $(PLATFORM)"
	@echo "    BOARD            = $(BOARD)"
	@echo "    BSP_DIR          = $(BSP_DIR)"
	@echo "    RTOS             = $(RTOS)"
	@echo "    USE_EXACTLE      = $(USE_EXACTLE)"
	@echo "    DEBUG            = $(DEBUG)"
	@echo "    TRACE            = $(TRACE)"
	@echo "    USE_LTO          = $(USE_LTO)"
	@echo "    CFG_DEV          = $(strip $(CFG_DEV))"

show.config:
	@for s in $(CFG_DEV); \
		do echo $$s; \
	done

show.includes:
	@for f in $(subst $(ROOT_DIR)/,,$(INC_DIRS)); \
		do echo $$f; \
	done

show.headers:
	@for f in $(subst $(ROOT_DIR)/,,$(sort $(shell find $(INC_DIRS) -name *.h -print))); \
		do echo $$f; \
	done

show.sources:
	@for f in $(subst $(ROOT_DIR)/,,$(sort $(C_FILES) $(wildcard $(addsuffix *.h,$(dir $(C_FILES)))))); \
		do echo $$f; \
	done

show.platform:
	@echo $(PLATFORM_LIST)

show.rtos:
	@echo $(RTOS_LIST)

-include $(DEP_FILES)

.PHONY: all clean
.PHONY: show.options show.includes show.sources show.platform show.rtos
