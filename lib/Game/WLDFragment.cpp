#include "OpenEQ/Game/WLDFragment.h"
#include "OpenEQ/Game/WLDData.h"
#include "OpenEQ/Game/Fragments.h"

WLDFragment::WLDFragment(uint32_t kind, QString name)
{
    m_kind = kind;
    m_name = name;
}

WLDFragment::~WLDFragment()
{
}

uint32_t WLDFragment::kind() const
{
    return m_kind;
}

QString WLDFragment::name() const
{
    return m_name;
}

WLDFragment *WLDFragment::fromStream(WLDReader *sr)
{
    WLDFragmentHeader fh;
    QString fragmentName;
    qint64 pos = sr->stream()->pos();

    // read fragment header
    if(!sr->unpackStruct("IIi", &fh))
        return 0;
    else if(fh.nameRef < 0)
        fragmentName = sr->wld()->lookupString(-fh.nameRef);
    else
        fragmentName = QString::null;

    // unpack fragment contents
    WLDFragment *f = createByKind(fh.kind, fragmentName);
    if(f)
        f->unpack(sr);

    // skip to next fragment
    sr->stream()->seek(pos + 8 + fh.size);
    return f;
}

bool WLDFragment::unpack(WLDReader *s)
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
    case HierSpriteDefFragment::ID:
        return new HierSpriteDefFragment(name);
    case HierSpriteFragment::ID:
        return new HierSpriteFragment(name);
    case TrackDefFragment::ID:
        return new TrackDefFragment(name);
    case TrackFragment::ID:
        return new TrackFragment(name);
    case ActorDefFragment::ID:
        return new ActorDefFragment(name);
    case ActorFragment::ID:
        return new ActorFragment(name);
    case SpellParticleDefFragment::ID:
        return new SpellParticleDefFragment(name);
    case SpellParticleFragment::ID:
        return new SpellParticleFragment(name);
    case Fragment34::ID:
        return new Fragment34(name);
    case MaterialDefFragment::ID:
        return new MaterialDefFragment(name);
    case MaterialPaletteFragment::ID:
        return new MaterialPaletteFragment(name);
    case MeshDefFragment::ID:
        return new MeshDefFragment(name);
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
