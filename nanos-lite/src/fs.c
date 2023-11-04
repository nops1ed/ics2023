#include <fs.h>

size_t serial_write(const void *buf, size_t offset, size_t len);
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

static size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

static size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, serial_write},
#include "files.h"
};

static size_t do_read(void *buf, size_t offset, size_t len) {
  return ramdisk_read(buf, offset, len);
}

static size_t do_write(const void *buf, size_t offset, size_t len) {
  return ramdisk_write(buf, offset, len);
}

#define NR_FILE sizeof(file_table) / sizeof(Finfo)

void init_fs() {
  for (size_t fd = 0; fd < NR_FILE; ++fd) {
    if (file_table[fd].write == NULL)
      file_table[fd].write = do_write;
    if (file_table[fd].read == NULL)
      file_table[fd].read = do_read;
  }
  // TODO: initialize the size of /dev/fb
}

static int i;

int fs_open(const char *pathname, int flags, int mode) {
  /* flags and mode are disabled in nano-lite. 
  * And in sFS we just return the index as the fd
  */
  
  /* just compare the filename here. */
  for(i = 0; i < NR_FILE; i++)
    if(!strcmp(pathname, file_table[i].name)) {
      //file_table[i].open_offset = file_table[i].disk_offset;
      file_table[i].open_offset = 0;
      return i;
    }

  /* In sFS, operation 'open' must be successful otherwise interrupt it. */
  assert(0);
  /* Should not reach here. */
  return -1;
}

void fs_curfilename(void) {
  printf("%s ", file_table[i].name);
}

/* Same as above, we should call SYS_read here. */
size_t fs_read(int fd, void *buf, size_t len) {
  /* Here we just call ramdisk_read().*/
  //do_read(fd, buf, len);
  size_t ret_val = file_table[fd].read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret_val;
  return ret_val;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  int ret_val = -1;  
  if(fd < NR_FILE) {
    ret_val = file_table[fd].write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
    file_table[fd].open_offset += ret_val;
  }
  else
    printf("sys_write: Error\n");
  return ret_val;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  switch(whence) {
    case SEEK_SET: file_table[fd].open_offset = offset; break;
    case SEEK_CUR: file_table[fd].open_offset += offset; break;
    case SEEK_END: file_table[fd].open_offset = file_table[fd].size + offset; break;
    default:
      panic("No whence. \n");
  }
  return file_table[fd].open_offset;
}

int fs_close(int fd) {
  return 0;
}



