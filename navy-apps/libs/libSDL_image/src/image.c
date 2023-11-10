#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"
#include <stdio.h>

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  printf("Loading file %s\n", filename);
  FILE *fp = fopen(filename, "r");
  if(fp == NULL) {
    printf("ERROR: Couldn't open %s\n", filename);
    exit(1);
  }
  //printf("read success\n");
  fseek(fp, 0, SEEK_END);
  size_t _size = ftell(fp);
  printf("size could be %ld\n",_size);
  fseek(fp, 0 ,SEEK_SET);
  char *buf = (char *)malloc(sizeof(char) * _size);
  fread(buf, sizeof(char), _size, fp);
  printf("%s\n",buf);
  printf("Traping into STB...\n");
  SDL_Surface *ret_surf = STBIMG_LoadFromMemory(buf, _size);
  printf("STB Back...\n");
  if(ret_surf == NULL) {
    printf("ERROR: Couldn't load %s\n",filename);
    exit(1);
  }
  fclose(fp);
  free(buf); 
  printf("Read success\n");
  return ret_surf;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
