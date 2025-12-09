#ifndef __SDL_AUDIO_H__
#define __SDL_AUDIO_H__

typedef struct {
  int freq;
  uint16_t format;
  uint8_t channels;
  uint16_t samples;
  uint32_t size;
  void (*callback)(void *userdata, uint8_t *stream, int len);
  void *userdata;
} SDL_AudioSpec;

#define AUDIO_U8 8
#define AUDIO_S16 16
#define AUDIO_S16LSB    0x8010 // little-endian signed 16-bit
#define AUDIO_S16SYS AUDIO_S16

#define SDL_MIX_MAXVOLUME  128

typedef struct SDL_AudioCVT {
    int needed;                 // 是否需要转换
    uint16_t src_format;        // 源音频格式
    uint16_t dst_format;        // 目标音频格式
    double rate_incr;           // 速率增量
    uint8_t *buf;               // 音频缓冲区
    int len;                    // 缓冲区长度
    int len_cvt;                // 转换后长度
    int len_mult;               // 长度倍数（PAL 需要）
    double len_ratio;           // 长度比例
    void (*filters[10])(struct SDL_AudioCVT *cvt, uint16_t format);
    int filter_index;
} SDL_AudioCVT;

static inline int SDL_BuildAudioCVT(SDL_AudioCVT *cvt,
                                    uint16_t src_format,
                                    uint8_t src_channels,
                                    int src_rate,
                                    uint16_t dst_format,
                                    uint8_t dst_channels,
                                    int dst_rate) {
    // 简单实现：假设不需要转换
    cvt->needed = 0;
    cvt->src_format = src_format;
    cvt->dst_format = dst_format;
    cvt->len_mult = 1;      // PAL 需要这个字段
    cvt->len_ratio = 1.0;
    return 0;
}

static inline int SDL_ConvertAudio(SDL_AudioCVT *cvt) {
    if (cvt->needed) {
        // 这里应该实现实际的音频转换
        // 但为了编译通过，我们只设置长度
        if (cvt->buf && cvt->len > 0) {
            cvt->len_cvt = cvt->len;
        }
    } else {
        cvt->len_cvt = cvt->len;
    }
    return 0;
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
void SDL_CloseAudio();
void SDL_PauseAudio(int pause_on);
SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len);
void SDL_FreeWAV(uint8_t *audio_buf);
void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume);
void SDL_LockAudio();
void SDL_UnlockAudio();

#endif
