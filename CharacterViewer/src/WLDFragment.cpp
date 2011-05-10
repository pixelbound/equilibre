#include <QIODevice>
#include "WLDFragment.h"
#include "WLDData.h"

WLDFragment::WLDFragment(uint32_t kind, QString name, QByteArray data)
{
    m_kind = kind;
    m_name = name;
    m_data = data;
}

uint32_t WLDFragment::kind() const
{
    return m_kind;
}

QString WLDFragment::name() const
{
    return m_name;
}

QByteArray WLDFragment::data() const
{
    return m_data;
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
    return new WLDFragment(fh.kind, fragmentName, fragmentData);
    //frag = Fragment.decode(fragID, type, name, data)
    //frag.unpack(self)
}
