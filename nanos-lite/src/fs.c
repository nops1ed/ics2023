#include <fs.h>

size_t fb_write(const void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len); 
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

#define NR_FILE sizeof(file_table) / sizeof(Finfo)

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
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [FD_FB]     = {"/dev/fb", 0, 0, invalid_read, fb_write},
                {"/dev/events", 0, 0, events_read, invalid_write},
                {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};

static int i;
void fs_curfilename(void) {
  printf("%s ", file_table[i].name);
}

static size_t do_read(void *buf, size_t offset, size_t len) {
  //printf("Calling ramdisk_read offset = %d len = %d\n",offset, len);
  return ramdisk_read(buf, offset, len);
}

static size_t do_write(const void *buf, size_t offset, size_t len) {
  return ramdisk_write(buf, offset, len);
}

void init_fs() {
  for (size_t fd = 0; fd < NR_FILE; ++fd) {
    if (file_table[fd].write == NULL) {
      file_table[fd].write = do_write;
      //printf("%s: ",file_table[fd].name);
      //printf("diskoffset = %d\n", file_table[fd].disk_offset);
    }
    if (file_table[fd].read == NULL)
      file_table[fd].read = do_read;
  }
  AM_GPU_CONFIG_T gpuinfo;
  ioe_read(AM_GPU_CONFIG, &gpuinfo);
  file_table[FD_FB].size = gpuinfo.width * gpuinfo.height *4;
  printf("sFS: FD_FB was initialized as %d Bytes\n", file_table[FD_FB].size);
}


int fs_open(const char *pathname, int flags, int mode) {
  /* flags and mode are disabled in nano-lite. 
  * And in sFS we just return the index as the fd
  */
  
  /* just compare the filename here. */
  for(i = 0; i < NR_FILE; i++) {
    //printf("pathname: %s\n", pathname);
    //printf("filetablename: %s\n", file_table[i].name);
    if(!strcmp(pathname, file_table[i].name)) {
      //printf("Opening file %s\n",file_table[i].name);
      file_table[i].open_offset = 0;
      return i;
    }
  }

  //printf("Opening file %s failed\n", pathname);
  /* In sFS, operation 'open' must be successful otherwise interrupt it. */
  assert(0);
  /* Should not reach here. */
  return -1;
}


size_t fs_read(int fd, void *buf, size_t len) {
  //do_read(fd, buf, len);
  size_t len_read = len;
  if(fd > 5) len_read = len > file_table[fd].size - file_table[fd].open_offset ? file_table[fd].size - file_table[fd].open_offset : len;
  //printf("disk_offset is %d and open_offset is %d\n", file_table[fd].disk_offset, file_table[fd].open_offset);
  //printf("Now offset is %d and len is %d\n", file_table[fd].disk_offset + file_table[fd].open_offset, len_read);
  size_t ret_val = file_table[fd].read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len_read);
  file_table[fd].open_offset += ret_val;
  //printf("Now offset is %d\n\n", file_table[fd].disk_offset + file_table[fd].open_offset);
  return ret_val;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  int ret_val = -1;  
  if(fd < NR_FILE) {
    size_t len_write = len;
    if(fd > 5) len_write = len > file_table[fd].size - file_table[fd].open_offset ? file_table[fd].size - file_table[fd].open_offset : len;
    ret_val = file_table[fd].write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len_write);
    file_table[fd].open_offset += ret_val;
  }
  else
    panic("sys_write: Error\n");
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
  //printf("fs_close called\n");
  file_table[fd].open_offset = 0;
  return 0;
}



