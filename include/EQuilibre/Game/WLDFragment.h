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

#ifndef EQUILIBRE_WLD_FRAGMENT_H
#define EQUILIBRE_WLD_FRAGMENT_H

#include <QObject>
#include "EQuilibre/Render/Platform.h"

class WLDData;
class WLDReader;
class WLDFragmentRef;

/*!
  \brief Describes the header of fragment contained in a .wld file.
  */
typedef struct
{
    uint32_t size;
    uint32_t kind;
    int32_t nameRef;
} WLDFragmentHeader;

/*!
  \brief Data type found in WLD files that serve an unknown purpose.
  */
class WLDPair
{
public:
    uint32_t first;
    float second;
};

/*!
  \brief Holds the content of a WLD fragment (e.g. texture, mesh, skeleton, etc).
  */
class GAME_DLL WLDFragment
{
public:
    WLDFragment();
    WLDFragment(uint32_t kind);
    virtual ~WLDFragment();
    static WLDFragment *fromStream(WLDReader *s);

    uint32_t kind() const;
    void setKind(uint32_t newKind);
    
    QString name() const;
    void setName(QString newName);

    virtual bool unpack(WLDReader *s);

    template<typename T>
    T * cast()
    {
        if(m_kind == T::ID)
            return static_cast<T *>(this);
        else
            return 0;
    }

private:
    static WLDFragment *createByKind(uint32_t kind);

    uint32_t m_kind;
    QString m_name;
};

/*!
  \brief Refers to another fragment, either directly or through a name.
  */
class GAME_DLL WLDFragmentRef
{
public:
    WLDFragmentRef();
    WLDFragmentRef(WLDFragment *f);
    WLDFragmentRef(QString name);

    WLDFragment *fragment() const;
    QString name() const;

private:
    WLDFragment *m_fragment;
    QString m_name;
};

#endif
