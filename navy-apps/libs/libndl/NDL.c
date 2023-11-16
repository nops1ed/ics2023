#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int audiodev = -1;
static int audioinfo = -1;
static int screen_w = 0, screen_h = 0;
static int dispinfodev = -1;
static int disp_w = 0;
static int disp_h = 0;

typedef struct _AudioData {
  int freq, channels, samples, sbuf_size;
}_AudioData;

//AM_DEVREG(17, AUDIO_PLAY,   WR, Area buf);
typedef struct {
  void *start, *end;
} Area;

//static _AudioData *audio = {};

uint32_t NDL_GetTicks() {
  static struct timeval time;
  gettimeofday(&time, NULL);
  return (uint32_t)time.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  /* Buffer should be empty. */
  buf[0] = '\0';
  return read(evtdev, buf, len);
}

void NDL_OpenCanvas(int *w, int *h) {
  printf("Now w is %d h is %d \ndisp_w is %d disp_h is %d\n",*w, *h, disp_w, disp_h);
  if(*w > disp_w || *h > disp_h) assert(0);
  *w == 0 ? disp_w : *w;
  *h == 0 ? disp_h : *h;
  //printf("Now the w is %d and h is %d\n", *w, *h);
  if (getenv("NWM_APP")) {
    printf("I am in NWM_APP\n");
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  if (w == 0 && h == 0) {
    w = disp_w;
    h = disp_h;
  }
  //printf("NDL: Now w is %d and www is %d\n", w, www);
  assert(w > 0 && w <= disp_w);
  //printf("NDL: Now h is %d \n", h);
  assert(h > 0 && h <= disp_h);

  for(size_t row = 0; row < h; row++) {
    lseek(fbdev, x + (y + row) * disp_w, SEEK_SET);
    //printf("NDL: writing to %ld\n", x + (y + row) * disp_w);
    write(fbdev, pixels + row * w, w);  
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
  /*
  audio = {freq, channels, samples};
  write(sbctl, audio, 1); 
  */
}

void NDL_CloseAudio() {
  
}

int NDL_PlayAudio(void *buf, int len) {
  //ioe_write()
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  evtdev = open("/dev/events", 0, 0);
  fbdev = open("/dev/fb", 0, 0);
  dispinfodev = open("/proc/dispinfo", 0, 0);
  /*
  audioinfo = open("/dev/sbctl", 0, 0);
  audiodev = open("/dev/sb", 0, 0);
  */

  FILE *fip = fopen("/proc/dispinfo", "r");
  fscanf(fip, "WIDTH:%d\nHEIGHT:%d\n", &disp_w, &disp_h);
  fclose(fip);
  printf("NDL: Now www is %d, hhh is %d\n", disp_w, disp_h);
  return 0;
}

void NDL_Quit() {
}
