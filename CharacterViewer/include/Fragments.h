#ifndef OPENEQ_FRAGMENTS_H
#define OPENEQ_FRAGMENTS_H

#include <QString>
#include <QVector>
#include <QColor>
#include <QPair>
#include "Platform.h"
#include "Vertex.h"
#include "WLDFragment.h"

typedef QPair<uint16_t, uint16_t> vec2us;

/*!
  \brief This type of fragment (0x03) holds the name of a texture bitmap.
  */
class BitmapNameFragment : public WLDFragment
{
public:
    BitmapNameFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x03;
    uint32_t m_flags;
    QString m_fileName;
};

/*!
  \brief This type of fragment (0x04) defines sprites (texture) which can have
    multiple bitmaps (e.g. animated sprite).
  */
class SpriteDefFragment : public WLDFragment
{
public:
    SpriteDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x04;
    uint32_t m_flags, m_param1, m_param2;
    QVector<BitmapNameFragment *> m_bitmaps;
};

/*!
  \brief This type of fragment (0x05) defines instances of a sprite (fragment 0x04).
  */
class SpriteFragment : public WLDFragment
{
public:
    SpriteFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x05;
    SpriteDefFragment *m_def;
    uint32_t m_flags;
};

/*!
  \brief This type of fragment (0x14) defines actors (e.g. placeable objects or characters).
  */
class ActorDefFragment : public WLDFragment
{
public:
    ActorDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x14;
    uint32_t m_flags;
    WLDFragment *m_fragment1, *m_fragment2;
    QList< QVector<WLDPair> > m_entries;
    QList<WLDFragment *> m_models;
};

/*!
  \brief This type of fragment (0x15) describes actor instances (fragment 0x14).
  */
class ActorFragment : public WLDFragment
{
public:
    ActorFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x15;
    WLDFragmentRef m_def;
    uint32_t m_flags;
    WLDFragment *m_fragment1;
    vec3 m_location;
    vec3 m_rotation;
    float m_param1;
    vec3 m_scale;
    WLDFragment *m_fragment2;
};

/*!
  \brief This type of fragment (0x30) defines materials (i.e. how to render part of a mesh).
  */
class MaterialDefFragment : public WLDFragment
{
public:
    MaterialDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x30;
    uint32_t m_flags, m_param1, m_param2;
    float m_brightness, m_scaledAmbient;
    SpriteFragment *m_sprite;
    float m_param3;
};

/*!
  \brief This type of fragment (0x31) defines palettes of materials.
  */
class MaterialPaletteFragment : public WLDFragment
{
public:
    MaterialPaletteFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x31;
    uint32_t m_flags;
    QVector<MaterialDefFragment *> m_materials;
};

/*!
  \brief This type of fragment (0x36) defines meshes.
  */
class MeshDefFragment : public WLDFragment
{
public:
    MeshDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    VertexGroup *toGroup() const;

    const static uint32_t ID = 0x36;
    uint32_t m_flags;
    MaterialPaletteFragment *m_palette;
    WLDFragmentRef m_ref[3];
    vec3 m_center;
    uint32_t m_param2[3];
    float m_maxDist;
    vec3 m_min, m_max;
    uint16_t m_size9;
    QVector<vec3> m_vertices;
    QVector<vec2> m_texCoords;
    QVector<vec3> m_normals;
    QVector<QRgb> m_colors;
    QVector<uint16_t> m_indices;
    QVector<vec2us> m_vertexPieces;
    QVector<vec2us> m_polygonsByTex;
    QVector<vec2us> m_verticesByTex;
};

/*!
  \brief This type of fragment (0x2D) describes instances of meshes (fragment 0x36).
  */
class MeshFragment : public WLDFragment
{
public:
    MeshFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x2D;
    MeshDefFragment *m_def;
    uint32_t m_flags;
};

#endif

