#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  /*
  printf("Loading file %s\n", filename);
  int fd = open(filename, 0, 0);
  lseek(fd, 0, SEEK_END);
  int size = lseek(fd, 0, SEEK_CUR);
  char *buf = (char *)malloc(sizeof(char) * size);
  lseek(fd, 0, SEEK_SET);
  read(fd, buf, size);
  printf("traping...\n");
  SDL_Surface *ret_surf = STBIMG_LoadFromMemory(buf, size);
  printf("STB Back...\n");
  if(ret_surf == NULL) {
    printf("ERROR: Couldn't load %s\n",filename);
    exit(1);
  }
  close(fd);
  free(buf); 
  printf("Read success\n");
  return ret_surf;
  */
  /*
  FILE *fp = fopen(filename, "r");
  if(fp == NULL) {
    printf("ERROR: Couldn't open %s\n", filename);
    exit(1);
  }
  fseek(fp, 0, SEEK_END);
  size_t _size = ftell(fp);
  fseek(fp, 0 ,SEEK_SET);
  char *buf = (char *)malloc( _size);
  fread(buf, _size, 1, fp);
  SDL_Surface *ret_surf = STBIMG_LoadFromMemory(buf, _size);
  if(ret_surf == NULL) {
    printf("ERROR: Couldn't load %s\n",filename);
    exit(1);
  }
  fclose(fp);
  free(buf); 
  printf("Read success\n");
  return ret_surf;
  */
  FILE* fp = fopen(filename, "r");
  
  fseek(fp, 0L, SEEK_END);
  size_t f_size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  char *buf = (char*)malloc(f_size);
  if(fread(buf, 1, f_size, fp) != f_size) assert("read img fail!\n");

  // printf("file %s with size %d, f_size is %d\n", filename, strlen(buf), f_size);
  SDL_Surface* img = STBIMG_LoadFromMemory(buf, f_size);
  
  free(buf);
  fclose(fp);
  return img;
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
