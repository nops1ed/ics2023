#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)
#define PGD(X) ((((uintptr_t)X >> 30) & 0x1FF))   //VPN[2]
#define PMD(X) ((((uintptr_t)X >> 21) & 0x1FF))   //VPN[1]
#define PTE(X) ((((uintptr_t)X >> 12) & 0x1FF))   //VPN[0]
#define VA_OFFSET(X) ((uintptr_t)X & 0xFFF)
#define PTE_PPN_MASK (0x00FFFFFFFFFFF000)
#define PTE_PPN(X) (((uintptr_t)X & PTE_PPN_MASK) >> 12)

#define PGSHIFT         12
/* extract the three 9-bit page table indices from a virtual address. */
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64_t) (va)) >> PXSHIFT(level)) & PXMASK)

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

typedef uint64_t *pagetable_t; // 512 PTEs，一个级别页表含有512个PTE，正好对应4K的页大小

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
void map(AddrSpace *as, void *va, void *pa, int prot) {
  va = (void *)(((uintptr_t)va) & (~0xfff));
  pa = (void *)(((uintptr_t)pa) & (~0xfff));

  PTE *page_table_entry = as->ptr + PGD(va) * 8;
  if (!(*page_table_entry & PTE_V)){ 
    void *alloced_page = pgalloc_usr(PGSIZE);
    *page_table_entry = (*page_table_entry & ~PTE_PPN_MASK) | (PTE_PPN_MASK & ((uintptr_t)alloced_page >> 2));
    *page_table_entry = (*page_table_entry | PTE_V);
  }
 
  page_table_entry = (PTE *)(PTE_PPN(*page_table_entry) * PGSIZE + PMD(va) * 8);
  if (page_table_entry == NULL || !(*page_table_entry & PTE_V)){ 
    void *alloced_page = pgalloc_usr(PGSIZE);
    *page_table_entry = (*page_table_entry & ~PTE_PPN_MASK) | (PTE_PPN_MASK & ((uintptr_t)alloced_page >> 2));
    *page_table_entry = (*page_table_entry | PTE_V);
  }

  page_table_entry = (PTE *)(PTE_PPN(*page_table_entry) * 4096 + PTE(va) * 8);
  *page_table_entry = (PTE_PPN_MASK & ((uintptr_t)pa >> 2)) | (PTE_V | PTE_R | PTE_W | PTE_X) | (prot ? PTE_U : 0);
  printf("It is done\n");
  /*
  pagetable_t pagetable = as->ptr;
  for(int level = 2; level > 0; level--) {
    	// 索引到对应的PTE项
    PTE *pte = &pagetable[PX(level, va)];
    // 确认一下索引到的PTE项是否有效(valid位是否为1)
    if(*pte & PTE_V) {
      // 如果有效接着进行下一层索引
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      // 如果无效(说明对应页表没有分配)
      // 则根据alloc标志位决定是否需要申请新的页表
      // < 注意，当且仅当低两级页表页(中间级、叶子级页表页)不存在且不需要分配时，walk函数会返回0 >
      // 所以我们可以通过返回值来确定walk函数失败的原因
      pagetable = (pagetable_t)pgalloc_usr(PGSIZE);
      // 将申请的页表填满0
      memset(pagetable, 0, PGSIZE);
      // 将申请来的页表物理地址，转化为PTE并将有效位置1，记录在当前级页表
      // 这样在下一次访问时，就可以直接索引到这个页表项
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  */
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *kctx = (Context *)(kstack.end - sizeof(Context)); 

  kctx->mepc = (uintptr_t)entry;
  kctx->pdir = as->ptr;
  return kctx;
}
