/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>
#include <isa.h>

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

/* The form of idx we defined here was like 0x$$$. */
static inline int csr_idx(uint32_t idx) {
  switch(idx) {
    case 0x300: return CSR_MSTATUS;
    case 0x304: return CSR_MIE;
    case 0x305: return CSR_MTVEC;
    //case 0x340: return CSR_MSCRATCH;
    case 0x341: return CSR_MEPC;
    case 0x342: return CSR_MCAUSE;
    case 0x343: return CSR_MTVAL;
    case 0x344: return CSR_MIP;
    case 0x180: return CSR_SATP;
    default:  
      printf("\033[031mGonna to visit CSR %x\033[0m\n", idx);
      panic("Undefined CSR\n");
  }
  return 0;
} 
  
#define csr(idx) (cpu.csr[csr_idx(idx)])
/*
  0x300 MRW mstatus     Machine status register.
  0x304 MRW mie         Machine interrupt-enable register.
  0x305 MRW mtvec       Machine trap-handler base address.
  0x340 MRW mscratch    Scratch register for machine trap handlers.
  0x341 MRW mepc        Machine exception program counter.
  0x342 MRW mcause      Machine trap cause.
  0x343 MRW mtval       Machine bad address or instruction.
  0x344 MRW mip         Machine interrupt pending.
*/
#endif
