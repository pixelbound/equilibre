#ifndef OPENEQ_WLD_FRAGMENT_H
#define OPENEQ_WLD_FRAGMENT_H

#include <QObject>
#include "OpenEQ/Render/Platform.h"

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
class WLDFragment
{
public:
    WLDFragment(uint32_t kind, QString name);
    virtual ~WLDFragment();
    static WLDFragment *fromStream(WLDReader *s);

    uint32_t kind() const;
    QString name() const;

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
    static WLDFragment *createByKind(uint32_t kind, QString name);

    uint32_t m_kind;
    QString m_name;
};

/*!
  \brief Refers to another fragment, either directly or through a name.
  */
class WLDFragmentRef
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
