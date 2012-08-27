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
