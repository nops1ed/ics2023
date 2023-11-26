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
/* shift a physical address to the right place for a PTE. */
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  printf("\033[31mTraping into mmu translate\033[0m\n");
  printf("\033[32mNow satp has val %lx\033[0m\n", cpu.csr[CSR_SATP].val);

  word_t va_raw = (uint64_t)vaddr;
  paddr_t *pt_1 = (paddr_t *)guest_to_host((paddr_t)(cpu.csr[CSR_SATP].val << PGSHIFT));
  assert(pt_1 != NULL);
  word_t *pt_2 = (word_t *)guest_to_host(pt_1[PX(2, va_raw)]);
  assert(pt_2 != NULL);
  word_t *pt_3 = (word_t *)guest_to_host(pt_2[PX(1, va_raw)]);
  assert(pt_3 != NULL);

  paddr_t paddr = (paddr_t)((pt_3[PX(0, va_raw)] & (~0xfff)) | (va_raw & 0xfff));
  assert(vaddr >= 0x40000000 && vaddr <= 0xa1200000);
  printf("Successfully convert %lx to %x\n", vaddr, paddr);
  return paddr;
}
