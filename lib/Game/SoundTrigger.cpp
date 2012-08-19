#include <QFile>
#include "OpenEQ/Game/SoundTrigger.h"
#include "OpenEQ/Game/StreamReader.h"

const uint32_t SoundEntry::Size = 84;

void SoundEntry::read(StreamReader &reader)
{
    reader.unpackFields("IIII", &Offset1, &Field2, &Field3, &ID);
    reader.unpackFields("ffff", &Pos.x, &Pos.y, &Pos.z, &Radius);
    reader.unpackFields("IIII", &Offset2, &Field10, &Duration, &Offset3);
    reader.unpackFields("IIHH", &SoundID1, &SoundID2, &Flags1, &Field16);
    reader.unpackFields("IIIIII", &Flags2, &Field18, &Field19, &Field20, &Field21, &Field22);
}

bool SoundEntry::fromFile(QVector<SoundEntry *> &entries, QString path)
{
    SoundEntry *e = NULL;
    QFile f(path);
    if(f.open(QFile::ReadOnly))
    {
        StreamReader reader(&f);
        while(!f.atEnd())
        {
            e = new SoundEntry();
            e->read(reader);
            entries.append(e);
        }
        return true;
    }
    return false;
}
