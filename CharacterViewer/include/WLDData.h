#ifndef OPENEQ_WLD_DATA_H
#define OPENEQ_WLD_DATA_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include "Platform.h"
#include "WLDFragment.h"
#include "StreamReader.h"

class QIODevice;
class PFSArchive;

/*!
  \brief Holds the content of a .wld file (mostly a list of fragments such as
  textures, meshes, skeletons, etc).
  */
class WLDData : public QObject
{
public:
    WLDData(QObject *parent = 0);
    virtual ~WLDData();
    static WLDData *fromStream(QIODevice *s, QObject *parent = 0);
    static WLDData *fromFile(QString path, QObject *parent = 0);
    static WLDData *fromArchive(PFSArchive *a, QString name, QObject *parent = 0);

    const QList<WLDFragment *> &fragments() const;
    QString lookupString(int start) const;
    static QByteArray decodeString(QByteArray data);
    WLDFragmentRef lookupReference(int32_t ref) const;
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

    template<typename T>
    QList<T *> fragmentsByType() const
    {
        QList<T *> byType;
        foreach(WLDFragment *f, m_fragments)
        {
            T * frag = f->cast<T>();
            if(frag)
                byType.append(frag);
        }
        return byType;
    }

private:
    QByteArray m_stringData;
    QList<WLDFragment *> m_fragments;
};

class WLDReader : public StreamReader
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
