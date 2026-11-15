#include "platform_audio.h"
#include <SDL.h>

static SDL_AudioDeviceID device;
static Uint8 *audio_data;
static Uint32 audio_length;

void platform_audio_init(void) {
    SDL_AudioSpec spec;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return;
    if (!SDL_LoadWAV("assets/llegada-ascensor.wav", &spec, &audio_data, &audio_length)) return;
    device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (!device) { SDL_FreeWAV(audio_data); audio_data = NULL; return; }
    SDL_PauseAudioDevice(device, 0);
}
void platform_audio_play_arrival(void) {
    if (!device || !audio_data) return;
    SDL_ClearQueuedAudio(device);
    SDL_QueueAudio(device, audio_data, audio_length);
}
