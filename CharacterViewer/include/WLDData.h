#ifndef OPENEQ_WLD_DATA_H
#define OPENEQ_WLD_DATA_H

#include <QObject>
#include <QList>
#include "Platform.h"

class QIODevice;
class WLDFragment;
class WLDFragmentRef;

/*!
  \brief Describes the header of a .wld file.
  */
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t fragmentCount;
    uint32_t header3;
    uint32_t header4;
    uint32_t stringDataSize;
    uint32_t header6;
} WLDHeader;

/*!
  \brief Holds the content of a .wld file (mostly a list of fragments such as
  textures, meshes, skeletons, etc).
  */
class WLDData : public QObject
{
public:
    WLDData(WLDHeader header, QObject *parent = 0);
    virtual ~WLDData();
    static WLDData *fromStream(QIODevice *s, QObject *parent = 0);
    static WLDData *fromFile(QString path, QObject *parent = 0);

    const QList<WLDFragment *> &fragments() const;
    QString lookupString(int start) const;
    static QByteArray decodeString(QByteArray data);
    WLDFragmentRef *lookupReference(int32_t ref) const;
    QList<WLDFragment *> fragmentsByType(uint32_t type) const;
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
    WLDHeader m_header;
    QByteArray m_stringData;
    QList<WLDFragment *> m_fragments;
};

#endif
