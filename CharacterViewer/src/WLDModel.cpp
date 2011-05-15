#include <QImage>
#include <QRegExp>
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
    m_palName = "00";
    addPalette(m_palName, new WLDMaterialPalette(m_palName, archive, this));
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

WLDMaterialPalette * WLDModel::palette() const
{
    return m_palettes.value(m_palName);
}

void WLDModel::setPalette(QString palName)
{
    m_palName = palName;
}

const QMap<QString, WLDMaterialPalette *> & WLDModel::palettes() const
{
    return m_palettes;
}

void WLDModel::addPalette(QString palName, WLDMaterialPalette *palette)
{
    if(palette)
        m_palettes.insert(palName, palette);
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
    {
        if(meshFrag->m_def)
            importMesh(meshFrag->m_def);
        else
        {
            qDebug("HierMesh without Mesh (%s)", def->name().toLatin1().constData());
        }
    }
}

void WLDModel::importMesh(MeshDefFragment *frag)
{
    MaterialPaletteFragment *palDef = frag->m_palette;
    WLDMaterialPalette *pal = palette();
    if(palDef && pal)
        pal->addPaletteDef(palDef);
    m_parts.append(new WLDModelPart(this, frag, this));
}

void WLDModel::draw(RenderState *state, double currentTime)
{
    foreach(WLDModelPart *part, m_parts)
        part->draw(state, currentTime);
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
    MaterialPaletteFragment *palDef = m_meshDef->m_palette;
    WLDMaterialPalette *pal = m_model->palette();
    uint32_t pos = 0;
    foreach(vec2us g, m_meshDef->m_polygonsByTex)
    {
        MaterialDefFragment *matDef = palDef->m_materials[g.second];
        uint32_t vertexCount = g.first * 3;
        MaterialGroup mg;
        mg.offset = pos;
        mg.count = vertexCount;
        // invisible groups have no material
        if((matDef->m_param1 == 0) || !pal)
        {
            mg.palette = 0;
        }
        else
        {
            mg.matName = pal->materialName(palDef->m_materials[g.second]);
            mg.palette = pal;
        }
        vg->matGroups.append(mg);
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

////////////////////////////////////////////////////////////////////////////////

WLDMaterialPalette::WLDMaterialPalette(QString paletteID, PFSArchive *archive, QObject *parent) : QObject(parent)
{
    m_paletteID = paletteID;
    m_archive = archive;
}

QString WLDMaterialPalette::paletteID() const
{
    return m_paletteID;
}

void WLDMaterialPalette::addPaletteDef(MaterialPaletteFragment *def)
{
    foreach(MaterialDefFragment *matDef, def->m_materials)
        addMaterialDef(matDef);
}

QString WLDMaterialPalette::addMaterialDef(MaterialDefFragment *def)
{
    QString name = materialName(def);
    importMaterial(name, def);
    return name;
}

bool WLDMaterialPalette::explodeName(QString defName, QString &charName,
                        QString &palName, QString &partName)
{
    // e.g. defName == 'ORCCH0201_MDF'
    // 'ORC' : character
    // 'CH' : piece (part 1)
    // '02' : palette ID
    // '01' : piece (part 2)
    static QRegExp r("^\\w{5}\\d{4}_MDF$");
    if(r.exactMatch(defName))
    {
        charName = defName.left(3);
        palName = defName.mid(5, 2);
        partName = defName.mid(3, 2) + defName.mid(7, 2);
        return true;
    }
    return false;
}

bool WLDMaterialPalette::explodeName(MaterialDefFragment *def, QString &charName,
                        QString &palName, QString &partName)
{
    return explodeName(def->name(), charName, palName, partName);
}

QString WLDMaterialPalette::materialName(QString defName) const
{
    QString charName, palName, partName;
    if(explodeName(defName, charName, palName, partName))
        return charName + "00" + partName;
    else
        return defName.replace("_MDF", "");
}

QString WLDMaterialPalette::materialName(MaterialDefFragment *def) const
{
    return materialName(def->name());
}

Material * WLDMaterialPalette::material(QString name) const
{
    return m_materials.value(materialName(name));
}

void WLDMaterialPalette::importMaterial(QString key, MaterialDefFragment *frag)
{
    if(!frag || m_materials.contains(key))
        return;
    SpriteFragment *sprite = frag->m_sprite;
    if(!sprite)
        return;
    SpriteDefFragment *spriteDef = sprite->m_def;
    if(!spriteDef)
        return;
    BitmapNameFragment *bmp = spriteDef->m_bitmaps.value(0);
    if(!bmp)
        return;

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
    m_materials.insert(key, mat);
}
