#include <QImage>
#include "WLDModel.h"
#include "Fragments.h"
#include "WLDSkeleton.h"
#include "PFSArchive.h"
#include "RenderState.h"

WLDModel::WLDModel(PFSArchive *archive, ActorDefFragment *def, WLDSkeleton *skel, QObject *parent) : QObject(parent)
{
    m_archive = archive;
    m_skel = skel;
    m_animName = "POS";
    if(def)
        importDefinition(def);
}

WLDModel::~WLDModel()
{
}

WLDSkeleton * WLDModel::skeleton() const
{
    return m_skel;
}

QString WLDModel::animName() const
{
    return m_animName;
}

void WLDModel::setAnimName(QString name)
{
    m_animName = name;
}

void WLDModel::importDefinition(ActorDefFragment *def)
{
    foreach(WLDFragment *modelFrag, def->m_models)
    {
        MeshFragment *meshFrag = modelFrag->cast<MeshFragment>();
        if(meshFrag)
            importMesh(meshFrag->m_def);
        HierSpriteFragment *skelFrag = modelFrag->cast<HierSpriteFragment>();
        if(skelFrag)
            importHierMesh(skelFrag->m_def);
    }
}

void WLDModel::importHierMesh(HierSpriteDefFragment *def)
{
    foreach(MeshFragment *meshFrag, def->m_meshes)
        importMesh(meshFrag->m_def);
}

void WLDModel::importMesh(MeshDefFragment *frag)
{
    m_parts.append(new WLDModelPart(this, frag, this));
}

void WLDModel::draw(RenderState *state, double currentTime)
{
    foreach(WLDModelPart *part, m_parts)
        part->draw(state, currentTime);
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

////////////////////////////////////////////////////////////////////////////////

WLDModelPart::WLDModelPart(WLDModel *model, MeshDefFragment *meshDef, QObject *parent) : QObject(parent)
{
    m_model = model;
    m_meshDef = meshDef;
    m_mesh = 0;
}

void WLDModelPart::draw(RenderState *state, double currentTime)
{
    // create the mesh on first use
    if(!m_mesh)
    {
        m_mesh = state->createMesh();
        importMaterialGroups(m_mesh, currentTime);
    }

    // draw the mesh
    state->pushMatrix();
    state->translate(m_meshDef->m_center);
    state->drawMesh(m_mesh);
    state->popMatrix();

    // HACK until skinning is implemented in shaders
    delete m_mesh;
    m_mesh = 0;
}

void WLDModelPart::importMaterialGroups(Mesh *m, double currentTime)
{
    // load vertices, texCoords, normals, faces
    VertexGroup *vg = new VertexGroup(GL_TRIANGLES, m_meshDef->m_vertices.count());
    VertexData *vd = vg->data;
    for(uint32_t i = 0; i < vg->count; i++, vd++)
    {
        vd->position = m_meshDef->m_vertices.value(i);
        vd->normal = m_meshDef->m_normals.value(i);
        vd->texCoords = m_meshDef->m_texCoords.value(i);
    }
    for(uint32_t i = 0; i < (uint32_t)m_meshDef->m_indices.count(); i++)
        vg->indices.push_back(m_meshDef->m_indices[i]);

    // load material groups
    MaterialPaletteFragment *palette = m_meshDef->m_palette;
    uint32_t pos = 0;
    foreach(vec2us g, m_meshDef->m_polygonsByTex)
    {
        MaterialGroup mg;
        MaterialDefFragment *matDef = palette->m_materials[g.second];
        uint32_t vertexCount = g.first * 3;
        // skip invisible groups
        if(matDef->m_param1 != 0)
        {
            mg.offset = pos;
            mg.count = vertexCount;
            mg.mat = m_model->importMaterial(matDef);
            vg->matGroups.append(mg);
        }
        pos += vertexCount;
    }

    // skin mesh if there is a skeleton
    WLDSkeleton *skel = m_model->skeleton();
    if(skel)
    {
        WLDAnimation *anim = skel->animations().value(m_model->animName());
        if(!anim)
            anim = skel->pose();
        QVector<BoneTransform> trans = anim->transformationsAtTime(currentTime);
        pos = 0;
        vd = vg->data;
        foreach(vec2us g, m_meshDef->m_vertexPieces)
        {
            uint16_t count = g.first;
            uint16_t pieceID = g.second;
            BoneTransform pieceTrans = trans[pieceID];
            for(uint32_t i = 0; i < count; i++, vd++)
                vd->position = pieceTrans.map(vd->position);
            pos += count;
        }
    }
    m->addGroup(vg);
}
