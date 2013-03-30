#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <algorithm>
#include "SDL/SDL.h"
#include "SDL/SDL_main.h"

struct WAVEHeader
{
    uint8_t ChunkID[4];     // 'RIFF'
    uint32_t ChunkSize;     // RIFF data length
    uint8_t Format[4];      // 'WAVE'
    uint8_t Subchunk1ID[4]; // 'fmt '
    uint32_t Subchunk1Size; // Format data length
    uint16_t AudioFormat;
    uint16_t NumOfChan;
    uint32_t SamplesPerSec;
    uint32_t BytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    uint8_t Subchunk2ID[4];  // 'data'
    uint32_t Subchunk2Size;  // Sampled data length
};

class WAVEPlayer
{
public:
    WAVEHeader header;
    uint32_t current_sample;
    uint32_t num_samples;
    uint8_t *sample_data;
    bool sdl_initialized;
    SDL_AudioSpec audioSpec;

    WAVEPlayer();
    ~WAVEPlayer();

    bool loadFile(const char *path);
    bool play();

private:
    static void playCallback(void *userdata, uint8_t *stream, int num_bytes);
    void playCallback(uint8_t *stream, int num_bytes);
    bool validateHeader() const;
};

WAVEPlayer::WAVEPlayer()
{
    memset(&header, 0, sizeof(WAVEHeader));
    memset(&audioSpec, 0, sizeof(SDL_AudioSpec));
    current_sample = 0;
    num_samples = 0;
    sample_data = NULL;
    sdl_initialized = false;
}

WAVEPlayer::~WAVEPlayer()
{
    if(sdl_initialized)
    {
        SDL_Quit();
    }
    free(sample_data);
}

bool WAVEPlayer::validateHeader() const
{
    if(memcmp(header.ChunkID, "RIFF", sizeof(header.ChunkID)) ||
       memcmp(header.Format, "WAVE", sizeof(header.Format)) ||
       memcmp(header.Subchunk1ID, "fmt ", sizeof(header.Subchunk1ID)) ||
       memcmp(header.Subchunk2ID, "data", sizeof(header.Subchunk2ID)))
    {
        fprintf(stderr, "Invalid RIFF tags for a WAVE file.\n");
        return false;
    }

    if(header.AudioFormat != 1)
    {
        fprintf(stderr, "Audio formats other than LPCM are not supproted.\n");
        return false;
    }

    if(header.NumOfChan < 1 || header.NumOfChan > 2)
    {
        fprintf(stderr, "Only mono and stero data is supported.\n");
        return false;
    }

    if((header.BitsPerSample != 8) && (header.BitsPerSample != 16))
    {
        fprintf(stderr, "Only 8-bit and 16-bit data is supported.\n");
        return false;
    }

    return true;
}

bool WAVEPlayer::loadFile(const char *path)
{
    bool loaded = false;
    FILE *f = fopen(path, "rb");
    if(!f)
    {
        fprintf(stderr, "Could not open '%s'.\n", path);
        goto cleanup;
    }

    // XXX This doesn't work on big endian systems.
    if(1 != fread(&header, sizeof(WAVEHeader), 1, f))
    {
        fprintf(stderr, "Could not read WAVE header.\n");
        goto cleanup;
    }
    else if(!validateHeader())
    {
        fprintf(stderr, "WAVE header failed validation.\n");
        goto cleanup;
    }

    sample_data = (uint8_t *)malloc(header.Subchunk2Size);
    if(!sample_data)
    {
        fprintf(stderr, "Could not allocate memory for samples.\n");
        goto cleanup;
    }

    if(1 != fread(sample_data, header.Subchunk2Size, 1, f))
    {
        fprintf(stderr, "Could not read sample data from the file.\n");
        goto cleanup;
    }

    num_samples = header.Subchunk2Size / (header.BitsPerSample / 8);
    loaded = true;

cleanup:
    if(f)
    {
        fclose(f);
    }
    if(!loaded)
    {
       free(sample_data);
       sample_data = NULL;
    }
    return loaded;
}

bool WAVEPlayer::play()
{
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fprintf(stderr, "Could not initialize SDL.\n");
        return false;
    }
    sdl_initialized = true;

    SDL_AudioSpec desiredSpec;
    memset(&desiredSpec, 0, sizeof(SDL_AudioSpec));
    desiredSpec.freq = header.SamplesPerSec;
    switch(header.BitsPerSample)
    {
    case 8:
        desiredSpec.format = AUDIO_S8;
        break;
    case 16:
        desiredSpec.format = AUDIO_S16LSB;
        break;
    }
    desiredSpec.channels = header.NumOfChan;
    desiredSpec.samples = 4096;
    desiredSpec.callback = playCallback;
    desiredSpec.userdata = this;

    if(SDL_OpenAudio(&desiredSpec, &audioSpec) < 0)
    {
        fprintf(stderr, "Could not initialize audio.\n");
        return false;
    }

    SDL_Surface *screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
    SDL_WM_SetCaption("WAVE Player", NULL);
    SDL_PauseAudio(0);

    SDL_Event event;
    while(SDL_WaitEvent(&event))
    {
        if(event.type == SDL_KEYDOWN)
        {
            if(event.key.keysym.sym == SDLK_ESCAPE)
            {
                break;
            }
        }
        else if(event.type == SDL_QUIT)
        {
            break;
        }
    }

    return true;
}

void WAVEPlayer::playCallback(void *userdata, uint8_t *stream, int num_bytes)
{
    WAVEPlayer *player = reinterpret_cast<WAVEPlayer*>(userdata);
    player->playCallback(stream, num_bytes);
}

void WAVEPlayer::playCallback(uint8_t *stream, int num_bytes)
{
    int sample_size = 2;
    if((audioSpec.format == AUDIO_S8) || (audioSpec.format == AUDIO_U8))
        sample_size = 1;
    memset(stream, 0, num_bytes);
    if(current_sample < num_samples)
    {
        int requested_samples = num_bytes / sample_size;
        int samples_left = num_samples - current_sample;
        int samples_to_copy = std::min(requested_samples, samples_left);
        memcpy(stream,
               sample_data + (current_sample * sample_size),
               samples_to_copy * sample_size);
        current_sample += samples_to_copy;
    }
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <WAVE file>\n", argv[0]);
        return 1;
    }

    WAVEPlayer player;
    if(!player.loadFile(argv[1]))
    {
        fprintf(stderr, "Could not load WAVE file.\n");
        return 1;
    }
    fprintf(stderr, "Loaded '%s'.\n", argv[1]);
    if(!player.play())
    {
        fprintf(stderr, "Could not play WAVE file.\n");
        return 1;
    }
    return 0;
}
