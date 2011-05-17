/*
	DDS GIMP plugin

	Copyright (C) 2004-2008 Shawn Kirst <skirst@insightbb.com>,
   with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; see the file COPYING.  If not, write to
	the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef IMATH_H
#define IMATH_H

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#endif

#define IS_POW2(x)     (!((x) & ((x) - 1)))
#define IS_MUL4(x)     (((x) & 3) == 0)

/* round integer x up to next multiple of 4 */
#define RND_MUL4(x)    ((x) + (4 - ((x) & 3)))

static inline int mul8bit(int a, int b)
{
   int t = a * b + 128;
   return((t + (t >> 8)) >> 8);
}

static inline int blerp(int a, int b, int x)
{
   return(a + mul8bit(b - a, x));
}

static inline int icerp(int a, int b, int c, int d, int x)
{
   int p = (d - c) - (a - b);
   int q = (a - b) - p;
   int r = c - a;
   return((x * (x * (x * p + (q << 7)) + (r << 14)) + (b << 21)) >> 21);
}

#define GETL64(buf) \
   (((unsigned long long)(buf)[0]      ) | \
    ((unsigned long long)(buf)[1] <<  8) | \
    ((unsigned long long)(buf)[2] << 16) | \
    ((unsigned long long)(buf)[3] << 24) | \
    ((unsigned long long)(buf)[4] << 32) | \
    ((unsigned long long)(buf)[5] << 40) | \
    ((unsigned long long)(buf)[6] << 48) | \
    ((unsigned long long)(buf)[7] << 56))

#define GETL32(buf) \
   (((unsigned int)(buf)[0]      ) | \
    ((unsigned int)(buf)[1] <<  8) | \
    ((unsigned int)(buf)[2] << 16) | \
    ((unsigned int)(buf)[3] << 24))

#define GETL16(buf) \
   (((unsigned short)(buf)[0]     ) | \
    ((unsigned short)(buf)[1] << 8))

#define PUTL16(buf, s) \
   (buf)[0] = ((s)     ) & 0xff; \
   (buf)[1] = ((s) >> 8) & 0xff;

#define PUTL32(buf, l) \
   (buf)[0] = ((l)      ) & 0xff; \
    (buf)[1] = ((l) >>  8) & 0xff; \
    (buf)[2] = ((l) >> 16) & 0xff; \
    (buf)[3] = ((l) >> 24) & 0xff;

#define PUTL64(buf, ll) \
   (buf)[0] = ((ll)      ) & 0xff; \
    (buf)[1] = ((ll) >>  8) & 0xff; \
    (buf)[2] = ((ll) >> 16) & 0xff; \
    (buf)[3] = ((ll) >> 24) & 0xff; \
   (buf)[4] = ((ll) >> 32) & 0xff; \
   (buf)[5] = ((ll) >> 40) & 0xff; \
   (buf)[6] = ((ll) >> 48) & 0xff; \
   (buf)[7] = ((ll) >> 56) & 0xff;

#endif
