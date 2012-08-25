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
    m_mainWld = 0;
    m_terrain = NULL;
    m_objects = NULL;
    m_playerPos = vec3(0.0, 0.0, 0.0);
    m_playerOrient = 0.0;
    m_cameraPos = vec3(0.0, 0.0, 0.0);
    m_cameraOrient = vec3(0.0, 0.0, 0.0);
    m_showZone = true;
    m_showObjects = true;
    m_cullObjects = true;
    m_showSoundTriggers = false;
    m_frustumIsFrozen = false;
}

Zone::~Zone()
{
    clear();
}

ZoneTerrain * Zone::terrain() const
{
    return m_terrain;
}

ZoneObjects * Zone::objects() const
{
    return m_objects;
}

QList<CharacterPack *> Zone::characterPacks() const
{
    return m_charPacks;
}

QList<ObjectPack *> Zone::objectPacks() const
{
    return m_objectPacks;
}

bool Zone::load(QString path, QString name)
{
    m_name = name;

    // Load the main archive and WLD file.
    QString zonePath = QString("%1/%2.s3d").arg(path).arg(name);
    QString zoneFile = QString("%1.wld").arg(name);
    m_mainArchive = new PFSArchive(zonePath, this);
    m_mainWld = WLDData::fromArchive(m_mainArchive, zoneFile, this);
    
    // Load the zone's terrain.
    m_terrain = new ZoneTerrain(this);
    if(!m_terrain->load(m_mainArchive, m_mainWld))
    {
        delete m_terrain;
        m_terrain = NULL;
        return false;
    }
    
    // Load the zone's static objects.
    m_objects = new ZoneObjects(this);
    if(!m_objects->load(path, name, m_mainArchive))
    {
        delete m_objects;
        m_objects = NULL;
        return false;
    }
    
    // Load the zone's characters.
    QString charPath = QString("%1/%2_chr.s3d").arg(path).arg(name);
    QString charFile = QString("%1_chr.wld").arg(name);
    loadCharacters(charPath, charFile);
    
    // Load the zone's sound triggers.
    QString triggersFile = QString("%1/%2_sounds.eff").arg(path).arg(name);
    SoundTrigger::fromFile(m_soundTriggers, triggersFile);
    return true;
}

CharacterPack * Zone::loadCharacters(QString archivePath, QString wldName)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        if(baseName == "global_chr1")
            baseName = "global_chr";
        wldName = baseName + ".wld";
    }
    
    CharacterPack *charPack = new CharacterPack();
    if(!charPack->load(archivePath, wldName))
    {
        delete charPack;
        return NULL;
    }
    m_charPacks.append(charPack);
    return charPack;
}

ObjectPack * Zone::loadObjects(QString archivePath, QString wldName)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        wldName = baseName + ".wld";
    }
    
    ObjectPack *objPack = new ObjectPack();
    if(!objPack->load(archivePath, wldName))
    {
        delete objPack;
        return NULL;
    }
    m_objectPacks.append(objPack);
    return objPack;
}

WLDActor * Zone::findCharacter(QString name) const
{
    foreach(CharacterPack *pack, m_charPacks)
    {
        if(pack->models().contains(name))
            return pack->models().value(name);
    }
    return NULL;
}

void Zone::clear()
{
    foreach(CharacterPack *pack, m_charPacks)
        delete pack;
    foreach(ObjectPack *pack, m_objectPacks)
        delete pack;
    m_charPacks.clear();
    m_objectPacks.clear();
    delete m_objects;
    delete m_terrain;
    delete m_mainWld;
    delete m_mainArchive;
    m_objects = NULL;
    m_terrain = NULL;
    m_mainWld = 0;
    m_mainArchive = 0;
}

void Zone::setPlayerViewFrustum(Frustum &frustum) const
{
    vec3 rot = vec3(0.0, 0.0, m_playerOrient) + m_cameraOrient;
    matrix4 viewMat = matrix4::rotate(rot.x, 1.0, 0.0, 0.0) *
        matrix4::rotate(rot.y, 0.0, 1.0, 0.0) *
        matrix4::rotate(rot.z, 0.0, 0.0, 1.0);
    frustum.setEye(m_playerPos);
    frustum.setFocus(m_playerPos + viewMat.map(vec3(0.0, 1.0, 0.0)));
    frustum.setUp(vec3(0.0, 0.0, 1.0));
    frustum.update();
}

void Zone::draw(RenderState *state)
{
    Frustum &frustum = state->viewFrustum();
    setPlayerViewFrustum(frustum);
    state->pushMatrix();
    state->multiplyMatrix(frustum.camera());
    
    Frustum &realFrustum(m_frustumIsFrozen ? m_frozenFrustum : frustum);
    
    // Draw the zone's static objects.
    if(m_showObjects && m_objects)
        m_objects->draw(state, realFrustum);

    // Draw the zone's terrain.
    if(m_showZone && m_terrain)
        m_terrain->draw(state, realFrustum);
    
    // draw sound trigger volumes
    if(m_showSoundTriggers)
    {
        foreach(SoundTrigger *trigger, m_soundTriggers)
            state->drawBox(trigger->bounds());
    }
    
    // Draw the viewing frustum and bounding boxes of visible zone parts. if frozen.
    if(m_frustumIsFrozen)
    {
        state->drawFrustum(m_frozenFrustum);
        //foreach(WLDZoneActor *actor, m_visibleZoneParts)
        //    state->drawBox(actor->boundsAA);
        //foreach(WLDZoneActor *actor, m_visibleObjects)
        //    state->drawBox(actor->boundsAA);
    }
    
    state->popMatrix();
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

void Zone::setCullObjects(bool enabled)
{
    m_cullObjects = enabled;
}

bool Zone::frustumIsFrozen() const
{
    return m_frustumIsFrozen;
}

void Zone::freezeFrustum(RenderState *state)
{
    if(!m_frustumIsFrozen)
    {
        m_frozenFrustum = state->viewFrustum();
        setPlayerViewFrustum(m_frozenFrustum);
        m_frustumIsFrozen = true;    
    }
}

void Zone::unFreezeFrustum()
{
    m_frustumIsFrozen = false;
}

bool Zone::showSoundTriggers() const
{
    return m_showSoundTriggers;
}

void Zone::setShowSoundTriggers(bool show)
{
    m_showSoundTriggers = show;
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

////////////////////////////////////////////////////////////////////////////////

ZoneTerrain::ZoneTerrain(Zone *zone)
{
    m_zone = zone;
    m_zoneBuffer = NULL;
    m_zoneMaterials = NULL;
    m_zoneStat = NULL;
    m_zoneStatGPU = NULL;
}

ZoneTerrain::~ZoneTerrain()
{
    clear();
}

const AABox & ZoneTerrain::bounds() const
{
    return m_zoneBounds;
}

void ZoneTerrain::clear()
{
    delete m_zoneBuffer;
    delete m_zoneMaterials;
    m_zoneBuffer = NULL;
    m_zoneMaterials = NULL;
    m_zone = NULL;
}

bool ZoneTerrain::load(PFSArchive *archive, WLDData *wld)
{
    if(!archive || !wld)
        return false;
    
    // Load zone regions as model parts, computing the zone's bounding box.
    int partID = 0;
    m_zoneBounds = AABox();
    foreach(MeshDefFragment *meshDef, wld->fragmentsByType<MeshDefFragment>())
    {
        WLDMesh *meshPart = new WLDMesh(meshDef, partID++);
        m_zoneBounds.extendTo(meshPart->boundsAA());
        m_zoneParts.append(meshPart);
    }
    
    // Load zone textures into the material palette.
    WLDMaterialPalette zonePalette(archive);
    foreach(MaterialDefFragment *matDef, wld->fragmentsByType<MaterialDefFragment>())
        zonePalette.addMaterialDef(matDef);
    m_zoneMaterials = zonePalette.loadMaterials();
    
    // Add zone regions to the zone octree index.
    m_zoneTree = new OctreeIndex(m_zoneBounds, 8);
    foreach(WLDMesh *meshPart, m_zoneParts)
        m_zoneTree->add(new WLDActor(NULL, meshPart, m_zone));
    
    return true;
}

MeshBuffer * ZoneTerrain::upload(RenderState *state)
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
    
    //m_gpuBuffers.append(meshBuf->vertexBuffer);
    //m_gpuBuffers.append(meshBuf->indexBuffer);
    return meshBuf;
}

void ZoneTerrain::draw(RenderState *state, Frustum &frustum)
{
    // draw geometry
    if(!m_zoneStat)
        m_zoneStat = state->createStat("Zone CPU (ms)", FrameStat::CPUTime);
    if(!m_zoneStatGPU)
        m_zoneStatGPU = state->createStat("Zone GPU (ms)", FrameStat::GPUTime);
    m_zoneStat->beginTime();
    m_zoneStatGPU->beginTime();
    
    // Create a GPU buffer for the zone's vertices and indices if needed.
    if(m_zoneBuffer == NULL)
        m_zoneBuffer = upload(state);
    
#if !defined(COMBINE_ZONE_PARTS)
    // Build a list of visible zone parts.
    m_zoneTree->findVisible(m_visibleZoneParts, frustum, m_zone->cullObjects());
    
    // Import material groups from the visible parts.
    m_zoneBuffer->matGroups.clear();
    foreach(const WLDActor *actor, m_visibleZoneParts)
        m_zoneBuffer->addMaterialGroups(actor->simpleModel()->data());
#endif
    
    // Draw the visible parts as one big mesh.
    state->beginDrawMesh(m_zoneBuffer, m_zoneMaterials);
    state->drawMesh();
    state->endDrawMesh();
    
    m_zoneStatGPU->endTime();
    m_zoneStat->endTime();
    m_visibleZoneParts.clear();
}

////////////////////////////////////////////////////////////////////////////////

ZoneObjects::ZoneObjects(Zone *zone)
{
    m_zone = zone;
    m_pack = NULL;
    m_objDefWld = 0;
    m_objectsStat = NULL;
    m_objectsStatGPU = NULL;
    m_objectTree = NULL;
    m_drawnObjectsStat = NULL;
}

ZoneObjects::~ZoneObjects()
{
    clear();
}

const QMap<QString, WLDMesh *> & ZoneObjects::models() const
{
    return m_pack->models();
}

void ZoneObjects::clear()
{
    delete m_objectTree;
    delete m_pack;
    delete m_objDefWld;
    m_objectTree = NULL;
    m_pack = NULL;
    m_objDefWld = NULL;
}

bool ZoneObjects::load(QString path, QString name, PFSArchive *mainArchive)
{
    m_objDefWld = WLDData::fromArchive(mainArchive, "objects.wld", m_zone);
    if(!m_objDefWld)
        return false;
    
    QString objMeshPath = QString("%1/%2_obj.s3d").arg(path).arg(name);
    QString objMeshFile = QString("%1_obj.wld").arg(name);
    m_pack = new ObjectPack();
    if(!m_pack->load(objMeshPath, objMeshFile))
    {
        delete m_pack;
        m_pack = NULL;
        return false;
    }
    importActors();
    return true;
}

void ZoneObjects::importActors()
{
    // import actors through Actor fragments
    AABox bounds = m_zone->terrain()->bounds();
    const QMap<QString, WLDMesh *> &models = m_pack->models();
    foreach(ActorFragment *actorFrag, m_objDefWld->fragmentsByType<ActorFragment>())
    {
        QString actorName = actorFrag->m_def.name();
        WLDMesh *model = models.value(actorName);
        if(model)
        {
            WLDActor *actor = new WLDActor(actorFrag, model, m_zone);
            bounds.extendTo(actor->boundsAA());
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
    foreach(WLDActor *actor, m_objects)
        m_objectTree->add(actor);   
}

static bool zoneActorGroupLessThan(const WLDActor *a, const WLDActor *b)
{
    return a->simpleModel()->def() < b->simpleModel()->def();
}

void ZoneObjects::draw(RenderState *state, Frustum &frustum)
{
    if(!m_objectsStat)
        m_objectsStat = state->createStat("Objects CPU (ms)", FrameStat::CPUTime);
    if(!m_objectsStatGPU)
        m_objectsStatGPU = state->createStat("Objects GPU (ms)", FrameStat::GPUTime);
    m_objectsStat->beginTime();
    m_objectsStatGPU->beginTime();
    
    // Create a GPU buffer for the objects' vertices and indices if needed.
    if(m_pack->buffer() == NULL)
        m_pack->upload(state);
    
    // Build a list of visible objects and sort them by mesh.
    m_objectTree->findVisible(m_visibleObjects, frustum, m_zone->cullObjects());
    qSort(m_visibleObjects.begin(), m_visibleObjects.end(), zoneActorGroupLessThan);
    
    // Draw one batch of objects (beginDraw/endDraw) per mesh.
    int meshCount = 0;
    WLDMesh *previousMesh = NULL;
    MeshBuffer *meshBuf = m_pack->buffer();
    QVector<matrix4> mvMatrices;
    foreach(const WLDActor *actor, m_visibleObjects)
    {
        WLDMesh *currentMesh = actor->simpleModel();
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
            meshBuf->matGroups.clear();
            meshBuf->addMaterialGroups(currentMesh->data());
            state->beginDrawMesh(meshBuf, m_pack->materials());
            previousMesh = currentMesh;
            meshCount++;
        }
        
        // Draw the zone object.
        state->pushMatrix();
        state->multiplyMatrix(actor->modelMatrix());
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
    m_objectsStatGPU->endTime();
    m_objectsStat->endTime();
}

////////////////////////////////////////////////////////////////////////////////

ObjectPack::ObjectPack()
{
    m_archive = NULL;
    m_wld = NULL;
    m_meshBuf = NULL;
}

ObjectPack::~ObjectPack()
{
    clear();
}

const QMap<QString, WLDMesh *> ObjectPack::models() const
{
    return m_models;
}

MeshBuffer * ObjectPack::buffer() const
{
    return m_meshBuf;
}

MaterialMap * ObjectPack::materials() const
{
    return m_materials;
}

void ObjectPack::clear()
{
    delete m_meshBuf;
    foreach(WLDMesh *actor, m_models)
        delete actor;
    m_models.clear();
    delete m_wld;
    delete m_archive;
    m_meshBuf = NULL;
    m_wld = NULL;
    m_archive = NULL;
}

bool ObjectPack::load(QString archivePath, QString wldName)
{
    m_archive = new PFSArchive(archivePath);
    m_wld = WLDData::fromArchive(m_archive, wldName);
    if(!m_wld)
        return false;

    // Import models through ActorDef fragments.
    WLDMaterialPalette palette(m_archive);
    foreach(ActorDefFragment *actorDef, m_wld->fragmentsByType<ActorDefFragment>())
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
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        WLDMesh *model = new WLDMesh(mesh->m_def, 0);
        palette.addPaletteDef(mesh->m_def->m_palette);
        m_models.insert(actorName, model);
    }
    m_materials = palette.loadMaterials();
    return true;
}

MeshBuffer * ObjectPack::upload(RenderState *state)
{
    m_materials->uploadArray(state);
    
    // Import vertices and indices for each mesh.
    m_meshBuf = new MeshBuffer();
    foreach(WLDMesh *mesh, m_models.values())
    {
        MeshData *meshData = mesh->importFrom(m_meshBuf);
        meshData->updateTexCoords(m_materials);
    }
    
    // Create the GPU buffers.
    m_meshBuf->upload(state);
    m_meshBuf->clearVertices();
    m_meshBuf->clearIndices();
    //m_gpuBuffers.append(m_meshBuf->vertexBuffer);
    //m_gpuBuffers.append(m_meshBuf->indexBuffer);
    return m_meshBuf;
}

////////////////////////////////////////////////////////////////////////////////

CharacterPack::CharacterPack()
{
    m_archive = NULL;
    m_wld = NULL;
}

CharacterPack::~CharacterPack()
{
    clear();
}

const QMap<QString, WLDActor *> CharacterPack::models() const
{
    return m_models;
}

void CharacterPack::clear()
{
    foreach(WLDActor *actor, m_models)
        delete actor;
    m_models.clear();
    delete m_wld;
    delete m_archive;
    m_wld = 0;
    m_archive = 0;
}

bool CharacterPack::load(QString archivePath, QString wldName)
{
    m_archive = new PFSArchive(archivePath);
    m_wld = WLDData::fromArchive(m_archive, wldName);
    if(!m_wld)
    {
        delete m_archive;
        m_archive = 0;
        return false;
    }

    importCharacters(m_archive, m_wld);
    importCharacterPalettes(m_archive, m_wld);
    importSkeletons(m_wld);
    return true;
}

void CharacterPack::importSkeletons(WLDData *wld)
{
    // XXX add a actorName+animName -> WLDAnimation map that contains all animations in the pack (no duplicate).
    // Makes it easier to free the resources even if some animations are shared between actors.
    
    // import skeletons which contain the pose animation
    foreach(HierSpriteDefFragment *skelDef, wld->fragmentsByType<HierSpriteDefFragment>())
    {
        QString actorName = skelDef->name().replace("_HS_DEF", "");
        WLDActor *actor = m_models.value(actorName);
        if(!actor)
            continue;
        actor->complexModel()->setSkeleton(new WLDSkeleton(skelDef));
    }

    // import other animations
    foreach(TrackFragment *track, wld->fragmentsByType<TrackFragment>())
    {
        QString animName = track->name().left(3);
        QString actorName = track->name().mid(3, 3);
        WLDActor *actor = m_models.value(actorName);
        if(!actor)
            continue;
        WLDSkeleton *skel = actor->complexModel()->skeleton();
        if(skel && track->m_def)
            skel->addTrack(animName, track->m_def);
    }
}

void CharacterPack::importCharacterPalettes(PFSArchive *archive, WLDData *wld)
{
    foreach(MaterialDefFragment *matDef, wld->fragmentsByType<MaterialDefFragment>())
    {
        QString charName, palName, partName;
        if(WLDMaterialPalette::explodeName(matDef, charName, palName, partName))
        {
            WLDActor *actor = m_models.value(charName);
            if(!actor)
                continue;
            WLDModel *model = actor->complexModel();
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
        WLDActor *actor = m_models.value(actorName);
        if(!actor || meshName.isEmpty())
            continue;
        WLDModel *model = actor->complexModel();
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

void CharacterPack::importCharacters(PFSArchive *archive, WLDData *wld)
{
    foreach(ActorDefFragment *actorDef, wld->fragmentsByType<ActorDefFragment>())
    {
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        WLDModel *model = new WLDModel(archive);
        WLDActor *actor = new WLDActor(model);
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

        m_models.insert(actorName, actor);
    }
}

void CharacterPack::upload(RenderState *state)
{
    foreach(WLDActor *actor, m_models)
        upload(state, actor);
}

void CharacterPack::upload(RenderState *state, WLDActor *actor)
{
    // Make sure we haven't uploaded this character before.
    WLDModel *model = actor->complexModel();
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
    //m_gpuBuffers.append(meshBuf->vertexBuffer);
    //m_gpuBuffers.append(meshBuf->indexBuffer);

    // Free the memory used for indices. We need to keep the vertices around for software skinning.
    meshBuf->clearIndices();
}
