#include <QIODevice>
#include <QFile>
#include <QBuffer>
#include "OpenEQ/Game/WLDData.h"
#include "OpenEQ/Game/PFSArchive.h"

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

WLDData::WLDData(QObject *parent) : QObject(parent)
{
    m_stringData = 0;
}

WLDData::~WLDData()
{
    while(m_fragments.count() > 0)
        delete m_fragments.takeLast();
}

const QList<WLDFragment *> & WLDData::fragments() const
{
    return m_fragments;
}

WLDData *WLDData::fromFile(QString path, QObject *parent)
{
    QFile f(path);
    if(f.open(QFile::ReadOnly))
        return fromStream(&f, parent);
    return 0;
}

WLDData *WLDData::fromArchive(PFSArchive *a, QString name, QObject *parent)
{
    if(!a || !a->isOpen())
        return 0;
    QByteArray data = a->unpackFile(name);
    QBuffer buffer(&data);
    buffer.open(QBuffer::ReadOnly);
    return fromStream(&buffer, parent);
}

WLDData *WLDData::fromStream(QIODevice *s, QObject *parent)
{
    WLDData *wld = new WLDData(parent);
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

    // load fragments
    for(uint i = 0; i < h.fragmentCount; i++)
    {
        WLDFragment *f = WLDFragment::fromStream(&reader);
        if(!f)
            break;
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

QList<WLDFragment *> WLDData::fragmentsByType(uint32_t type) const
{
    QList<WLDFragment *> byType;
    foreach(WLDFragment *f, m_fragments)
        if(f->kind() == type)
            byType.append(f);
    return byType;
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