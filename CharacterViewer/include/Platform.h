#ifndef OPENEQ_PLATFORM_H
#define OPENEQ_PLATFORM_H

#include <GL/glew.h>
#include <cstdio>
#include <string>
#include <inttypes.h>

char *loadFileData(std::string path);
void freeFileData(char *data);
double currentTime();

#endif
