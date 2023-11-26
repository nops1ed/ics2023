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
#define PTE_A 0x40
#define PTE_D 0x80

typedef uint64_t PTE;

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  //printf("\033[31mStarting translate\n");
  paddr_t page_table_entry_addr = (cpu.csr[CSR_SATP].val << 12) + VA_VPN_2(vaddr) * 8;
  PTE page_table_entry = paddr_read(page_table_entry_addr, 8);
  // if ((page_table_entry & PTE_V) == 0){
  //   Log("page_table_entry not valid, vaddr: %#x", vaddr);
  // }
  paddr_t page1_table_entry_addr = PTE_PPN(page_table_entry) * 4096 + VA_VPN_1(vaddr) * 8;
  PTE page1_table_entry = paddr_read(page1_table_entry_addr, 8);
  
  paddr_t leaf_page_table_entry_addr = PTE_PPN(page1_table_entry) * 4096 + VA_VPN_0(vaddr) * 8;
  PTE leaf_page_table_entry = paddr_read(leaf_page_table_entry_addr, 8);
  // if ((leaf_page_table_entry & PTE_V) == 0){
  //   Log("leaf_page_table_entry not valid, vaddr: %#x, pte_entry: %#x", vaddr, leaf_page_table_entry_addr);
  // }

  if (type == 0){//读
    paddr_write(leaf_page_table_entry_addr, 8, leaf_page_table_entry | PTE_A);
  }else if (type == 1){//写
    paddr_write(leaf_page_table_entry_addr, 8, leaf_page_table_entry | PTE_D);
  }
  paddr_t pa = PTE_PPN(leaf_page_table_entry) * 4096 + VA_OFFSET(vaddr);
  //assert(pa == vaddr);

  return pa;
}
