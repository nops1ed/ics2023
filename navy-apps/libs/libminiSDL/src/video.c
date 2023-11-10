#include <NDL.h>
#include <sdl-video.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(dst && src);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  /* src is the source surface to be copied, srcrect is the rectangular area to be copied, 
  * if NULL, the entire surface is copied. dst is the destination surface, 
  * dstrect is the destination position, only its top-left coordinate is used, ignoring its width and height. 
  * If NULL, the destination position is (0, 0). 
  * The function will save the final copy area in dstrect after clipping, without modifying srcrect. 
  */
  uint32_t src_col = src -> w, src_row = src -> h;
  uint32_t src_pos = 0;
  if(srcrect != NULL) {
    src_col = srcrect -> w;
    src_row = srcrect -> h;
    src_pos = srcrect -> y * src -> w + srcrect -> x;
  }
  uint32_t dst_pos = dstrect == NULL ? 0 : dstrect -> y * dst -> w + dstrect -> x;
  //printf("Copying...\n");
  //printf("Debug info: src_col = %d src_row = %d src_pos = %d \ndst_pos = %d dst->w = %d dst->h = %d\n",src_col,src_row,src_pos,dst_pos,dst->w,dst->h);
  if(src -> format -> BytesPerPixel == 4)
    for(int i = 0; i < src_row; i++)
      memcpy((uint32_t*)(dst -> pixels) + dst_pos + i * dst -> w, 
              (uint32_t *)(src -> pixels) + src_pos + i * src -> w, sizeof(uint32_t) * src_col);
  else if(src -> format -> BytesPerPixel == 1)
    for(int i = 0; i < src_row; i++)
      memcpy((uint8_t *)dst -> pixels + dst_pos + i * dst -> w, 
              (uint8_t *)src -> pixels + src_pos + i * src -> w, sizeof(uint8_t) * src_col);
  else printf("Pixel format cannot found");
}

void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
  assert(dst);
  /* Assume Little endian. */
  uint32_t dst_col = dst -> w, dst_row = dst -> h, dst_pos = 0;
  if(dstrect != NULL) {
    dst_col = dstrect -> w;
    dst_row = dstrect -> h;
    dst_pos = dstrect -> y * dst -> w + dstrect -> x;
  }
  //printf("Copying...\n");
  if(dst -> format -> BytesPerPixel == 4)
    for(int i = 0; i < dst_row; i++)
      memset((uint32_t *)dst -> pixels + dst_pos + dst -> w * i,
              color, sizeof(uint32_t) * dst_row);
  else if(dst -> format -> BytesPerPixel == 1)
    for(int i = 0; i < dst_row; i++)
     memset((uint8_t *)dst -> pixels + dst_pos + dst -> w * i,
              color, sizeof(uint8_t) * dst_row);
  else printf("Pixel format cannot found");
  //printf("Copy success ~\n");
}

void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h) {
  //printf("SDL: x is %d y is %d w is %d h is %d\n",x ,y , w ,h);
  if(x == 0 && y == 0 && w == 0 && h == 0) {
    // Update the whole screen
    w = s -> w;
    h = s -> h;
    //printf("SDL: Now w is %d h is %d\n",w ,h);
  }
  uint32_t pos = 0;
  uint32_t *_buf = (uint32_t *)malloc(sizeof(uint32_t) * w * h);
  for(uint32_t row = 0; row < h; row++)
    /* Color depth is 8. */
    if(s -> format -> BytesPerPixel == 1)
      for(uint32_t col = 0; col < w; col++) {
      /* The concept of using a palette at 8-bit color depth. */
        printf("Should not reach here\n");
        SDL_Color sdlcolor = s -> format -> palette -> colors[((row + y) * (s -> w) + x + col) * 4];
        _buf[pos++] = sdlcolor.val;
      }
    /* Color depth is 32. */
    else {
      /* Each pixel is described as a color using a 32-bit integer in the form of 00RRGGBB. */
      for(uint32_t col = 0; col < w; col++) {
          //uint32_t offset = ((row + y) * (s -> w) + x + col) * 4;
          uint32_t offset = x + y * s -> w + (col + row * s->w) * 4;
          _buf[pos++] = s -> pixels[offset + 3] << 24 | s -> pixels[offset + 2] << 16 |
                        s -> pixels[offset + 1] << 8 | s -> pixels[offset + 0];
      }
    }
    //printf("SDL: BUF initialized successfullly\n");
    NDL_DrawRect(_buf, x, y, w, h);
    //printf("Drawing success ~\n");
    free(_buf);
}

// APIs below are already implemented.

static inline int maskToShift(uint32_t mask) {
  switch (mask) {
    case 0x000000ff: return 0;
    case 0x0000ff00: return 8;
    case 0x00ff0000: return 16;
    case 0xff000000: return 24;
    case 0x00000000: return 24; // hack
    default: assert(0);
  }
}

SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
    uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  assert(depth == 8 || depth == 32);
  SDL_Surface *s = malloc(sizeof(SDL_Surface));
  assert(s);
  s->flags = flags;
  s->format = malloc(sizeof(SDL_PixelFormat));
  assert(s->format);
  if (depth == 8) {
    s->format->palette = malloc(sizeof(SDL_Palette));
    assert(s->format->palette);
    s->format->palette->colors = malloc(sizeof(SDL_Color) * 256);
    assert(s->format->palette->colors);
    memset(s->format->palette->colors, 0, sizeof(SDL_Color) * 256);
    s->format->palette->ncolors = 256;
  } else {
    s->format->palette = NULL;
    s->format->Rmask = Rmask; s->format->Rshift = maskToShift(Rmask); s->format->Rloss = 0;
    s->format->Gmask = Gmask; s->format->Gshift = maskToShift(Gmask); s->format->Gloss = 0;
    s->format->Bmask = Bmask; s->format->Bshift = maskToShift(Bmask); s->format->Bloss = 0;
    s->format->Amask = Amask; s->format->Ashift = maskToShift(Amask); s->format->Aloss = 0;
  }

  s->format->BitsPerPixel = depth;
  s->format->BytesPerPixel = depth / 8;

  s->w = width;
  s->h = height;
  s->pitch = width * depth / 8;
  assert(s->pitch == width * s->format->BytesPerPixel);

  if (!(flags & SDL_PREALLOC)) {
    s->pixels = malloc(s->pitch * height);
    assert(s->pixels);
  }

  return s;
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
    int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  SDL_Surface *s = SDL_CreateRGBSurface(SDL_PREALLOC, width, height, depth,
      Rmask, Gmask, Bmask, Amask);
  assert(pitch == s->pitch);
  s->pixels = pixels;
  return s;
}

void SDL_FreeSurface(SDL_Surface *s) {
  if (s != NULL) {
    if (s->format != NULL) {
      if (s->format->palette != NULL) {
        if (s->format->palette->colors != NULL) free(s->format->palette->colors);
        free(s->format->palette);
      }
      free(s->format);
    }
    if (s->pixels != NULL && !(s->flags & SDL_PREALLOC)) free(s->pixels);
    free(s);
  }
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
  if (flags & SDL_HWSURFACE) NDL_OpenCanvas(&width, &height);
  return SDL_CreateRGBSurface(flags, width, height, bpp,
      DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(src && dst);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  assert(dst->format->BitsPerPixel == 8);

  int x = (srcrect == NULL ? 0 : srcrect->x);
  int y = (srcrect == NULL ? 0 : srcrect->y);
  int w = (srcrect == NULL ? src->w : srcrect->w);
  int h = (srcrect == NULL ? src->h : srcrect->h);

  assert(dstrect);
  if(w == dstrect->w && h == dstrect->h) {
    /* The source rectangle and the destination rectangle
     * are of the same size. If that is the case, there
     * is no need to stretch, just copy. */
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_BlitSurface(src, &rect, dst, dstrect);
  }
  else {
    assert(0);
  }
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int firstcolor, int ncolors) {
  assert(s);
  assert(s->format);
  assert(s->format->palette);
  assert(firstcolor == 0);

  s->format->palette->ncolors = ncolors;
  memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);

  if(s->flags & SDL_HWSURFACE) {
    assert(ncolors == 256);
    for (int i = 0; i < ncolors; i ++) {
      uint8_t r = colors[i].r;
      uint8_t g = colors[i].g;
      uint8_t b = colors[i].b;
    }
    SDL_UpdateRect(s, 0, 0, 0, 0);
  }
}

static void ConvertPixelsARGB_ABGR(void *dst, void *src, int len) {
  int i;
  uint8_t (*pdst)[4] = dst;
  uint8_t (*psrc)[4] = src;
  union {
    uint8_t val8[4];
    uint32_t val32;
  } tmp;
  int first = len & ~0xf;
  for (i = 0; i < first; i += 16) {
#define macro(i) \
    tmp.val32 = *((uint32_t *)psrc[i]); \
    *((uint32_t *)pdst[i]) = tmp.val32; \
    pdst[i][0] = tmp.val8[2]; \
    pdst[i][2] = tmp.val8[0];

    macro(i + 0); macro(i + 1); macro(i + 2); macro(i + 3);
    macro(i + 4); macro(i + 5); macro(i + 6); macro(i + 7);
    macro(i + 8); macro(i + 9); macro(i +10); macro(i +11);
    macro(i +12); macro(i +13); macro(i +14); macro(i +15);
  }

  for (; i < len; i ++) {
    macro(i);
  }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, uint32_t flags) {
  assert(src->format->BitsPerPixel == 32);
  assert(src->w * src->format->BytesPerPixel == src->pitch);
  assert(src->format->BitsPerPixel == fmt->BitsPerPixel);

  SDL_Surface* ret = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
    fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  assert(fmt->Gmask == src->format->Gmask);
  assert(fmt->Amask == 0 || src->format->Amask == 0 || (fmt->Amask == src->format->Amask));
  ConvertPixelsARGB_ABGR(ret->pixels, src->pixels, src->w * src->h);

  return ret;
}

uint32_t SDL_MapRGBA(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  assert(fmt->BytesPerPixel == 4);
  uint32_t p = (r << fmt->Rshift) | (g << fmt->Gshift) | (b << fmt->Bshift);
  if (fmt->Amask) p |= (a << fmt->Ashift);
  return p;
}

int SDL_LockSurface(SDL_Surface *s) {
  return 0;
}

void SDL_UnlockSurface(SDL_Surface *s) {
}
