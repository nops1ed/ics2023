#include "ftrace.h"
#include <elf.h>

#define MAX_BUF_SIZE 64;
#define MAX_DEPTH 32;

/* Symbol table defined like this
* typedef struct {
*    uint32_t      st_name;
*    Elf32_Addr    st_value;
*    uint32_t      st_size;
*    unsigned char st_info;
*    unsigned char st_other;
*    uint16_t      st_shndx;
* } Elf32_Sym;
*/
#define NR_ST 64

typedef struct symbol_table{
    char *func_name;
    uint64_t addr, size;    // start addr and size
}symbol_table;

/* define symbol table */
static symbol_table ST[NR_ST] = {};
static uint64_t ST_SIZE = 0;

static void debug_SYMTAB() {
    for (int i = 0; i < ST_SIZE; i++) {
        printf("func_name : %s\n",ST[i].func_name);
        printf("address : 0x%lx\n",ST[i].addr);
    }
}

/* Description in manual
*   The ELF header is described by the type Elf32_Ehdr or Elf64_Ehdr:
*    #define EI_NIDENT 16
*    typedef struct {
*        unsigned char e_ident[EI_NIDENT];
*        uint16_t      e_type;
*        uint16_t      e_machine;
*        uint32_t      e_version;
*        ElfN_Addr     e_entry;
*        ElfN_Off      e_phoff;
*        ElfN_Off      e_shoff;
*        uint32_t      e_flags;
*        uint16_t      e_ehsize;
*        uint16_t      e_phentsize;
*        uint16_t      e_phnum;
*        uint16_t      e_shentsize;
*        uint16_t      e_shnum;
*        uint16_t      e_shstrndx;
* } ElfN_Ehdr;
*/
void INIT_SYMBOL_TABLE(const char *elf_filename) {
    FILE *fp;
    assert((fp = fopen(elf_filename, "rb")) != NULL);
    Elf64_Ehdr _ehdr;
    assert(fread(&_ehdr, sizeof(_ehdr), 1, fp));
        /*
        * typedef struct {
        *        uint32_t   sh_name;
        *        uint32_t   sh_type;
        *        uint32_t   sh_flags;
        *        Elf32_Addr sh_addr;
        *        Elf32_Off  sh_offset;
        *        uint32_t   sh_size;
        *        uint32_t   sh_link;
        *        uint32_t   sh_info;
        *        uint32_t   sh_addralign;
        *        uint32_t   sh_entsize;
        *  } Elf32_Shdr;
        */
    Elf64_Shdr _shdr;
    uint64_t _strtab_offset = 0;
    /* Well, The function below may be not elegant but useful 
    * Cause We have to locate STR_TAB before using SYM_TAB
    * Could it be optimized ?
    */
    fseek(fp, _ehdr.e_shoff, SEEK_SET);
    for(int i = 0; i < _ehdr.e_shnum; i++) {
        assert(fread(&_shdr, sizeof(_shdr), 1, fp));
        /* Locate String table */
        if(_shdr.sh_type == SHT_STRTAB) {
            _strtab_offset = _shdr.sh_offset;
            break;
        }
    } 

    Elf64_Sym _sym;
    fseek(fp, _ehdr.e_shoff, SEEK_SET);
    for(int i = 0; i < _ehdr.e_shnum; i++) {
        assert(fread(&_shdr, sizeof(_shdr), 1, fp));
        if (_shdr.sh_type == SHT_SYMTAB) {
            fseek(fp, _shdr.sh_offset, SEEK_SET);
            for(int j = 0; j < _shdr.sh_size/sizeof(_sym); j++) {
                assert(fread(&_sym, sizeof(_sym), 1, fp));
                /* Prototype:
                * ELF32_ST_TYPE(info), ELF64_ST_TYPE(info)
                *     Extract a type from an st_info value.
                */
                if (ELF64_ST_TYPE(_sym.st_info) == STT_FUNC) { 
                    uint32_t cur_pos = ftell(fp);
                    // read function name
                    // This should be enough
                    char *func_name = (char *)malloc(sizeof(char) * 32);
                    fseek(fp, _strtab_offset + _sym.st_name, SEEK_SET);
                    assert(fread(func_name, sizeof(char) * 32, 1, fp));

                    symbol_table __tmp = {func_name, _sym.st_value, _sym.st_size};
                    ST[ST_SIZE++] = __tmp;
                    fseek(fp, cur_pos, SEEK_SET);
                }
            }
        }
    }
    printf("SYMTAB initialized\n");
    debug_SYMTAB();
}

/* Establish mini Trapframe to record jmp action */
typedef struct _Trace_Node{
  struct _Trace_Node *next;
  char *func_name;
  word_t addr;
}_Trace_Node;

static _Trace_Node *top = NULL;
static bool _Trace_Init = false;
static uint64_t _depth = 0;

static void Init_Trace_Node(void) {
  top = (_Trace_Node *)malloc(sizeof(_Trace_Node));
  top->next = NULL;
  _Trace_Init = true; 
  _depth = 0;
}

#define FFFFFF 1

/* trace frame should store present addr and its indent */
static void Push_Trace_Frame(word_t _addr) {
  if(!_Trace_Init) Init_Trace_Node();
  _Trace_Node *_tmp = (_Trace_Node *)malloc(sizeof(_Trace_Node));
  _tmp->addr = _addr;
  _tmp->next = top;
  _tmp->func_name = '\0';
  _depth += 2;
 for(int i = 0; i < ST_SIZE; i++) {
    if (FFFFFF == 1) {
        printf("Now ST[%d].addr equal to 0x%lx\n", i, ST[i].addr);
        printf("Now addr equal to 0x%lx\n", _addr);
    }
    if (ST[i].addr == _addr) {
        _tmp->func_name = ST[i].func_name;
        break;
    }
  }
  for(int i = 0 ; i < _depth; i++) printf(" "); 
  printf("call [%s@0x%lx]\n", _tmp->func_name, _tmp->addr);
  top = _tmp;
}

/* Pop the top node and print its info */
static void Pop_Trace_Frame(void) {
  _Trace_Node *_tmp = top;
  top = top->next;
  for(int i = 0 ; i < _depth; i++) printf(" "); 
  printf("ret [%s]\n", _tmp->func_name);
  _depth -= 2;
  free(_tmp);
}

void ftrace_call(word_t snpc, word_t dnpc) {
    printf("0x%lx: ", snpc);
    Push_Trace_Frame(dnpc);
}

void ftrace_ret(word_t snpc) {
    printf("0x%lx: ", snpc);
    Pop_Trace_Frame();
}