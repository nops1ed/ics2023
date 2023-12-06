#include <common.h>
#include <string.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

void schedule_proc(int);

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

static char* __itoa(int num, char* str, int base) {  
  int i = 0;  
  int sign = num < 0 ? -1 : 1;  
  if (sign == -1) num = -num;  
  while (num) {  
    int rem = num % base;  
    str[i++] = rem > 9 ? (rem - 10) + 'a' : rem + '0';  
    num = num / base;  
  }  
  if (i == 0) str[i++] = '0';  
  if (sign == -1) str[i++] = '-';  
  str[i] = '\0';  
  int j = 0, len = strlen(str) - 1;  
  while (j < len) {  
    char tmp = str[j];  
    str[j++] = str[len];  
    str[len--] = tmp;  
  }  
  return str;  
}  

static AM_GPU_CONFIG_T gpuinfo = {};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  unsigned long int stream = (long int)buf;
  int ret_val = -1;
  /* Indicate stdout/stderr and just call putch(). */
  for(ret_val = 0; ret_val < len ; ret_val++) {
    unsigned char __x = ((unsigned char *) stream)[0];
    stream++;
    /* Write to serial. */
    putch(__x);
  }
  return ret_val;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  static AM_INPUT_KEYBRD_T kbd;
  ioe_read(AM_INPUT_KEYBRD, &kbd);
  if (kbd.keycode == AM_KEY_NONE) return 0;
  switch(kbd.keycode) {
    case AM_KEY_F1:
      schedule_proc(1);
      return 0;
    case AM_KEY_F2:
      schedule_proc(2);
      return 0;
    case AM_KEY_F3:
      schedule_proc(3);
      return 0;
    default:
      ; 
  }
  if (kbd.keydown) strncat(buf, "kd ", len);
  else
    strncat(buf, "ku ", len);
  strncat(buf, keyname[kbd.keycode], len - 3);
  strncat(buf, "\n", len - strlen(keyname[kbd.keycode] - 3));
  return strlen(buf);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  /* buf does not support lseek. */
  //printf("**********\nDev-Display info: %d * %d\n**********\n", gpuinfo.width, gpuinfo.height);
  char _tmp[32], _tmp2[32];
  sprintf(buf, "WIDTH:%s\nHEIGHT:%s\n", __itoa(gpuinfo.width, _tmp, 10), __itoa(gpuinfo.height, _tmp2, 10));
  return len;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  /* Call IOE to draw pixels. */
  if (len == 0) {
    AM_GPU_FBDRAW_T fbdraw = {0, 0, (void *)buf, 0, 0, 1};
    ioe_write(AM_GPU_FBDRAW, &fbdraw);
    return 0;
  }
  AM_GPU_FBDRAW_T fbdraw = {offset % gpuinfo.width, offset / gpuinfo.width, (void *)buf, len, 1, 0};
  ioe_write(AM_GPU_FBDRAW, &fbdraw);
  return len;
}

/* Initialize audio control information. */
size_t audioctl_write(const void *buf, size_t offset, size_t len) {
  /* Stream does not support lseek, so offset and len are disabled. */
  ioe_write(AM_AUDIO_CTRL, (AM_AUDIO_CTRL_T *)buf);
  return len;
}

/* Query remained buffer size. */
size_t audioctl_read(void *buf, size_t offset, size_t len) {
  int audio_bufsize, count;
  ioe_read(AM_AUDIO_CONFIG + 0x04, audio_bufsize);
  ioe_read(AM_AUDIO_STATUS, count);
  *(int *)buf = audio_bufsize - count;
  return audio_bufsize - count;
}

/* Stream. */
size_t audio_write(void *buf, size_t offset, size_t len) {
  /* Blocked. */
  Area audio_buf = {buf, (void *)((uint8_t *)buf + len)};
  ioe_write(AM_AUDIO_PLAY, audio_buf);
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
  ioe_read(AM_GPU_CONFIG, &gpuinfo);
}
