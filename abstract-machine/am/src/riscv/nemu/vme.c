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
#define PGSHIFT         12
/* extract the three 9-bit page table indices from a virtual address. */
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64_t) (va)) >> PXSHIFT(level)) & PXMASK)

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

typedef uint64_t *pagetable_t; // 512 PTEs

static inline void set_satp(void *pdir) {
  printf("Now pdir is %x\n", pdir);
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  printf("So the mode is %x\n", mode);
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

/* The risc-v Sv39 scheme has three levels of page-table
* pages. A page-table page contains 512 64-bit PTEs.
* A 64-bit virtual address is split into five fields:
*   39..63 -- must be zero.
*   30..38 -- 9 bits of level-2 index.
*   21..29 -- 9 bits of level-1 index.
*   12..20 -- 9 bits of level-0 index.
*    0..11 -- 12 bits of byte offset within the page.
*/
void map(AddrSpace *as, void *va, void *pa, int prot) {
  /* Perform a page table walk. */
  pagetable_t pagetable = as->ptr;
  PTE *pte;
  for(int level = 2; level > 0; level--) {
    pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } 
    else {
      pagetable = (pagetable_t)pgalloc_usr(PGSIZE);
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
    //printf("\033[34mpagetable is %p and *pte is %p\033[0m\n", pagetable, *pte);
  }
  /* Fill PTE fields. */
  pte = &pagetable[PX(0, va)];
  *pte = PA2PTE(pa) | PTE_V;

  //printf("Finally store %p to %p\n\n", *pte, pte);
  //printf("Mapping %p to %p successfully\n", pa, va);
  /*
  uint64_t pa_raw = (uint64_t)pa;
  uint64_t va_raw = (uint64_t)va;
  uint64_t **pt_1 = (uint64_t **)as->ptr;
  if (pt_1[PX(2, va_raw)] == NULL)
    pt_1[PX(2, va_raw)] = (uint64_t *)pgalloc_usr(PGSIZE);

  uint64_t **pt_2 = (uint64_t **)pt_1[PX(2, va_raw)];
  if (pt_2[PX(1, va_raw)] == NULL)
    pt_2[PX(1, va_raw)] = (uint64_t *)pgalloc_usr(PGSIZE);

  uint64_t *pt_3 = pt_2[PX(1, va_raw)];
  if (pt_3[PX(0, va_raw)] == 0)
    pt_3[PX(0, va_raw)] = (pa_raw & (~0xfff)) | prot;
  else
  {
    //Assert(0, "remap virtual address\n!");
    assert(0);
  }
   //printf("map vrirtual address %p to physical address %p, pt2 id is %p, store addr %p\n", va_raw, pa_raw, PGT2_ID(va_raw), pt_2[PGT2_ID(va_raw)] >> 12);
   */
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *kctx = (Context *)(kstack.end - sizeof(Context)); 

  /* Bug occured here. */
  kctx->pdir = as->ptr;

  kctx->mepc = (uintptr_t)entry;
  printf("\033[033mUser context created\033[0m\n");
  return kctx;
}
