#include <common.h>
#include <string.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

char* itoa(int num, char* str, int base) {  
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
  if (kbd.keydown) strncat(buf, "kd ", len);
  else
    strncat(buf, "ku ", len);
  strncat(buf, keyname[kbd.keycode], len - 3);
  strncat(buf, "\n", len - strlen(keyname[kbd.keycode] - 3));
  return strlen(buf);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  /* buf do not support lseek. */
  char _tmp[32];
  AM_GPU_CONFIG_T gpuinfo;
  ioe_read(AM_GPU_CONFIG, &gpuinfo);
  strncpy(buf, "WIDTH:", len);
  strncpy(buf, itoa(gpuinfo.width, _tmp, 10), len - 6);
  int sssss = strlen(_tmp);
  strncpy(buf, "\n", len - 7 - sssss);
  strncpy(buf, "HEIGHT:", len - 8 - sssss);
  strncpy(buf, itoa(gpuinfo.height, _tmp, 10), len - 15 - sssss);
  return strlen(buf);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
