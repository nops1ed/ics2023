#include <fs.h>

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
  [FD_STDIN]  = {"stdin", 0, 0, 0, (ReadFn)invalid_read, (WriteFn)invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, invalid_write},
#include "files.h"
};

size_t do_read(int fd, void *buf, size_t len) {
  ramdisk_read(buf, file_table[fd].open_offset, len);
  return len;
}

size_t do_write(int fd, const void *buf, size_t len) {
  ramdisk_write(buf, file_table[fd].open_offset, len);
  return len;
}

#define NR_FILE sizeof(file_table) / sizeof(Finfo)

void init_fs() {
  // TODO: initialize the size of /dev/fb

}

/* SYS_open should be called here. */
int fs_open(const char *pathname, int flags, int mode) {
  /* flags and mode are disabled in nano-lite. 
  * And in sFS we just return the index as the fd
  */
  
  int i;
  /* just compare the filename here. */
  for(i = 0; i < NR_FILE; i++)
    if(!strcmp(pathname, file_table[i].name)) {
      file_table[i].open_offset = file_table[i].disk_offset;
      return i;
    }

  /* In sFS, operation 'open' must be successful otherwise interrupt it. */
  assert(0);
  /* Should not reach here. */
  return -1;
}

/* Same as above, we should call SYS_read here. */
size_t fs_read(int fd, void *buf, size_t len) {
  /* Here we just call ramdisk_read().*/
  do_read(fd, buf, len);
  file_table[fd].open_offset += len;
  return len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  do_write(fd, buf, len);
  file_table[fd].open_offset += len;
  return len;
}
size_t fs_lseek(int fd, size_t offset, int whence) {
  size_t cur;
  switch(whence) {
    case SEEK_SET: cur = 0; break;
    case SEEK_CUR: cur = file_table[fd].open_offset; break;
    case SEEK_END: cur = file_table[fd].disk_offset + file_table[fd].size; break;
    default:
      panic("No whence. \n");
  }
  file_table[fd].open_offset = cur + offset;
  return 0;
}

int fs_close(int fd) {
  return 0;
}



