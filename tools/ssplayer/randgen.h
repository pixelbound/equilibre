/*
Copyright (C) 2000, 2001  Ryan Nunn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// System Shock Random Song Generator Header

#ifndef RANDGEN_H
#define RANDGEN_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#ifndef DEBUG
#define VERSION_STRING "%i.%i"
#else
#define VERSION_STRING "%i.%i Debug Build"
#endif

typedef char THEME[32][32];

static const unsigned char header[] = { 255, 255, 255, 243, 0, 0, 12, 32, 'R'+128, 'S'+128, 'G'+128 };

#define random(a)	((a)*(rand() / ((float)RAND_MAX+1)))

extern int max_width;
extern bool	show_drum;
extern bool	show_notes;
extern int vis_speed;

int rsgcreate(int argc, char **argv);


#endif //RANDGEN_H

