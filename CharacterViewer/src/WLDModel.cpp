#include <QImage>
#include <QRegExp>
#include "WLDModel.h"
#include "Fragments.h"
#include "WLDSkeleton.h"
#include "PFSArchive.h"
#include "RenderState.h"
#include "Material.h"

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

WLDMesh::WLDMesh(MeshDefFragment *meshDef, uint32_t partID, QObject *parent) : QObject(parent)
{
    m_partID = partID;
    m_meshDef = meshDef;
    m_data = new VertexGroup(VertexGroup::Triangle);
    m_boundsAA.low = meshDef->m_boundsAA.low + meshDef->m_center;
    m_boundsAA.high = meshDef->m_boundsAA.high + meshDef->m_center;
}

WLDMesh::~WLDMesh()
{
    delete m_data;
}

VertexGroup * WLDMesh::data() const
{
    return m_data;
}

MeshDefFragment * WLDMesh::def() const
{
    return m_meshDef;
}

const AABox & WLDMesh::boundsAA() const
{
    return m_boundsAA;
}

void WLDMesh::beginDraw(RenderState *state,  WLDMaterialPalette *palette,
                        const BoneTransform *bones, uint32_t boneCount)
{
    if(m_data->vertices.count() == 0)
    {
        importVertexData(m_data, m_data->vertexBuffer);
        importMaterialGroups(m_data, palette);
        importIndexData(m_data, m_data->indexBuffer, m_data->vertexBuffer,
                        0, (uint32_t)m_meshDef->m_indices.count());
    }
    state->beginDrawMesh(m_data, palette, bones, boneCount);
}

void WLDMesh::draw(RenderState *state)
{
    state->drawMesh();
}

void WLDMesh::endDraw(RenderState *state)
{
    state->endDrawMesh();
}

void WLDMesh::importVertexData(VertexGroup *vg, BufferSegment &dataLoc)
{
    // update the mesh location
    QVector<VertexData> &vertices(vg->vertices);
    uint32_t vertexCount = (uint32_t)m_meshDef->m_vertices.count();
    uint32_t vertexIndex = vertices.count();
    dataLoc.offset = vertexIndex;
    dataLoc.count = vertexCount;
    dataLoc.elementSize = sizeof(VertexData);
    
    // load vertices, texCoords, normals, faces
    for(uint32_t i = 0; i < vertexCount; i++)
    {
        VertexData vd;
        vd.position = m_meshDef->m_vertices.value(i) + m_meshDef->m_center;
        vd.normal = m_meshDef->m_normals.value(i);
        vd.texCoords = m_meshDef->m_texCoords.value(i);
        vd.bone = 0;
        vertices.append(vd);
    }
    // load bone indices
    foreach(vec2us g, m_meshDef->m_vertexPieces)
    {
        uint16_t count = g.first, pieceID = g.second;
        for(uint32_t i = 0; i < count; i++, vertexIndex++)
            vertices[i].bone = pieceID;
    }
}

void WLDMesh::importIndexData(VertexGroup *vg, BufferSegment &indexLoc,
                                   const BufferSegment &dataLoc, uint32_t offset, uint32_t count)
{
    indexLoc.offset = vg->indices.count();
    indexLoc.count = count;
    indexLoc.elementSize = sizeof(uint32_t);
    for(uint32_t i = 0; i < count; i++)
        vg->indices.push_back(m_meshDef->m_indices[i + offset] + dataLoc.offset);
}

void WLDMesh::importMaterialGroups(VertexGroup *vg, WLDMaterialPalette *palette)
{
    // load material groups
    MaterialPaletteFragment *palDef = m_meshDef->m_palette;
    uint32_t meshOffset = 0;
    foreach(vec2us g, m_meshDef->m_polygonsByTex)
    {
        MaterialDefFragment *matDef = palDef->m_materials[g.second];
        uint32_t vertexCount = g.first * 3;
        MaterialGroup mg;
        mg.id = m_partID;
        mg.offset = meshOffset;
        mg.count = vertexCount;
        // invisible groups have no material
        if((matDef->m_param1 == 0) || !palette)
            mg.matName = QString::null;
        else
            mg.matName = palette->materialName(palDef->m_materials[g.second]);
        vg->matGroups.append(mg);
        meshOffset += vertexCount;
    }
}

static bool materialGroupLessThan(const MaterialGroup &a, const MaterialGroup &b)
{
    return a.matName < b.matName;
}

VertexGroup * WLDMesh::combine(const QList<WLDMesh *> &meshes, WLDMaterialPalette *palette)
{
    // import each part (vertices and material groups) into a single vertex group
    VertexGroup *vg = new VertexGroup(VertexGroup::Triangle);
    foreach(WLDMesh *mesh, meshes)
    {
        mesh->importMaterialGroups(vg, palette);
        mesh->importVertexData(vg, mesh->data()->vertexBuffer);
    }
    vg->vertexBuffer.offset = 0;
    vg->vertexBuffer.count = vg->vertices.count();
    vg->vertexBuffer.elementSize = sizeof(VertexData);

    // sort the polygons per material and import indices
    qSort(vg->matGroups.begin(), vg->matGroups.end(), materialGroupLessThan);
    uint32_t indiceOffset = 0;
    for(int i = 0; i < vg->matGroups.count(); i++)
    {
        MaterialGroup &mg(vg->matGroups[i]);
        WLDMesh *mesh = meshes[mg.id];
        mesh->importIndexData(vg, mesh->data()->indexBuffer, mesh->data()->vertexBuffer,
                              mg.offset, mg.count);
        mg.offset = indiceOffset;
        indiceOffset += mg.count;
    }
    vg->indexBuffer.offset = 0;
    vg->indexBuffer.count = indiceOffset;
    vg->indexBuffer.elementSize = sizeof(uint32_t);

    // merge material groups with common material
    QVector<MaterialGroup> newGroups;
    MaterialGroup group;
    group.id = vg->matGroups[0].id;
    group.offset = 0;
    group.count = 0;
    group.matName = vg->matGroups[0].matName;
    for(int i = 0; i < vg->matGroups.count(); i++)
    {
        MaterialGroup &mg(vg->matGroups[i]);
        if(mg.matName != group.matName)
        {
            // new material - output the current group
            newGroups.append(group);
            group.id = mg.id;
            group.offset += group.count;
            group.count = 0;
            group.matName = mg.matName;
        }
        group.count += mg.count;
    }
    newGroups.append(group);
    vg->matGroups = newGroups;
    return vg;
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

    bool opaque = true;
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
    if(renderMode == 0x01)
    {
        // normal rendering
    }
    else if(renderMode == 0x13)
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
        opaque = false;
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
        opaque = false;
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
        opaque = false;
    }
    else
    {
        qDebug("Unknown render mode %x", renderMode);
    }
    // 0x53 == ?

    float ambient = frag->m_scaledAmbient;
    Material *mat = new Material();
    mat->setAmbient(vec4(ambient, ambient, ambient, 1.0));
    mat->setDiffuse(vec4(1.0, 1.0, 1.0, 1.0));
    mat->setOpaque(opaque);
    mat->loadTexture(img, true, !dds);
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
        foreach(WLDMesh *part, defaultSkin->parts())
            addPart(part->def());
    }
}

WLDModelSkin::~WLDModelSkin()
{
}

QString WLDModelSkin::name() const
{
    return m_name;
}

WLDMaterialPalette *WLDModelSkin::palette() const
{
    return m_palette;
}

const QList<WLDMesh *> & WLDModelSkin::parts() const
{
    return m_parts;
}

void WLDModelSkin::addPart(MeshDefFragment *frag, bool importPalette)
{
    if(!frag)
        return;
    if(importPalette)
        m_palette->addPaletteDef(frag->m_palette);
    uint32_t partID = m_parts.count();
    m_parts.append(new WLDMesh(frag, partID, m_model));
}

bool WLDModelSkin::explodeMeshName(QString defName, QString &actorName,
                            QString &meshName, QString &skinName)
{
    // e.g. defName == 'ELEHE00_DMSPRITEDEF'
    // 'ELE' : character
    // 'HE' : mesh
    // '00' : skin ID
    static QRegExp r("^(\\w{3})(.*)(\\d{2})_DMSPRITEDEF$");
    if(r.exactMatch(defName))
    {
        actorName = r.cap(1);
        meshName = r.cap(2);
        skinName = r.cap(3);
        return true;
    }
    return false;
}

void WLDModelSkin::draw(RenderState *state, const BoneTransform *bones, uint32_t boneCount)
{
    // XXX
    //foreach(WLDModelPart *part, m_parts)
    //    part->draw(state, this->palette(), bones, boneCount);
}
