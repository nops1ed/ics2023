#ifndef __FS_H__
#define __FS_H__

#include <common.h>

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);
void fs_curfilename(void);

size_t fb_write(const void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len); 
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t audioctl_write(const void *buf, size_t offset, size_t);
size_t audioctl_read(void *, size_t, size_t);
size_t audio_write(void *, size_t, size_t);

#ifndef SEEK_SET
enum {SEEK_SET, SEEK_CUR, SEEK_END};
#endif

#endif
