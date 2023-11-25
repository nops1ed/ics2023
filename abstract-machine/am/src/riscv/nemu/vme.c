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
#define PGD(X) ((((uintptr_t)X >> 30) & 0x1FF))   //VPN[2]
#define PMD(X) ((((uintptr_t)X >> 21) & 0x1FF))   //VPN[1]
#define PTE(X) ((((uintptr_t)X >> 12) & 0x1FF))   //VPN[0]
#define VA_OFFSET(X) ((uintptr_t)X & 0xFFF)
#define PTE_PPN_MASK (0x00FFFFFFFFFFF000)
#define PTE_PPN(X) (((uintptr_t)X & PTE_PPN_MASK) >> 12)
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
  va = (void *)(((uintptr_t)va) & (~0xfff));
  pa = (void *)(((uintptr_t)pa) & (~0xfff));

  PTE *page_table_entry = as->ptr + PGD(va) * 8;

  printf("So the va is %p\n", va);
  printf("So the satp is %p\n", page_table_entry);
  printf("So the as is %p\n", as->ptr);
  printf("So the PGD is %p\n", PGD(va) * 8);
  printf("So the PLUS is %p\n", as->ptr + PGD(va) * 8);
  if (!(*page_table_entry & PTE_V)){ 
    void *alloced_page = pgalloc_usr(PGSIZE);
    *page_table_entry = (*page_table_entry & ~PTE_PPN_MASK) | (PTE_PPN_MASK & ((uintptr_t)alloced_page >> 2));
    *page_table_entry = (*page_table_entry | PTE_V);
  }
  printf("page_table_entry ia %p\n", page_table_entry); 
  printf("pppppaddr is %p\n", (PTE *)(PTE_PPN(*page_table_entry) * 4096+ PMD(va) * 8));
  page_table_entry = (PTE *)(PTE_PPN(*page_table_entry) * PGSIZE + PMD(va) * 8);
  printf("addr is %p\n", page_table_entry);
    printf("stat done\n");
  if (page_table_entry == NULL || !(*page_table_entry & PTE_V)){ 
    printf("if done\n");
    void *alloced_page = pgalloc_usr(PGSIZE);
    *page_table_entry = (*page_table_entry & ~PTE_PPN_MASK) | (PTE_PPN_MASK & ((uintptr_t)alloced_page >> 2));
    *page_table_entry = (*page_table_entry | PTE_V);
  }

  page_table_entry = (PTE *)(PTE_PPN(*page_table_entry) * 4096 + PTE(va) * 8);
  *page_table_entry = (PTE_PPN_MASK & ((uintptr_t)pa >> 2)) | (PTE_V | PTE_R | PTE_W | PTE_X) | (prot ? PTE_U : 0);
  printf("It is done\n");
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
     printf("二级表未分配\t二级表项地址:%p\t虚拟地址:%p\n", page_table_entry, va);
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
  kctx->pdir = as->ptr;
  return kctx;
}
