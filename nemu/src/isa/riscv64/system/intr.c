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

#include <isa.h>
#include "../local-include/reg.h"

#ifndef __MACRO_IRQ_NUM__
#define __MACRO_IRQ_NUM__
#if defined(__ISA_X86__)
#define IRQ_TIMER 32        
#elif defined(__riscv)
#define IRQ_TIMER 0x8000000000000007 
#elif defined(__ISA_MIPS32__)
#define IRQ_TIMER 0      
#elif defined(__ISA_LOONGARCH32R__)
#endif
#endif

#define IRQ_TIMER 0x8000000000000007 
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  cpu.csr[CSR_MEPC].val = epc;
  cpu.csr[CSR_MCAUSE].val = NO;

  cpu.csr[CSR_MSTATUS].status.MPIE = cpu.csr[CSR_MSTATUS].status.MIE;
  /* Disable Interrupt. */
  cpu.csr[CSR_MSTATUS].status.MIE = 0;
  /* Indicate Machine mode. */
  //cpu.csr[CSR_MSTATUS].status.MPP = 11;
  return cpu.csr[CSR_MTVEC].val;
}

word_t isa_query_intr() {
  /* Query clock interrupt. */
  if (cpu.INTR && (cpu.csr[CSR_MSTATUS].status.MIE == 1)) {
    cpu.INTR = false;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}