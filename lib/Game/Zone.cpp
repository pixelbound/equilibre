#include <QFileInfo>
#include <QImage>
#include "OpenEQ/Game/Zone.h"
#include "OpenEQ/Game/PFSArchive.h"
#include "OpenEQ/Game/WLDData.h"
#include "OpenEQ/Game/WLDModel.h"
#include "OpenEQ/Game/WLDActor.h"
#include "OpenEQ/Game/WLDSkeleton.h"
#include "OpenEQ/Game/Fragments.h"
#include "OpenEQ/Game/SoundTrigger.h"
#include "OpenEQ/Render/RenderState.h"
#include "OpenEQ/Render/Material.h"
#include "OpenEQ/Render/FrameStat.h"

Zone::Zone(QObject *parent) : QObject(parent)
{
    m_mainArchive = 0;
    m_objMeshArchive = 0;
    m_charArchive = 0;
    m_mainWld = 0;
    m_objMeshWld = m_objDefWld = 0;
    m_charWld = 0;
    m_zoneBuffer = 0;
    m_zoneMaterials = 0;
    m_playerPos = vec3(0.0, 0.0, 0.0);
    m_playerOrient = 0.0;
    m_cameraPos = vec3(0.0, 0.0, 0.0);
    m_cameraOrient = vec3(0.0, 0.0, 0.0);
    m_showZone = true;
    m_showObjects = true;
    m_cullObjects = true;
    m_showSoundTriggers = false;
    m_zoneStat = NULL;
    m_objectsStat = NULL;
    m_zoneStatGPU = NULL;
    m_objectsStatGPU = NULL;
    m_objectsBuffer = NULL;
    m_objectTree = NULL;
    m_drawnObjectsStat = NULL;
}

Zone::~Zone()
{
    clear();
}

const QMap<QString, WLDMesh *> & Zone::objectModels() const
{
    return m_objModels;
}

const QMap<QString, WLDActor *> & Zone::charModels() const
{
    return m_charModels;
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
    
    // import sound triggers
    QString triggersFile = QString("%1/%2_sounds.eff").arg(path).arg(name);
    SoundTrigger::fromFile(m_soundTriggers, triggersFile);
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
    foreach(WLDMesh *model, m_objModels)
        delete model;
    foreach(WLDActor *actor, m_charModels)
        delete actor;
    m_objModels.clear();
    m_charModels.clear();
    delete m_objectTree;
    delete m_objectsBuffer;
    delete m_zoneBuffer;
    delete m_zoneMaterials;
    delete m_mainWld;
    delete m_objMeshWld;
    delete m_objDefWld;
    delete m_charWld;
    delete m_mainArchive;
    delete m_objMeshArchive;
    delete m_charArchive;
    m_objectTree = 0;
    m_objectsBuffer = 0;
    m_zoneBuffer = 0;
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
    // Load zone regions as model parts, computing the zone's bounding box.
    int partID = 0;
    m_zoneBounds = AABox();
    foreach(MeshDefFragment *meshDef, m_mainWld->fragmentsByType<MeshDefFragment>())
    {
        WLDMesh *meshPart = new WLDMesh(meshDef, partID++);
        m_zoneBounds.extendTo(meshPart->boundsAA());
        m_zoneParts.append(meshPart);
    }
    
    // Load zone textures into the material palette.
    WLDMaterialPalette zonePalette(m_mainArchive, this);
    foreach(MaterialDefFragment *matDef, m_mainWld->fragmentsByType<MaterialDefFragment>())
        zonePalette.addMaterialDef(matDef);
    m_zoneMaterials = zonePalette.loadMaterials();
    
    // Add zone regions to the zone octree index.
    m_zoneTree = new OctreeIndex(m_zoneBounds, 8);
    foreach(WLDMesh *meshPart, m_zoneParts)
        m_zoneTree->add(new WLDZoneActor(NULL, meshPart));
}

void Zone::importObjects()
{
    importObjectsMeshes(m_objMeshArchive, m_objMeshWld);

    // import actors through Actor fragments
    AABox bounds = m_zoneBounds;
    foreach(ActorFragment *actorFrag, m_objDefWld->fragmentsByType<ActorFragment>())
    {
        QString actorName = actorFrag->m_def.name();
        WLDMesh *model = m_objModels.value(actorName);
        if(model)
        {
            WLDZoneActor *actor = new WLDZoneActor(actorFrag, model);
            bounds.extendTo(actor->boundsAA);
            m_objects.append(actor);
        }
        else
        {
            qDebug("Actor '%s' not found", actorName.toLatin1().constData());
        }
    }
    
    // Add actors to the actors octree index.
    // XXX use the same octree than for the geometry?
    m_objectTree = new OctreeIndex(bounds, 8);
    foreach(WLDZoneActor *actor, m_objects)
        m_objectTree->add(actor);   
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
                skin->palette()->copyFrom(model->skin()->palette());
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
            if((meshName2 == meshName) && (skinName2 != skinName))
                skin->replacePart(part, meshDef);
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
        {
            skin->addPart(meshDef);
            skin->palette()->addPaletteDef(meshDef->m_palette);
        }
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
    frustum.setEye(m_playerPos);
    frustum.setFocus(m_playerPos + viewMat.map(vec3(0.0, 1.0, 0.0)));
    frustum.setUp(vec3(0.0, 0.0, 1.0));
    frustum.update();
    state->pushMatrix();
    state->multiplyMatrix(frustum.camera());
    
    // draw objects
    if(!m_objectsStat)
        m_objectsStat = state->createStat("Objects CPU (ms)", FrameStat::CPUTime);
    if(!m_objectsStatGPU)
        m_objectsStatGPU = state->createStat("Objects GPU (ms)", FrameStat::GPUTime);
    if(m_showObjects && (m_objMeshWld != NULL))
    {
        m_objectsStat->beginTime();
        m_objectsStatGPU->beginTime();
        drawObjects(state);
        m_objectsStatGPU->endTime();
        m_objectsStat->endTime();
    }

    // draw geometry
    if(!m_zoneStat)
        m_zoneStat = state->createStat("Zone CPU (ms)", FrameStat::CPUTime);
    if(!m_zoneStatGPU)
        m_zoneStatGPU = state->createStat("Zone GPU (ms)", FrameStat::GPUTime);
    if(m_showZone && (m_mainWld != NULL))
    {
        m_zoneStat->beginTime();
        m_zoneStatGPU->beginTime();
        drawGeometry(state);
        m_zoneStatGPU->endTime();
        m_zoneStat->endTime();
    }
    
    // draw sound trigger volumes
    if(m_showSoundTriggers)
    {
        foreach(SoundTrigger *trigger, m_soundTriggers)
            state->drawBox(trigger->bounds());
    }
    
    state->popMatrix();
}

void Zone::drawGeometry(RenderState *state)
{
    // Create a GPU buffer for the zone's vertices and indices if needed.
    if(m_zoneBuffer == NULL)
        m_zoneBuffer = uploadZone(state);
    
#if !defined(COMBINE_ZONE_PARTS)
    // Build a list of visible zone parts.
    m_zoneTree->findVisible(m_visibleZoneParts, state->viewFrustum(), m_cullObjects);
    
    // Import material groups from the visible parts.
    m_zoneBuffer->matGroups.clear();
    foreach(const WLDZoneActor *actor, m_visibleZoneParts)
        m_zoneBuffer->addMaterialGroups(actor->mesh->data());
    m_visibleZoneParts.clear();
#endif
    
    // Draw the visible parts as one big mesh.
    state->beginDrawMesh(m_zoneBuffer, m_zoneMaterials);
    state->drawMesh();
    state->endDrawMesh();
}

void Zone::drawObjects(RenderState *state)
{
    // Create a GPU buffer for the objects' vertices and indices if needed.
    if(m_objectsBuffer == NULL)
        m_objectsBuffer = uploadObjects(state);
    
    // Build a list of visible objects and sort them by mesh.
    m_objectTree->findVisible(m_visibleObjects, state->viewFrustum(), m_cullObjects);
    qSort(m_visibleObjects.begin(), m_visibleObjects.end(), zoneActorGroupLessThan);
    
    // Draw one batch of objects (beginDraw/endDraw) per mesh.
    int meshCount = 0;
    WLDMesh *previousMesh = NULL;
    QVector<matrix4> mvMatrices;
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
            m_objectsBuffer->matGroups.clear();
            m_objectsBuffer->addMaterialGroups(currentMesh->data());
            state->beginDrawMesh(m_objectsBuffer, currentMesh->materials());
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
    if(m_drawnObjectsStat == NULL)
        m_drawnObjectsStat = state->createStat("Objects", FrameStat::Counter);
    m_drawnObjectsStat->setCurrent(m_visibleObjects.count());
    m_visibleObjects.clear();
}

MeshBuffer * Zone::uploadZone(RenderState *state)
{
    MeshBuffer *meshBuf = NULL;
    
    // Upload the materials as a texture array, assigning z coordinates to materials.
    m_zoneMaterials->uploadArray(state);
    
#if !defined(COMBINE_ZONE_PARTS)
    meshBuf = new MeshBuffer();
    
    // Import vertices and indices for each mesh.
    foreach(WLDMesh *mesh, m_zoneParts)
    {
        MeshData *meshData = mesh->importFrom(meshBuf);
        meshData->updateTexCoords(m_zoneMaterials);
    }
#else
    meshBuf = WLDMesh::combine(m_zoneParts);
    meshBuf->updateTexCoords(m_zoneMaterials);
#endif
    
    // Create the GPU buffers and free the memory used for vertices and indices.
    meshBuf->upload(state);
    meshBuf->clearVertices();
    meshBuf->clearIndices();
    
    m_gpuBuffers.append(meshBuf->vertexBuffer);
    m_gpuBuffers.append(meshBuf->indexBuffer);
    return meshBuf;
}

MeshBuffer * Zone::uploadObjects(RenderState *state)
{
    MeshBuffer *meshBuf = new MeshBuffer();
    
    // Import vertices and indices for each mesh.
    foreach(WLDMesh *mesh, m_objModels.values())
    {
        MeshData *meshData = mesh->importFrom(meshBuf);
        MaterialMap *materials = mesh->materials();
        if(materials)
        {
            materials->uploadArray(state);
            meshData->updateTexCoords(materials);
        }
    }
    
    // Create the GPU buffers.
    meshBuf->upload(state);
    meshBuf->clearVertices();
    meshBuf->clearIndices();
    m_gpuBuffers.append(meshBuf->vertexBuffer);
    m_gpuBuffers.append(meshBuf->indexBuffer);
    return meshBuf;
}

void Zone::uploadCharacters(RenderState *state)
{
    foreach(WLDActor *actor, m_charModels)
        uploadCharacter(state, actor);
}

void Zone::uploadCharacter(RenderState *state, WLDActor *actor)
{
    // Make sure we haven't uploaded this character before.
    WLDModel *model = actor->model();
    if(model->buffer())
        return;

    // Import mesh geometry.
    MeshBuffer *meshBuf = new MeshBuffer();
    model->setBuffer(meshBuf);
    foreach(WLDModelSkin *skin, model->skins())
    {
        foreach(WLDMesh *mesh, skin->parts())
            mesh->importFrom(meshBuf);

        // Upload materials (textures).
        MaterialMap *materials = skin->materials();
        if(!materials)
        {
            materials = skin->palette()->loadMaterials();
            skin->setMaterials(materials);
        }
        materials->upload(state);
    }

    // Create the GPU buffers.
    meshBuf->upload(state);
    m_gpuBuffers.append(meshBuf->vertexBuffer);
    m_gpuBuffers.append(meshBuf->indexBuffer);

    // Free the memory used for indices. We need to keep the vertices around for software skinning.
    meshBuf->clearIndices();
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

bool Zone::showZone() const
{
    return m_showZone;
}

void Zone::setShowZone(bool show)
{
    m_showZone = show;
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

bool Zone::showSoundTriggers() const
{
    return m_showSoundTriggers;
}

void Zone::setShowSoundTriggers(bool show)
{
    m_showSoundTriggers = show;
}

void Zone::setCullObjects(bool enabled)
{
    m_cullObjects = enabled;
}

void Zone::currentSoundTriggers(QVector<SoundTrigger *> &triggers) const
{
    vec3 pos = m_playerPos + m_cameraPos;
    foreach(SoundTrigger *trigger, m_soundTriggers)
    {
        if(trigger->bounds().contains(pos))
            triggers.append(trigger);
    }
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
    m_playerPos = m_playerPos + m.map(vec3(-distSideways, distForward, distUpDown));
}
