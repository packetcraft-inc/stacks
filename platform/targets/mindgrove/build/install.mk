###################################################################################################
#
# Install make targets
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

# Toolchain
GDB             := $(CROSS_COMPILE)gdb
GDBSERVER       := JLinkGDBServer
ifeq ($(OS),Windows_NT)
JLINKEXE        := JLink.exe
ECHO            := echo -e
else
JLINKEXE        := JLinkExe
ECHO            := echo
endif

ifeq (NRF51,$(findstring NRF51,$(CFG_DEV)))
GDBSERVER       += -if SWD -device nRF51 -speed 10000
JLINKEXE        += -if SWD -device nRF51 -speed 10000
else
GDBSERVER       += -if SWD -device nRF52 -speed 100000
JLINKEXE        += -if SWD -device nRF52 -speed 100000
endif

#--------------------------------------------------------------------------------------------------
#     Targets
#--------------------------------------------------------------------------------------------------

install: install.flash

install.flash:
	@$(ECHO) "$(if $(SN), "SelectEmuBySN $(SN)",)\nhalt\nloadbin $(BIN:.elf=.bin),0\nr\ng\nexit\n" > loadbin.jlinkexe
	@$(JLINKEXE) -CommandFile loadbin.jlinkexe

install.flashall:
	@rm -f loadbin.jlinkexe
	@for sn in $(shell $(MAKE) PLATFORM=$(PLATFORM) device.sn); do \
		$(ECHO) "SelectEmuBySN $$sn\nhalt\nloadbin $(BIN:.elf=.bin),0\nr\ng\n" >> loadbin.jlinkexe; \
		done;
	@test -s loadbin.jlinkexe || { $(ECHO) "error: no devices found" >&2; exit 1; }
	@$(ECHO) "exit\n" >> loadbin.jlinkexe
	@$(JLINKEXE) -CommandFile loadbin.jlinkexe

install.server:
	@$(GDBSERVER) $(if $(SN),-select usb=$(SN),)

device.sn:
	@$(ECHO) "ShowEmuList\nexit\n" > showlist.jlinkexe
	@$(JLINKEXE) -CommandFile showlist.jlinkexe | sed -n 's/^.*Serial number: \([0-9]*\).*/\1/p' | grep [0-9]

device.reset:
	@$(ECHO) $(if $(SN), "SelectEmuBySN $(SN)",)  > reset.jlinkexe
	@$(ECHO) "h"                                 >> reset.jlinkexe
	@$(ECHO) "r"                                 >> reset.jlinkexe
	@$(ECHO) "g"                                 >> reset.jlinkexe
	@$(ECHO) "exit"                              >> reset.jlinkexe
	@$(JLINKEXE) -CommandFile reset.jlinkexe

device.resetall:
	@rm -f reset.jlinkexe
	@for sn in $(shell $(MAKE) PLATFORM=$(PLATFORM) device.sn); do \
		$(ECHO) "SelectEmuBySN $$sn" >> reset.jlinkexe; \
		$(ECHO) "h"                  >> reset.jlinkexe; \
		$(ECHO) "r"                  >> reset.jlinkexe; \
		$(ECHO) "g"                  >> reset.jlinkexe; \
		done
	@test -s reset.jlinkexe || { $(ECHO) "error: no devices found" >&2; exit 1; }
	@$(ECHO)    "exit"               >> reset.jlinkexe
	@$(JLINKEXE) -CommandFile reset.jlinkexe

device.erase:
	@rm -f erase.jlinkexe
	@$(ECHO) "halt\nw4 4001E504, 2\nw4 4001e50C, 1\nSleep 100\nexit\n" > erase.jlinkexe
	@$(JLINKEXE) -CommandFile erase.jlinkexe

.PHONY: install install.flash install.flashall install.server \
        device.sn device.reset device.resetall device.erase
