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
static int www = 0;
static int hhh = 0;

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
  int offset =  w * y + x;
    lseek(fbdev, offset, SEEK_SET);
    write(fbdev, pixels, sizeof(pixels));
    */

   if (w == 0 && h == 0)
  {
    w = www;
    h = hhh;
  }
  //printf("NDL: Now w is %d and www is %d\n", w, www);
  assert(w > 0 && w <= www);
  //printf("NDL: Now h is %d \n", h);
  assert(h > 0 && h <= hhh);

  // write(1, "here\n", 10);
  // printf("draw [%d, %d] to [%d, %d]\n", w, h, x, y);
  /*
  for (size_t row = 0; row < h; ++row)
  {
     printf("draw row %ld with len %d\n", row, w);
    lseek(fbdev, x + (y + row) * www, SEEK_SET);
    printf("pixels pos %p with len %d\n",pixels + row * w, w);
    write(fbdev, pixels + row * w, w);
    printf("draw row %ld with len %d\n", row, w);
  }
  write(fbdev, 0, 0);
  */
  for(size_t row = 0; row < h; row++) {
    lseek(fbdev, x + (y + row) * www, SEEK_SET);
    printf("NDL: writing to %ld\n", x + (y + row) * www);
    write(fbdev, pixels + row * w, w);  
  }
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
  dispinfodev = open("/proc/dispinfo", 0, 0);

  /*
  FILE *fp = fopen("/proc/dispinfo", "r");
  fscanf(fp, "WIDTH:%d\nHEIGHT:%d", &www, &hhh);
  */
  www = 400;
  hhh = 300;
  printf("NDL: Now www is %d, hhh is %d\n", www, hhh);
  return 0;
}

void NDL_Quit() {
}
