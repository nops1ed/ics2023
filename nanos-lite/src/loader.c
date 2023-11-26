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


#define MAX(a, b)((a) > (b) ? (a) : (b))
static void read(int fd, void *buf, size_t offset, size_t len){
  fs_lseek(fd, offset, SEEK_SET);
  fs_read(fd, buf, len);
}

static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  if (fd == -1){ 
    assert(0); //filename指向文件不存在
  }
  
  AddrSpace *as = &pcb->as;
  
  Elf_Ehdr elf_header;
  read(fd, &elf_header, 0, sizeof(elf_header));
  //根据小端法 0x7F E L F
  assert(*(uint32_t *)elf_header.e_ident == 0x464c457f);
  
  Elf_Off program_header_offset = elf_header.e_phoff;
  size_t headers_entry_size = elf_header.e_phentsize;
  int headers_entry_num = elf_header.e_phnum;

  for (int i = 0; i < headers_entry_num; ++i){
    Elf_Phdr section_entry;
    read(fd, &section_entry, 
      i * headers_entry_size + program_header_offset, sizeof(elf_header));
    void *phys_addr;
    uintptr_t virt_addr;
    switch (section_entry.p_type) {
    case PT_LOAD:
      //virt_addr = (void *)section_entry.p_vaddr; 
      // phys_addr = (void *)alloced_page_start + (section_entry.p_vaddr - 0x40000000); // 这里是把0x40000000加载到他对应的实际地址
      virt_addr = section_entry.p_vaddr;
      phys_addr = alloc_section_space(as, virt_addr, section_entry.p_memsz);

      // printf("Load to %x with offset %x\n", phys_addr, section_entry.p_offset);
      //做一个偏移
      read(fd, phys_addr + (virt_addr & 0xfff), section_entry.p_offset, section_entry.p_filesz);
      //同样做一个偏移
      memset(phys_addr + (virt_addr & 0xfff) + section_entry.p_filesz, 0, 
        section_entry.p_memsz - section_entry.p_filesz);
      
      if (section_entry.p_filesz < section_entry.p_memsz){// 应该是.bss节
        //做一个向上的4kb取整数
        // if (pcb->max_brk == 0){
        printf("Setting .bss end %x\n", section_entry.p_vaddr + section_entry.p_memsz);
        // 我们虽然用max_brk记录了最高达到的位置，但是在新的PCB中，我们并未在页表目录中更新这些信息，oH，所以就会失效啦。
        // 于是我们就做了一些权衡。
        //pcb->max_brk = MAX(pcb->max_brk, ROUNDUP(section_entry.p_vaddr + section_entry.p_memsz, 0xfff));
        //TODO: Trade-off
        pcb->max_brk = ROUNDUP(section_entry.p_vaddr + section_entry.p_memsz, 0xfff);
        
        // }
      }
      
      break;
    
    default:
      break;
    }

  }
  
  printf("Entry: %p\n", elf_header.e_entry);
  return elf_header.e_entry;
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

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  /* Each process holds 32kb of stack space, which we think is sufficient for ics processes. */
  void *page_alloc = new_page(NR_PAGE) + NR_PAGE * PGSIZE;
  AddrSpace *as = &pcb->as;
  protect(as);

  /* Mapping user stack here. */
  for(int i = NR_PAGE; i >= 0; i--) 
    map(as, as->area.end - i * PGSIZE, page_alloc - i * PGSIZE, 1);

  /* deploy user stack layout. */
  char *brk = (char *)(page_alloc - 4);
  int argc = 0, envc = 0;
  if (envp) for (; envp[envc]; ++envc) ;
  if (argv) for (; argv[argc]; ++argc) ;
  char **args = (char **)malloc(sizeof(char*) * argc);
  char **envs = (char **)malloc(sizeof(char*) * envc);

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

  free(args);
  free(envs);

  uintptr_t entry = loader(pcb, filename);
  printf("loader finished\n");
  Area stack;
  stack.start = &pcb->cp;
  stack.end = &pcb->cp + STACK_SIZE;
  Context *ucxt = ucontext(as, stack, (void *)entry);
  printf("safe here\n");
  pcb->cp = ucxt;
  ucxt->GPRx = (intptr_t)ptr_brk;
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





