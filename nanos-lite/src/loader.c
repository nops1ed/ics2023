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


/*
 #define EI_NIDENT 16
 typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    ElfN_Addr     e_entry;
    ElfN_Off      e_phoff;
    ElfN_Off      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
  } ElfN_Ehdr;
*/

/*
  typedef struct {
    uint32_t   p_type;
    uint32_t   p_flags;
    Elf64_Off  p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    uint64_t   p_filesz;
    uint64_t   p_memsz;
    uint64_t   p_align;
  } Elf64_Phdr;
*/
static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  //ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
  int fd = fs_open(filename, 0, 0);
  fs_read(fd, &ehdr, sizeof(ehdr));  

  /* check magic number. */
  assert((*(uint64_t *)ehdr.e_ident == 0x010102464c457f));
  /* check architecture. */
  assert(ehdr.e_machine == EXPECT_TYPE);

  Elf_Phdr phdr[ehdr.e_phnum];
  //ramdisk_read(phdr, ehdr.e_phoff, sizeof(Elf_Phdr) * ehdr.e_phnum);
  fs_lseek(fd, ehdr.e_phoff, SEEK_SET);
  fs_read(fd, phdr, sizeof(Elf_Phdr) * ehdr.e_phnum);
  for (int i = 0; i < ehdr.e_phnum; i++) {
    if (phdr[i].p_type == PT_LOAD) {
      //ramdisk_read((void *)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_memsz);
      fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
      fs_read(fd, (void *)phdr[i].p_vaddr, phdr[i].p_memsz);
      //printf("Read data from %p, size %d\n", phdr[i].p_offset, phdr[i].p_memsz);
      //printf("Write it to %p\n", phdr[i].p_vaddr);
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

void context_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Area stack = RANGE((char *)(uintptr_t)pcb, (char *)(uintptr_t)pcb + STACK_SIZE);
  Context *ucxt = ucontext(NULL, stack, (void *)entry);
  pcb->cp = ucxt;
  ucxt->GPRx = (uintptr_t)stack.end;
}