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

#ifndef EQUILIBRE_WLD_DATA_H
#define EQUILIBRE_WLD_DATA_H

#include <QList>
#include <QByteArray>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Game/StreamReader.h"

class QIODevice;
class PFSArchive;
class WLDFragment;
class WLDFragmentRef;
class WLDFragmentTable;
class WLDReader;

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
  \brief Holds the content of a .wld file (mostly a list of fragments such as
  textures, meshes, skeletons, etc).
  */
class GAME_DLL WLDData
{
public:
    WLDData();
    virtual ~WLDData();
    static WLDData *fromStream(QIODevice *s);
    static WLDData *fromFile(QString path);
    static WLDData *fromArchive(PFSArchive *a, QString name);

    WLDFragmentTable *table() const;
    const QList<WLDFragment *> &fragments() const;
    QString lookupString(int start) const;
    static QByteArray decodeString(QByteArray data);
    WLDFragmentRef lookupReference(int32_t ref) const;
    WLDFragment * findFragment(uint32_t type, QString name) const;

    template<typename T>
    T * findFragment(QString name) const
    {
        WLDFragment *f = findFragment(T::ID, name);
        if(f)
            return static_cast<T *>(f);
        else
            return 0;
    }

private:
    static const int MAX_FRAGMENT_KINDS = 0x40;
    QByteArray m_stringData;
    WLDFragmentTable *m_fragTable;
    QList<WLDFragment *> m_fragments;
};

/*!
  \brief Holds the content of a WLD fragment (e.g.\ texture, mesh, skeleton, etc).
  */
class GAME_DLL WLDFragment
{
public:
    WLDFragment();
    virtual ~WLDFragment();
    
    static bool readHeader(WLDReader *sr, WLDFragmentHeader &fh, QString *name);

    uint16_t kind() const;
    void setKind(uint16_t newKind);
    
    uint32_t ID() const;
    void setID(uint32_t newID);
    
    bool handled() const;
    void setHandled(bool newHandled);
    
    QString name() const;
    void setName(QString newName);

    virtual bool unpack(WLDReader *s);

    template<typename T>
    T * cast()
    {
        if(m_kind == T::KIND)
            return static_cast<T *>(this);
        else
            return 0;
    }

private:
    uint16_t m_kind;
    uint16_t m_info;
    uint32_t m_id;
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

class GAME_DLL WLDReader : public StreamReader
{
public:
    WLDReader(QIODevice *stream, WLDData *wld);

    WLDData *wld() const;
    void setWld(WLDData *wld);

    virtual bool unpackField(char type, void *field);
    bool readEncodedData(uint32_t size, QByteArray *dest);
    bool readEncodedString(uint32_t size, QString *dest);

    template<typename T>
    bool unpackReference(T **ref)
    {
        WLDFragment *frag;
        if(!unpackField('r', &frag))
            return false;
        else if(!frag)
            *ref = 0;
        else
            *ref = frag->cast<T>();
        return true;
    }

protected:
    uint32_t fieldSize(char c) const;

private:
    bool readReference(WLDFragmentRef *dest);
    bool readFragmentReference(WLDFragment **dest);

    WLDData *m_wld;
};

#endif
