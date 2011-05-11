#ifndef OPENEQ_WLD_FRAGMENT_H
#define OPENEQ_WLD_FRAGMENT_H

#include <QObject>
#include <QByteArray>
#include "Platform.h"
#include "Vertex.h"

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

    //! \brief The fragment is a mesh.
    const static uint32_t MESH;

    virtual bool unpack(WLDData *wld);
    bool unpackField(char type, void *field);
    bool unpackFields(char *types, ...);
    bool unpackStruct(char *types, void *first);
    bool unpackArray(char *types, uint32_t count, void *first);
    uint32_t structSize(char *types) const;

private:
    static WLDFragment *createByKind(uint32_t kind, QString name, QByteArray data);
    static uint32_t fieldSize(char c);
    bool readUint16(uint16_t *dest);
    bool readInt16(int16_t *dest);
    bool readUint32(uint32_t *dest);
    bool readInt32(int32_t *dest);
    bool readFloat32(float *dest);
    uint32_t m_kind;
    QString m_name;
    QByteArray m_data;
    int m_pos;
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

/*!
  \brief This type of fragment describes a mesh.
  */
class Fragment36 : public WLDFragment
{
public:
    Fragment36(uint32_t kind, QString name, QByteArray data);
    virtual bool unpack(WLDData *wld);

    uint32_t m_flags;
    int32_t m_ref[4];
    vec3 m_center;
    uint32_t m_param2[3];
    float m_maxDist;
    vec3 m_min, m_max;
    uint16_t m_vertexCount, m_texCoordsCount, m_normalCount, m_colorCount, m_polyCount;
    uint16_t m_vertexPieceCount, m_polyTexCount, m_vertexTexCount, m_size9, m_scale;
};

#endif

