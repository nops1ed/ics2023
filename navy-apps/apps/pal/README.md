# PAL

This is a fork of SDLPAL(https://github.com/sdlpal/sdlpal).
It is ported to Navy-apps.

## Game data

The source repository does not contain the original PAL game data. Set
`PAL_DATA_DIR` to a directory copied from a licensed DOS or Win95 game before
building the Navy ramdisk. Linux paths are case-sensitive, and the filenames
must be lower-case.

The port checks these core files before installation:

```text
abc.mkf  ball.mkf  data.mkf  f.mkf    fbp.mkf  fire.mkf  gop.mkf
map.mkf  mgo.mkf   pat.mkf   rgm.mkf  sss.mkf  m.msg     word.dat
```

Audio files such as `mus.mkf`, `voc.mkf`, or `sounds.mkf` depend on the game
edition. The current Navy audio path is incomplete, so they do not enable
sound yet.

Add a lower-case `sdlpal.cfg` to the data directory:

```ini
WindowWidth=320
WindowHeight=200
```

NEMU exposes a 400x300 framebuffer, while PAL otherwise defaults to 640x400
and this miniSDL port cannot scale it. Do not put a host path in `GamePath`;
inside Navy the data is mounted at `/share/games/pal/`.
