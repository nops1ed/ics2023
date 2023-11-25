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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

// functional-programming-like macro (X-macro)
// apply the function `f` to each element in the container `c`
// NOTE1: `c` should be defined as a list like:
//   f(a0) f(a1) f(a2) ...
// NOTE2: each element in the container can be a tuple
#define CSR(f) f(MTVEC), f(MEPC), f(MSTATUS), f(MCAUSE), \
            f(MIE), f(MIP), f(MTVAL), f(MSCRATCH), f(SATP)
#define CSRPREFIX(name) concat(CSR_, name)
    /* CSR FILE. */
enum { CSR(CSRPREFIX) };

typedef union _csr_ {  
  struct {  
    uint32_t UIE: 1;      // User Interrupt Enable  
    uint32_t SIE: 1;      // Supervisor Interrupt Enable  
    uint32_t WPRI_0: 1;   // Waste Bits  
    uint32_t MIE: 1;      // Machine Interrupt Enable  
    uint32_t UPIE: 1;     // User Previous Interrupt Enable  
    uint32_t SPIE: 1;     // Supervisor Previous Interrupt Enable  
    uint32_t WPRI: 1;     // Waste Bits  
    uint32_t MPIE: 1;     // Machine Previous Interrupt Enable  
    uint32_t SPP: 1;      // Supervisor Previous Privilege  
    uint32_t WPRI_1_2: 2; // Waste Bits  
    uint32_t MPP: 2;      // Machine Previous Privilege  
    uint32_t FS: 2;       // Floating-point Status  
    uint32_t XS: 2;       // Extension Status  
    uint32_t MPRV: 1;     // Modified Privilege  
    uint32_t SUM: 1;      // Allow Supervisor User Memory access  
    uint32_t MXR: 1;      // Make eXecutable Readable  
    uint32_t TVM: 1;      // Trap Virtual Memory  
    uint32_t TW: 1;       // Timeout Wait  
    uint32_t TSR: 1;      // Task Set Register  
    uint32_t WPRI_3_10: 8;// Waste Bits  
    uint32_t SD: 1;       // Secure Delegation  
  } status;  
  word_t val;  
} _csr_;  

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  _csr_ csr[9];
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

//#define isa_mmu_check(vaddr, len, type) (cpu.csr[CSR_SATP].val >> 31)
#define isa_mmu_check(vaddr, len, type) (printf("NNNNNNN\n"))

#endif
