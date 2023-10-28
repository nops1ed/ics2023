#include <am.h>
#include <nemu.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

typedef struct _AudioData {
  int freq, channels, samples, sbuf_size;
}_AudioData;

//static _AudioData audio; 
static uint32_t buf_pos = 0;

void __am_audio_init() {
  /*
  audio.freq = inl(AUDIO_FREQ_ADDR);
  audio.channels = inl(AUDIO_CHANNELS_ADDR);
  audio.samples = inl(AUDIO_SAMPLES_ADDR);
  audio.sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
  */
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  /* Buf_size will not change. */
  /*
  cfg->bufsize = audio.sbuf_size;
  */
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  /*
  ctrl->freq = audio.freq;
  ctrl->channels = audio.channels;
  ctrl->samples = audio.samples;
  */
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  outl(AUDIO_INIT_ADDR, 1);
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
 //add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
  uint8_t *audio = ctl->buf.start;
  uint32_t buf_size = inl(AUDIO_SBUF_SIZE_ADDR);
  uint32_t cnt = inl(AUDIO_COUNT_ADDR);
  uint32_t len = ctl->buf.end - ctl->buf.start;
  
  while(len > buf_size - cnt){;}

  uint8_t *AB = (uint8_t *)(uintptr_t)AUDIO_SBUF_ADDR;
  for(int i = 0; i < len; ++i) {
    AB[buf_pos] = audio[i];
    buf_pos = (buf_pos + 1) % buf_size;  
  }

  outl(AUDIO_COUNT_ADDR, cnt + len);
}
