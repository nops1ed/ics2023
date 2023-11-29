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


#define VA_VPN_0(x) (((vaddr_t)x & 0x001FF000u) >> 12)
#define VA_VPN_1(x) (((vaddr_t)x & 0x3FE00000u) >> 21)
#define VA_VPN_2(x) (((vaddr_t)x & 0x7FC0000000u) >> 30)
#define VA_OFFSET(x) ((vaddr_t)x & 0x00000FFFu)

#define PTE_PPN_MASK (0x3FFFFFFFFFFC00u)
#define PTE_PPN(x) (((vaddr_t)x & PTE_PPN_MASK) >> 10)

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

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

#define CONFIG_FFF 1
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  printf("\033[31mStarting translate\n");
#ifdef CONFIG_FFF
  paddr_t pagetable = (cpu.csr[CSR_SATP].val << 12) + PX(2, vaddr) * 8;
  //printf("Pagetable is %x\n", pagetable);
  PTE pte; 
  for(int level = 2; level > 0; level--) {
    pte = paddr_read(pagetable, 8);
    //assert(pte & PTE_V);
    pagetable = PTE2PA(pte) + PX(level - 1, vaddr) * 8;
  }
  pte = paddr_read(pagetable, 8);
  uint64_t MODE_PTE = type == 0 ? PTE_A : PTE_D;
  paddr_write(pagetable, 8, pte | MODE_PTE);
  paddr_t pa = PTE2PA(pte) + VA_OFFSET(vaddr);
  //assert(pa == vaddr);
  //printf("Now pa equals to %lx + %lx = %x\n",PTE2PA(pte), VA_OFFSET(vaddr), pa);
  //printf("MMU: Translate %lx to %x\n", vaddr, pa);
  return pa;
#else
  //static int a = 0;
  paddr_t page_table_entry_addr = (cpu.csr[CSR_SATP].val << 12);
  //printf("Pagetable is %x\n", page_table_entry_addr);
  PTE page_table_entry = paddr_read(page_table_entry_addr  + PX(2, vaddr) * 8, 8);
  paddr_t page1_table_entry_addr = PTE2PA(page_table_entry);

  PTE page1_table_entry = paddr_read(page1_table_entry_addr  + PX(1, vaddr) * 8, 8);
  paddr_t leaf_page_table_entry_addr = PTE2PA(page1_table_entry) + PX(0, vaddr) * 8;

  PTE leaf_page_table_entry = paddr_read(leaf_page_table_entry_addr, 8);

  if (type == 0){//读
    paddr_write(leaf_page_table_entry_addr, 8, leaf_page_table_entry | PTE_A);
  }else if (type == 1){//写
    paddr_write(leaf_page_table_entry_addr, 8, leaf_page_table_entry | PTE_D);
  }
  paddr_t pa = PTE2PA(leaf_page_table_entry) + VA_OFFSET(vaddr);
  //printf("Now pa equals to %lx + %lx = %x\n",PTE_PPN(leaf_page_table_entry) * 4096 , VA_OFFSET(vaddr), pa);
  //a++;
  //if(a > 200) assert(0);
  //assert(pa == vaddr);

/*
#define SATP_MASK 0X3fffff
#define PG_SHIFT 12
#define PGT1_ID(val) (val >> 22)
#define PGT2_ID(val) ((val & 0x3fffff) >> 12)
#define OFFSET(val) (val & 0xfff)
  word_t va_raw = (uint32_t)vaddr;
  paddr_t *pt_1 = (paddr_t *)guest_to_host((paddr_t)(cpu.csr[CSR_SATP].val << PG_SHIFT));
  // printf("pt1[id1(vaddr)] is 0x%x\n", pt_1[PGT1_ID(va_raw)]);
  assert(pt_1 != NULL);
  word_t *pt_2 = (word_t *)guest_to_host(pt_1[PGT1_ID(va_raw)]);
  // if (vaddr == 0x7ffffded)

  assert(pt_2 != NULL);
  paddr_t paddr = (paddr_t)((pt_2[PGT2_ID(va_raw)] & (~0xfff)) | OFFSET(va_raw));
  // if (vaddr >= 0x40000000 && vaddr <= 0x8000000)
  // {
  // printf("mmu translate vaddr %p\n", (void *)(long)vaddr);
  // printf("translate ui is 0x%x, pt2 id is %x\n", pt_2[PGT2_ID(va_raw)] >> 12, PGT2_ID(va_raw));
  // printf("mmu translate to paddr %p\n", (void *)(long)paddr);
  // } // printf("vaddr is 0x%x, paddr is 0x%x\n", vaddr, paddr);
  assert(vaddr >= 0x40000000 && vaddr <= 0xa1200000);
  // assert(paddr == vaddr);
  // return MEM_RET_FAIL;
  return paddr;
  */
/*
   //static int a = 0;
  paddr_t page_table_entry_addr = (cpu.csr[CSR_SATP].val << 12) + VA_VPN_2(vaddr) * 8;
  //printf("Pagetable is %x\n", page_table_entry_addr);
  PTE page_table_entry = paddr_read(page_table_entry_addr, 8);
  paddr_t page1_table_entry_addr = PTE_PPN(page_table_entry) * 4096 + VA_VPN_1(vaddr) * 8;

  PTE page1_table_entry = paddr_read(page1_table_entry_addr, 8);
  paddr_t leaf_page_table_entry_addr = PTE_PPN(page1_table_entry) * 4096 + VA_VPN_0(vaddr) * 8;

  PTE leaf_page_table_entry = paddr_read(leaf_page_table_entry_addr, 8);
  if (type == 0){//读
    paddr_write(leaf_page_table_entry_addr, 8, leaf_page_table_entry | PTE_A);
  }else if (type == 1){//写
    paddr_write(leaf_page_table_entry_addr, 8, leaf_page_table_entry | PTE_D);
  }
  paddr_t pa = PTE_PPN(leaf_page_table_entry) * 4096 + VA_OFFSET(vaddr);
  //printf("Now pa equals to %lx + %lx = %x\n",PTE_PPN(leaf_page_table_entry) * 4096 , VA_OFFSET(vaddr), pa);
  //a++;
  //if(a > 200) assert(0);
  //assert(pa == vaddr);


  return pa;
  */
#endif
}

