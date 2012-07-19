#include <QFileInfo>
#include <QImage>
#include "Zone.h"
#include "PFSArchive.h"
#include "WLDData.h"
#include "WLDModel.h"
#include "WLDActor.h"
#include "WLDSkeleton.h"
#include "Fragments.h"
#include "RenderState.h"
#include "Material.h"

Zone::Zone(QObject *parent) : QObject(parent)
{
    m_mainArchive = 0;
    m_objMeshArchive = 0;
    m_charArchive = 0;
    m_mainWld = 0;
    m_objMeshWld = m_objDefWld = 0;
    m_charWld = 0;
    m_zoneGeometry = 0;
    m_zoneMaterials = 0;
    m_playerPos = vec3(0.0, 0.0, 0.0);
    m_playerOrient = 0.0;
    m_cameraPos = vec3(0.0, 0.0, 0.0);
    m_cameraOrient = vec3(0.0, 0.0, 0.0);
    m_showObjects = true;
    m_cullObjects = true;
    m_objectsGeometry = NULL;
    m_index = new ActorIndex();
}

Zone::~Zone()
{
    clear();
    delete m_index;
}

const QMap<QString, WLDMesh *> & Zone::objectModels() const
{
    return m_objModels;
}

const QMap<QString, WLDActor *> & Zone::charModels() const
{
    return m_charModels;
}

const QList<WLDZoneActor> & Zone::actors() const
{
    return m_index->actors();
}

bool Zone::load(QString path, QString name)
{
    m_name = name;

    // load archives and WLD files
    QString zonePath = QString("%1/%2.s3d").arg(path).arg(name);
    QString zoneFile = QString("%1.wld").arg(name);
    m_mainArchive = new PFSArchive(zonePath, this);
    m_mainWld = WLDData::fromArchive(m_mainArchive, zoneFile, this);
    m_objDefWld = WLDData::fromArchive(m_mainArchive, "objects.wld", this);
    if(!m_mainWld || !m_objDefWld)
        return false;
    QString objMeshPath = QString("%1/%2_obj.s3d").arg(path).arg(name);
    QString objMeshFile = QString("%1_obj.wld").arg(name);
    m_objMeshArchive = new PFSArchive(objMeshPath, this);
    m_objMeshWld = WLDData::fromArchive(m_objMeshArchive, objMeshFile, this);
    if(!m_objMeshWld)
        return false;
    QString charPath = QString("%1/%2_chr.s3d").arg(path).arg(name);
    QString charFile = QString("%1_chr.wld").arg(name);
    m_charArchive = new PFSArchive(charPath, this);
    m_charWld = WLDData::fromArchive(m_charArchive, charFile, this);
    if(!m_charWld)
    {
        delete m_charArchive;
        m_charArchive = 0;
    }

    // import geometry, objects, characters
    importGeometry();
    importObjects();
    if(m_charWld)
    {
        importCharacters(m_charArchive, m_charWld);
        importCharacterPalettes(m_charArchive, m_charWld);
        importSkeletons(m_charArchive, m_charWld);
    }
    return true;
}

bool Zone::loadCharacters(QString archivePath, QString wldName)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        if(baseName == "global_chr1")
            baseName = "global_chr";
        wldName = baseName + ".wld";
    }
    m_charArchive = new PFSArchive(archivePath, this);
    m_charWld = WLDData::fromArchive(m_charArchive, wldName, this);
    if(!m_charWld)
        return false;
    importCharacters(m_charArchive, m_charWld);
    importCharacterPalettes(m_charArchive, m_charWld);
    importSkeletons(m_charArchive, m_charWld);
    return true;
}

void Zone::clear()
{
    m_index->clear();
    foreach(WLDMesh *model, m_objModels)
        delete model;
    foreach(WLDActor *actor, m_charModels)
        delete actor;
    m_objModels.clear();
    m_charModels.clear();
    delete m_objectsGeometry;
    delete m_zoneGeometry;
    delete m_zoneMaterials;
    delete m_mainWld;
    delete m_objMeshWld;
    delete m_objDefWld;
    delete m_charWld;
    delete m_mainArchive;
    delete m_objMeshArchive;
    delete m_charArchive;
    m_objectsGeometry = 0;
    m_zoneGeometry = 0;
    m_zoneMaterials = 0;
    m_mainWld = 0;
    m_objMeshWld = 0;
    m_objDefWld = 0;
    m_charWld = 0;
    m_mainArchive = 0;
    m_objMeshArchive = 0;
    m_charArchive = 0;
}

void Zone::importGeometry()
{
    // load zone regions as model parts
    QList<WLDMesh *> parts;
    int partID = 0;
    foreach(MeshDefFragment *meshDef, m_mainWld->fragmentsByType<MeshDefFragment>())
        parts.append(new WLDMesh(meshDef, partID++));
    // load zone textures into the material palette
    WLDMaterialPalette zonePalette(m_mainArchive, this);
    foreach(MaterialDefFragment *matDef, m_mainWld->fragmentsByType<MaterialDefFragment>())
        zonePalette.addMaterialDef(matDef);
    m_zoneMaterials = zonePalette.loadMaterials();
    // combine all regions into a single mesh
    m_zoneGeometry = WLDMesh::combine(parts);
    foreach(WLDMesh *part, parts)
        delete part;
    int maxWidth = 0, maxHeight = 0;
    size_t totalMem = 0, usedMem = 0;
    m_zoneMaterials->textureArrayInfo(maxWidth, maxHeight, totalMem, usedMem);
    float usage = (float)usedMem / (float)totalMem;
    qDebug("Zone texture array: %d used / %d allocated (%f)", usedMem, totalMem, usage);
}

void Zone::importObjects()
{
    importObjectsMeshes(m_objMeshArchive, m_objMeshWld);

    // import actors through Actor fragments
    foreach(ActorFragment *actorFrag, m_objDefWld->fragmentsByType<ActorFragment>())
    {
        QString actorName = actorFrag->m_def.name();
        WLDMesh *model = m_objModels.value(actorName);
        if(model)
        {
            WLDZoneActor actor(actorFrag, model);
            m_index->add(actor);
        }
        else
        {
            qDebug("Actor '%s' not found", actorName.toLatin1().constData());
        }
    }
}

void Zone::importObjectsMeshes(PFSArchive *archive, WLDData *wld)
{
    // import models through ActorDef fragments
    foreach(ActorDefFragment *actorDef, wld->fragmentsByType<ActorDefFragment>())
    {
        WLDFragment *subModel = actorDef->m_models.value(0);
        if(!subModel)
            continue;
        MeshFragment *mesh = subModel->cast<MeshFragment>();
        if(!mesh)
        {
            if(subModel->kind() == HierSpriteFragment::ID)
                qDebug("Hierarchical model in zone objects (%s)",
                       actorDef->name().toLatin1().constData());
            continue;
        }
        WLDMesh *model = new WLDMesh(mesh->m_def, 0, this);
        WLDMaterialPalette palette(archive, this);
        palette.addPaletteDef(mesh->m_def->m_palette);
        model->setMaterials(palette.loadMaterials());
        m_objModels.insert(actorDef->name(), model);
    }
}

void Zone::importSkeletons(PFSArchive *archive, WLDData *wld)
{
    // import skeletons which contain the pose animation
    foreach(HierSpriteDefFragment *skelDef, wld->fragmentsByType<HierSpriteDefFragment>())
    {
        QString actorName = skelDef->name().replace("_HS_DEF", "");
        WLDActor *actor = m_charModels.value(actorName);
        if(!actor)
            continue;
        actor->model()->setSkeleton(new WLDSkeleton(skelDef, this));
    }

    // import other animations
    foreach(TrackFragment *track, wld->fragmentsByType<TrackFragment>())
    {
        QString animName = track->name().left(3);
        QString actorName = track->name().mid(3, 3);
        WLDActor *actor = m_charModels.value(actorName);
        if(!actor)
            continue;
        WLDSkeleton *skel = actor->model()->skeleton();
        if(skel && track->m_def)
            skel->addTrack(animName, track->m_def);
    }
}

void Zone::importCharacterPalettes(PFSArchive *archive, WLDData *wld)
{
    foreach(MaterialDefFragment *matDef, wld->fragmentsByType<MaterialDefFragment>())
    {
        QString charName, palName, partName;
        if(WLDMaterialPalette::explodeName(matDef, charName, palName, partName))
        {
            WLDActor *actor = m_charModels.value(charName);
            if(!actor)
                continue;
            WLDModel *model = actor->model();
            WLDModelSkin *skin = model->skins().value(palName);
            if(!skin)
            {
                skin = model->newSkin(palName, archive);
                skin->setPalette(new WLDMaterialPalette(archive));
            }
            skin->palette()->addMaterialDef(matDef);
        }
    }

    // look for alternate meshes (e.g. heads)
    foreach(MeshDefFragment *meshDef, wld->fragmentsByType<MeshDefFragment>())
    {
        QString actorName, meshName, skinName;
        WLDModelSkin::explodeMeshName(meshDef->name(), actorName, meshName, skinName);
        WLDActor *actor = m_charModels.value(actorName);
        if(!actor || meshName.isEmpty())
            continue;
        WLDModel *model = actor->model();
        WLDModelSkin *skin = model->skins().value(skinName);
        if(!skin)
            continue;
        foreach(WLDMesh *part, model->skin()->parts())
        {
            QString actorName2, meshName2, skinName2;
            WLDModelSkin::explodeMeshName(part->def()->name(), actorName2, meshName2, skinName2);
            if(meshName2 == meshName)
                skin->addPart(meshDef, false);
        }
    }
}

void Zone::importCharacters(PFSArchive *archive, WLDData *wld)
{
    foreach(ActorDefFragment *actorDef, wld->fragmentsByType<ActorDefFragment>())
    {
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        WLDModel *model = new WLDModel(archive, this);
        WLDActor *actor = new WLDActor(model, this);
        WLDModelSkin *skin = model->skin();
        foreach(MeshDefFragment *meshDef, WLDModel::listMeshes(actorDef))
            skin->addPart(meshDef);
        foreach(WLDFragment *frag, actorDef->m_models)
        {
            switch(frag->kind())
            {
            case HierSpriteFragment::ID:
            case MeshFragment::ID:
                break;
            default:
                qDebug("Unknown model fragment kind (0x%02x) %s",
                       frag->kind(), actorName.toLatin1().constData());
                break;
            }
        }

        m_charModels.insert(actorName, actor);
    }
}

void Zone::createGPUBuffer(VertexGroup *vg, RenderState *state)
{
    if(!vg->vertexBuffer.buffer)
    {
        vg->vertexBuffer.buffer = state->createBuffer(vg->vertices.constData(), vg->vertexBuffer.size());
        if(vg->vertexBuffer.buffer)
            m_gpuBuffers.append(vg->vertexBuffer.buffer);
    }
    if(!vg->indexBuffer.buffer)
    {
        vg->indexBuffer.buffer = state->createBuffer(vg->indices.constData(), vg->indexBuffer.size());
        if(vg->indexBuffer.buffer)
            m_gpuBuffers.append(vg->indexBuffer.buffer);
    }
}

static bool zoneActorGroupLessThan(const WLDZoneActor *a, const WLDZoneActor *b)
{
    return a->mesh->def()->name() < b->mesh->def()->name();
}

void Zone::draw(RenderState *state)
{
    vec3 rot = vec3(0.0, 0.0, m_playerOrient) + m_cameraOrient;
    matrix4 viewMat = matrix4::rotate(rot.x, 1.0, 0.0, 0.0) *
        matrix4::rotate(rot.y, 0.0, 1.0, 0.0) *
        matrix4::rotate(rot.z, 0.0, 0.0, 1.0);
    Frustum &frustum = state->viewFrustum();
    frustum.setEye(-m_playerPos);
    frustum.setFocus(-m_playerPos + viewMat.map(vec3(0.0, 1.0, 0.0)));
    frustum.setUp(vec3(0.0, 0.0, 1.0));
    frustum.update();
    state->pushMatrix();
    state->multiplyMatrix(frustum.camera());

    // draw geometry
    if(m_zoneGeometry)
    {
        uploadZone(state);
        state->setRenderMode(RenderState::Basic);
        state->beginDrawMesh(m_zoneGeometry, m_zoneMaterials);
        state->drawMesh();
        state->endDrawMesh();
    }

    // draw objects
    if(m_showObjects && (m_objMeshWld != NULL))
        drawObjects(state);
    
    state->popMatrix();
}

void Zone::drawObjects(RenderState *state)
{
    // Create a GPU buffer for the objects' vertices and indices if needed.
    if(m_objectsGeometry == NULL)
        m_objectsGeometry = uploadObjects(state);
    
    // Build a list of visible objects and sort them by mesh.
    Frustum &frustum = state->viewFrustum();
    findVisibleObjects(m_index->root(), frustum, m_cullObjects);
    qSort(m_visibleObjects.begin(), m_visibleObjects.end(), zoneActorGroupLessThan);
    
    // Draw one batch of objects (beginDraw/endDraw) per mesh.
    int meshCount = 0;
    WLDMesh *previousMesh = NULL;
    QVector<matrix4> mvMatrices;
    state->setRenderMode(RenderState::Instanced);
    foreach(const WLDZoneActor *actor, m_visibleObjects)
    {
        WLDMesh *currentMesh = actor->mesh;
        if(currentMesh != previousMesh)
        {
            if(previousMesh)
            {
                if(mvMatrices.count() > 0)
                {
                    state->drawMeshBatch(mvMatrices.constData(), mvMatrices.count());
                    mvMatrices.clear();
                }
                state->endDrawMesh();
            }
            state->beginDrawMesh(currentMesh->data(), currentMesh->materials());
            previousMesh = currentMesh;
            meshCount++;
        }
        
        // Draw the zone object.
        state->pushMatrix();
        state->translate(actor->location);
        state->rotate(actor->rotation.x, 1.0, 0.0, 0.0);
        state->rotate(actor->rotation.y, 0.0, 1.0, 0.0);
        state->rotate(actor->rotation.z, 0.0, 0.0, 1.0);
        state->scale(actor->scale);
        mvMatrices.append(state->matrix(RenderState::ModelView));
        state->popMatrix();
        
        if(mvMatrices.count() >= RenderState::MAX_OBJECT_INSTANCES)
        {
            state->drawMeshBatch(mvMatrices.constData(), mvMatrices.count());
            mvMatrices.clear();
        }
    }
    if(previousMesh)
    {
        if(mvMatrices.count() > 0)
        {
            state->drawMeshBatch(mvMatrices.constData(), mvMatrices.count());
            mvMatrices.clear();
        }
        state->endDrawMesh();
    }
    m_visibleObjects.clear();
}

void Zone::uploadZone(RenderState *state)
{
    createGPUBuffer(m_zoneGeometry, state);
    m_zoneMaterials->upload(state);
}

VertexGroup * Zone::uploadObjects(RenderState *state)
{
    VertexGroup *geom = new VertexGroup(VertexGroup::Triangle);
    
    // Import vertices and indices for each mesh.
    foreach(WLDMesh *mesh, m_objModels.values())
    {
        mesh->importVertexData();
        mesh->importIndexData(geom, mesh->data()->indexBuffer,
                              mesh->data()->vertexBuffer,
                              0, (uint32_t)mesh->def()->m_indices.count());
        mesh->importMaterialGroups();
        if(mesh->materials())
            mesh->materials()->upload(state);
    }
    
    // Create the GPU buffers.
    geom->vertexBuffer.elementSize = sizeof(VertexData);
    geom->vertexBuffer.count = geom->vertices.count();
    geom->indexBuffer.elementSize = sizeof(uint32_t);
    geom->indexBuffer.count = geom->indices.count();
    createGPUBuffer(geom, state);
    
    // Set the buffer handles for each mesh.
    foreach(WLDMesh *mesh, m_objModels.values())
    {
        mesh->data()->vertexBuffer.buffer = geom->vertexBuffer.buffer;
        mesh->data()->indexBuffer.buffer = geom->indexBuffer.buffer;
    }

    // Free the memory used for vertices and indices.
    geom->vertices.clear();
    geom->indices.clear();
    geom->vertices.squeeze();
    geom->indices.squeeze();
    
    return geom;
}

void Zone::uploadCharacters(RenderState *state)
{
    foreach(WLDActor *actor, m_charModels)
        uploadCharacter(state, actor);
}

void Zone::uploadCharacter(RenderState *state, WLDActor *actor)
{
    WLDModel *model = actor->model();
    foreach(WLDModelSkin *skin, model->skins())
    {
        // Import mesh geometry.
        foreach(WLDMesh *mesh, skin->parts())
        {
            if(mesh->data()->vertices.count() == 0)
            {
                mesh->importVertexData();
                mesh->importIndexData();
                mesh->importMaterialGroups();
                createGPUBuffer(mesh->data(), state);
            }
        }

        // Upload materials (textures).
        MaterialMap *materials = skin->materials();
        if(!materials)
        {
            materials = skin->palette()->loadMaterials();
            skin->setMaterials(materials);
        }
        materials->upload(state);
    }
}

void Zone::findVisibleObjects(ActorIndexNode *node, const Frustum &f, bool cull)
{
    if(!node)
        return;
    Frustum::TestResult r = cull ? f.containsAABox(node->bounds()) : Frustum::INSIDE;
    if(r == Frustum::OUTSIDE)
        return;
    cull = (r != Frustum::INSIDE);
    for(int i = 0; i < 8; i++)
        findVisibleObjects(node->children()[i], f, cull);
    const QList<WLDZoneActor> &actors = m_index->actors();
    foreach(int actorIndex, node->actors())
        m_visibleObjects.append(&actors[actorIndex]);
}

const vec3 & Zone::playerPos() const
{
    return m_playerPos;
}

float Zone::playerOrient() const
{
    return m_playerOrient;
}

void Zone::setPlayerOrient(float rot)
{
    m_playerOrient = rot;
}

const vec3 & Zone::cameraOrient() const
{
    return m_cameraOrient;
}

void Zone::setCameraOrient(const vec3 &rot)
{
    m_cameraOrient = rot;
}

const vec3 & Zone::cameraPos() const
{
    return m_cameraPos;
}

bool Zone::showObjects() const
{
    return m_showObjects;
}

void Zone::setShowObjects(bool show)
{
    m_showObjects = show;
}

bool Zone::cullObjects() const
{
    return m_cullObjects;
}

void Zone::setCullObjects(bool enabled)
{
    m_cullObjects = enabled;
}

void Zone::step(float distForward, float distSideways, float distUpDown)
{
    const bool ghost = true;
    matrix4 m;
    if(ghost)
        m = matrix4::rotate(m_cameraOrient.x, 1.0, 0.0, 0.0);
    else
        m.setIdentity();
    m = m * matrix4::rotate(m_playerOrient, 0.0, 0.0, 1.0);
    m_playerPos = m_playerPos + m.map(vec3(distSideways, -distForward, distUpDown));
}
