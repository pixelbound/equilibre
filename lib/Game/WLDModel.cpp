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

#include <QImage>
#include <QRegExp>
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/WLDSkeleton.h"
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Material.h"

using namespace std;

WLDModel::WLDModel(WLDMesh *mainMesh)
{
    Q_ASSERT(mainMesh != NULL);
    m_buffer = 0;
    m_mainMesh = mainMesh;
    m_meshes.append(mainMesh);
    m_skel = 0;
    m_skin = 0;
    m_skin = newSkin("00");
}

WLDModel::~WLDModel()
{
    foreach(WLDModelSkin *skin, m_skins)
        delete skin;
    foreach(WLDMesh *mesh, m_meshes)
        delete mesh;
}

WLDMesh * WLDModel::mainMesh() const
{
    return m_mainMesh;
}

MeshBuffer * WLDModel::buffer() const
{
    return m_buffer;
}

void WLDModel::setBuffer(MeshBuffer *newBuffer)
{
    m_buffer = newBuffer;
}

WLDSkeleton * WLDModel::skeleton() const
{
    return m_skel;
}

WLDModelSkin * WLDModel::skin() const
{
    return m_skin;
}

void WLDModel::setSkeleton(WLDSkeleton *skeleton)
{
    m_skel = skeleton;
}

const QMap<QString, WLDModelSkin *> & WLDModel::skins() const
{
    return m_skins;
}

WLDModelSkin * WLDModel::newSkin(QString name)
{
    WLDModelSkin *skin = new WLDModelSkin(name, this);
    m_skins.insert(name, skin);
    return skin;
}

const QList<WLDMesh *> & WLDModel::meshes() const
{
    return m_meshes;
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

WLDMesh::WLDMesh(MeshDefFragment *meshDef, uint32_t partID)
{
    Q_ASSERT(meshDef != NULL);
    m_partID = partID;
    m_meshDef = meshDef;
    m_data = NULL;
    m_materials = NULL;
    m_palette = NULL;
    m_materialMap = NULL;
    m_boundsAA.low = meshDef->m_boundsAA.low + meshDef->m_center;
    m_boundsAA.high = meshDef->m_boundsAA.high + meshDef->m_center;
    meshDef->setHandled(true);
}

WLDMesh::~WLDMesh()
{
    delete m_materialMap;
    delete m_materials;
    delete m_palette;
}

MeshData * WLDMesh::data() const
{
    return m_data;
}

void WLDMesh::setData(MeshData *mesh)
{
    m_data = mesh;
}

MeshDefFragment * WLDMesh::def() const
{
    return m_meshDef;
}

MaterialArray * WLDMesh::materials() const
{
    return m_materials;
}

MaterialMap * WLDMesh::materialMap() const
{
    return m_materialMap;
}

void WLDMesh::setMaterialMap(MaterialMap *map)
{
    m_materialMap = map;
}

WLDMaterialPalette * WLDMesh::palette() const
{
    return m_palette;
}

uint32_t WLDMesh::partID() const
{
    return m_partID;
}

const AABox & WLDMesh::boundsAA() const
{
    return m_boundsAA;
}

WLDMaterialPalette * WLDMesh::importPalette(PFSArchive *archive)
{
    m_palette = new WLDMaterialPalette(archive);
    m_palette->setDef(m_meshDef->m_palette);
    m_palette->createSlots();
    return m_palette;
}

MaterialArray * WLDMesh::materialsFromPalette()
{
    if(!m_materials)
    {
        m_materials = new MaterialArray();
        m_palette->exportTo(m_materials);
    }
    return m_materials;
}

MeshData * WLDMesh::importFrom(MeshBuffer *meshBuf, uint32_t paletteOffset)
{
    MeshData *meshData = importMaterialGroups(meshBuf, paletteOffset);
    importVertexData(meshBuf, meshData->vertexSegment);
    importIndexData(meshBuf, meshData->indexSegment, meshData->vertexSegment,
                    0, (uint32_t)m_meshDef->m_indices.count());
    return meshData;
}

void WLDMesh::importVertexData(MeshBuffer *buffer, BufferSegment &dataLoc)
{
    // Update the location of the mesh in the buffer.
    QVector<Vertex> &vertices(buffer->vertices);
    uint32_t vertexCount = (uint32_t)m_meshDef->m_vertices.count();
    uint32_t vertexIndex = vertices.count();
    dataLoc.offset = vertexIndex;
    dataLoc.count = vertexCount;
    dataLoc.elementSize = sizeof(Vertex);
    
    // Load vertices, texCoords, normals, faces.
    bool hasColors = m_meshDef->m_colors.count() > 0;
    uint32_t defaultColorABGR = 0xbfffffff; // A=0.75, B=1, G=1, R=1
    for(uint32_t i = 0; i < vertexCount; i++)
    {
        Vertex v;
        v.position = m_meshDef->m_vertices.value(i) + m_meshDef->m_center;
        v.normal = m_meshDef->m_normals.value(i);
        v.texCoords = vec3(m_meshDef->m_texCoords.value(i), 0.0f);
        v.color = hasColors ? m_meshDef->m_colors.value(i) : defaultColorABGR;
        v.bone = 0;
        vertices.append(v);
    }
    
    // Load bone indices.
    foreach(vec2us g, m_meshDef->m_vertexPieces)
    {
        uint16_t count = g.first, pieceID = g.second;
        for(uint32_t i = 0; i < count; i++, vertexIndex++)
            vertices[vertexIndex].bone = pieceID;
    }
}

void WLDMesh::importIndexData(MeshBuffer *buffer, BufferSegment &indexLoc,
                                   const BufferSegment &dataLoc, uint32_t offset, uint32_t count)
{
    QVector<uint32_t> &indices = buffer->indices;
    indexLoc.offset = indices.count();
    indexLoc.count = count;
    indexLoc.elementSize = sizeof(uint32_t);
    for(uint32_t i = 0; i < count; i++)
        indices.push_back(m_meshDef->m_indices[i + offset] + dataLoc.offset);
}

MeshData * WLDMesh::importMaterialGroups(MeshBuffer *buffer, uint32_t paletteOffset)
{
    // Load material groups.
    uint32_t meshOffset = 0;
    m_data = buffer->createMesh(m_meshDef->m_polygonsByTex.count());
    for(int i = 0; i < m_meshDef->m_polygonsByTex.count(); i++)
    {
        vec2us g = m_meshDef->m_polygonsByTex[i];
        uint32_t vertexCount = g.first * 3;
        MaterialGroup &mg(m_data->matGroups[i]);
        mg.id = m_partID;
        mg.offset = meshOffset;
        mg.count = vertexCount;
        mg.matID = paletteOffset + g.second;
        meshOffset += vertexCount;
    }
    return m_data;
}

static bool materialGroupLessThan(const MaterialGroup &a, const MaterialGroup &b)
{
    return a.matID < b.matID;
}

MeshBuffer * WLDMesh::combine(const QVector<WLDMesh *> &meshes)
{
    // import each part (vertices and material groups) into a single vertex group
    MeshBuffer *meshBuf = new MeshBuffer();
    foreach(WLDMesh *mesh, meshes)
    {
        MeshData *meshData = mesh->importMaterialGroups(meshBuf, 0);
        meshBuf->addMaterialGroups(meshData);
        mesh->importVertexData(meshBuf, meshData->vertexSegment);
    }

    // sort the polygons per material and import indices
    qSort(meshBuf->matGroups.begin(), meshBuf->matGroups.end(), materialGroupLessThan);
    uint32_t indiceOffset = 0;
    for(int i = 0; i < meshBuf->matGroups.count(); i++)
    {
        MaterialGroup &mg(meshBuf->matGroups[i]);
        WLDMesh *mesh = meshes[mg.id];
        mesh->importIndexData(meshBuf, mesh->data()->indexSegment, mesh->data()->vertexSegment,
                              mg.offset, mg.count);
        mg.offset = indiceOffset;
        indiceOffset += mg.count;
    }

    // merge material groups with common material
    QVector<MaterialGroup> newGroups;
    MaterialGroup group;
    group.id = meshBuf->matGroups[0].id;
    group.offset = 0;
    group.count = 0;
    group.matID = meshBuf->matGroups[0].matID;
    for(int i = 0; i < meshBuf->matGroups.count(); i++)
    {
        MaterialGroup &mg(meshBuf->matGroups[i]);
        if(mg.matID != group.matID)
        {
            // new material - output the current group
            newGroups.append(group);
            group.id = mg.id;
            group.offset += group.count;
            group.count = 0;
            group.matID = mg.matID;
        }
        group.count += mg.count;
    }
    newGroups.append(group);
    meshBuf->matGroups = newGroups;
    return meshBuf;
}

////////////////////////////////////////////////////////////////////////////////

WLDModelSkin::WLDModelSkin(QString name, WLDModel *model)
{
    m_name = name;
    m_model = model;
    WLDModelSkin *defaultSkin = model->skin();
    if(defaultSkin)
    {
        foreach(WLDMesh *part, defaultSkin->parts())
            m_parts.append(part);
    }
    else
    {
        m_parts.append(model->mainMesh());
    }
    updateBounds();
}

WLDModelSkin::~WLDModelSkin()
{
}

QString WLDModelSkin::name() const
{
    return m_name;
}

const AABox & WLDModelSkin::boundsAA() const
{
    return m_boundsAA;
}

const QList<WLDMesh *> & WLDModelSkin::parts() const
{
    return m_parts;
}

WLDMesh * WLDModelSkin::addPart(MeshDefFragment *frag)
{
    if(!frag)
        return NULL;
    uint32_t partID = m_parts.count();
    WLDMesh *meshPart = new WLDMesh(frag, partID);
    m_parts.append(meshPart);
    m_model->m_meshes.append(meshPart);
    updateBounds();
    return meshPart;
}

void WLDModelSkin::replacePart(WLDMesh *basePart, MeshDefFragment *frag)
{
    uint32_t partID = basePart->partID();
    if(partID >= m_parts.size())
        return;
    if((basePart == m_parts[partID]) && (basePart->def() != frag))
    {
        WLDMesh *meshPart = new WLDMesh(frag, partID);
        m_parts[partID] = meshPart;
        m_model->m_meshes.append(meshPart);
    }
    updateBounds();
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

void WLDModelSkin::updateBounds()
{
    if(m_parts.count() == 0)
    {
        m_boundsAA = AABox();
    }
    else
    {
        m_boundsAA = m_parts[0]->boundsAA();
        for(int i = 1; i < m_parts.count(); i++)
        {
            // XXX sort out skinning
            m_boundsAA.extendTo(m_parts[i]->boundsAA());
        }
    }
}

void WLDModelSkin::draw(RenderProgram *prog, const QVector<BoneTransform> &bones,
                        MaterialMap *materialMap)
{
    MeshBuffer *meshBuf = m_model->buffer();
    if(!meshBuf)
        return;

    // Gather material groups from all the mesh parts we want to draw.
    meshBuf->matGroups.clear();
    foreach(WLDMesh *mesh, m_parts)
        meshBuf->addMaterialGroups(mesh->data());
    
    // Map the slot indices to material indices if requested.
    MaterialArray *materials = m_model->mainMesh()->materials();
    if(materialMap)
    {
        const uint32_t *mappings = materialMap->mappings();
        for(size_t i = 0; i < meshBuf->matGroups.size(); i++)
        {
            MaterialGroup &mg(meshBuf->matGroups[i]);
            mg.matID = materialMap->mappingAt(mg.matID);;
        }
        prog->setMaterialMap(materials, materialMap);
    }

    // Draw all the material groups in one draw call.
    prog->beginDrawMesh(meshBuf, materials, bones.constData(), bones.size());
    prog->drawMesh();
    prog->endDrawMesh();
    
    if(materialMap)
        prog->setMaterialMap(NULL);
}
