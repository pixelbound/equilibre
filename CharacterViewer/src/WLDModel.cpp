#include <QImage>
#include "WLDModel.h"
#include "Fragments.h"

WLDModel::WLDModel(RenderState *state, QString path, QObject *parent) : QObject(parent), StateObject(state)
{
    m_path = path;
}

WLDModel::~WLDModel()
{
}

void WLDModel::addMesh(MeshFragment *frag)
{
    m_meshFrags.append(frag);
    m_meshes.append(0);
}

void WLDModel::draw()
{
    for(int i = 0; i < m_meshFrags.count(); i++)
    {
        MeshFragment *frag = m_meshFrags[i];
        Mesh *m = m_meshes[i];
        if(!m)
        {
            // load textures
            if(frag->m_palette)
            {
                foreach(MaterialDefFragment *matDef, frag->m_palette->m_materials)
                    importMaterial(matDef);
            }

            // load vertices, texCoords, normals, faces
            m = m_state->createMesh();
            importVertexGroups(frag, m);
            m_meshes[i] = m;
        }
        m_state->drawMesh(m);
    }
}

void WLDModel::importVertexGroups(MeshFragment *frag, Mesh *m)
{
    VertexGroup *vg = new VertexGroup(GL_TRIANGLES, frag->m_vertices.count());
    VertexData *vd = vg->data;
    for(uint32_t i = 0; i < vg->count; i++, vd++)
    {
        vd->position = frag->m_vertices.value(i);
        vd->normal = frag->m_normals.value(i);
        vd->texCoords = frag->m_texCoords.value(i);
    }
    for(uint32_t i = 0; i < (uint32_t)frag->m_indices.count(); i++)
        vg->indices.push_back(frag->m_indices[i]);

    MaterialPaletteFragment *palette = frag->m_palette;
    uint32_t pos = 0;
    foreach(vec2us g, frag->m_polygonsByTex)
    {
        MaterialGroup mg;
        mg.offset = pos;
        mg.count = g.first * 3;
        mg.mat = importMaterial(palette->m_materials[g.second]);
        vg->matGroups.append(mg);
        pos += mg.count;
    }
    m->addGroup(vg);
}

Material * WLDModel::importMaterial(MaterialDefFragment *frag)
{
    if(!frag)
        return 0;
    if(m_materials.contains(frag->name()))
        return m_materials.value(frag->name());
    SpriteFragment *sprite = frag->m_sprite;
    if(!sprite)
        return 0;
    SpriteDefFragment *spriteDef = sprite->m_def;
    if(!spriteDef)
        return 0;
    BitmapNameFragment *bmp = spriteDef->m_bitmaps.value(0);
    if(!bmp)
        return 0;

    QString fileName = QString("%1/%2").arg(m_path).arg(bmp->m_fileName.toLower());
    QImage img(fileName);
    // masked bitmap?
    if((frag->m_param1 & 0x3) == 0x3)
    {
        if(img.colorCount() > 0)
        {
            // replace the color of the first pixel by a transparent color in the table
            uchar index = *img.bits();
            QVector<QRgb> colors = img.colorTable();
            colors[index] = qRgba(0, 0, 0, 0);
            img.setColorTable(colors);
        }
    }

    Material *mat = new Material();
    mat->setAmbient(vec4(0.2, 0.2, 0.2, 1.0));
    mat->setDiffuse(vec4(1.0, 1.0, 1.0, 1.0));
    mat->loadTexture(img);
    m_materials.insert(frag->name(), mat);
    return mat;
}
