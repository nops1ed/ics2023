#include <proc.h>
#include <elf.h>
#include <fs.h>
#include <memory.h>

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

#define NR_PAGE 8

 __attribute__ ((__used__)) 
 static void * alloc_section_space(AddrSpace *as, uintptr_t vaddr, size_t p_memsz){
  //size_t page_n = p_memsz % PAGESIZE == 0 ? p_memsz / 4096 : (p_memsz / 4096 + 1);
  size_t page_n = ((vaddr + p_memsz - 1) >> 12) - (vaddr >> 12) + 1;
  void *page_start = new_page(page_n);
  
  for (int i = 0; i < page_n; ++i)
    map(as, (void *)((vaddr & ~0xfff) + i * PGSIZE), (void *)(page_start + i * PGSIZE), 1);

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
  printf("\033[33mOpening file...\033[0m\n");
  int fd = fs_open(filename, 0, 0);
  printf("\033[33mFile opened\033[0m\n");
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
      /*
      void *paddr = alloc_section_space(&pcb->as, phdr[i].p_vaddr, phdr[i].p_memsz);
      fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
      fs_read(fd, (void *)((phdr[i].p_vaddr & 0xFFF) + paddr), phdr[i].p_memsz);
      memset((void *)((phdr[i].p_vaddr & 0xFFF) + phdr[i].p_filesz + paddr), 0, phdr[i].p_memsz - phdr[i].p_filesz);
      */
      fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
      fs_read(fd, (void *)(phdr[i].p_vaddr), phdr[i].p_memsz);
      memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
    }
  }
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

Context *context_kload(PCB* pcb, void(*func)(void *), void *args) {
  Area stack = RANGE((char *)(uintptr_t)pcb, (char *)(uintptr_t)pcb + STACK_SIZE);
  Context *kcxt = kcontext(stack, func, args);
  pcb->cp = kcxt;
  return kcxt;
} 

static size_t ceil_4_bytes(size_t size){
  if (size & 0x3)
    return (size & (~0x3)) + 0x4;
  return size;
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  

   int envc = 0, argc = 0;
  AddrSpace *as = &pcb->as;
  //protect(as);
  
  if (envp){
    for (; envp[envc]; ++envc){}
  }
  if (argv){
    for (; argv[argc]; ++argc){}
  }
  char *envp_ustack[envc];

  void *alloced_page = new_page(NR_PAGE) + NR_PAGE * 4096; //得到栈顶

  //这段代码有古怪，一动就会出问题，莫动
  //这个问题确实已经被修正了，TMD，真cao dan
  // 2021/12/16
  
  //map(as, as->area.end - 8 * PAGESIZE, alloced_page - 8 * PAGESIZE, 1); 
  //map(as, as->area.end - 7 * PAGESIZE, alloced_page - 7 * PAGESIZE, 1);
  //map(as, as->area.end - 6 * PAGESIZE, alloced_page - 6 * PAGESIZE, 1); 
  //map(as, as->area.end - 5 * PAGESIZE, alloced_page - 5 * PAGESIZE, 1);
  //map(as, as->area.end - 4 * PAGESIZE, alloced_page - 4 * PAGESIZE, 1); 
  //map(as, as->area.end - 3 * PAGESIZE, alloced_page - 3 * PAGESIZE, 1);
  //map(as, as->area.end - 2 * PAGESIZE, alloced_page - 2 * PAGESIZE, 1); 
  //map(as, as->area.end - 1 * PAGESIZE, alloced_page - 1 * PAGESIZE, 1); 
  
  char *brk = (char *)(alloced_page - 4);
  // 拷贝字符区
  for (int i = 0; i < envc; ++i){
    brk -= (ceil_4_bytes(strlen(envp[i]) + 1)); // 分配大小
    envp_ustack[i] = brk;
    strcpy(brk, envp[i]);
  }

  char *argv_ustack[envc];
  for (int i = 0; i < argc; ++i){
    brk -= (ceil_4_bytes(strlen(argv[i]) + 1)); // 分配大小
    argv_ustack[i] = brk;
    strcpy(brk, argv[i]);
  }
  
  intptr_t *ptr_brk = (intptr_t *)(brk);

  // 分配envp空间
  ptr_brk -= 1;
  *ptr_brk = 0;
  ptr_brk -= envc;
  for (int i = 0; i < envc; ++i){
    ptr_brk[i] = (intptr_t)(envp_ustack[i]);
  }

  // 分配argv空间
  ptr_brk -= 1;
  *ptr_brk = 0;
  ptr_brk = ptr_brk - argc;
  
  // printf("%p\n", ptr_brk);
  printf("%p\t%p\n", alloced_page, ptr_brk);
  //printf("%x\n", ptr_brk);
  //assert((intptr_t)ptr_brk == 0xDD5FDC);
  for (int i = 0; i < argc; ++i){
    ptr_brk[i] = (intptr_t)(argv_ustack[i]);
  }

  ptr_brk -= 1;
  *ptr_brk = argc;
  
  //这条操作会把参数的内存空间扬了，要放在最后
  uintptr_t entry = loader(pcb, filename);
  Area karea;
  karea.start = &pcb->cp;
  karea.end = &pcb->cp + STACK_SIZE;

  Context* context = ucontext(as, karea, (void *)entry);
  pcb->cp = context;

  printf("新分配ptr=%p\n", as->ptr);
  printf("UContext Allocted at %p\n", context);
  printf("Alloced Page Addr: %p\t PTR_BRK_ADDR: %p\n", alloced_page, ptr_brk);

  ptr_brk -= 1;
  *ptr_brk = 0;//为了t0_buffer
  //设置了sp
  context->gpr[2]  = (uintptr_t)ptr_brk - (uintptr_t)alloced_page + (uintptr_t)as->area.end;

  //似乎不需要这个了，但我还不想动
  context->GPRx = (uintptr_t)ptr_brk - (uintptr_t)alloced_page + (uintptr_t)as->area.end + 4;
  //context->GPRx = (intptr_t)(ptr_brk);
}

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





