#include <SDL.h>
#include <SDL_bmp.h>
#include <stdio.h>
#include <assert.h>

#define W 400
#define H 300

// USAGE:
//   j/down - page down
//   k/up - page up
//   gg - first page

// number of slides
const int N = 2;
// slides path pattern (starts from 0)
const char *path = "/share/slides/slides-%d.bmp";

static SDL_Surface *slide = NULL;
static int cur = 0;

void render() {
  printf("Freeing...\n");
  if (slide) {
    SDL_FreeSurface(slide);
  }
  printf("Free success !\n");
  printf("Loading...\n");
  char fname[256];
  sprintf(fname, path, cur);
  printf("Load success ~\n");
  slide = SDL_LoadBMP(fname);
  assert(slide);
  SDL_UpdateRect(slide, 0, 0, 0, 0);
}

void prev(int rep) {
  if (rep == 0) rep = 1;
printf("rep get ~\n");
printf("Now cur is %d rep is %d\n", cur, rep);
  cur -= rep;
 printf("cur get ~\n"); 
  if (cur < 0) cur = 0;
  render();
}

void next(int rep) {
  printf("In next now...\n");
  if (rep == 0) rep = 1;
  printf("rep get ~\n");
  printf("Now cur is %d rep is %d\n", cur, rep);
  cur += rep;
  printf("Cur get ~\n");
  if (cur >= N) cur = N - 1;
  printf("Calling render...\n");
  render();
}

int main() {
  SDL_Init(0);
  SDL_Surface *screen = SDL_SetVideoMode(W, H, 32, SDL_HWSURFACE);

  int rep = 0, g = 0;

  render();

  while (1) {
    SDL_Event e;
    SDL_WaitEvent(&e);

    if (e.type == SDL_KEYDOWN) {
      switch(e.key.keysym.sym) {
        case SDLK_0: rep = rep * 10 + 0; break;
        case SDLK_1: rep = rep * 10 + 1; break;
        case SDLK_2: rep = rep * 10 + 2; break;
        case SDLK_3: rep = rep * 10 + 3; break;
        case SDLK_4: rep = rep * 10 + 4; break;
        case SDLK_5: rep = rep * 10 + 5; break;
        case SDLK_6: rep = rep * 10 + 6; break;
        case SDLK_7: rep = rep * 10 + 7; break;
        case SDLK_8: rep = rep * 10 + 8; break;
        case SDLK_9: rep = rep * 10 + 9; break;
        case SDLK_J:
        case SDLK_DOWN: printf("event J detected ~\n"); next(rep); rep = 0; g = 0; break;
        case SDLK_K:
        case SDLK_UP: printf("event K detected ~\n");prev(rep); rep = 0; g = 0; break;
        case SDLK_G:
          g ++;
          if (g > 1) {
            prev(100000);
            rep = 0; g = 0;
          }
          break;
      }
    }
  }

  return 0;
}
