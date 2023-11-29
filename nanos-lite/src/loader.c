


#include <proc.h>
#include <elf.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
# define ELF_Off  Elf64_Off

#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
# define ELF_Off  Elf32_Off
#endif

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);


#define NR_PAGE 8
#define PAGESIZE 4096
#define min(x, y) ((x < y) ? x : y)

#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_A 0x40
#define PTE_D 0x80


#define PG_MASK (~0xfff)
#define ISALIGN(vaddr) ((vaddr) == ((vaddr)&PG_MASK))
#define OFFSET(vaddr) ((vaddr) & (~PG_MASK))
#define NEXT_PAGE(vaddr) ((ISALIGN(vaddr)) ? (vaddr) : ((vaddr)&PG_MASK) + PGSIZE)
static uintptr_t loader(PCB *pcb, const char *filename)
{
  Elf_Ehdr ehdr;

  // ramdisk_read(&ehdr, 0, sizeof(ehdr));
  int fd = fs_open(filename, 0, 0);
  assert(fd != -1);

  fs_read(fd, &ehdr, sizeof(ehdr));

  char riscv64_magic_num[] = {0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // printf("magic number is %s\n", (char *)(ehdr.e_ident));
  assert(strcmp((char *)(ehdr.e_ident), riscv64_magic_num) == 0);

  uint32_t entry = ehdr.e_entry;
  uint32_t ph_offset = ehdr.e_phoff;
  uint32_t ph_num = ehdr.e_phnum;

  Elf_Phdr phdr;
  for (int i = 0; i < ph_num; ++i)
  {
    // ramdisk_read(&phdr, ph_offset + i * sizeof(phdr), sizeof(phdr));
    fs_lseek(fd, ph_offset + i * sizeof(phdr), SEEK_SET);
    fs_read(fd, &phdr, sizeof(phdr));
    if (phdr.p_type != PT_LOAD)
      continue;

    // printf("load program header %d", i);

    uint32_t offset = phdr.p_offset;
    uint32_t file_size = phdr.p_filesz;
    uint32_t p_vaddr = phdr.p_vaddr;
    uint32_t mem_size = phdr.p_memsz;

    printf("load program from [%p, %p] to [%p, %p]\n", offset, file_size, p_vaddr, mem_size);
    int left_size = file_size;
    fs_lseek(fd, offset, SEEK_SET);
    // printf("vaddr is %p\n", p_vaddr);
    if (!ISALIGN(p_vaddr))
    {
      void *pg_p = new_page(1);
      int read_len = min(PGSIZE - OFFSET(p_vaddr), left_size);
      left_size -= read_len;
      assert(fs_read(fd, pg_p + OFFSET(p_vaddr), read_len) >= 0);
      map(&pcb->as, (void *)(uintptr_t)p_vaddr, pg_p, PTE_R | PTE_W | PTE_X);
      p_vaddr += read_len;
    }

    for (; p_vaddr < phdr.p_vaddr + file_size; p_vaddr += PGSIZE)
    {
      assert(ISALIGN(p_vaddr));
      void *pg_p = new_page(1);
      memset(pg_p, 0, PGSIZE);
      // int len = min(PGSIZE, file_size - fs_lseek(fd, 0, SEEK_CUR));
      int read_len = min(PGSIZE, left_size);
      left_size -= read_len;
      assert(fs_read(fd, pg_p, read_len) >= 0);
      map(&pcb->as, (void *)(uintptr_t)p_vaddr, pg_p, PTE_R | PTE_W | PTE_X);
    }
    // printf("p_vaddr is %p\n", (void *)p_vaddr);
    p_vaddr = NEXT_PAGE(p_vaddr);
    printf("p_vaddr is %p next page, end of uninitialized space is %p\n", (void *)(uintptr_t)p_vaddr, (void *)(phdr.p_vaddr + mem_size));
    for (; p_vaddr < phdr.p_vaddr + mem_size; p_vaddr += PGSIZE)
    {
      assert(ISALIGN(p_vaddr));
      void *pg_p = new_page(1);
      memset(pg_p, 0, PGSIZE);
      map(&pcb->as, (void *)(uintptr_t)p_vaddr, pg_p, PTE_R | PTE_W | PTE_X);
    }
    assert(mem_size >= file_size);
  }

  // printf("max brk is at %p when load\n", pcb->max_brk);
  assert(fs_close(fd) != -1);

  return entry;
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
  printf("kcontext地址为:%p\n", pcb->cp);
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  /* Each process holds 32kb of stack space, which we think is sufficient for ics processes. */
  AddrSpace *as = &pcb->as;
  /* Mapping address space. */
  printf("\033[33mCreating kernel address space...\033[0m\n");
  protect(&pcb->as);
  printf("\033[33mKernel address space created\033[0m\n");
  void *page_alloc = new_page(NR_PAGE) + NR_PAGE * PGSIZE;

  /* Mapping user stack here. */
  printf("\033[33m\nMapping user stack...\033[0m\n");
  for(int i = NR_PAGE; i > 0; i--) 
    map(as, as->area.end - i * PGSIZE, page_alloc - i * PGSIZE, 1);
  
  printf("\033[33mUser stack mapped\033[0m\n");

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

  //free(args);
  //free(envs);

  printf("\033[33mLoading program entry...\033[0m\n");
  uintptr_t entry = loader(pcb, filename);
  printf("loader finished\n");
  Area stack;
  stack.start = &pcb->cp;
  stack.end = &pcb->cp + STACK_SIZE;
  Context *ucxt = ucontext(as, stack, (void *)entry);
  pcb->cp = ucxt;
  ucxt->GPRx = (intptr_t)ptr_brk;
}

/*


static size_t ceil_4_bytes(size_t size){
  if (size & 0x3)
    return (size & (~0x3)) + 0x4;
  return size;
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]){
  int envc = 0, argc = 0;
  AddrSpace *as = &pcb->as;
  protect(as);
  
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
  
  map(as, as->area.end - 8 * PAGESIZE, alloced_page - 8 * PAGESIZE, 1); 
  map(as, as->area.end - 7 * PAGESIZE, alloced_page - 7 * PAGESIZE, 1);
  map(as, as->area.end - 6 * PAGESIZE, alloced_page - 6 * PAGESIZE, 1); 
  map(as, as->area.end - 5 * PAGESIZE, alloced_page - 5 * PAGESIZE, 1);
  map(as, as->area.end - 4 * PAGESIZE, alloced_page - 4 * PAGESIZE, 1); 
  map(as, as->area.end - 3 * PAGESIZE, alloced_page - 3 * PAGESIZE, 1);
  map(as, as->area.end - 2 * PAGESIZE, alloced_page - 2 * PAGESIZE, 1); 
  map(as, as->area.end - 1 * PAGESIZE, alloced_page - 1 * PAGESIZE, 1); 
  
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
  //printf("%p\t%p\n", alloced_page, ptr_brk);
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

  //printf("新分配ptr=%p\n", as->ptr);
  //printf("UContext Allocted at %p\n", context);
  //printf("Alloced Page Addr: %p\t PTR_BRK_ADDR: %p\n", alloced_page, ptr_brk);

  //设置了sp
  //context->gpr[2]  = (uintptr_t)ptr_brk - (uintptr_t)alloced_page + (uintptr_t)as->area.end;

  //似乎不需要这个了，但我还不想动
  //context->GPRx = (uintptr_t)ptr_brk - (uintptr_t)alloced_page + (uintptr_t)as->area.end + 4;
  context->GPRx = (uintptr_t)(ptr_brk);
}
*/


