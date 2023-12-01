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
#include <memory/vaddr.h>
#include <memory/paddr.h>
#define VA_OFFSET(x) ((vaddr_t)x & 0x00000FFFu)

#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_A 0x40
#define PTE_D 0x80

typedef uint64_t PTE;
#define PGSHIFT         12
/* extract the three 9-bit page table indices from a virtual address. */
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64_t) (va)) >> PXSHIFT(level)) & PXMASK)

/* shift a physical address to the right place for a PTE. */
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  paddr_t pagetable = (cpu.csr[CSR_SATP].val << 12) + PX(2, vaddr) * 8;
  PTE pte; 
  for(int level = 2; level > 0; level--) {
    pte = paddr_read(pagetable, 8);
    assert(pte & PTE_V);
    pagetable = PTE2PA(pte) + PX(level - 1, vaddr) * 8;
  }
  pte = paddr_read(pagetable, 8);
  /* PTE dirty bit is ready for TLB. */
  uint64_t MODE_PTE = type == 0 ? PTE_A : PTE_D;
  paddr_write(pagetable, 8, pte | MODE_PTE);
  paddr_t pa = PTE2PA(pte) + VA_OFFSET(vaddr);
  return pa;
}