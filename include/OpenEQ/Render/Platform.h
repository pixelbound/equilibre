#ifndef OPENEQ_PLATFORM_H
#define OPENEQ_PLATFORM_H

#include <cstdio>
#include <string>
#include <inttypes.h>

#ifdef _WIN32

#ifdef BUILD_RENDER_DLL
#define RENDER_DLL __declspec(dllexport)
#else
#define RENDER_DLL __declspec(dllimport)
#endif

#ifdef BUILD_GAME_DLL
#define GAME_DLL __declspec(dllexport)
#else
#define GAME_DLL __declspec(dllimport)
#endif

#else

#define RENDER_DLL
#define GAME_DLL
 
#endif // _WIN32

typedef unsigned int buffer_t;
typedef unsigned int texture_t;

char RENDER_DLL * loadFileData(std::string path);
void RENDER_DLL freeFileData(char *data);
double RENDER_DLL currentTime();

#endif // OPENEQ_PLATFORM_H
