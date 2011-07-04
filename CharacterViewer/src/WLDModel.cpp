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

WLDModelPart::WLDModelPart(MeshDefFragment *meshDef, uint32_t partID, QObject *parent) : QObject(parent)
{
    m_partID = partID;
    m_meshDef = meshDef;
    m_mesh = 0;
}

WLDModelPart::~WLDModelPart()
{
    delete m_mesh;
}

VertexGroup * WLDModelPart::mesh() const
{
    return m_mesh;
}

MeshDefFragment * WLDModelPart::def() const
{
    return m_meshDef;
}

void WLDModelPart::draw(RenderState *state, WLDModelSkin *skin, const BoneTransform *bones, uint32_t boneCount)
{
    if(!m_mesh)
    {
        m_mesh = new VertexGroup(GL_TRIANGLES, m_meshDef->m_vertices.count());
        importVertexData(m_mesh, 0);
        importMaterialGroups(m_mesh, 0, skin);
        // load indices
        for(uint32_t i = 0; i < (uint32_t)m_meshDef->m_indices.count(); i++)
            m_mesh->indices.push_back(m_meshDef->m_indices[i]);
    }
    state->drawMesh(m_mesh, skin->palette(), bones, boneCount);
}

void WLDModelPart::importVertexData(VertexGroup *vg, uint32_t offset)
{
    // load vertices, texCoords, normals, faces
    VertexData *vd = vg->data + offset;
    for(uint32_t i = 0; i < (uint32_t)m_meshDef->m_vertices.count(); i++, vd++)
    {
        vd->position = m_meshDef->m_vertices.value(i) + m_meshDef->m_center;
        vd->normal = m_meshDef->m_normals.value(i);
        vd->texCoords = m_meshDef->m_texCoords.value(i);
        vd->bone = 0;
    }
    // load bone indices
    vd = vg->data + offset;
    foreach(vec2us g, m_meshDef->m_vertexPieces)
    {
        uint16_t count = g.first, pieceID = g.second;
        for(uint32_t i = 0; i < count; i++, vd++)
            vd->bone = pieceID;
    }
}

void WLDModelPart::importMaterialGroups(VertexGroup *vg, uint32_t offset, WLDModelSkin *skin)
{
    // load material groups
    MaterialPaletteFragment *palDef = m_meshDef->m_palette;
    uint32_t pos = offset;
    foreach(vec2us g, m_meshDef->m_polygonsByTex)
    {
        MaterialDefFragment *matDef = palDef->m_materials[g.second];
        uint32_t vertexCount = g.first * 3;
        MaterialGroup mg;
        mg.id = m_partID;
        mg.offset = pos;
        mg.count = vertexCount;
        // invisible groups have no material
        if((matDef->m_param1 == 0) || !skin || !skin->palette())
            mg.matName = QString::null;
        else
            mg.matName = skin->palette()->materialName(palDef->m_materials[g.second]);
        vg->matGroups.append(mg);
        pos += vertexCount;
    }
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
    m_aggregMesh = 0;
    WLDModelSkin *defaultSkin = model->skin();
    if(defaultSkin)
    {
        foreach(WLDModelPart *part, defaultSkin->parts())
            addPart(part->def());
    }
}

WLDModelSkin::~WLDModelSkin()
{
    delete m_aggregMesh;
}

QString WLDModelSkin::name() const
{
    return m_name;
}

WLDMaterialPalette *WLDModelSkin::palette() const
{
    return m_palette;
}

const QList<WLDModelPart *> & WLDModelSkin::parts() const
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
    m_parts.append(new WLDModelPart(frag, partID, m_model));
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

static bool materialGroupLessThan(const MaterialGroup &a, const MaterialGroup &b)
{
    return a.matName < b.matName;
}

void WLDModelSkin::combineParts()
{
    // sum the vertice count for each part
    uint32_t totalVertices = 0;
    foreach(WLDModelPart *part, m_parts)
        totalVertices += part->def()->m_vertices.count();

    // import each part (vertices and material groups) into a single vertex group
    VertexGroup *vg = new VertexGroup(GL_TRIANGLES, totalVertices);
    uint32_t dataOffset = 0, indiceOffset = 0;
    QVector<uint32_t> partDataOffsets, partIndiceOffsets;
    foreach(WLDModelPart *part, m_parts)
    {
        part->importMaterialGroups(vg, indiceOffset, this);
        part->importVertexData(vg, dataOffset);
        partDataOffsets.append(dataOffset);
        partIndiceOffsets.append(indiceOffset);
        dataOffset += part->def()->m_vertices.count();
        indiceOffset += part->def()->m_indices.count();
    }

    // sort the polygons per material and reorder indices
    qSort(vg->matGroups.begin(), vg->matGroups.end(), materialGroupLessThan);
    indiceOffset = 0;
    for(int i = 0; i < vg->matGroups.count(); i++)
    {
        MaterialGroup &mg(vg->matGroups[i]);
        WLDModelPart *part = m_parts[mg.id];
        uint32_t partDataOffset = partDataOffsets[mg.id];
        uint32_t partIndiceOffset = partIndiceOffsets[mg.id];
        uint32_t groupOffset = mg.offset - partIndiceOffset;
        const QVector<uint16_t> &indices(part->def()->m_indices);
        for(uint32_t i = 0; i < mg.count; i++)
        {
            uint32_t indice = indices[groupOffset + i] + partDataOffset;
            vg->indices.append(indice);
        }
        mg.offset = indiceOffset;
        indiceOffset += mg.count;
    }

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

    // copy the vertex group to the GPU
    uint32_t dataSize = totalVertices * sizeof(VertexData);
    glGenBuffers(1, &vg->dataBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vg->dataBuffer);
    glBufferData(GL_ARRAY_BUFFER, dataSize, vg->data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //uint32_t indicesSize = vg->indices.count() * sizeof(uint32_t);
    //uint32_t buffers[2];
    //glGenBuffers(2, buffers);
    //glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    //glBufferData(GL_ARRAY_BUFFER, dataSize, vg->data, GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    //glBufferData(GL_ARRAY_BUFFER, indicesSize, vg->indices.data(), GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //vg->dataBuffer = buffers[0];
    //vg->indicesBuffer = buffers[1];

    m_aggregMesh = vg;
}

void WLDModelSkin::draw(RenderState *state, const BoneTransform *bones, uint32_t boneCount)
{
    if(m_aggregMesh)
    {
        state->drawMesh(m_aggregMesh, m_palette, bones, boneCount);
    }
    else
    {
        foreach(WLDModelPart *part, m_parts)
            part->draw(state, this, bones, boneCount);
    }
}
