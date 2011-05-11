#include "Fragments.h"

BitmapNameFragment::BitmapNameFragment(QString name) : WLDFragment(ID, name)
{
}

bool BitmapNameFragment::unpack(WLDFragmentStream *s)
{
    uint16_t size;
    s->unpackFields("IH", &m_flags, &size);
    s->readEncodedString(size, &m_fileName);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

SpriteDefFragment::SpriteDefFragment(QString name) : WLDFragment(ID, name)
{
}

bool SpriteDefFragment::unpack(WLDFragmentStream *s)
{
    uint32_t fileCount;
    s->unpackFields("IIII", &m_flags, &fileCount, &m_param1, &m_param2);
    for(uint32_t i = 0; i < fileCount; i++)
    {
        BitmapNameFragment *frag;
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

bool SpriteFragment::unpack(WLDFragmentStream *s)
{
    s->unpackReference(&m_def);
    s->unpackField('I', &m_flags);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MaterialDefFragment::MaterialDefFragment(QString name) : WLDFragment(ID, name)
{
}

bool MaterialDefFragment::unpack(WLDFragmentStream *s)
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

bool MaterialPaletteFragment::unpack(WLDFragmentStream *s)
{
    uint32_t materialCount;
    s->unpackFields("II", &m_flags, &materialCount);
    for(uint32_t i = 0; i < materialCount; i++)
    {
        MaterialDefFragment *frag;
        s->unpackReference(&frag);
        if(frag)
            m_materials.append(frag);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MeshFragment::MeshFragment(QString name) : WLDFragment(ID, name)
{
}

bool MeshFragment::unpack(WLDFragmentStream *s)
{
    uint16_t vertexCount, texCoordsCount, normalCount, colorCount, polyCount;
    uint16_t vertexPieceCount, polyTexCount, vertexTexCount, scaleFactor;
    s->unpackField('I', &m_flags);
    s->unpackReference(&m_palette);
    s->unpackArray("R", 3, m_ref);
    s->unpackArray("f", 3, &m_center);
    s->unpackArray("I", 3, &m_param2);
    s->unpackField('f', &m_maxDist);
    s->unpackArray("f", 3, &m_min);
    s->unpackArray("f", 3, &m_max);
    s->unpackFields("hhhhhhhhhh", &vertexCount, &texCoordsCount, &normalCount,
                 &colorCount, &polyCount, &vertexPieceCount, &polyTexCount,
                 &vertexTexCount, &m_size9, &scaleFactor);

    float scale = 1.0 / float(1 << scaleFactor);
    int16_t vertex[3], texCoord[2];
    int8_t normal[3], color[4];
    uint16_t polygon[4], vertexPiece[2], polyTex[2], vertexTex[2];
    for(uint16_t i = 0; i < vertexCount; i++)
    {
        s->unpackStruct("hhh", vertex);
        m_vertices.append(vec3(vertex[0] * scale, vertex[1] * scale, vertex[2] * scale));
    }
    for(uint16_t i = 0; i < texCoordsCount; i++)
    {
        s->unpackStruct("hh", texCoord);
        m_texCoords.append(vec2(texCoord[0] / 256.0, texCoord[1] / 256.0));
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

VertexGroup *MeshFragment::toGroup() const
{
    if(m_vertices.count() == 0)
        return 0;
    VertexGroup *vg = new VertexGroup(GL_TRIANGLES, m_vertices.count());
    VertexData *vd = vg->data;
    for(uint32_t i = 0; i < vg->count; i++, vd++)
    {
        vd->position = m_vertices.value(i);
        vd->normal = m_normals.value(i);
        vd->texCoords = m_texCoords.value(i);
    }
    for(uint32_t i = 0; i < (uint32_t)m_indices.count(); i++)
        vg->indices.push_back(m_indices[i]);
    return vg;
}
