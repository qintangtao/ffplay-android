#ifndef SDL_audio_c_h_
#define SDL_audio_c_h_

/* Functions to get a list of "close" audio formats */
extern SDL_AudioFormat SDL_FirstAudioFormat(SDL_AudioFormat format);
extern SDL_AudioFormat SDL_NextAudioFormat(void);

extern Uint8 SDL_SilenceValueForFormat(const SDL_AudioFormat format);
extern void SDL_CalculateAudioSpec(SDL_AudioSpec * spec);

#endif /*SDL_audio_c_h_*/