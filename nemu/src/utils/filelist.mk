#***************************************************************************************
# Copyright (c) 2014-2022 Zihao Yu, Nanjing University
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/

ifneq ($(CONFIG_ITRACE)$(CONFIG_IQUEUE),)
CXXSRC = src/utils/disasm.cc
LLVM_CONFIG := $(shell which llvm-config 2>/dev/null || echo /opt/homebrew/Cellar/llvm/19.1.7_1/bin/llvm-config)
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags) -fPIE
LIBS += $(shell $(LLVM_CONFIG) --libs)
LDFLAGS += $(shell $(LLVM_CONFIG) --ldflags)
endif
