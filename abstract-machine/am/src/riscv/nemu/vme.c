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
/*
#define PGD(f) (uint64_t)((uint64_t)(f >> 30) & 0x1ff)
#define PMD(f) (uint64_t)((uint64_t)(f >> 21) & 0x1ff)
#define PTE(f) (uint64_t)((uint64_t)(f >> 12) & 0x1ff)
#define OFF(f) (uint64_t)((uint64_t)(f) & 0x1ff)
*/

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

/*
void map(AddrSpace *as, void *va, void *pa, int prot) {
  uint64_t **pt_addr = (uint64_t **)as->ptr; 
  uint64_t vaddr = (uint64_t)va;
  uint64_t paddr = (uint64_t)pa;
  if(pt_addr[PGD(vaddr)] == NULL)
    pt_addr[PGD(vaddr)] = (uint64_t *)pgalloc_usr(PGSIZE);
  pt_addr = (uint64_t **)pt_addr[PGD(vaddr)];
  if(pt_addr[PMD(vaddr)] == NULL)
    pt_addr[PMD(vaddr)] = (uint64_t *)pgalloc_usr(PGSIZE);
  //pt_addr = (uint64_t *)pt_addr[PMD(vaddr)];
  uint64_t addr = (uint64_t)pt_addr[PMD(vaddr)];
  if(pt_addr[PTE(vaddr)] == NULL)
    pt_addr[PTE(vaddr)] = paddr & ~0xfff;
}
*/

#define VA_VPN_0(x) (((uintptr_t)x & 0x003FF000u) >> 12)
#define VA_VPN_1(x) (((uintptr_t)x & 0xFFC00000u) >> 22)
#define VA_OFFSET(x) ((uintptr_t)x & 0x00000FFFu)

#define PTE_PPN_MASK (0xFFFFFC00u)
#define PTE_PPN(x) (((uintptr_t)x & PTE_PPN_MASK) >> 10)

void map(AddrSpace *as, void *va, void *pa, int prot) {
  va = (void *)(((uintptr_t)va) & (~0xfff));
  pa = (void *)(((uintptr_t)pa) & (~0xfff));

  PTE *page_table_entry = as->ptr + VA_VPN_1(va) * 4;
  // assert((uintptr_t)as->ptr + VA_VPN_1(va) * 4 == get_satp() + VA_VPN_1(va) * 4);

  if (!(*page_table_entry & PTE_V)){ // 说明二级表未分配
    void *alloced_page = pgalloc_usr(PGSIZE);
    *page_table_entry = (*page_table_entry & ~PTE_PPN_MASK) | (PTE_PPN_MASK & ((uintptr_t)alloced_page >> 2));
    *page_table_entry = (*page_table_entry | PTE_V);
    // printf("二级表未分配\t二级表项地址:%p\t虚拟地址:%p\n", page_table_entry, va);
    //assert(((PTE_PPN(*page_table_entry) * 4096 + VA_VPN_0(va) * 4) & ~0xFFFFFF) == ((uintptr_t)alloced_page& ~0xFFFFFF));
  }
  // 找到二级表中的表项
  PTE *leaf_page_table_entry = (PTE *)(PTE_PPN(*page_table_entry) * 4096 + VA_VPN_0(va) * 4);
  // if ((uintptr_t)va <= 0x40100000){
  //   printf("设置二级表项\t虚拟地址:%p\t实际地址:%p\t表项:%p\n", va, pa, leaf_page_table_entry);
  // }
  // 设置PPN
  *leaf_page_table_entry = (PTE_PPN_MASK & ((uintptr_t)pa >> 2)) | (PTE_V | PTE_R | PTE_W | PTE_X) | (prot ? PTE_U : 0);
  //assert(PTE_PPN(*leaf_page_table_entry) * 4096 + VA_OFFSET(va) == (uintptr_t)pa);
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *kctx = (Context *)(kstack.end - sizeof(Context)); 

  kctx->mepc = (uintptr_t)entry;
  return kctx;
}
