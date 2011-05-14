#include <QImage>
#include "WLDModel.h"
#include "Fragments.h"
#include "PFSArchive.h"
#include "RenderState.h"

WLDModel::WLDModel(ActorDefFragment *def, PFSArchive *archive, QObject *parent) : QObject(parent)
{
    m_def = def;
    m_archive = archive;
    importDefinition(def);
}

WLDModel::~WLDModel()
{
}

void WLDModel::importDefinition(ActorDefFragment *def)
{
    //TODO: handle skeletons
    foreach(WLDFragment *modelFrag, def->m_models)
    {
        MeshFragment *meshFrag = modelFrag->cast<MeshFragment>();
        if(meshFrag)
            importMesh(meshFrag->m_def);
    }
}

void WLDModel::importMesh(MeshDefFragment *frag)
{
    m_meshFrags.append(frag);
    m_meshes.append(0);
}

void WLDModel::draw(RenderState *state)
{
    for(int i = 0; i < m_meshFrags.count(); i++)
    {
        MeshDefFragment *frag = m_meshFrags[i];
        Mesh *m = m_meshes[i];
        // create the mesh on first use
        if(!m)
        {
            m = state->createMesh();
            importMaterialGroups(frag, m);
            m_meshes[i] = m;
        }
        state->drawMesh(m);
    }
}

void WLDModel::importMaterialGroups(MeshDefFragment *frag, Mesh *m)
{
    // load vertices, texCoords, normals, faces
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

    // load material groups
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

    QImage img;
    // XXX case-insensitive lookup
    if(m_archive)
        img.loadFromData(m_archive->unpackFile(bmp->m_fileName.toLower()));

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

    float ambient = frag->m_scaledAmbient;
    Material *mat = new Material();
    mat->setAmbient(vec4(ambient, ambient, ambient, 1.0));
    mat->setDiffuse(vec4(1.0, 1.0, 1.0, 1.0));
    mat->loadTexture(img);
    m_materials.insert(frag->name(), mat);
    return mat;
}
