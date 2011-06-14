#include <QImage>
#include <QRegExp>
#include "WLDModel.h"
#include "Fragments.h"
#include "WLDSkeleton.h"
#include "PFSArchive.h"
#include "RenderState.h"

WLDModel::WLDModel(PFSArchive *archive, QObject *parent) : QObject(parent)
{
    m_skel = 0;
    m_skin = 0;
    m_skin = newSkin("00", archive);
}

WLDModel::~WLDModel()
{
}

WLDSkeleton * WLDModel::skeleton() const
{
    return m_skel;
}

WLDModelSkin *WLDModel::skin() const
{
    return m_skin;
}

void WLDModel::setSkeleton(WLDSkeleton *skeleton)
{
    m_skel = skeleton;
}

QMap<QString, WLDModelSkin *> & WLDModel::skins()
{
    return m_skins;
}

const QMap<QString, WLDModelSkin *> & WLDModel::skins() const
{
    return m_skins;
}

WLDModelSkin * WLDModel::newSkin(QString name, PFSArchive *archive)
{
    WLDModelSkin *skin = new WLDModelSkin(name, this, archive, this);
    m_skins.insert(name, skin);
    return skin;
}

QList<MeshDefFragment *> WLDModel::listMeshes(ActorDefFragment *def)
{
    QList<MeshDefFragment *> meshes;
    if(!def)
        return meshes;
    foreach(WLDFragment *modelFrag, def->m_models)
    {
        MeshFragment *meshFrag = modelFrag->cast<MeshFragment>();
        if(meshFrag)
            meshes.append(meshFrag->m_def);
        HierSpriteFragment *skelFrag = modelFrag->cast<HierSpriteFragment>();
        if(skelFrag)
        {
            foreach(MeshFragment *meshFrag2, skelFrag->m_def->m_meshes)
            {
                if(meshFrag2->m_def)
                   meshes.append(meshFrag2->m_def);
            }
        }
    }
    return meshes;
}

////////////////////////////////////////////////////////////////////////////////

WLDModelPart::WLDModelPart(MeshDefFragment *meshDef, QObject *parent) : QObject(parent)
{
    m_meshDef = meshDef;
    m_mesh = 0;
}

MeshDefFragment * WLDModelPart::def() const
{
    return m_meshDef;
}

void WLDModelPart::draw(RenderState *state, WLDModelSkin *skin, const BoneTransform *bones, uint32_t boneCount)
{
    if(!m_mesh)
    {
        m_mesh = state->createMesh();
        importMaterialGroups(m_mesh, skin);
    }
    state->pushMatrix();
    state->translate(m_meshDef->m_center);
    state->drawMesh(m_mesh, bones, boneCount);
    state->popMatrix();
}

void WLDModelPart::importMaterialGroups(Mesh *m, WLDModelSkin *skin)
{
    // load vertices, texCoords, normals, faces
    VertexGroup *vg = new VertexGroup(GL_TRIANGLES, m_meshDef->m_vertices.count());
    VertexData *vd = vg->data;
    for(uint32_t i = 0; i < vg->count; i++, vd++)
    {
        vd->position = m_meshDef->m_vertices.value(i);
        vd->normal = m_meshDef->m_normals.value(i);
        vd->texCoords = m_meshDef->m_texCoords.value(i);
        vd->bone = 0;
    }
    for(uint32_t i = 0; i < (uint32_t)m_meshDef->m_indices.count(); i++)
        vg->indices.push_back(m_meshDef->m_indices[i]);

    // load material groups
    MaterialPaletteFragment *palDef = m_meshDef->m_palette;
    uint32_t pos = 0;
    foreach(vec2us g, m_meshDef->m_polygonsByTex)
    {
        MaterialDefFragment *matDef = palDef->m_materials[g.second];
        uint32_t vertexCount = g.first * 3;
        MaterialGroup mg;
        mg.offset = pos;
        mg.count = vertexCount;
        // invisible groups have no material
        if((matDef->m_param1 == 0) || !skin || !skin->palette())
        {
            mg.palette = 0;
        }
        else
        {
            mg.matName = skin->palette()->materialName(palDef->m_materials[g.second]);
            mg.palette = skin->palette();
        }
        vg->matGroups.append(mg);
        pos += vertexCount;
    }

    // load bone indices
    vd = vg->data;
    foreach(vec2us g, m_meshDef->m_vertexPieces)
    {
        uint16_t count = g.first, pieceID = g.second;
        for(uint32_t i = 0; i < count; i++, vd++)
            vd->bone = pieceID;
    }
    m->setGroup(vg);
}

////////////////////////////////////////////////////////////////////////////////

WLDMaterialPalette::WLDMaterialPalette(PFSArchive *archive, QObject *parent) : QObject(parent)
{
    m_archive = archive;
}

void WLDMaterialPalette::addPaletteDef(MaterialPaletteFragment *def)
{
    if(!def)
        return;
    foreach(MaterialDefFragment *matDef, def->m_materials)
        addMaterialDef(matDef);
}

QString WLDMaterialPalette::addMaterialDef(MaterialDefFragment *def)
{
    if(!def)
        return QString::null;
    QString name = materialName(def);
    m_materialDefs.insert(name, def);
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

Material * WLDMaterialPalette::material(QString name)
{
    QString canName = materialName(name);
    Material *mat = m_materials.value(canName);
    if(mat)
        return mat;
    MaterialDefFragment *matDef = m_materialDefs.value(canName);
    if(!matDef)
        return 0;
    mat = loadMaterial(matDef);
    m_materials.insert(canName, mat);
    return mat;
}

Material * WLDMaterialPalette::loadMaterial(MaterialDefFragment *frag)
{
    if(!frag)
        return 0;
    SpriteFragment *sprite = frag->m_sprite;
    if(!sprite)
        return 0;
    SpriteDefFragment *spriteDef = sprite->m_def;
    if(!spriteDef)
        return 0;
    BitmapNameFragment *bmp = spriteDef->m_bitmaps.value(0);
    if(!bmp)
        return 0;

    bool dds = false;
    QImage img;
    if(m_archive)
    {
        // XXX case-insensitive lookup
        QByteArray data = m_archive->unpackFile(bmp->m_fileName.toLower());
        if(!img.loadFromData(data))
        {
            Material::loadTextureDDS(data.constData(), data.length(), img);
            dds = true;
        }
    }

    // masked bitmap?
    uint32_t renderMode = (frag->m_param1 & 0xff);
    //qDebug("'%s' has param1 %02x", bmp->m_fileName.toLatin1().constData(), frag->m_param1 & 0xff);
    if(renderMode == 0x13)
    {
        // masked texture
        if(img.colorCount() > 0)
        {
            // replace the mask color by a transparent color in the table
            QVector<QRgb> colors = img.colorTable();
            uint8_t maskColor = 0;
            colors[maskColor] = qRgba(0, 0, 0, 0);
            img.setColorTable(colors);
        }
    }
    else if(renderMode == 0x17)
    {
        // semi-transparent (e.g. the sleeper, wasp, bixie)
        // depends on how dark/light the color is
        if(img.colorCount() > 0)
        {
            QVector<QRgb> colors = img.colorTable();
            for(int i = 0; i < colors.count(); i++)
            {
                QRgb c = colors[i];
                int alpha = (qRed(c) + qGreen(c) + qBlue(c)) / 3;
                colors[i] = qRgba(qRed(c), qGreen(c), qBlue(c), alpha);
            }
            img.setColorTable(colors);
        }
    }
    else if(renderMode == 0x05)
    {
        // semi-transparent (water elemental, air elemental, ghost wolf)
        if(img.colorCount() > 0)
        {
            QVector<QRgb> colors = img.colorTable();
            int alpha = 127; // arbitrary value XXX find the real value
            for(int i = 0; i < colors.count(); i++)
            {
                QRgb c = colors[i];
                colors[i] = qRgba(qRed(c), qGreen(c), qBlue(c), alpha);
            }
            img.setColorTable(colors);
        }
    }
    // 0x53 == ?

    float ambient = frag->m_scaledAmbient;
    Material *mat = new Material();
    mat->setAmbient(vec4(ambient, ambient, ambient, 1.0));
    mat->setDiffuse(vec4(1.0, 1.0, 1.0, 1.0));
    mat->loadTexture(img, false, !dds);
    return mat;
}

////////////////////////////////////////////////////////////////////////////////

WLDModelSkin::WLDModelSkin(QString name, WLDModel *model, PFSArchive *archive, QObject *parent) : QObject(parent)
{
    m_name = name;
    m_model = model;
    m_palette = new WLDMaterialPalette(archive, this);
    WLDModelSkin *defaultSkin = model->skin();
    if(defaultSkin)
    {
        foreach(WLDModelPart *part, defaultSkin->parts())
            addPart(part->def());
    }
}

QString WLDModelSkin::name() const
{
    return m_name;
}

WLDMaterialPalette *WLDModelSkin::palette() const
{
    return m_palette;
}

const QMap<QString, WLDModelPart *> & WLDModelSkin::parts() const
{
    return m_parts;
}

void WLDModelSkin::addPart(MeshDefFragment *frag, bool importPalette)
{
    if(!frag)
        return;
    QString actorName, meshName, skinName;
    explodeMeshName(frag->name(), actorName, meshName, skinName);
    if(importPalette)
        m_palette->addPaletteDef(frag->m_palette);
    m_parts.insert(meshName, new WLDModelPart(frag, m_model));
}

bool WLDModelSkin::explodeMeshName(QString defName, QString &actorName,
                            QString &meshName, QString &skinName)
{
    // e.g. defName == 'ELEHE00_DMSPRITEDEF'
    // 'ELE' : character
    // 'HE' : mesh
    // '00' : skin ID
    static QRegExp r("^(\\w{3})(.*)(\\d{2})_DMSPRITEDEF$");
    static QRegExp r2("^(\\w{3})(.*)_DMSPRITEDEF$");
    if(r.exactMatch(defName))
    {
        actorName = r.cap(1);
        meshName = r.cap(2);
        skinName = r.cap(3);
        return true;
    }
    else if(r2.exactMatch(defName))
    {
        actorName = r2.cap(1);
        meshName = QString::null;
        skinName = r2.cap(2);
        return true;
    }
    return false;
}

void WLDModelSkin::draw(RenderState *state, const BoneTransform *bones, uint32_t boneCount)
{
    foreach(WLDModelPart *part, m_parts)
        part->draw(state, this, bones, boneCount);
}
