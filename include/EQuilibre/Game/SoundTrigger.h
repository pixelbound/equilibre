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

#ifndef EQUILIBRE_SOUND_TRIGGER_H
#define EQUILIBRE_SOUND_TRIGGER_H

#include <QString>
#include <QVector>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Geometry.h"

class StreamReader;

class GAME_DLL SoundEntry
{
public:
    /**
     * Looks like a memory address or file offset. For the last entry this is always zero.
     */
    uint32_t Offset1;
    /**
     * Almost always zero. Found 35420640 for the last entry of 'oot'.
     */
    uint32_t Field2;
    /**
     * Always zero.
     */
    uint32_t Field3;
    /**
     * Increases by one for each entry. Does not seem unique across all *_sounds.eff files.
     */
    uint32_t ID;
    /**
     * XYZ coordinates of sphere where the sound is heard.
     */
    vec3 Pos;
    /**
     * Radius of sphere where the sound is heard.
     */
    float Radius;
    /**
     * Usually zero, although this can contain a large offset in Kunark or Velious zones.
     * Seems to be set only when Duration and Offset3 are also set (exception: 'frozenshadow').
     */
    uint32_t Offset2;
    /**
     * Almost always zero. Found 6000 in 'wakening'.
     */
    uint32_t Field10;
    /**
     * Usually zero. Sometimes, a few thousands (most often n000, few occurences of n500)
     * Unit could be ms. Found 2 in 'befallen'.
     */
    uint32_t Duration;
    /**
     * Usually zero. When set, large number that looks like a memory address.
     * Seems to be set only when Duration and Offset2 are set.
     */
    uint32_t Offset3;
    /**
     * Either zero, 1-30ish or >=162.
     * In the second case this is an index into the 'EMIT' entries of the zone's *_sndbnk.eff file.
     * In the third case this is an index into the 'LOOP' entries of the same file.
     */
    uint32_t SoundID1;
    /**
     *  Usually either zero or the same value than SoundID1.
     */
    uint32_t SoundID2;
    /**
     * Zero when SoundID1 >= 162. One or two when 1 <= SoundID1 < 160.
     * Sometimes 0x0100 is ORd with this field. This could mean SoundID1 is an index into the zone's *.xmi file.
     */
    uint16_t Flags1;
    /**
     * Unknown.
     */
    uint16_t Field16;
    /**
     * Usually zero or 0x64.
     */
    uint32_t Flags2;
    /**
     * Unknown.
     */
    uint32_t Field18;
    /**
     * Unknown.
     */
    uint32_t Field19;
    /**
     * Unknown.
     */
    uint32_t Field20;
    /**
     * Unknown.
     */
    uint32_t Field21;
    /**
     * Unknown.
     */
    uint32_t Field22;
    
    void read(StreamReader &reader);
    
    static const uint32_t Size;
};

class GAME_DLL SoundTrigger
{
public:
    SoundTrigger(SoundEntry entry);
    
    const SoundEntry & entry() const;
    const AABox & bounds() const;
    
    static bool fromFile(QVector<SoundTrigger *> &triggers, QString path);
    
private:
    SoundEntry m_entry;
    AABox m_bounds;
};

#endif
