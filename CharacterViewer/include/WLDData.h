#ifndef OPENEQ_WLD_DATA_H
#define OPENEQ_WLD_DATA_H

#include <QObject>
#include <QList>
#include "Platform.h"

class QIODevice;
class WLDFragment;

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

private:
    WLDHeader m_header;
    QByteArray m_stringData;
    QList<WLDFragment *> m_fragments;
};

#endif
