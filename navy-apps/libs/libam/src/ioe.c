#include <am.h>

/*
#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

static uint64_t start_time = 0;
void _navy_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uptime -> us = NDL_GetTicks();
}

void _navy_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

#define KEYDOWN_MASK 0x8000

static char buf[BUFLEN] = {};
void _navy_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
   //char *buf = (char *)malloc(sizeof(char) * BUFLEN);
  memset(buf, 0, BUFLEN);
  if(!NDL_PollEvent(buf, BUFLEN)) {
    //printf("Exiting...\n");
    return 0;
  }
  //printf("Detecing event\n");
  if(!strncmp(buf, "ku", 2)) {
    ev -> type = SDL_KEYUP;
  }
  else if(!strncmp(buf, "kd", 2)) {
    ev -> type = SDL_KEYDOWN;
  }
  else {
    printf("User evnet here\n");
    ev -> type = SDL_USEREVENT;
  }
  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
    if(((strlen(buf + 3) - 1) == strlen(keyname[i])) && !strncmp(buf + 3, keyname[i], strlen(keyname[i]))) {
      ev -> key.keysym.sym = i;
      //keysnap[i] = (ev->type == SDL_KEYDOWN)? 1: 0;
      //printf("Get key %d here\n",i);
      break;
    }
  }
  return;
}

void __navy_gpu_config(AM_GPU_CONFIG_T *cfg){
  FILE *fp = fopen("/proc/dispinfo", "r");
  fscanf(fp, "WIDTH:%d\nHEIGHT:%d\n", &cfg->width, &cfg->height);
  fclose(fp);
  cfg->present = true;
  cfg->has_accel = false;
  cfg->vmemsz = 0;
}

void __navy_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int i, pi;
  uint32_t* pixels = ctl->pixels;
  AM_GPU_CONFIG_T cfg;
  __navy_gpu_config(&cfg);
  uint32_t width = cfg.width;
  int fd = open("/dev/fb", O_WRONLY);
  size_t base_offset = (ctl->y * width + ctl->x) * sizeof(uint32_t);
  size_t pixel_offset = 0;
  for (i = 0; i < ctl->h; ++ i) {
    int ret_seek = lseek(fd, base_offset, SEEK_SET);
    int ret_write = write(fd, pixels+pixel_offset, ctl->w * sizeof(uint32_t));
    // printf("(%d, %d, %s) ", ret_seek, ret_write, strerror(errno));
    pixel_offset += ctl->w;
    base_offset += width * sizeof(uint32_t);
  }
}

void __navy_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}

static void __navy_timer_config(AM_TIMER_CONFIG_T *cfg) { cfg->present = true; cfg->has_rtc = true; }
static void __navy_input_config(AM_INPUT_CONFIG_T *cfg) { cfg->present = true;  }
static void __navy_uart_config(AM_UART_CONFIG_T *cfg)   { cfg->present = false; }

typedef void (*handler_t)(void *buf);
static void *lut[128] = {
  [AM_TIMER_CONFIG] = __navy_timer_config,
  [AM_TIMER_RTC   ] = __navy_timer_rtc,
  [AM_TIMER_UPTIME] = __navy_timer_uptime,
  [AM_INPUT_CONFIG] = __navy_input_config,
  [AM_INPUT_KEYBRD] = __navy_input_keybrd,
  [AM_GPU_CONFIG  ] = __navy_gpu_config,
  [AM_GPU_FBDRAW  ] = __navy_gpu_fbdraw,
  [AM_GPU_STATUS  ] = __navy_gpu_status,
  // [AM_GPU_MEMCPY  ] = __navy_gpu_memcpy,
  [AM_UART_CONFIG ] = __navy_uart_config,
  // [AM_AUDIO_CONFIG] = __navy_audio_config,
  // [AM_AUDIO_CTRL  ] = __navy_audio_ctrl,
  // [AM_AUDIO_STATUS] = __navy_audio_status,
  // [AM_AUDIO_PLAY  ] = __navy_audio_play,
};

static void fail(void *buf) { puts("access nonexist register"); assert(0); }

bool ioe_init() {
  for (int i = 0; i < LENGTH(lut); i++)
    if (!lut[i]) lut[i] = fail;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  start_time = tv.tv_sec * 1000000 + tv.tv_usec;
  return true;
}

void ioe_read (int reg, void *buf) { ((handler_t)lut[reg])(buf); }
void ioe_write(int reg, void *buf) { ((handler_t)lut[reg])(buf); }
*/