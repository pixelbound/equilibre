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

#include <cmath>
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/WLDData.h"

WLDFragmentTable::WLDFragmentTable()
{
    for(int i = 0; i < MAX_FRAGMENT_KINDS; i++)
    {
        m_fragCounts[i] = m_fragSize[i] = 0;
        m_frags[i] = NULL;
        m_current[i] = NULL;
    }
}

WLDFragmentTable::~WLDFragmentTable()
{
    for(int i = 0; i < MAX_FRAGMENT_KINDS; i++)
        deleteArray(i);
}

uint32_t WLDFragmentTable::count(uint32_t kind) const
{
    Q_ASSERT(kind < MAX_FRAGMENT_KINDS && "Exceeded maximum number of fragment kinds.");
    return m_fragCounts[kind];
}

void WLDFragmentTable::incrementFragmentCount(uint32_t kind)
{
    Q_ASSERT(kind < MAX_FRAGMENT_KINDS && "Exceeded maximum number of fragment kinds.");
    m_fragCounts[kind]++;
}

void WLDFragmentTable::allocate()
{
    // XXX change all fragment classes to be zero-initializable and fill the arrays with zeros.
    for(uint32_t i = 0; i < MAX_FRAGMENT_KINDS; i++)
    {
        m_frags[i] = createArray(i);
        m_current[i] = (uint8_t *)m_frags[i];
    }
}

void WLDFragmentTable::next(uint32_t kind)
{
    Q_ASSERT(kind < MAX_FRAGMENT_KINDS && "Exceeded maximum number of fragment kinds.");
    m_current[kind] += m_fragSize[kind];
}

WLDFragment * WLDFragmentTable::current(uint32_t kind) const
{
    Q_ASSERT(kind < MAX_FRAGMENT_KINDS && "Exceeded maximum number of fragment kinds.");
    return (WLDFragment *)m_current[kind];
}

#define CREATE_FRAGMENT_CASE(T) case T::KIND: fragSize = sizeof(T); return new T[count];
#define DELETE_FRAGMENT_CASE(T) case T::KIND: delete [] (T *)array; break;

WLDFragment * WLDFragmentTable::createArray(uint32_t kind)
{
    uint32_t count = m_fragCounts[kind];
    uint32_t &fragSize = m_fragSize[kind];
    if(count == 0)
    {
        fragSize = 0;
        return NULL;
    }
    switch(kind)
    {
    CREATE_FRAGMENT_CASE(BitmapNameFragment);
    CREATE_FRAGMENT_CASE(SpriteDefFragment);
    CREATE_FRAGMENT_CASE(SpriteFragment);
    CREATE_FRAGMENT_CASE(HierSpriteDefFragment);
    CREATE_FRAGMENT_CASE(HierSpriteFragment);
    CREATE_FRAGMENT_CASE(TrackDefFragment);
    CREATE_FRAGMENT_CASE(TrackFragment);
    CREATE_FRAGMENT_CASE(ActorDefFragment);
    CREATE_FRAGMENT_CASE(ActorFragment);
    CREATE_FRAGMENT_CASE(LightDefFragment);
    CREATE_FRAGMENT_CASE(LightFragment);
    CREATE_FRAGMENT_CASE(LightSourceFragment);
    CREATE_FRAGMENT_CASE(RegionLightFragment);
    CREATE_FRAGMENT_CASE(SpellParticleDefFragment);
    CREATE_FRAGMENT_CASE(SpellParticleFragment);
    CREATE_FRAGMENT_CASE(Fragment34);
    CREATE_FRAGMENT_CASE(MaterialDefFragment);
    CREATE_FRAGMENT_CASE(MaterialPaletteFragment);
    CREATE_FRAGMENT_CASE(MeshLightingDefFragment);
    CREATE_FRAGMENT_CASE(MeshLightingFragment);
    CREATE_FRAGMENT_CASE(MeshDefFragment);
    CREATE_FRAGMENT_CASE(MeshFragment);
    CREATE_FRAGMENT_CASE(RegionTreeFragment);
    CREATE_FRAGMENT_CASE(RegionFragment);
    default:
        fragSize = sizeof(WLDFragment);
        return new WLDFragment[count];
    }
}

void WLDFragmentTable::deleteArray(uint32_t kind)
{
    WLDFragment *array = m_frags[kind];
    if(!array)
        return;
    switch(kind)
    {
    DELETE_FRAGMENT_CASE(BitmapNameFragment);
    DELETE_FRAGMENT_CASE(SpriteDefFragment);
    DELETE_FRAGMENT_CASE(SpriteFragment);
    DELETE_FRAGMENT_CASE(HierSpriteDefFragment);
    DELETE_FRAGMENT_CASE(HierSpriteFragment);
    DELETE_FRAGMENT_CASE(TrackDefFragment);
    DELETE_FRAGMENT_CASE(TrackFragment);
    DELETE_FRAGMENT_CASE(ActorDefFragment);
    DELETE_FRAGMENT_CASE(ActorFragment);
    DELETE_FRAGMENT_CASE(LightDefFragment);
    DELETE_FRAGMENT_CASE(LightFragment);
    DELETE_FRAGMENT_CASE(LightSourceFragment);
    DELETE_FRAGMENT_CASE(RegionLightFragment);
    DELETE_FRAGMENT_CASE(SpellParticleDefFragment);
    DELETE_FRAGMENT_CASE(SpellParticleFragment);
    DELETE_FRAGMENT_CASE(Fragment34);
    DELETE_FRAGMENT_CASE(MaterialDefFragment);
    DELETE_FRAGMENT_CASE(MaterialPaletteFragment);
    DELETE_FRAGMENT_CASE(MeshLightingDefFragment);
    DELETE_FRAGMENT_CASE(MeshLightingFragment);
    DELETE_FRAGMENT_CASE(MeshDefFragment);
    DELETE_FRAGMENT_CASE(MeshFragment);
    DELETE_FRAGMENT_CASE(RegionTreeFragment);
    DELETE_FRAGMENT_CASE(RegionFragment);
    default:
        delete [] array;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////

bool BitmapNameFragment::unpack(WLDReader *s)
{
    uint16_t size;
    s->unpackFields("IH", &m_flags, &size);
    s->readEncodedString(size, &m_fileName);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool SpriteDefFragment::unpack(WLDReader *s)
{
    uint32_t fileCount;
    s->unpackFields("II", &m_flags, &fileCount);
    if(m_flags & 0x4)
        s->unpackField('I', &m_param1);
    else
        m_param1 = 0;
    if(m_flags & 0x8)
        s->unpackField('I', &m_param2);
    else
        m_param2 = 0;
    for(uint32_t i = 0; i < fileCount; i++)
    {
        BitmapNameFragment *frag = 0;
        s->unpackReference(&frag);
        if(frag)
            m_bitmaps.append(frag);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool SpriteFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool HierSpriteDefFragment::unpack(WLDReader *s)
{
    uint32_t nodeCount, childrenCount, meshCount;
    s->unpackFields("IIr", &m_flags, &nodeCount, &m_fragment);
    if((m_flags & 0x1) == 0x1)
        s->unpackArray("I", 3, &m_param1);
    else
        m_param1[0] = m_param1[1] = m_param1[2] = 0;
    if((m_flags & 0x2) == 0x2)
        s->unpackField('f', &m_boundingRadius);
    else
        m_boundingRadius = 0.0;
    for(uint32_t i = 0; i < nodeCount; i++)
    {
        SkeletonNode node;
        s->unpackFields("RIrrI", &node.name, &node.flags, &node.track, &node.mesh, &childrenCount);
        node.children.resize(childrenCount);
        s->unpackArray("I", childrenCount, node.children.data());
        m_tree.append(node);
    }
    if((m_flags & 0x200) == 0x200)
    {
        s->unpackField('I', &meshCount);
        m_meshes.resize(meshCount);
        s->unpackArray("r", meshCount, m_meshes.data());
        m_linkSkinUpdatesWithTreeNode.resize(meshCount);
        s->unpackArray("I", meshCount, m_linkSkinUpdatesWithTreeNode.data());
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool HierSpriteFragment::unpack(WLDReader *s)
{
    s->unpackFields("rI", &m_def, &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool TrackDefFragment::unpack(WLDReader *s)
{
    uint32_t frameCount;
    int16_t rw, rx, ry, rz, dx, dy, dz, scale;
    float l;
    s->unpackFields("II", &m_flags, &frameCount);
    for(uint32_t i = 0; i < frameCount; i++)
    {
        BoneTransform frame;
        s->unpackFields("hhhhhhhh", &rw, &rx, &ry, &rz, &dx, &dy, &dz, &scale);
        if(rw != 0)
        {
            // normalize the quaternion, since it is stored as a 16-bit integer
            l = sqrt((float)(rw * rw + rx * rx + ry * ry + rz * rz));
            frame.rotation = QQuaternion(rw / l, rx / l, ry / l, rz / l);
        }
        if(scale != 0)
        {
            // rescale the location vector
            l = 1.0 / scale;
            frame.location = QVector3D(dx * l, dy * l, dz * l);
        }
        m_frames.append(frame);
    }
    return true;
}

BoneTransform TrackDefFragment::frame(uint32_t frameIndex) const
{
    int n = m_frames.count();
    if(n > 0)
        return m_frames[frameIndex % n];
    else
        return BoneTransform();
}

////////////////////////////////////////////////////////////////////////////////

bool TrackFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    if((m_flags & HasSleep) == HasSleep)
        s->unpackField('I', &m_sleepMs);
    else
        m_sleepMs = 0;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ActorDefFragment::unpack(WLDReader *s)
{
    uint32_t size1, modelCount, entrySize;
    s->unpackFields("IrIIr", &m_flags, &m_fragment1, &size1, &modelCount, &m_fragment2);

    // load entries (unknown purpose)
    for(uint32_t i = 0; i < size1; i++)
    {
        QVector<WLDPair> entry;
        WLDPair p;
        s->unpackField('I', &entrySize);
        for(uint32_t j = 0; j < entrySize; j++)
        {
            s->unpackStruct("If", &p);
            entry.append(p);
        }
        m_entries.append(entry);
    }

    // load model references
    WLDFragment *f = 0;
    for(uint32_t i = 0; i < modelCount; i++)
    {
        s->unpackField('r', &f);
        m_models.append(f);
    }
    //s->unpackField('I', &m_nameSize);
    //s->readString(m_nameSize, &m_name);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ActorFragment::unpack(WLDReader *s)
{
    float scaleX, scaleY, rotX, rotY, rotZ;
    s->unpackFields("RIr", &m_def, &m_flags, &m_fragment1);
    s->unpackStruct("fff", &m_location);
    s->unpackFields("ffff", &rotZ, &rotY, &rotX, &m_param1); // param1: rotW (quaternion)?
    float rotFactor = 1.0 / (512.0 / 360.0);
    m_rotation = vec3(rotX * rotFactor, rotY * rotFactor, rotZ * rotFactor);
    s->unpackFields("ff", &scaleX, &scaleY);
    m_scale = vec3(scaleX, scaleY, 1.0);
    s->unpackReference(&m_lighting);
    // Fix pathological z locations.
    if((m_location.z > -32768.0f) && (m_location.z < -32767.0f))
        m_location.z = 0.0f;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool LightDefFragment::unpack(WLDReader *s)
{
    s->unpackFields("II", &m_flags, &m_params2);
    if(m_flags & 0x10)
    {
        // ARGB light source.
        if(m_flags & 0x08)
            s->unpackFields("I", &m_attenuation);
        else
            m_attenuation = 0;
        s->unpackFields("ffff", &m_color.w, &m_color.x, &m_color.y, &m_color.z);
    }
    else
    {
        // White light source.
        s->unpackField('f', &m_color.x);
        m_color.y = m_color.z = m_color.x;
        m_color.w = 1.0f;
        m_attenuation = 0;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool LightFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool LightSourceFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_ref);
    s->unpackFields("Iffff", &m_flags, &m_pos.x, &m_pos.y, &m_pos.z, &m_radius);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool RegionLightFragment::unpack(WLDReader *s)
{
    uint32_t regionCount = 0;
    s->unpackReference(&m_ref);
    s->unpackFields("II", &m_flags, &regionCount);
    m_regions.resize(regionCount);
    s->unpackArray("I", regionCount, m_regions.data());
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool SpellParticleDefFragment::unpack(WLDReader *s)
{
    s->unpackField('I', &m_flags);
    s->unpackReference(&m_sprite);
    // render mode + flag? common values: 80000017, 80000018, 80000019
    s->unpackField('I', &m_param1);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool SpellParticleFragment::unpack(WLDReader *s)
{
    //<0x26 fragment> flags
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool Fragment34::unpack(WLDReader *s)
{
    s->unpackFields("IIII", &m_param0, &m_param1, &m_param2, &m_flags);
    s->unpackStruct("IIIIIIffIffffIff", &m_data3);
    s->unpackReference(&m_particle);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MaterialDefFragment::unpack(WLDReader *s)
{
    s->unpackFields("IIIff", &m_flags, &m_renderMode, &m_rgbPen, &m_brightness, &m_scaledAmbient);
    s->unpackReference(&m_sprite);
    s->unpackField('f', &m_param3);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MaterialPaletteFragment::unpack(WLDReader *s)
{
    uint32_t materialCount;
    s->unpackFields("II", &m_flags, &materialCount);
    for(uint32_t i = 0; i < materialCount; i++)
    {
        MaterialDefFragment *frag = 0;
        s->unpackReference(&frag);
        if(frag)
            m_materials.append(frag);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MeshLightingDefFragment::unpack(WLDReader *s)
{
    uint8_t r, g, b, a;
    s->unpackFields("IIIII", &m_data1, &m_size1, &m_data2, &m_data3, &m_data4);
    for(uint32_t i = 0; i < m_size1; i++)
    {
        s->unpackFields("BBBB", &r, &g, &b, &a);
        uint32_t rgba = r + (g << 8) + (b << 16) + (a << 24);
        m_colors.append(rgba);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MeshLightingFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MeshDefFragment::unpack(WLDReader *s)
{
    uint16_t vertexCount, texCoordsCount, normalCount, colorCount, polyCount;
    uint16_t vertexPieceCount, polyTexCount, vertexTexCount, scaleFactor;
    s->unpackField('I', &m_flags);
    s->unpackReference(&m_palette);
    s->unpackArray("R", 3, m_ref);
    s->unpackArray("f", 3, &m_center);
    s->unpackArray("I", 3, &m_param2);
    s->unpackField('f', &m_maxDist);
    // This is not always defined, we will calculate it later on.
    s->unpackArray("f", 3, &m_boundsAA.low);
    s->unpackArray("f", 3, &m_boundsAA.high);
    s->unpackFields("hhhhhhhhhh", &vertexCount, &texCoordsCount, &normalCount,
                 &colorCount, &polyCount, &vertexPieceCount, &polyTexCount,
                 &vertexTexCount, &m_size9, &scaleFactor);

    float scale = 1.0 / float(1 << scaleFactor);
    int16_t vertex[3], texCoord[2];
    int8_t normal[3], color[4];
    uint16_t polygon[4], vertexPiece[2], polyTex[2], vertexTex[2];
    s->unpackStruct("hhh", vertex);
    vec3 scaledVertex = vec3(vertex[0] * scale, vertex[1] * scale, vertex[2] * scale);
    m_boundsAA = AABox(scaledVertex, scaledVertex);
    m_vertices.append(scaledVertex);
    for(uint16_t i = 1; i < vertexCount; i++)
    {
        s->unpackStruct("hhh", vertex);
        scaledVertex = vec3(vertex[0] * scale, vertex[1] * scale, vertex[2] * scale);
        m_boundsAA.extendTo(scaledVertex);
        m_vertices.append(scaledVertex);
    }
    for(uint16_t i = 0; i < texCoordsCount; i++)
    {
        s->unpackStruct("hh", texCoord);
        m_texCoords.append(vec2((texCoord[0] / 256.0), (texCoord[1] / 256.0)));
    }
    for(uint16_t i = 0; i < normalCount; i++)
    {
        s->unpackStruct("bbb", normal);
        m_normals.append(vec3(normal[0] / 127.0, normal[1] / 127.0, normal[2] / 127.0));
    }
    for(uint16_t i = 0; i < colorCount; i++)
    {
        s->unpackStruct("BBBB", color);
        m_colors.append(qRgba(color[0], color[1], color[2], color[3]));
    }
    for(uint16_t i = 0; i < polyCount; i++)
    {
        s->unpackStruct("HHHH", polygon);
        m_indices.append(polygon[1]);
        m_indices.append(polygon[2]);
        m_indices.append(polygon[3]);
    }
    for(uint16_t i = 0; i < vertexPieceCount; i++)
    {
        s->unpackStruct("HH", vertexPiece);
        m_vertexPieces.append(vec2us(vertexPiece[0], vertexPiece[1]));
    }
    for(uint16_t i = 0; i < polyTexCount; i++)
    {
        s->unpackStruct("HH", polyTex);
        m_polygonsByTex.append(vec2us(polyTex[0], polyTex[1]));
    }
    for(uint16_t i = 0; i < vertexTexCount; i++)
    {
        s->unpackStruct("HH", vertexTex);
        m_verticesByTex.append(vec2us(vertexTex[0], vertexTex[1]));
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MeshFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool RegionTreeFragment::unpack(WLDReader *s)
{
    uint32_t count;
    s->unpackField('I', &count);
    m_nodes.resize(count);
    s->unpackArray("ffffIII", count, m_nodes.data());
    // Make sure the indices are all in-bounds.
    uint32_t maxNodeIdx = 0;
    for(uint32_t i = 0; i < count; i++)
    {
        const RegionTreeNode &node = m_nodes[i];
        maxNodeIdx = qMax(maxNodeIdx, qMax(node.left, node.right));
    }
    Q_ASSERT(maxNodeIdx <= count);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool RegionFragment::unpack(WLDReader *s)
{
    s->unpackFields("IrIIIIIIII", &m_flags, &m_ref, &m_size1, &m_size2, &m_param1,
                    &m_size3, &m_size4, &m_param2, &m_size5, &m_size6);
    // Skip Data1 and Data2
    s->stream()->seek(s->stream()->pos() + 12 * m_size1);
    s->stream()->seek(s->stream()->pos() + 8 * m_size2);
    // TODO Data3, Data4
    // Skip Data5
    s->stream()->seek(s->stream()->pos() + 4 * 7 * m_size5);
    
    // Decode nearby region list.
    bool byteEntries = (m_flags & 0x80), wordEntries = (m_flags & 0x10);
    if(byteEntries || wordEntries)
    {
        QVector<uint8_t> regionData;
        Q_ASSERT(byteEntries);
        Q_ASSERT((byteEntries ^ wordEntries) && "Region lists must have either byte- or word-sized entries.");
        uint32_t entrySize = byteEntries ? 1 : 2;
        for(uint32_t i = 0; i < m_size6; i++)
        {
            uint16_t entries = 0;
            s->unpackField('H', &entries);
            regionData.resize(entries *  entrySize);
            s->unpackArray(byteEntries ? "B" : "H", entries, regionData.data());
            decodeRegionList(regionData, m_nearbyRegions);
        }
    }
    return true;
}

void RegionFragment::decodeRegionList(const QVector<uint8_t> &data, QVector<uint16_t> &regions)
{
    uint32_t RID = 1;
    uint32_t pos = 0;
    while(pos < data.count())
    {
        uint8_t b = data[pos];
        if(b < 0x3f)
        {
            RID += b;
            pos += 1;
        }
        else if(b == 0x3f)
        {
            Q_ASSERT((pos + 2) < data.count());
            uint8_t lo = data[pos + 1];
            uint8_t hi = data[pos + 2];
            uint16_t skip = ((hi << 8) + lo);
            RID += skip;
            pos += 3;
        }
        else if(b < 0x80)
        {
            uint8_t skip = (b & 0x38) >> 3;
            uint8_t mark = (b & 0x07);
            RID += skip;
            for(uint32_t i = 0; i < mark; i++)
                regions.append(RID + i);
            RID += mark;
            pos += 1;
        }
        else if(b < 0xc0)
        {
            uint8_t mark = (b & 0x38) >> 3;
            uint8_t skip = (b & 0x07);
            for(uint32_t i = 0; i < mark; i++)
                regions.append(RID + i);
            RID += mark;
            RID += skip;
            pos += 1;
        }
        else if(b < 0xff)
        {
            uint8_t mark = b - 0xc0;
            for(uint32_t i = 0; i < mark; i++)
                regions.append(RID + i);
            RID += mark;
            pos += 1;
        }
        else //if(b == 0xff)
        {
            Q_ASSERT((pos + 2) < data.count());
            uint8_t lo = data[pos + 1];
            uint8_t hi = data[pos + 2];
            uint16_t mark = ((hi << 8) + lo);
            for(uint32_t i = 0; i < mark; i++)
                regions.append(RID + i);
            RID += mark;
            pos += 3;
        }
    }
}
