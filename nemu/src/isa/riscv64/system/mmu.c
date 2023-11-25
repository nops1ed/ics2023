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

#define PGSHIFT         12
/* extract the three 9-bit page table indices from a virtual address. */
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64_t) (va)) >> PXSHIFT(level)) & PXMASK)

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  paddr_t *pagetable = (paddr_t *)guest_to_host((paddr_t)(cpu.csr[CSR_SATP].val << PGSHIFT));
  word_t *pte;
  for(int level = 2; level > 0; level--) {
    pte = (word_t *)guest_to_host(pagetable[PX(level, vaddr)]);
    assert(pte != NULL);
    pagetable = (paddr_t *)PTE2PA(*pte);
    assert(pagetable != NULL);
  }
  assert(vaddr >= 0x40000000 && vaddr <= 0xa1200000);
  paddr_t paddr = (paddr_t)(pagetable[PX(0, vaddr)]);
  assert(paddr == vaddr);
  printf("Successfully convert %lx to %x\n", vaddr, paddr);
  return paddr;
}
