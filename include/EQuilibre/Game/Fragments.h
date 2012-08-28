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

#ifndef EQUILIBRE_FRAGMENTS_H
#define EQUILIBRE_FRAGMENTS_H

#include <QString>
#include <QVector>
#include <QColor>
#include <QPair>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"
#include "EQuilibre/Game/WLDFragment.h"
#include "EQuilibre/Game/WLDSkeleton.h"

typedef QPair<uint16_t, uint16_t> vec2us;

class TrackFragment;
class MeshFragment;
class MeshLightingFragment;

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
  \brief This type of fragment (0x10) defines hierarchical meshes (i.e. skeleton).
  */
class HierSpriteDefFragment : public WLDFragment
{
public:
    HierSpriteDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x10;
    uint32_t m_flags;
    WLDFragment *m_fragment;
    uint32_t m_param1[3];
    float m_param2;
    QVector<SkeletonNode> m_tree;
    QVector<MeshFragment *> m_meshes;
    QVector<uint32_t> m_data3;
};

/*!
  \brief This type of fragment (0x11) defines instances of hierarchical meshes (fragment 0x10).
  */
class HierSpriteFragment : public WLDFragment
{
public:
    HierSpriteFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x11;
    HierSpriteDefFragment *m_def;
    uint32_t m_flags;
};

/*!
  \brief This type of fragment (0x12) defines animation tracks (set of frames
  each containing animation parameters relative to a skeleton piece).
  */
class TrackDefFragment : public WLDFragment
{
public:
    TrackDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    BoneTransform frame(uint32_t frameIndex = 0) const;

    const static uint32_t ID = 0x12;
    uint32_t m_flags;
    QVector<BoneTransform> m_frames;
};

/*!
  \brief This type of fragment (0x13) defines instances of animation tracks (fragment 0x12).
  */
class TrackFragment : public WLDFragment
{
public:
    TrackFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x13;
    TrackDefFragment *m_def;
    uint32_t m_flags, m_param1;
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
    MeshLightingFragment *m_lighting;
};

/*!
  \brief This type of fragment (0x1B) defines light sources.
  */
class LightDefFragment : public WLDFragment
{
public:
    LightDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x1B;
    uint32_t m_flags;
    uint32_t m_params2;
    uint32_t m_attenuation;
    vec4 m_color;
};

/*!
  \brief This type of fragment (0x1C) defines instances of light sources (fragment 0x1B).
  */
class LightFragment : public WLDFragment
{
public:
    LightFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x1C;
    LightDefFragment *m_def;
    uint32_t m_flags;
};

/*!
  \brief This type of fragment (0x28) defines placeable light sources.
  */
class LightSourceFragment : public WLDFragment
{
public:
    LightSourceFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x28;
    LightFragment *m_ref;
    uint32_t m_flags;
    vec3 m_pos;
    float m_radius;
};

/*!
  \brief This type of fragment (0x26) defines spell particle bolts.
  */
class SpellParticleDefFragment : public WLDFragment
{
public:
    SpellParticleDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x26;
    uint32_t m_flags;
    SpriteFragment *m_sprite;
    uint32_t m_param1;
};

/*!
  \brief This type of fragment (0x27) defines instances of spell particle bolts.
  */
class SpellParticleFragment : public WLDFragment
{
public:
    SpellParticleFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x27;
    SpellParticleDefFragment *m_def;
    uint32_t m_flags;
};

class Fragment34Data
{
public:
    uint32_t param1, param2, param3, param4, param5, param6;
    float param7, param8;
    uint32_t param9;
    float param10, param11, param12, param13;
    uint32_t param14;
    float param15, param16;
};

/*!
  \brief This type of fragment (0x34) has something to do with weapon particles.
  */
class Fragment34 : public WLDFragment
{
public:
    Fragment34(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x34;
    uint32_t m_param0, m_param1, m_param2, m_flags;
    Fragment34Data m_data3;
    SpellParticleDefFragment *m_particle;
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
  \brief This type of fragment (0x32) defines a mesh's lighting.
  */
class MeshLightingDefFragment : public WLDFragment
{
public:
    MeshLightingDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x32;
    uint32_t m_data1, m_size1, m_data2, m_data3, m_data4;
    QVector<QRgb> m_colors;
};

/*!
  \brief This type of fragment (0x33) defines instances of mesh lighting (fragment 0x32).
  */
class MeshLightingFragment : public WLDFragment
{
public:
    MeshLightingFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x33;
    uint32_t m_flags;
    MeshLightingDefFragment *m_def;
};

/*!
  \brief This type of fragment (0x36) defines meshes.
  */
class MeshDefFragment : public WLDFragment
{
public:
    MeshDefFragment(QString name);
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x36;
    uint32_t m_flags;
    MaterialPaletteFragment *m_palette;
    WLDFragmentRef m_ref[3];
    vec3 m_center;
    uint32_t m_param2[3];
    float m_maxDist;
    AABox m_boundsAA;
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

