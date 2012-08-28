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

#include "EQuilibre/Game/WLDFragment.h"
#include "EQuilibre/Game/WLDData.h"
#include "EQuilibre/Game/Fragments.h"

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
    case MeshLightingDefFragment::ID:
        return new MeshLightingDefFragment(name);
    case MeshLightingFragment::ID:
        return new MeshLightingFragment(name);
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
