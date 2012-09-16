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
#include "EQuilibre/Game/WLDData.h"
#include "EQuilibre/Game/WLDSkeleton.h"

typedef QPair<uint16_t, uint16_t> vec2us;

class TrackFragment;
class MeshFragment;
class MeshLightingFragment;

/*!
  \brief A fragment array contains the data for several fragments of the same kind.
  */
template<typename T>
class WLDFragmentArray
{
public:
    WLDFragmentArray()
    {
        m_data = NULL;
        m_count = 0;
        m_fragSize = 0;
    }
    
    WLDFragmentArray(T *data, uint32_t count, uint32_t fragSize)
    {
        m_data = data;
        m_count = count;
        m_fragSize = fragSize;
    }
    
    uint32_t count() const
    {
        return m_count;
    }
    
    T* operator[] (uint32_t index)
    {
        if(!m_data || (index >= m_count) || (m_fragSize == 0))
            return NULL;
        return (T *)((uint8_t *)m_data + (m_fragSize * index));
    }

private:
    T *m_data;
    uint32_t m_count;
    uint32_t m_fragSize;
};

/*!
  \brief A fragment table contains arrays of fragments. There is one array per fragment kind.
  */
class WLDFragmentTable
{
public:
    WLDFragmentTable();
    virtual ~WLDFragmentTable();
    void incrementFragmentCount(uint32_t kind);
    void allocate();
    uint32_t count(uint32_t kind) const;
    WLDFragment *current(uint32_t kind) const;
    void next(uint32_t kind);
    
    template<typename T>
    WLDFragmentArray<T> byKind() const
    {
        WLDFragmentArray<T> array((T *)m_frags[T::ID],
                                  m_fragCounts[T::ID],
                                  m_fragSize[T::ID]);
        return array;
    }
    
private:
    WLDFragment *createArray(uint32_t kind);
    void deleteArray(uint32_t kind);
    
    static const int MAX_FRAGMENT_KINDS = 0x40;
    QList<WLDFragment *> m_fragments;
    uint32_t m_fragCounts[MAX_FRAGMENT_KINDS];
    uint32_t m_fragSize[MAX_FRAGMENT_KINDS];
    WLDFragment *m_frags[MAX_FRAGMENT_KINDS];
    uint8_t *m_current[MAX_FRAGMENT_KINDS];
};

/*!
  \brief This type of fragment (0x03) holds the name of a texture bitmap.
  */
class BitmapNameFragment : public WLDFragment
{
public:
    BitmapNameFragment();
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
    SpriteDefFragment();
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
    SpriteFragment();
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
    HierSpriteDefFragment();
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
    HierSpriteFragment();
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
    TrackDefFragment();
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
    TrackFragment();
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
    ActorDefFragment();
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
    ActorFragment();
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
    LightDefFragment();
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
    LightFragment();
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
    LightSourceFragment();
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x28;
    LightFragment *m_ref;
    uint32_t m_flags;
    vec3 m_pos;
    float m_radius;
};

/*!
  \brief This type of fragment (0x2A) describes the ambient light used for a list of regions.
  */
class RegionLightFragment : public WLDFragment
{
public:
    RegionLightFragment();
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x2a;
    LightFragment *m_ref;
    uint32_t m_flags;
    QVector<uint32_t> m_regions;
};

/*!
  \brief This type of fragment (0x26) defines spell particle bolts.
  */
class SpellParticleDefFragment : public WLDFragment
{
public:
    SpellParticleDefFragment();
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
    SpellParticleFragment();
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
    Fragment34();
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
    MaterialDefFragment();
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
    MaterialPaletteFragment();
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
    MeshLightingDefFragment();
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x32;
    uint32_t m_data1, m_size1, m_data2, m_data3, m_data4;
    QVector<uint32_t> m_colors;
};

/*!
  \brief This type of fragment (0x33) defines instances of mesh lighting (fragment 0x32).
  */
class MeshLightingFragment : public WLDFragment
{
public:
    MeshLightingFragment();
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
    MeshDefFragment();
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
    QVector<uint32_t> m_colors;
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
    MeshFragment();
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x2D;
    MeshDefFragment *m_def;
    uint32_t m_flags;
};

struct RegionTreeNode
{
    vec3 normal;
    float distance;
    uint32_t regionID;
    uint32_t left;
    uint32_t right;
};

/*!
  \brief This type of fragment (0x21) describes a BSP tree of zone regions (fragment 0x22).
  */
class RegionTreeFragment : public WLDFragment
{
public:
    RegionTreeFragment();
    virtual bool unpack(WLDReader *s);

    const static uint32_t ID = 0x21;
    QVector<RegionTreeNode> m_nodes;
};

/*!
  \brief This type of fragment (0x22) describes a zone region.
  */
class RegionFragment : public WLDFragment
{
public:
    RegionFragment();
    virtual bool unpack(WLDReader *s);
    static void decodeRegionList(const QVector<uint8_t> &regionData, QVector<uint16_t> &regions);

    const static uint32_t ID = 0x22;
    uint32_t m_flags;
    WLDFragment *m_ref;
    uint32_t m_size1, m_size2, m_param1;
    uint32_t m_size3, m_size4, m_param2;
    uint32_t m_size5, m_size6;
    QVector<uint16_t> m_nearbyRegions;
};

#endif

