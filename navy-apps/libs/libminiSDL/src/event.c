#include <NDL.h>
#include <SDL.h>


#define keyname(k) #k,
#define BUFLEN 32

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  printf("In SDL_Pollevent now\n");
  char buf[BUFLEN];
  memset(buf, 0, BUFLEN);
  /* Listening for events. */
  if(!NDL_PollEvent(buf, BUFLEN)) return 0;
  if(!strncmp(buf, "ku", 2)) {
    ev -> type = SDL_KEYUP;
  }
  else if(!strncmp(buf, "kd", 2)) {
    ev -> type = SDL_KEYDOWN;
  }
  else 
    ev -> type = SDL_USEREVENT;
  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
    //printf("SDL: Comparing event %s\n", keyname[i]);
    //printf("SDL: receiving event %s\n", buf + 3);
    if(!strncmp(buf + 3, keyname[i], strlen(keyname[i]))) {
      ev -> key.keysym.sym = i;
      //printf("SDL: Event detected~ \n");
      break;
    }
  }
  printf("And now the key is %d\n", ev -> key.keysym.sym);
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  printf("In SDL_Waitevent now\n");
  char buf[BUFLEN];
  memset(buf, 0, BUFLEN);
  /* Listening for events. */
  while(!NDL_PollEvent(buf, BUFLEN)) ;
  if(!strncmp(buf, "ku", 2)) {
    event -> type = SDL_KEYUP;
  }
  else if(!strncmp(buf, "kd", 2)) {
    event -> type = SDL_KEYDOWN;
  }
  else 
    event -> type = SDL_USEREVENT;
  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
    //printf("SDL: Comparing event %s\n", keyname[i]);
    //printf("SDL: receiving event %s\n", buf + 3);
    if(!strncmp(buf + 3, keyname[i], strlen(keyname[i]))) {
      event -> key.keysym.sym = i;
      //printf("SDL: Event detected~ \n");
      break;
    }
  }
  printf("And now the key is %d\n", event -> key.keysym.sym);
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
