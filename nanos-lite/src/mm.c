#include <memory.h>
#include <proc.h>

static void *pf = NULL;
extern PCB *current;

/* The new_page() function manages the heap with a pf pointer to allocate a contiguous memory area of nr_page * 4KB in size, 
* and returns the *first* address of the area.
*/
void* new_page(size_t nr_page) {
  void *old = pf;
  pf = (void *)((char *)old + nr_page * PGSIZE);
  return old;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
/* The parameter of pg_alloc() is the number of bytes to allocate, 
* but we must ensure that the space requested by AM is always an integer multiple of the page size
*/
  if(n < 0) {
    printf("\033[31mFatal Error occured: page to be alloced could not be negetive.\033[0m\n");
    assert(0);
  }
  if(n == 0) 
    printf("\033[33mThe number of pages to be allocated is 0, \
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

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
#define PG_MASK ~0xfff
  if (current->max_brk == 0)
  {
    current->max_brk = (brk & ~PG_MASK) ? ((brk & PG_MASK) + PGSIZE) : brk;
    printf("first malloc is at %p\n", (void *)current->max_brk);
    return 0;
  }

  for (; current->max_brk < brk; current->max_brk += PGSIZE)
  {
    map(&current->as, (void *)current->max_brk, pg_alloc(PGSIZE), 1);
  }

  return 0;
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
