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

#include <QFile>
#include "EQuilibre/Game/SoundTrigger.h"
#include "EQuilibre/Game/StreamReader.h"

const uint32_t SoundEntry::Size = 84;

void SoundEntry::read(StreamReader &reader)
{
    reader.unpackFields("IIII", &Offset1, &Field2, &Field3, &ID);
    reader.unpackFields("ffff", &Pos.x, &Pos.y, &Pos.z, &Radius);
    reader.unpackFields("IIII", &Offset2, &Field10, &Duration, &Offset3);
    reader.unpackFields("IIHH", &SoundID1, &SoundID2, &Flags1, &Field16);
    reader.unpackFields("IIIIII", &Flags2, &Field18, &Field19, &Field20, &Field21, &Field22);
}

////////////////////////////////////////////////////////////////////////////////

SoundTrigger::SoundTrigger(SoundEntry entry)
{
    m_entry = entry;
    
    // Compute trigger volume.
    vec3 extent(entry.Radius * 0.5f, entry.Radius * 0.5f, entry.Radius * 0.5f);
    m_bounds.low = entry.Pos - extent;
    m_bounds.high = entry.Pos + extent;
}

const AABox & SoundTrigger::bounds() const
{
    return m_bounds;
}

const SoundEntry & SoundTrigger::entry() const
{
    return m_entry;
}

bool SoundTrigger::fromFile(QVector<SoundTrigger *> &triggers, QString path)
{
    SoundEntry entry;
    QFile f(path);
    if(f.open(QFile::ReadOnly))
    {
        StreamReader reader(&f);
        while(!f.atEnd())
        {
            entry.read(reader);
            triggers.append(new SoundTrigger(entry));
        }
        return true;
    }
    return false;
}
