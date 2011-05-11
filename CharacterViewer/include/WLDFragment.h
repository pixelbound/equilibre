#ifndef OPENEQ_WLD_FRAGMENT_H
#define OPENEQ_WLD_FRAGMENT_H

#include <QObject>
#include <QByteArray>
#include "Platform.h"
#include "Vertex.h"

class QIODevice;
class WLDData;
class WLDFragmentStream;

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
  \brief Holds the content of a WLD fragment (e.g. texture, mesh, skeleton, etc).
  */
class WLDFragment
{
public:
    WLDFragment(uint32_t kind, QString name);
    static WLDFragment *fromStream(QIODevice *s, WLDData *wld);

    uint32_t kind() const;
    QString name() const;

    virtual bool unpack(WLDFragmentStream *s);

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

class WLDFragmentStream
{
public:
    WLDFragmentStream(QByteArray data, WLDData *wld);

    QByteArray data() const;
    WLDData *wld() const;

    bool unpackField(char type, void *field);
    bool unpackFields(char *types, ...);
    bool unpackStruct(char *types, void *first);
    bool unpackArray(char *types, uint32_t count, void *first);
    uint32_t structSize(char *types) const;

private:
    static uint32_t fieldSize(char c);
    bool readUint16(uint16_t *dest);
    bool readInt16(int16_t *dest);
    bool readUint32(uint32_t *dest);
    bool readInt32(int32_t *dest);
    bool readFloat32(float *dest);

    QByteArray m_data;
    int m_pos;
    WLDData *m_wld;
};

/*!
  \brief Refers to another fragment, either directly or through a name.
  */
class WLDFragmentRef
{
public:
    WLDFragmentRef(WLDFragment *f);
    WLDFragmentRef(QString name);

    WLDFragment *fragment() const;
    QString name() const;

private:
    WLDFragment *m_fragment;
    QString m_name;
};

#endif
