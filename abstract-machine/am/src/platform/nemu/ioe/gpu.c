#include <am.h>
#include <nemu.h>
#include "../../../riscv/riscv.h"

#define SYNC_ADDR (VGACTL_ADDR + 4)

#define W 400
#define H 300

static uint32_t _width, _height;

void __am_gpu_init() {
  uint32_t vga_info = inl(VGACTL_ADDR);
  _width = (vga_info >> 16) & 0xFFFF;
  _height = vga_info & 0xFFFF;
  /*
  uint32_t vga_info = inl(VGACTL_ADDR);
  int w = (vga_info >> 16) & 0xFFFF;
  int h = vga_info & 0xFFFF;
  int i;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i ++) fb[i] = i;
  outl(SYNC_ADDR, 1);
  //vgactl_port_base[0] = (screen_width() << 16) | screen_height();
  */
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = _width, .height = _height,
    .vmemsz = 0
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
/*
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
*/
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  if (w == 0 || h == 0) return;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pi = ctl->pixels;
  for (int i = 0; i < h; i ++) {
    for (int j = 0; j < w; j ++) {
      fb[(y + i) * W + x + j] = pi[i * w + j];
    }
  }
  outl(SYNC_ADDR, 1);
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
