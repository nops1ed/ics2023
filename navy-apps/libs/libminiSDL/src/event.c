#include <NDL.h>
#include <SDL.h>
#include <stdlib.h>


#define keyname(k) #k,
#define NR_KEYS ( sizeof(keyname)/sizeof(keyname[0]) )
#define BUFLEN 32

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

static uint8_t keysnap[NR_KEYS] = {0};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

static char buf[BUFLEN] = {};
int SDL_PollEvent(SDL_Event *ev) {
  //char *buf = (char *)malloc(sizeof(char) * BUFLEN);
  memset(buf, 0, BUFLEN);
  /* Listening for events. */
  if(!NDL_PollEvent(buf, BUFLEN)) {
    //printf("Exiting...\n");
    return 0;
  }
  //printf("Detecing event\n");
  if(!strncmp(buf, "kd", 2)) {
    ev -> type = SDL_KEYUP;
  }
  else if(!strncmp(buf, "ku", 2)) {
    ev -> type = SDL_KEYDOWN;
  }
  else {
    printf("User evnet here\n");
    ev -> type = SDL_USEREVENT;
  }
  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
    if(((strlen(buf + 3) - 1) == strlen(keyname[i])) && 
            !strncmp(buf + 3, keyname[i], strlen(keyname[i]))) {
      ev -> key.keysym.sym = i;
      keysnap[i] = (ev->type == SDL_KEYDOWN)? 1: 0;
      //printf("Get key %d here\n",i);
      break;
    }
  }
  return 1;
}

static char buf2[BUFLEN] = {};
int SDL_WaitEvent(SDL_Event *event) {
  //char *buf = (char *)malloc(sizeof(char) * BUFLEN);
  memset(buf2, 0, BUFLEN);
  /* Listening for events. */
  while(!NDL_PollEvent(buf2, BUFLEN)) ;
  if(!strncmp(buf2, "kd", 2)) {
    event -> type = SDL_KEYUP;
  }
  else if(!strncmp(buf2, "ku", 2)) {
    event -> type = SDL_KEYDOWN;
  }
  else 
    event -> type = SDL_USEREVENT;
  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
    if(((strlen(buf2 + 3) - 1) == strlen(keyname[i])) && 
          !strncmp(buf2 + 3, keyname[i], strlen(keyname[i]))) {
      event -> key.keysym.sym = i;
      keysnap[i] = (event->type == SDL_KEYDOWN)? 1: 0;
      break;
    }
  }
  //free(buf);
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return keysnap;
}
