#include <memory.h>
#include <proc.h>

static void *pf = NULL;
extern PCB *current;
#ifndef PXMASK
  #define PXMASK 0xFFF
#endif

/* The new_page() function manages the heap with a pf pointer to allocate a contiguous memory area of nr_page * 4KB in size,
* and returns the *first* address of the area.
*/
void* new_page(size_t nr_page) {
  void *old = pf;
  memset(pf, 0, nr_page * PGSIZE);
  //pf = (void *)((char *)old + nr_page * PGSIZE);
  pf += nr_page * PGSIZE;
  return old;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
/* The parameter of pg_alloc() is the number of bytes to allocate,
* but we must ensure that the space requested by AM is
* always an integer multiple of the page size
*/
  if(n < 0) {
    Log("\033[31mFatal Error occured: page to be alloced could not be negative.\033[0m\n");
    assert(0);
  }
  if(n == 0)
    Log("\033[33mThe number of pages to be allocated is 0, \
            which may not meet your expectations.  \
            If there are any issues, please review your implementation again\033[0m\n");
  size_t nr_pg_to_alloc = (size_t)(n / PGSIZE) + (size_t)(n % PGSIZE == 0);
  void *alloc_ptr = new_page(nr_pg_to_alloc);
  memset(alloc_ptr, 0, nr_pg_to_alloc * PGSIZE);
  return alloc_ptr;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() syscall handler. */
int mm_brk(uintptr_t brk) {
// printf("mm_brk brk: 0x%08lx; max_brk 0x%08lx;", brk, current->max_brk);
  current->max_brk = ROUNDUP(current->max_brk, PGSIZE); // max_brk and brk are open -- [,brk)
  if (brk > current->max_brk) {
    int page_count = ROUNDUP(brk - current->max_brk, PGSIZE) >> 12;
    uintptr_t pages_start = (uintptr_t)new_page(page_count);
    for (int i = 0; i < page_count; ++ i) {
      map(&current->as,
          (void*)(current->max_brk + i * PGSIZE),
          (void*)(pages_start + i * PGSIZE),
          MMAP_READ|MMAP_WRITE
          );
    }
    current->max_brk += page_count * PGSIZE;
    // printf("--brked-- ");
  }
  // printf("max_brk 0x%08lx\n", current->max_brk);
  return 0;

}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
