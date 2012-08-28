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

BitmapNameFragment::BitmapNameFragment(QString name) : WLDFragment(ID, name)
{
}

bool BitmapNameFragment::unpack(WLDReader *s)
{
    uint16_t size;
    s->unpackFields("IH", &m_flags, &size);
    s->readEncodedString(size, &m_fileName);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

SpriteDefFragment::SpriteDefFragment(QString name) : WLDFragment(ID, name)
{
    m_flags = m_param1 = m_param2 = 0;
}

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

SpriteFragment::SpriteFragment(QString name) : WLDFragment(ID, name)
{
}

bool SpriteFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

HierSpriteDefFragment::HierSpriteDefFragment(QString name) : WLDFragment(ID, name)
{
}

bool HierSpriteDefFragment::unpack(WLDReader *s)
{
    uint32_t nodeCount, childrenCount, meshCount;
    s->unpackFields("IIr", &m_flags, &nodeCount, &m_fragment);
    if((m_flags & 0x1) == 0x1)
        s->unpackArray("I", 3, &m_param1);
    else
        m_param1[0] = m_param1[1] = m_param1[2] = 0;
    if((m_flags & 0x2) == 0x2)
        s->unpackField('I', &m_param2);
    else
        m_param2 = 0.0;
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
        m_data3.resize(meshCount);
        s->unpackArray("I", meshCount, m_data3.data());
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

HierSpriteFragment::HierSpriteFragment(QString name) : WLDFragment(ID, name)
{
}

bool HierSpriteFragment::unpack(WLDReader *s)
{
    s->unpackFields("rI", &m_def, &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

TrackDefFragment::TrackDefFragment(QString name) : WLDFragment(ID, name)
{
}

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

TrackFragment::TrackFragment(QString name) : WLDFragment(ID, name)
{
}

bool TrackFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    if((m_flags & 0x1) == 0x1)
        s->unpackField('I', &m_param1);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

ActorDefFragment::ActorDefFragment(QString name) : WLDFragment(ID, name)
{
}

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

ActorFragment::ActorFragment(QString name) : WLDFragment(ID, name)
{
}

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

SpellParticleDefFragment::SpellParticleDefFragment(QString name) : WLDFragment(ID, name)
{
}

bool SpellParticleDefFragment::unpack(WLDReader *s)
{
    s->unpackField('I', &m_flags);
    s->unpackReference(&m_sprite);
    // render mode + flag? common values: 80000017, 80000018, 80000019
    s->unpackField('I', &m_param1);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

SpellParticleFragment::SpellParticleFragment(QString name) : WLDFragment(ID, name)
{
}

bool SpellParticleFragment::unpack(WLDReader *s)
{
    //<0x26 fragment> flags
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

Fragment34::Fragment34(QString name) : WLDFragment(ID, name)
{
}

bool Fragment34::unpack(WLDReader *s)
{
    s->unpackFields("IIII", &m_param0, &m_param1, &m_param2, &m_flags);
    s->unpackStruct("IIIIIIffIffffIff", &m_data3);
    s->unpackReference(&m_particle);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MaterialDefFragment::MaterialDefFragment(QString name) : WLDFragment(ID, name)
{
}

bool MaterialDefFragment::unpack(WLDReader *s)
{
    s->unpackFields("IIIff", &m_flags, &m_param1, &m_param2, &m_brightness, &m_scaledAmbient);
    s->unpackReference(&m_sprite);
    s->unpackField('f', &m_param3);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MaterialPaletteFragment::MaterialPaletteFragment(QString name) : WLDFragment(ID, name)
{
}

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

MeshLightingDefFragment::MeshLightingDefFragment(QString name) : WLDFragment(ID, name)
{
}

bool MeshLightingDefFragment::unpack(WLDReader *s)
{
    uint8_t color[4];
    s->unpackFields("IIIII", &m_data1, &m_size1, &m_data2, &m_data3, &m_data4);
    for(uint32_t i = 0; i < m_size1; i++)
    {
        s->unpackStruct("BBBB", color);
        m_colors.append(qRgba(color[0], color[1], color[2], color[3]));
    }
}

////////////////////////////////////////////////////////////////////////////////

MeshLightingFragment::MeshLightingFragment(QString name) : WLDFragment(ID, name)
{
}

bool MeshLightingFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
}

////////////////////////////////////////////////////////////////////////////////

MeshDefFragment::MeshDefFragment(QString name) : WLDFragment(ID, name)
{
}

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

MeshFragment::MeshFragment(QString name) : WLDFragment(ID, name)
{
}

bool MeshFragment::unpack(WLDReader *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}
