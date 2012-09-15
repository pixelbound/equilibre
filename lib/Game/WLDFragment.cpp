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
