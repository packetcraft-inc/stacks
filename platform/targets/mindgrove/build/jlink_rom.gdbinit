###################################################################################################
#
# J-Link GDB startup script.
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

# Macros
define restart
  monitor reset 0
  monitor reg r13 = (0x00000000)
  monitor reg pc = (0x00000004)
  continue
end

# Environment
#set verbose on

# Connect to device
target remote localhost:2331
monitor interface swd
monitor speed 10000

# Enable flash options
monitor endian little
monitor flash device = NRF51822
monitor clrbp

# Halt CPU and download image
monitor reset 0
load

# Setup windows
layout sources
tabset 2

# Run to main
break main
restart
