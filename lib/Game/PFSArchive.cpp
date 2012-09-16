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

#include <stdio.h>
#include <zlib.h>
#include <QFile>
#include <QBuffer>
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Game/StreamReader.h"

class PFSHeader
{
public:
    uint32_t directoryOffset;
    char magic[4];
    uint32_t unknownSize;
};

class PFSFooter
{
public:
    char magic[4];
    uint32_t timestamp;
};

static bool compareEntries(PFSEntry a, PFSEntry b)
{
    return a.dataOffset < b.dataOffset;
}

PFSArchive::PFSArchive(QString path)
{
    m_file = 0;
    m_reader = 0;
    openArchive(path);
}

PFSArchive::~PFSArchive()
{
    close();
}

bool PFSArchive::isOpen() const
{
    return m_file && m_file->isOpen();
}

void PFSArchive::close()
{
    if(m_reader)
    {
        delete m_reader;
        m_reader = 0;
    }
    if(m_file)
    {
        m_file->close();
        delete m_file;
        m_file = 0;
    }
}

const QList<QString> & PFSArchive::files() const
{
    return m_fileNames;
}

void PFSArchive::openArchive(QString path)
{
    m_file = new QFile(path);
    if(!m_file->open(QFile::ReadOnly))
    {
        close();
        return;
    }
    m_reader = new StreamReader(m_file);

    // read the file header, entry list and optional file footer
    PFSHeader header;
    PFSEntry dir;
    QList<PFSEntry> entries;
    PFSFooter footer;

    m_reader->unpackStruct("IbbbbI", &header);
    if(QString::fromAscii(header.magic, 4) != "PFS ")
    {
        fprintf(stderr, "The file '%s' is not a PFS archive", path.toLatin1().constData());
        close();
        return;
    }
    m_file->seek(header.directoryOffset);
    if(!readEntries(dir, entries))
    {
        fprintf(stderr, "Could not read entries");
        close();
        return;
    }
    m_reader->unpackStruct("bbbbI", &footer);

    // extract the file name list
    QByteArray listData = unpackFileEntry(dir);
    QBuffer listBuffer(&listData);
    StreamReader listReader(&listBuffer);
    listBuffer.open(QBuffer::ReadOnly);
    if(!unpackFileList(&listReader, m_fileNames))
    {
        fprintf(stderr, "Could not read file name list");
        close();
        return;
    }

    // map each file name to an entry
    qSort(entries.begin(), entries.end(), compareEntries);
    int count = std::min(m_fileNames.count(), entries.count());
    for(int i = 0; i < count; i++)
        m_entries.insert(m_fileNames[i], entries[i]);
}

bool PFSArchive::readEntries(PFSEntry &dir, QList<PFSEntry> &entries)
{
    if(!m_reader)
        return false;
    uint32_t entryCount;
    bool dirFound = false;
    PFSEntry e;
    m_reader->unpackField('I', &entryCount);
    for(uint32_t i = 0; i < entryCount; i++)
    {
        m_reader->unpackStruct("III", &e);
        if(e.crc == DIRECTORY_CRC)
        {
            dir = e;
            dirFound = true;
        }
        else
        {
            entries.append(e);
        }
    }
    return dirFound;
}

QByteArray PFSArchive::unpackFileEntry(PFSEntry e)
{
    uint32_t read = 0, deflatedSize = 0, inflatedSize = 0;
    QByteArray data(e.inflatedSize, '\0'), deflatedData;
    uint8_t *d = (uint8_t *)data.data();

    m_file->seek(e.dataOffset);
    while(read < e.inflatedSize)
    {
        m_reader->unpackFields("II", &deflatedSize, &inflatedSize);
        deflatedData = m_file->read(deflatedSize);

        z_stream zs;
        int status;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.next_in = (uint8_t *)deflatedData.data();
        zs.avail_in = deflatedSize;
        zs.next_out = d;
        zs.avail_out = inflatedSize;

        inflateInit(&zs);
        status = inflate(&zs, Z_FINISH);
        if(status != Z_STREAM_END)
            break;
        inflateEnd(&zs);

        read += inflatedSize;
        d += inflatedSize;
    }
    return data;
}

bool PFSArchive::unpackFileList(StreamReader *sr, QList<QString> &names)
{
    if(!sr)
        return false;
    uint32_t fileCount, nameSize;
    QString name;
    if(!sr->unpackField('I', &fileCount))
        return false;
    for(uint32_t i = 0; i < fileCount; i++)
    {
        if(!sr->unpackField('I', &nameSize))
            return false;
        if(!sr->readString(nameSize, &name))
            return false;
        names.append(name);
    }
    return true;
}

QByteArray PFSArchive::unpackFile(QString name)
{
    QMap<QString, PFSEntry>::const_iterator i = m_entries.find(name);
    if(i == m_entries.constEnd())
        return QByteArray();
    return unpackFileEntry(*i);
}
