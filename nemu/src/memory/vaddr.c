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
#include <memory/paddr.h>

word_t vaddr_ifetch(vaddr_t addr, int len) {
  if(isa_mmu_check(addr, len, MEM_TYPE_IFETCH)) {
    printf("\033[31mTraping into vaddr_ifetch\033[0m\n");
    return paddr_read(isa_mmu_translate(addr, len, MEM_TYPE_IFETCH), len);
  }
  return paddr_read(addr, len);
}

word_t vaddr_read(vaddr_t addr, int len) {
#ifdef CONFIG_MTRACE
  //Log("Memory Read: " FMT_PADDR "at pc: " FMT_WORD, addr, cpu.pc);
  /* This performed better */
  printf("Memory Read : 0x%lx at pc: 0x%lx\n", addr, cpu.pc);
#endif
  if(isa_mmu_check(addr, len, MEM_TYPE_READ)) 
    return paddr_read(isa_mmu_translate(addr, len, MEM_TYPE_READ), len);
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE
  printf("Memory Write: 0x%lx at pc: 0x%lx\n", addr, cpu.pc);
  printf("Try to write %lx\n", data);
  //Log("Memory Write: " FMT_PADDR "at pc: " FMT_WORD, addr, cpu.pc);
  //Log("Try to write  " FMT_WORD, data);
#endif
  if(isa_mmu_check(addr, len, MEM_TYPE_WRITE)) 
    paddr_write(isa_mmu_translate(addr, len, MEM_TYPE_WRITE), len, data);
  else 
  paddr_write(addr, len, data);
}
