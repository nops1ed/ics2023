#include <proc.h>
#include <elf.h>

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

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

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
  Elf_Ehdr _ehdr;
  ramdisk_read(&_ehdr, 0, sizeof(Elf_Ehdr));

  Elf_Phdr _phdr;
  uint32_t _phnum = _ehdr.e_phnum;
  uint32_t _phoff = _ehdr.e_phoff;
  printf("The phoff = %d\n", _phoff);

  //printf("f**k you\n");
  for(int i = 0; i < _phnum; i++) {
    printf("The offset = %d\n", _phoff + i * sizeof(_phdr));
    ramdisk_read(&_phdr, _phoff + i * sizeof(_phdr), sizeof(_phdr));
    if(_phdr.p_type == PT_LOAD) {
      ramdisk_read((void *)_phdr.p_vaddr, _phdr.p_offset, _phdr.p_memsz);
      memset((void *)(_phdr.p_vaddr + _phdr.p_filesz), 0, _phdr.p_memsz - _phdr.p_filesz);
    }
  }

      printf("f**k you\n");
/*
  Elf_Ehdr ehdr;
  ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
  // check valid elf
  assert((*(uint32_t *)ehdr.e_ident == 0x464c457f));

  Elf_Phdr phdr[ehdr.e_phnum];
  ramdisk_read(phdr, ehdr.e_phoff, sizeof(Elf_Phdr) * ehdr.e_phnum);
  for (int i = 0; i < ehdr.e_phnum; i++)
  {
    if (phdr[i].p_type == PT_LOAD)
    {
      ramdisk_read((void *)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_memsz);
      // set .bss with zeros
      memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
    }
  }
  return ehdr.e_entry;
  */

  return _ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

