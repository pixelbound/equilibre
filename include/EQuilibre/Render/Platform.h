// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef EQUILIBRE_PLATFORM_H
#define EQUILIBRE_PLATFORM_H

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

#endif // EQUILIBRE_PLATFORM_H
