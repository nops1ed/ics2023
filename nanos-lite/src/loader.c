
/*
  |               |
  +---------------+ <---- ustack.end
  |  Unspecified  |
  +---------------+
  |               | <----------+
  |    string     | <--------+ |
  |     area      | <------+ | |
  |               | <----+ | | |
  |               | <--+ | | | |
  +---------------+    | | | | |
  |  Unspecified  |    | | | | |
  +---------------+    | | | | |
  |     NULL      |    | | | | |
  +---------------+    | | | | |
  |    ......     |    | | | | |
  +---------------+    | | | | |
  |    envp[1]    | ---+ | | | |
  +---------------+      | | | |
  |    envp[0]    | -----+ | | |
  +---------------+        | | |
  |     NULL      |        | | |
  +---------------+        | | |
  | argv[argc-1]  | -------+ | |
  +---------------+          | |
  |    ......     |          | |
  +---------------+          | |
  |    argv[1]    | ---------+ |
  +---------------+            |
  |    argv[0]    | -----------+
  +---------------+
  |      argc     |
  +---------------+ <---- cp->GPRx
  |               |
*/

#include <proc.h>
#include <elf.h>
#include <fs.h>

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV
#elif defined(__ISA_MIPS32__)
# define EXPECT_TYPE EM_MIPS
#elif defined(__ISA_LOONGARCH32R__)
# define EXPECT_TYPE EM_NONE
#elif
# error Unsupported ISA
#endif

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
# define Elf_Off  Elf64_Off
# define Elf_Addr Elf64_Addr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
# define Elf_Off  Elf32_Off
# define Elf_Addr Elf32_Addr
#endif

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);

#define NR_PAGE 8
#define PAGESIZE 4096

 __attribute__ ((__used__)) static void * 
 alloc_section_space(AddrSpace *as, uintptr_t vaddr, size_t p_memsz){
  size_t page_n = ((vaddr + p_memsz - 1) >> 12) - (vaddr >> 12) + 1;
  void *page_start = new_page(page_n);
  Log("\033[32mLoaded Segment from [%x to %x)\033[0m", vaddr, vaddr + p_memsz);
  for (int i = 0; i < page_n; ++i)
    map(as, (void *)((vaddr & ~0xfff) + i * PAGESIZE), (void *)(page_start + i * PAGESIZE), 1);
  return page_start;
}

/* After obtaining the size of the program, load it in pages:
* 1. Request a free physical page
* 2. Map() this physical page to the virtual address space of the user process.
*     Since AM native implements permission checks, 
*     in order for the program to run correctly on AM native, 
*     you need to set prot to read-write-execute when calling map()
* 3. Read a page of content from the file into this physical page.
*/
static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  Log("\033[33mOpening file...\033[0m");
  int fd = fs_open(filename, 0, 0);
  Log("\033[33mFile opened\033[0m");
  fs_read(fd, &ehdr, sizeof(ehdr));  

  /* check magic number. */
  assert((*(uint64_t *)ehdr.e_ident == 0x010102464c457f));
  /* check architecture. */
  assert(ehdr.e_machine == EXPECT_TYPE);

  Elf_Phdr phdr[ehdr.e_phnum];
  fs_lseek(fd, ehdr.e_phoff, SEEK_SET);
  fs_read(fd, phdr, sizeof(Elf_Phdr) * ehdr.e_phnum);
  for (int i = 0; i < ehdr.e_phnum; i++) {
    if (phdr[i].p_type == PT_LOAD) {
      void *paddr = alloc_section_space(&pcb->as, phdr[i].p_vaddr, phdr[i].p_memsz);
      fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
      fs_read(fd, (void *)((phdr[i].p_vaddr & 0xFFF) + paddr), phdr[i].p_memsz);
      memset((void *)((phdr[i].p_vaddr & 0xFFF) + phdr[i].p_filesz + paddr), 0, phdr[i].p_memsz - phdr[i].p_filesz);
      /*
      fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
      fs_read(fd, (void *)(phdr[i].p_vaddr), phdr[i].p_memsz);
      memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
      */
    }
  }
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
  assert(0);
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg){
  Area karea;
  karea.start = &pcb->cp;
  karea.end = &pcb->cp + STACK_SIZE;

  pcb->cp = kcontext(karea, entry, arg);
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  /* Each process holds 32kb of stack space, which we think is sufficient for ics processes. */
  AddrSpace *as = &pcb->as;
  /* Mapping address space. */
  Log("\033[33mCreating kernel address space...\033[0m");
  protect(&pcb->as);
  Log("\033[33mKernel address space created\033[0m");
  void *page_alloc = new_page(NR_PAGE) + NR_PAGE * PGSIZE;

  /* Mapping user stack. */
  Log("\033[33m\nMapping user stack...\033[0m");
  for(int i = NR_PAGE; i > 0; i--) 
    map(as, as->area.end - i * PGSIZE, page_alloc - i * PGSIZE, 1);
  Log("\033[33mUser stack established\033[0m");

  /* deploy user stack layout. */
  char *brk = (char *)(page_alloc);
  int argc = 0, envc = 0;
  if (envp) for (; envp[envc]; ++envc) ;
  if (argv) for (; argv[argc]; ++argc) ;
  char *args[argc], *envs[envc];

  /* Copy String Area. */
  for (int i = 0; i < argc; ++i) {
    /* Note that it is neccessary to make memory *align*. */
    brk -= ROUNDUP(strlen(argv[i]) + 1, sizeof(int));
    args[i] = brk;
    strcpy(brk, argv[i]);
  }
  for (int i = 0; i < envc; ++i) {
    brk -= ROUNDUP(strlen(envp[i]) + 1, sizeof(int));
    envs[i] = brk;
    strcpy(brk, envp[i]);
  }

  /* Copy envp & argv area. */
  intptr_t *ptr_brk = (intptr_t *)brk;
  *(--ptr_brk) = 0;
  ptr_brk -= envc;
  for (int i = 0; i < envc; ++i)  ptr_brk[i] = (intptr_t)(envs[i]);
  *(--ptr_brk) = 0;
  ptr_brk = ptr_brk - argc;
  for (int i = 0; i < argc; ++i)  ptr_brk[i] = (intptr_t)(args[i]);
  *(--ptr_brk) = argc;

  Log("\033[33mLoading program entry...\033[0m");
  uintptr_t entry = loader(pcb, filename);
  Log("\033[33mloader finished\033[0m");
  Area stack;
  stack.start = &pcb->cp;
  stack.end = &pcb->cp + STACK_SIZE;

  Context *ucxt = ucontext(as, stack, (void *)entry);
  pcb->max_brk = 0;
  pcb->cp = ucxt;
  *(--ptr_brk) = 0;
  ucxt->gpr[2]  = (uintptr_t)ptr_brk - (uintptr_t)page_alloc + (uintptr_t)as->area.end;
  ucxt->GPRx = (uintptr_t)ptr_brk - (uintptr_t)page_alloc + (uintptr_t)as->area.end + 8;
}