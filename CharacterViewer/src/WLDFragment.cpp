#include <QIODevice>
#include <stdarg.h>
#include "WLDFragment.h"
#include "WLDData.h"
#include "Fragments.h"

WLDFragment::WLDFragment(uint32_t kind, QString name)
{
    m_kind = kind;
    m_name = name;
}

uint32_t WLDFragment::kind() const
{
    return m_kind;
}

QString WLDFragment::name() const
{
    return m_name;
}

WLDFragment *WLDFragment::fromStream(QIODevice *s, WLDData *wld)
{
    WLDFragmentHeader fh;
    QString fragmentName;
    QByteArray fragmentData;

    //XXX: fix endianness issues
    qint64 read = s->read((char *)&fh, sizeof(WLDFragmentHeader));
    if(read < (qint64)sizeof(WLDFragmentHeader))
        return 0;
    else if(fh.nameRef < 0)
        fragmentName = wld->lookupString(-fh.nameRef);
    else
        fragmentName = QString::null;
    fragmentData = s->read(fh.size - 4);
    if((fragmentData.length() + 4) < fh.size)
        return 0;
    WLDFragment *f = createByKind(fh.kind, fragmentName);
    if(f)
    {
        WLDFragmentStream s(fragmentData, wld);
        f->unpack(&s);
    }
    return f;
}

bool WLDFragment::unpack(WLDFragmentStream *s)
{
    (void)s;
    return true;
}

WLDFragment *WLDFragment::createByKind(uint32_t kind, QString name)
{
    switch(kind)
    {
    case BitmapNameFragment::ID:
        return new BitmapNameFragment(name);
    case SpriteDefFragment::ID:
        return new SpriteDefFragment(name);
    case SpriteFragment::ID:
        return new SpriteFragment(name);
    case MaterialDefFragment::ID:
        return new MaterialDefFragment(name);
    case MaterialPaletteFragment::ID:
        return new MaterialPaletteFragment(name);
    case MeshFragment::ID:
        return new MeshFragment(name);
    default:
        return new WLDFragment(kind, name);
    }
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

////////////////////////////////////////////////////////////////////////////////

WLDFragmentStream::WLDFragmentStream(QByteArray data, WLDData *wld)
{
    m_data = data;
    m_pos = 0;
    m_wld = wld;
}

QByteArray WLDFragmentStream::data() const
{
    return m_data;
}

WLDData *WLDFragmentStream::wld() const
{
    return m_wld;
}

bool WLDFragmentStream::unpackField(char type, void *field)
{
    switch(type)
    {
    case 'I':
        return readUint32((uint32_t *)field);
    case 'i':
        return readInt32((int32_t *)field);
    case 'R':
        return readReference((WLDFragmentRef *)field);
    case 'r':
        return readFragmentReference((WLDFragment **)field);
    case 'H':
        return readUint16((uint16_t *)field);
    case 'h':
        return readInt16((int16_t *)field);
    case 'B':
        return readUint8((uint8_t *)field);
    case 'b':
        return readInt8((int8_t *)field);
    case 'f':
        return readFloat32((float *)field);
    default:
        return false;
    }
}

bool WLDFragmentStream::unpackFields(const char *types, ...)
{
    va_list args;
    va_start(args, types);
    while(*types)
    {
        void *dest = va_arg(args, void *);
        if(!unpackField(*types, dest))
            return false;
        types++;
    }
    va_end(args);
    return true;
}

bool WLDFragmentStream::unpackStruct(const char *types, void *first)
{
    uint8_t *dest = (uint8_t *)first;
    while(*types)
    {
        if(!unpackField(*types, dest))
            return false;
        dest += fieldSize(*types);
        types++;
    }
    return true;
}

bool WLDFragmentStream::unpackArray(const char *types, uint32_t count, void *first)
{
    uint8_t *dest = (uint8_t *)first;
    uint32_t size = structSize(types);
    for(uint32_t i = 0; i < count; i++)
    {
        if(!unpackStruct(types, dest))
            return false;
        dest += size;
    }
    return true;
}

uint32_t WLDFragmentStream::structSize(const char *types) const
{
    uint32_t s = 0;
    while(*types)
    {
        s += fieldSize(*types);
        types++;
    }
    return s;
}

bool WLDFragmentStream::readInt8(int8_t *dest)
{
    if((m_pos + 1) > m_data.length())
        return false;
    const uint8_t *src = (const uint8_t *)m_data.constData() + m_pos;
    *dest = src[0];
    m_pos += 1;
    return true;
}

bool WLDFragmentStream::readUint8(uint8_t *dest)
{
    if((m_pos + 1) > m_data.length())
        return false;
    const uint8_t *src = (const uint8_t *)m_data.constData() + m_pos;
    *dest = src[0];
    m_pos += 1;
    return true;
}

bool WLDFragmentStream::readInt16(int16_t *dest)
{
    if((m_pos + 2) > m_data.length())
        return false;
    const uint8_t *src = (const uint8_t *)m_data.constData() + m_pos;
    *dest = (src[1] << 8) | src[0];
    m_pos += 2;
    return true;
}

bool WLDFragmentStream::readUint16(uint16_t *dest)
{
    if((m_pos + 2) > m_data.length())
        return false;
    const uint8_t *src = (const uint8_t *)m_data.constData() + m_pos;
    *dest = (src[1] << 8) | src[0];
    m_pos += 2;
    return true;
}

bool WLDFragmentStream::readInt32(int32_t *dest)
{
    if((m_pos + 4) > m_data.length())
        return false;
    const uint8_t *src = (const uint8_t *)m_data.constData() + m_pos;
    *dest = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
    m_pos += 4;
    return true;
}

bool WLDFragmentStream::readUint32(uint32_t *dest)
{
    if((m_pos + 4) > m_data.length())
        return false;
    const uint8_t *src = (const uint8_t *)m_data.constData() + m_pos;
    *dest = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
    m_pos += 4;
    return true;
}

bool WLDFragmentStream::readFloat32(float *dest)
{
    if((m_pos + 4) > m_data.length())
        return false;
    const uint8_t *src = (const uint8_t *)m_data.constData() + m_pos;
    *dest = *((float *)src);
    m_pos += 4;
    return true;
}

bool WLDFragmentStream::readReference(WLDFragmentRef *dest)
{
    int32_t encoded;
    if(!readInt32(&encoded))
        return false;
    *dest = m_wld->lookupReference(encoded);
    return true;
}

bool WLDFragmentStream::readFragmentReference(WLDFragment **dest)
{
    int32_t encoded;
    if(!readInt32(&encoded))
        return false;
    *dest = m_wld->lookupReference(encoded).fragment();
    return true;
}

uint32_t WLDFragmentStream::fieldSize(char c)
{
    switch(c)
    {
    case 'I':
    case 'i':
    case 'f':
        return 4;
    case 'H':
    case 'h':
        return 2;
    case 'B':
    case 'b':
        return 1;
    case 'R':
        return sizeof(WLDFragmentRef);
    case 'r':
        return sizeof(WLDFragment *);
    default:
        return 0;
    }
}

bool WLDFragmentStream::readEncodedString(uint32_t size, QString *dest)
{
    if((m_pos + size) > m_data.length())
        return false;
    QByteArray decoded = m_wld->decodeString(m_data.mid(m_pos, size));
    *dest = QString::fromLatin1(decoded);
    m_pos += size;
    return true;
}
