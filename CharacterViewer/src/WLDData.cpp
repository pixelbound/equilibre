#include <QIODevice>
#include <QFile>
#include "WLDData.h"
#include "WLDFragment.h"

WLDData::WLDData(WLDHeader header, QObject *parent) : QObject(parent)
{
    m_header = header;
    m_stringData = 0;
}

WLDData::~WLDData()
{
    while(m_fragments.count() > 0)
        delete m_fragments.takeLast();
}

const QList<WLDFragment *> &WLDData::fragments() const
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

WLDData *WLDData::fromStream(QIODevice *s, QObject *parent)
{
    WLDHeader h;
    qint64 read = 0;
    QByteArray stringData;

    // read header
    // XXX: fix endianness issues
    read = s->read((char *)&h, sizeof(WLDHeader));
    if(read < (qint64)sizeof(WLDHeader))
    {
        fprintf(stderr, "Incomplete header");
        return 0;
    }
    WLDData *wld = new WLDData(h, parent);

    // read string table
    stringData = s->read(h.stringDataSize);
    if((uint32_t)stringData.length() < h.stringDataSize)
    {
        fprintf(stderr, "Incomplete string table");
        delete wld;
        return 0;
    }
    wld->m_stringData = decodeString(stringData);

    // load fragments
    for(uint i = 0; i < h.fragmentCount; i++)
    {
        WLDFragment *f = WLDFragment::fromStream(s, wld);
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

WLDFragmentRef *WLDData::lookupReference(int32_t ref) const
{
    if(ref < 0)
    {
        // reference by name
        return new WLDFragmentRef(lookupString(-ref));
    }
    else if((ref > 0) && (ref <= m_fragments.size()))
    {
        // reference by index
        return new WLDFragmentRef(m_fragments[ref - 1]);
    }
    else
        return 0;
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
