#include <memory.h>

static void *pf = NULL;

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
  return NULL;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
