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

WLDFragment::WLDFragment()
{
    m_kind = 0;
}

WLDFragment::WLDFragment(uint32_t kind)
{
    m_kind = kind;
}

WLDFragment::~WLDFragment()
{
}

uint32_t WLDFragment::kind() const
{
    return m_kind;
}

void WLDFragment::setKind(uint32_t newKind)
{
    m_kind = newKind;
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

#define CREATE_FRAGMENT_CASE(T) case T::ID: fragSize = sizeof(T); return new T[count];
#define DELETE_FRAGMENT_CASE(T) case T::ID: delete [] (T *)array; break;

WLDFragment * WLDFragment::createArray(uint32_t kind, uint32_t count, uint32_t &fragSize)
{
    if(count == 0)
    {
        fragSize = 0;
        return NULL;
    }
    switch(kind)
    {
    CREATE_FRAGMENT_CASE(BitmapNameFragment);
    CREATE_FRAGMENT_CASE(SpriteDefFragment);
    CREATE_FRAGMENT_CASE(SpriteFragment);
    CREATE_FRAGMENT_CASE(HierSpriteDefFragment);
    CREATE_FRAGMENT_CASE(HierSpriteFragment);
    CREATE_FRAGMENT_CASE(TrackDefFragment);
    CREATE_FRAGMENT_CASE(TrackFragment);
    CREATE_FRAGMENT_CASE(ActorDefFragment);
    CREATE_FRAGMENT_CASE(ActorFragment);
    CREATE_FRAGMENT_CASE(LightDefFragment);
    CREATE_FRAGMENT_CASE(LightFragment);
    CREATE_FRAGMENT_CASE(LightSourceFragment);
    CREATE_FRAGMENT_CASE(RegionLightFragment);
    CREATE_FRAGMENT_CASE(SpellParticleDefFragment);
    CREATE_FRAGMENT_CASE(SpellParticleFragment);
    CREATE_FRAGMENT_CASE(Fragment34);
    CREATE_FRAGMENT_CASE(MaterialDefFragment);
    CREATE_FRAGMENT_CASE(MaterialPaletteFragment);
    CREATE_FRAGMENT_CASE(MeshLightingDefFragment);
    CREATE_FRAGMENT_CASE(MeshLightingFragment);
    CREATE_FRAGMENT_CASE(MeshDefFragment);
    CREATE_FRAGMENT_CASE(MeshFragment);
    CREATE_FRAGMENT_CASE(RegionTreeFragment);
    CREATE_FRAGMENT_CASE(RegionFragment);
    default:
        fragSize = sizeof(WLDFragment);
        return new WLDFragment[count];
    }
}

void WLDFragment::deleteArray(uint32_t kind, WLDFragment *array)
{
    if(!array)
        return;
    switch(kind)
    {
    DELETE_FRAGMENT_CASE(BitmapNameFragment);
    DELETE_FRAGMENT_CASE(SpriteDefFragment);
    DELETE_FRAGMENT_CASE(SpriteFragment);
    DELETE_FRAGMENT_CASE(HierSpriteDefFragment);
    DELETE_FRAGMENT_CASE(HierSpriteFragment);
    DELETE_FRAGMENT_CASE(TrackDefFragment);
    DELETE_FRAGMENT_CASE(TrackFragment);
    DELETE_FRAGMENT_CASE(ActorDefFragment);
    DELETE_FRAGMENT_CASE(ActorFragment);
    DELETE_FRAGMENT_CASE(LightDefFragment);
    DELETE_FRAGMENT_CASE(LightFragment);
    DELETE_FRAGMENT_CASE(LightSourceFragment);
    DELETE_FRAGMENT_CASE(RegionLightFragment);
    DELETE_FRAGMENT_CASE(SpellParticleDefFragment);
    DELETE_FRAGMENT_CASE(SpellParticleFragment);
    DELETE_FRAGMENT_CASE(Fragment34);
    DELETE_FRAGMENT_CASE(MaterialDefFragment);
    DELETE_FRAGMENT_CASE(MaterialPaletteFragment);
    DELETE_FRAGMENT_CASE(MeshLightingDefFragment);
    DELETE_FRAGMENT_CASE(MeshLightingFragment);
    DELETE_FRAGMENT_CASE(MeshDefFragment);
    DELETE_FRAGMENT_CASE(MeshFragment);
    DELETE_FRAGMENT_CASE(RegionTreeFragment);
    DELETE_FRAGMENT_CASE(RegionFragment);
    default:
        delete [] array;
        break;
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
