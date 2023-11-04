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
static int screen_w = 0, screen_h = 0;
static int dispinfodev = -1;
static int w, h;

/*
struct timeval {
  uint32_t      tv_sec;     // seconds
  uint32_t tv_usec;    // microseconds 
};
*/

uint32_t NDL_GetTicks() {
  static struct timeval time;
  gettimeofday(&time, NULL);
  return (uint32_t)time.tv_usec;
}

int NDL_PollEvent(char *buf, int len) {
  buf[0] = '\0';
  int ret_val = read(evtdev, buf, len);
  return ret_val;
}

void NDL_OpenCanvas(int *w, int *h) {
  printf("Now the w is %d and h is %d\n", *w, *h);
  if (getenv("NWM_APP")) {
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
  /*
  for(int row = 0; row < h; row++) {
    lseek(fbdev, offset, SEEK_SET);
    write(fbdev, pixels, sizeof(pixels));
  }
  */
  for (size_t row = 0; row < h; ++row) {
    lseek(fbdev, x + (y + row) * w, SEEK_SET);
    write(fbdev, pixels + row * w, w);
  }
  write(fbdev, 0, 0);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
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
  FILE *dispinfodev = fopen("/proc/dispinfo", "r");
  fscanf(dispinfodev, "WIDTH:%d\nHEIGHT:%d", &w, &h);
  return 0;
}

void NDL_Quit() {
}
