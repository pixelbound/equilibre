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

#include <QIODevice>
#include <QFile>
#include <QBuffer>
#include "EQuilibre/Game/WLDData.h"
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Game/Fragments.h"

/*!
  \brief Describes the header of a .wld file.
  */
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t fragmentCount;
    uint32_t header3;
    uint32_t header4;
    uint32_t stringDataSize;
    uint32_t header6;
} WLDHeader;

WLDData::WLDData()
{
    m_stringData = 0;
    m_fragTable = NULL;
}

WLDData::~WLDData()
{
    m_fragments.clear();
    delete m_fragTable;
}

WLDFragmentTable * WLDData::table() const
{
    return m_fragTable;
}

const QList<WLDFragment *> & WLDData::fragments() const
{
    return m_fragments;
}

WLDData *WLDData::fromFile(QString path)
{
    QFile f(path);
    if(f.open(QFile::ReadOnly))
        return fromStream(&f);
    return 0;
}

WLDData *WLDData::fromArchive(PFSArchive *a, QString name)
{
    if(!a || !a->isOpen())
        return 0;
    QByteArray data = a->unpackFile(name);
    QBuffer buffer(&data);
    buffer.open(QBuffer::ReadOnly);
    return fromStream(&buffer);
}

WLDData *WLDData::fromStream(QIODevice *s)
{
    WLDData *wld = new WLDData();
    WLDReader reader(s, wld);
    WLDHeader h;

    // read header
    if(!reader.unpackStruct("IIIIIII", &h))
    {
        fprintf(stderr, "Incomplete header");
        delete wld;
        return 0;
    }

    // read string table
    if(!reader.readEncodedData(h.stringDataSize, &wld->m_stringData))
    {
        fprintf(stderr, "Incomplete string table");
        delete wld;
        return 0;
    }
    wld->m_fragTable = new WLDFragmentTable();
    
    // Count how many fragments of each kind there are.
    qint64 fragmentListStart = s->pos();
    WLDFragmentHeader fh;
    for(uint32_t i = 0; i < h.fragmentCount; i++)
    {
        qint64 fragmentStart = s->pos();
        WLDFragment::readHeader(&reader, fh, NULL);
        wld->m_fragTable->incrementFragmentCount(fh.kind);
        s->seek(fragmentStart + 8 + fh.size);
    }
    s->seek(fragmentListStart);
    
    // Load fragments.
    QString fragmentName;
    wld->m_fragTable->allocate();
    for(uint32_t i = 0; i < h.fragmentCount; i++)
    {
        qint64 fragmentStart = s->pos();
        WLDFragment::readHeader(&reader, fh, &fragmentName);
        WLDFragment *f = wld->m_fragTable->current(fh.kind);
        if(f)
        {
            f->setKind(fh.kind);
            f->setName(fragmentName);
            f->unpack(&reader);
            wld->m_fragTable->next(fh.kind);
        }
        s->seek(fragmentStart + 8 + fh.size);
        wld->m_fragments.append(f);
    }
    return wld;
}

QByteArray WLDData::decodeString(QByteArray data)
{
    static char key[] = {0x95, 0x3A, 0xC5, 0x2A, 0x95, 0x7A, 0x95, 0x6A};
    QByteArray decoded(data.size(), '\0');
    for(int i = 0; i < data.size(); i++)
        decoded[i] = data[i] ^ key[i % sizeof(key)];
    return decoded;
}

QString WLDData::lookupString(int start) const
{
    int len = m_stringData.length();
    if(len == 0)
        return QString::null;
    else if((start >= 0) && (start < len))
    {
        // find null character at the end of the string
        int size = 0;
        for(int i = start; i < len; i++)
        {
            if(m_stringData[i] == 0)
            {
                size = i - start;
                break;
            }
        }
        if(size >= 0)
            return QString::fromLatin1(m_stringData.constData() + start, size);
    }
    return QString::null;
}

WLDFragmentRef WLDData::lookupReference(int32_t ref) const
{
    if(ref < 0)
    {
        // reference by name
        return WLDFragmentRef(lookupString(-ref));
    }
    else if((ref > 0) && (ref <= m_fragments.size()))
    {
        // reference by index
        return WLDFragmentRef(m_fragments[ref - 1]);
    }
    else
        return WLDFragmentRef();
}

WLDFragment * WLDData::findFragment(uint32_t type, QString name) const
{
    foreach(WLDFragment *f, m_fragments)
        if((f->kind() == type) && (f->name() == name))
            return f;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

WLDReader::WLDReader(QIODevice *stream, WLDData *wld) : StreamReader(stream)
{
    m_wld = wld;
}

WLDData *WLDReader::wld() const
{
    return m_wld;
}

void WLDReader::setWld(WLDData *wld)
{
    m_wld = wld;
}

bool WLDReader::unpackField(char type, void *field)
{
    switch(type)
    {
    case 'R':
        return readReference((WLDFragmentRef *)field);
    case 'r':
        return readFragmentReference((WLDFragment **)field);
    }
    return StreamReader::unpackField(type, field);
}

bool WLDReader::readReference(WLDFragmentRef *dest)
{
    int32_t encoded;
    if(!readInt32(&encoded) || !m_wld)
        return false;
    *dest = m_wld->lookupReference(encoded);
    return true;
}

bool WLDReader::readFragmentReference(WLDFragment **dest)
{
    int32_t encoded;
    if(!readInt32(&encoded) || !m_wld)
        return false;
    *dest = m_wld->lookupReference(encoded).fragment();
    return true;
}

uint32_t WLDReader::fieldSize(char c) const
{
    switch(c)
    {
    case 'R':
        return sizeof(WLDFragmentRef);
    case 'r':
        return sizeof(WLDFragment *);
    default:
        return StreamReader::fieldSize(c);
    }
}

bool WLDReader::readEncodedData(uint32_t size, QByteArray *dest)
{
    QByteArray data = m_stream->read(size);
    if(((uint32_t)data.length() < size) || !m_wld)
        return false;
    *dest = m_wld->decodeString(data);
    return true;
}

bool WLDReader::readEncodedString(uint32_t size, QString *dest)
{
    QByteArray data;
    if(!readEncodedData(size, &data))
        return false;
    *dest = QString(data);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

WLDFragment::WLDFragment()
{
    m_kind = m_info = 0;
}

WLDFragment::~WLDFragment()
{
}

uint16_t WLDFragment::kind() const
{
    return m_kind;
}

void WLDFragment::setKind(uint16_t newKind)
{
    m_kind = newKind;
}

bool WLDFragment::handled() const
{
    return m_info & 1;
}

void WLDFragment::setHandled(bool newHandled)
{
    m_info = (m_info & ~1) | (newHandled & 1);
}

QString WLDFragment::name() const
{
    return m_name;
}

void WLDFragment::setName(QString newName)
{
    m_name = newName;
}

bool WLDFragment::readHeader(WLDReader *sr, WLDFragmentHeader &fh, QString *name)
{
    if(!sr->unpackStruct("IIi", &fh))
        return false;
    if(name)
        *name = (fh.nameRef < 0) ? sr->wld()->lookupString(-fh.nameRef) : QString::null;
    return true;
}

bool WLDFragment::unpack(WLDReader *s)
{
    (void)s;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

WLDFragmentRef::WLDFragmentRef()
{
    m_name = QString::null;
    m_fragment = 0;
}

WLDFragmentRef::WLDFragmentRef(WLDFragment *f)
{
    m_name = QString::null;
    m_fragment = f;
}

WLDFragmentRef::WLDFragmentRef(QString name)
{
    m_name = name;
    m_fragment = 0;
}

WLDFragment *WLDFragmentRef::fragment() const
{
    return m_fragment;
}

QString WLDFragmentRef::name() const
{
    return m_name;
}
