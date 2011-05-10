#ifndef OPENEQ_WLD_FRAGMENT_H
#define OPENEQ_WLD_FRAGMENT_H

#include <QObject>
#include <QByteArray>
#include "Platform.h"

class QIODevice;
class WLDData;

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
    WLDFragment(uint32_t kind, QString name, QByteArray data);
    static WLDFragment *fromStream(QIODevice *s, WLDData *wld);

    uint32_t kind() const;
    QString name() const;
    QByteArray data() const;

private:
    uint32_t m_kind;
    QString m_name;
    QByteArray m_data;
};

#endif

