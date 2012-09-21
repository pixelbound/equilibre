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

#include <QFileInfo>
#include <QImage>
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Game/WLDData.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDSkeleton.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/SoundTrigger.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/FrameStat.h"

Zone::Zone(Game *game)
{
    m_game = game;
    m_mainArchive = 0;
    m_mainWld = 0;
    m_terrain = NULL;
    m_objects = NULL;
    m_actorTree = NULL;
    m_playerPos = vec3(0.0, 0.0, 0.0);
    m_playerOrient = 0.0;
    m_cameraPos = vec3(0.0, 0.0, 0.0);
    m_cameraOrient = vec3(0.0, 0.0, 0.0);
}

Zone::~Zone()
{
    clear(NULL);
}

ZoneTerrain * Zone::terrain() const
{
    return m_terrain;
}

ZoneObjects * Zone::objects() const
{
    return m_objects;
}

const QVector<WLDLightActor *> & Zone::lights() const
{
    return m_lights;   
}

QList<CharacterPack *> Zone::characterPacks() const
{
    return m_charPacks;
}

OctreeIndex * Zone::actorIndex() const
{
    return m_actorTree;
}

const ZoneInfo & Zone::info() const
{
    return m_info;
}

void Zone::setInfo(const ZoneInfo &info)
{
    m_info = info;
}

bool Zone::load(QString path, QString name)
{
    m_info.name = name;

    // Load the main archive and WLD file.
    QString zonePath = QString("%1/%2.s3d").arg(path).arg(name);
    QString zoneFile = QString("%1.wld").arg(name);
    m_mainArchive = new PFSArchive(zonePath);
    m_mainWld = WLDData::fromArchive(m_mainArchive, zoneFile);
    
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
    
    m_actorTree = new OctreeIndex(m_objects->bounds(), 8);
    m_objects->addTo(m_actorTree);
    
    // Load the zone's light sources.
    if(!importLightSources(m_mainArchive))
        return false;
    
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
    CharacterPack *charPack = m_game->loadCharacters(archivePath, wldName, false);
    if(charPack)
        m_charPacks.append(charPack);
    return charPack;
}

bool Zone::importLightSources(PFSArchive *archive)
{
    WLDData *wld = WLDData::fromArchive(archive, "lights.wld");
    if(!wld)
        return false;
    uint16_t ID = 0;
    WLDFragmentArray<LightSourceFragment> lightFrags = wld->table()->byKind<LightSourceFragment>();
    for(uint32_t i = 0; i < lightFrags.count(); i++)
    {
        WLDLightActor *actor = new WLDLightActor(lightFrags[i], ID++);
        m_lights.append(actor);
        m_actorTree->add(actor);
    }
    delete wld;
    return true;
}

void Zone::clear(RenderContext *renderCtx)
{
    foreach(CharacterPack *pack, m_charPacks)
    {
        pack->clear(renderCtx);
        delete pack;
    }
    foreach(WLDLightActor *light, m_lights)
        delete light;
    m_lights.clear();
    m_charPacks.clear();
    if(m_objects)
    {
        m_objects->clear(renderCtx);
        delete m_objects;
    }
    if(m_terrain)
    {
        m_terrain->clear(renderCtx);
        delete m_terrain;
    }
    m_objects = NULL;
    m_terrain = NULL;
    delete m_actorTree;
    delete m_mainWld;
    delete m_mainArchive;
    m_actorTree = NULL;
    m_mainWld = 0;
    m_mainArchive = 0;
    m_playerPos = vec3(0.0, 0.0, 0.0);
    m_playerOrient = 0.0;
    m_cameraPos = vec3(0.0, 0.0, 0.0);
    m_cameraOrient = vec3(0.0, 0.0, 0.0);
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

void Zone::frustumCullingCallback(WLDActor *actor, void *user)
{
    Zone *z = (Zone *)user;
    WLDStaticActor *staticActor = actor->cast<WLDStaticActor>();
    if(staticActor && staticActor->frag())
        z->objects()->visibleObjects().append(staticActor);
}

void Zone::draw(RenderContext *renderCtx, RenderProgram *prog)
{
    if(!m_actorTree)
        return;
    
    // Setup the render program.
    vec4 ambientLight(0.4, 0.4, 0.4, 1.0);
    renderCtx->setCurrentProgram(prog);
    prog->setAmbientLight(ambientLight);
    
    FogParams fogParams;
    fogParams.color = m_info.fogColor;
    fogParams.start = m_info.fogMinClip;
    fogParams.end = m_info.fogMaxClip;
    fogParams.density = m_game->fogDensity();
    prog->setFogParams(fogParams);
    
    // Draw the sky first.
    ZoneSky *sky = m_game->sky();
    if(sky)
        sky->draw(renderCtx, prog, this);
    
    Frustum &frustum = renderCtx->viewFrustum();
    setPlayerViewFrustum(frustum);
    renderCtx->pushMatrix();
    renderCtx->multiplyMatrix(frustum.camera());
    
    // Build a list of visible actors.
    Frustum &realFrustum(m_game->frustumIsFrozen() ? m_frozenFrustum : frustum);
    m_actorTree->findVisible(realFrustum, frustumCullingCallback, this, m_game->cullObjects());
    if(m_terrain->findCurrentRegion(realFrustum.eye()))
        m_terrain->showNearbyRegions(realFrustum);
    else
        m_terrain->showAllRegions(realFrustum);

    // Draw the zone's static objects.
    if(m_game->showObjects() && m_objects)
        m_objects->draw(renderCtx, prog);

    // Draw the zone's terrain.
    if(m_game->showZone() && m_terrain)
        m_terrain->draw(renderCtx, prog);
    
    // draw sound trigger volumes
    if(m_game->showSoundTriggers())
    {
        foreach(SoundTrigger *trigger, m_soundTriggers)
            prog->drawBox(trigger->bounds());
    }
    
    // Draw the viewing frustum and bounding boxes of visible zone parts. if frozen.
    if(m_game->frustumIsFrozen())
    {
        prog->drawFrustum(m_frozenFrustum);
        //foreach(WLDZoneActor *actor, visibleZoneParts)
        //    prog->drawBox(actor->boundsAA);
        //foreach(WLDZoneActor *actor, visibleObjects)
        //    prog->drawBox(actor->boundsAA);
    }
    
    m_terrain->resetVisible();
    m_objects->resetVisible();
    
    renderCtx->popMatrix();
}

const vec3 & Zone::playerPos() const
{
    return m_playerPos;
}

void Zone::setPlayerPos(const vec3 &newPos)
{
    m_playerPos = newPos;
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

void Zone::freezeFrustum(RenderContext *renderCtx)
{
    m_frozenFrustum = renderCtx->viewFrustum();
    setPlayerViewFrustum(m_frozenFrustum);
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

ZoneInfo::ZoneInfo()
{
    skyID = 0;
    fogColor = vec4(0.4, 0.4, 0.6, 0.1);
    fogMinClip = minClip = 10.0f;
    fogMaxClip = maxClip = 300.0f;
    underworldZ = -1000.0f;
    flags = 0;
}

////////////////////////////////////////////////////////////////////////////////

ZoneTerrain::ZoneTerrain(Zone *zone)
{
    m_zone = zone;
    m_regionCount = 0;
    m_currentRegion = 0;
    m_zoneWld = NULL;
    m_regionTree = NULL;
    m_zoneBuffer = NULL;
    m_zoneMaterials = NULL;
    m_palette = NULL;
    m_zoneStat = NULL;
    m_zoneStatGPU = NULL;
}

ZoneTerrain::~ZoneTerrain()
{
    clear(NULL);
}

const AABox & ZoneTerrain::bounds() const
{
    return m_zoneBounds;
}

uint32_t ZoneTerrain::currentRegion() const
{
    return m_currentRegion;
}

void ZoneTerrain::clear(RenderContext *renderCtx)
{
    for(uint32_t i = 1; i <= m_regionCount; i++)
    {
        WLDStaticActor *part = m_regionActors[i];
        if(part)
        {
            delete part->mesh();
            delete part;
        }
    }
    m_regionActors.clear();
    m_visibleRegions.clear();
    m_regionCount = 0;
    m_currentRegion = 0;
    m_regionTree = NULL;
    
    if(m_zoneBuffer)
    {
        m_zoneBuffer->clear(renderCtx);
        delete m_zoneBuffer;
        m_zoneBuffer = NULL;
    }
    
    delete m_zoneMaterials;
    m_zoneMaterials = NULL;
    
    delete m_palette;
    m_palette = NULL;
    
    if(renderCtx)
    {
        renderCtx->destroyStat(m_zoneStat);
        renderCtx->destroyStat(m_zoneStatGPU);
        m_zoneStat = NULL;
        m_zoneStatGPU = NULL;
    }
    m_zoneWld = NULL;
}

bool ZoneTerrain::load(PFSArchive *archive, WLDData *wld)
{
    if(!archive || !wld)
        return false;
    m_zoneWld = wld;
    
    WLDFragmentArray<RegionTreeFragment> regionTrees = wld->table()->byKind<RegionTreeFragment>();
    if(regionTrees.count() != 1)
        return false;
    m_regionTree = regionTrees[0];
    WLDFragmentArray<RegionFragment> regionDefs = wld->table()->byKind<RegionFragment>();
    if(regionDefs.count() == 0)
        return false;
    m_regionCount = regionDefs.count();
    m_regionActors.resize(m_regionCount + 1, NULL);
    m_visibleRegions.reserve(m_regionCount);
    
    // Load zone regions as model parts, computing the zone's bounding box.
    m_zoneBounds = AABox();
    WLDFragmentArray<MeshDefFragment> meshDefs = wld->table()->byKind<MeshDefFragment>();
    for(uint32_t i = 0; i < meshDefs.count(); i++)
    {
        MeshDefFragment *meshDef = meshDefs[i];
        QString name = meshDef->name();
        int typePos = name.indexOf("_DMSPRITEDEF");
        if((name.length() < 2) || !name.startsWith("R") || (typePos < 0))
            continue;
        bool ok = false;
        uint32_t regionID = (uint32_t)name.mid(1, typePos - 1).toInt(&ok);
        if(!ok || (regionID >= m_regionCount))
            continue;
        WLDMesh *meshPart = new WLDMesh(meshDef, regionID);
        m_zoneBounds.extendTo(meshPart->boundsAA());
        // XXX stop using WLDStaticActor for zone regions?
        m_regionActors[regionID] = new WLDStaticActor(NULL, meshPart);
    }
    vec3 padding(1.0, 1.0, 1.0);
    m_zoneBounds.low = m_zoneBounds.low - padding;
    m_zoneBounds.high = m_zoneBounds.high + padding;
    
    // Load zone textures into the material palette.
    WLDFragmentArray<MaterialPaletteFragment> matPals = wld->table()->byKind<MaterialPaletteFragment>();
    Q_ASSERT(matPals.count() == 1);
    m_palette = new WLDMaterialPalette(archive);
    m_palette->setDef(matPals[0]);
    m_palette->createSlots();
    m_zoneMaterials = new MaterialMap();
    m_palette->exportTo(m_zoneMaterials);
    
    return true;
}

void ZoneTerrain::showAllRegions(const Frustum &frustum)
{
    for(uint32_t i = 0; i < m_regionCount; i++)
    {
        WLDStaticActor *actor = m_regionActors[i];
        if(actor)
        {
            if(frustum.containsAABox(actor->boundsAA()) != OUTSIDE)
                m_visibleRegions.push_back(actor);
        }
    }
}

void ZoneTerrain::showNearbyRegions(const Frustum &frustum)
{
    if(m_currentRegion == 0)
        return;
    WLDFragmentArray<RegionFragment> regions = m_zoneWld->table()->byKind<RegionFragment>();
    RegionFragment *region = regions[m_currentRegion - 1];
    uint32_t count = region->m_nearbyRegions.count();
    for(uint32_t i = 0; i < count; i++)
    {
        uint32_t regionID = region->m_nearbyRegions[i];
        Q_ASSERT(regionID <= m_regionCount);
        WLDStaticActor *actor = m_regionActors[regionID];
        if(actor)
        {
            if(frustum.containsAABox(actor->boundsAA()) != OUTSIDE)
                m_visibleRegions.push_back(actor);
        }
    }
}

uint32_t ZoneTerrain::findCurrentRegion(const vec3 &cameraPos)
{
    return (m_currentRegion = findCurrentRegion(cameraPos, m_regionTree->m_nodes.constData(), 1));
}

uint32_t ZoneTerrain::findCurrentRegion(const vec3 &cameraPos, const RegionTreeNode *nodes, uint32_t nodeIdx)
{
    if(nodeIdx == 0)
        return 0;
    const RegionTreeNode &node = nodes[nodeIdx-1];
    if(node.regionID == 0)
    {
        float distance = vec3::dot(node.normal, cameraPos) + node.distance;
        return findCurrentRegion(cameraPos, nodes, (distance >= 0.0f) ? node.left : node.right);
    }
    else
    {
        // Leaf node.
        return node.regionID;
    }
}

void ZoneTerrain::resetVisible()
{
    m_visibleRegions.clear();
}

MeshBuffer * ZoneTerrain::upload(RenderContext *renderCtx)
{
    MeshBuffer *meshBuf = NULL;
    
    // Upload the materials as a texture array, assigning z coordinates to materials.
    m_zoneMaterials->uploadArray(renderCtx);
    
#if !defined(COMBINE_ZONE_PARTS)
    meshBuf = new MeshBuffer();
    
    // Import vertices and indices for each mesh.
    for(uint32_t i = 1; i <= m_regionCount; i++)
    {
        WLDStaticActor *actor = m_regionActors[i];
        if(actor)
        {
            MeshData *meshData = actor->mesh()->importFrom(meshBuf, m_palette);
            meshData->updateTexCoords(m_zoneMaterials);
        }
    }
#else
    meshBuf = WLDMesh::combine(m_zoneParts);
    meshBuf->updateTexCoords(m_zoneMaterials);
#endif
    
    // Create the GPU buffers and free the memory used for vertices and indices.
    meshBuf->upload(renderCtx);
    meshBuf->clearVertices();
    meshBuf->clearIndices();
    return meshBuf;
}

void ZoneTerrain::draw(RenderContext *renderCtx, RenderProgram *prog)
{
    // draw geometry
    if(!m_zoneStat)
        m_zoneStat = renderCtx->createStat("Zone CPU (ms)", FrameStat::CPUTime);
    if(!m_zoneStatGPU)
        m_zoneStatGPU = renderCtx->createStat("Zone GPU (ms)", FrameStat::GPUTime);
    m_zoneStat->beginTime();
    m_zoneStatGPU->beginTime();
    
    // Create a GPU buffer for the zone's vertices and indices if needed.
    if(m_zoneBuffer == NULL)
        m_zoneBuffer = upload(renderCtx);
    
#if !defined(COMBINE_ZONE_PARTS)
    // Import material groups from the visible parts.
    m_zoneBuffer->matGroups.clear();
    uint32_t visibleRegions = m_visibleRegions.size();
    for(uint32_t i = 0; i < visibleRegions; i++)
    {
        WLDStaticActor *staticActor = m_visibleRegions[i];
        if(staticActor)
            m_zoneBuffer->addMaterialGroups(staticActor->mesh()->data());
    }
#endif
    
    // Draw the visible parts as one big mesh.
    prog->beginDrawMesh(m_zoneBuffer, m_zoneMaterials);
    prog->drawMesh();
    prog->endDrawMesh();
    
    m_zoneStatGPU->endTime();
    m_zoneStat->endTime();
}

////////////////////////////////////////////////////////////////////////////////

ZoneObjects::ZoneObjects(Zone *zone)
{
    m_zone = zone;
    m_bounds = zone->terrain()->bounds();
    m_pack = NULL;
    m_objDefWld = 0;
    m_objectsStat = NULL;
    m_objectsStatGPU = NULL;
    m_drawnObjectsStat = NULL;
}

ZoneObjects::~ZoneObjects()
{
    clear(NULL);
}

const AABox & ZoneObjects::bounds() const
{
    return m_bounds;
}

const QMap<QString, WLDMesh *> & ZoneObjects::models() const
{
    return m_pack->models();
}

QVector<WLDStaticActor *> & ZoneObjects::visibleObjects()
{
    return m_visibleObjects;
}

void ZoneObjects::clear(RenderContext *renderCtx)
{
    foreach(WLDActor *actor, m_objects)
        delete actor;
    m_objects.clear();
    if(m_pack)
    {
        m_pack->clear(renderCtx);
        delete m_pack;
        m_pack = NULL;
    }
    delete m_objDefWld;
    m_objDefWld = NULL;
    if(renderCtx)
    {
        renderCtx->destroyStat(m_objectsStat);
        renderCtx->destroyStat(m_objectsStatGPU);
        renderCtx->destroyStat(m_drawnObjectsStat);
        m_objectsStat = NULL;
        m_objectsStatGPU = NULL;
        m_drawnObjectsStat = NULL;
    }
}

bool ZoneObjects::load(QString path, QString name, PFSArchive *mainArchive)
{
    m_objDefWld = WLDData::fromArchive(mainArchive, "objects.wld");
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
    const QMap<QString, WLDMesh *> &models = m_pack->models();
    WLDFragmentArray<ActorFragment> actorFrags = m_objDefWld->table()->byKind<ActorFragment>();
    for(uint32_t i = 0; i < actorFrags.count(); i++)
    {
        ActorFragment *actorFrag = actorFrags[i];
        QString actorName = actorFrag->m_def.name().replace("_ACTORDEF", "");
        WLDMesh *model = models.value(actorName);
        if(model)
        {
            WLDStaticActor *actor = new WLDStaticActor(actorFrag, model);
            m_bounds.extendTo(actor->boundsAA());
            m_objects.append(actor);
        }
        else
        {
            qDebug("Actor '%s' not found", actorName.toLatin1().constData());
        }
    }
}

void ZoneObjects::addTo(OctreeIndex *tree)
{
    foreach(WLDStaticActor *actor, m_objects)
        tree->add(actor);   
}

void ZoneObjects::resetVisible()
{
    m_visibleObjects.clear();
}

void ZoneObjects::upload(RenderContext *renderCtx)
{
    // Copy objects' lighting colors to a GPU buffer.
    MeshBuffer *meshBuf = m_pack->upload(renderCtx);
    foreach(WLDStaticActor *actor, m_objects)
        actor->importColorData(meshBuf);
    meshBuf->colorBufferSize = meshBuf->colors.count() * sizeof(uint32_t);
    if(meshBuf->colorBufferSize > 0)
    {
        meshBuf->colorBuffer = renderCtx->createBuffer(meshBuf->colors.constData(), meshBuf->colorBufferSize);
        meshBuf->clearColors();
    }
}

static bool zoneActorGroupLessThan(const WLDStaticActor *a, const WLDStaticActor *b)
{
    return a->mesh()->def() < b->mesh()->def();
}

void ZoneObjects::draw(RenderContext *renderCtx, RenderProgram *prog)
{
    if(!m_objectsStat)
        m_objectsStat = renderCtx->createStat("Objects CPU (ms)", FrameStat::CPUTime);
    if(!m_objectsStatGPU)
        m_objectsStatGPU = renderCtx->createStat("Objects GPU (ms)", FrameStat::GPUTime);
    m_objectsStat->beginTime();
    m_objectsStatGPU->beginTime();
    
    // Create a GPU buffer for the objects' vertices and indices if needed.
    if(m_pack->buffer() == NULL)
        upload(renderCtx);
    
    // Sort the list of visible objects by mesh.
    qSort(m_visibleObjects.begin(), m_visibleObjects.end(), zoneActorGroupLessThan);
    
    // Draw one batch of objects (beginDraw/endDraw) per mesh.
    int meshCount = 0;
    WLDMesh *previousMesh = NULL;
    MeshBuffer *meshBuf = m_pack->buffer();
    const QVector<WLDLightActor *> &lightSources = m_zone->lights();
    foreach(WLDActor *actor, m_visibleObjects)
    {
        WLDStaticActor *staticActor = actor->cast<WLDStaticActor>();
        if(!staticActor)
            continue;
        WLDMesh *currentMesh = staticActor->mesh();
        if(currentMesh != previousMesh)
        {
            if(previousMesh)
                prog->endDrawMesh();
            meshBuf->matGroups.clear();
            meshBuf->addMaterialGroups(currentMesh->data());
            prog->beginDrawMesh(meshBuf, m_pack->materials());
            previousMesh = currentMesh;
            meshCount++;
        }
        
        // Draw the zone object.
        renderCtx->pushMatrix();
        renderCtx->multiplyMatrix(staticActor->modelMatrix());
        matrix4 mvMatrix = renderCtx->matrix(RenderContext::ModelView);
        prog->drawMeshBatch(&mvMatrix, &staticActor->colorSegment(), 1);
        renderCtx->popMatrix();
    }
    if(previousMesh)
        prog->endDrawMesh();
    
    if(m_drawnObjectsStat == NULL)
        m_drawnObjectsStat = renderCtx->createStat("Objects", FrameStat::Counter);
    m_drawnObjectsStat->setCurrent(m_visibleObjects.count());
    m_objectsStatGPU->endTime();
    m_objectsStat->endTime();
}

////////////////////////////////////////////////////////////////////////////////

SkyDef::SkyDef()
{
    mainLayer = secondLayer = NULL;
}

ZoneSky::ZoneSky()
{
    m_skyArchive = NULL;
    m_skyWld = NULL;
    m_skyMaterials = NULL;
    m_skyBuffer = NULL;
}

ZoneSky::~ZoneSky()
{
    clear(NULL);
}

void ZoneSky::clear(RenderContext *renderCtx)
{
    foreach(SkyDef def, m_skyDefs)
    {
        delete def.mainLayer;
        delete def.secondLayer;
    }
    m_skyDefs.clear();
    if(m_skyBuffer)
    {
        m_skyBuffer->clear(renderCtx);
        delete m_skyBuffer;
        m_skyBuffer = NULL;
    }
    delete m_skyArchive;
    delete m_skyWld;
    delete m_skyMaterials;
    m_skyArchive = NULL;
    m_skyWld = NULL;
    m_skyMaterials = NULL;
}

bool ZoneSky::load(QString path)
{
    QString archivePath = QString("%1/sky.s3d").arg(path);
    m_skyArchive = new PFSArchive(archivePath);
    m_skyWld = WLDData::fromArchive(m_skyArchive, "sky.wld");
    if(!m_skyWld)
    {
        delete m_skyArchive;
        m_skyArchive = NULL;
        return false;
    }
    
    QRegExp nameExp("^LAYER(\\d)(\\d)_DMSPRITEDEF$");
    WLDFragmentArray<MeshDefFragment> meshDefs = m_skyWld->table()->byKind<MeshDefFragment>();
    for(uint32_t i = 0; i < meshDefs.count(); i++)
    {
        MeshDefFragment *meshDef = meshDefs[i];
        QString name = meshDef->name();
        if(!nameExp.exactMatch(name))
            continue;
        uint32_t skyID = nameExp.cap(1).toInt();
        uint32_t skySubID = nameExp.cap(2).toInt();
        uint32_t layerID = (skyID * 10) + skySubID;
        if(m_skyDefs.size() < skyID)
            m_skyDefs.resize(skyID);
        SkyDef &def = m_skyDefs[skyID - 1];
        if(skySubID == 1)
            def.mainLayer = new WLDMesh(meshDef, layerID);
        else
            def.secondLayer = new WLDMesh(meshDef, layerID);
    }
    
    // Load sky textures into the material palette.
    WLDMaterialPalette skyPalette(m_skyArchive);
    // XXX fix this.
    /*WLDFragmentArray<MaterialPaletteFragment> matPals = m_skyWld->table()->byKind<MaterialPaletteFragment>();
    Q_ASSERT(matPals.count() == 1);
    skyPalette.setDef(matPals[0]);
    skyPalette.createSlots();*/
    m_skyMaterials = new MaterialMap();
    skyPalette.exportTo(m_skyMaterials);
    return true;
}

bool ZoneSky::upload(RenderContext *renderCtx)
{
    if(m_skyBuffer)
        return true;
    
    // Upload the materials as a texture array, assigning z coordinates to materials.
    m_skyMaterials->uploadArray(renderCtx);
    
    // Import vertices and indices for each mesh.
    m_skyBuffer = new MeshBuffer();
    /*for(uint32_t i = 0; i < m_skyDefs.size(); i++)
    {
        WLDMesh *mainMesh = m_skyDefs[i].mainLayer;
        WLDMesh *secondMesh = m_skyDefs[i].secondLayer;
        MeshData *mainMeshData = mainMesh->importFrom(m_skyBuffer, NULL); // XXX
        MeshData *secondMeshData = secondMesh->importFrom(m_skyBuffer, NULL); // XXX
        mainMeshData->updateTexCoords(m_skyMaterials);
        secondMeshData->updateTexCoords(m_skyMaterials);
    }*/
    
    // Create the GPU buffers and free the memory used for vertices and indices.
    m_skyBuffer->upload(renderCtx);
    m_skyBuffer->clearVertices();
    m_skyBuffer->clearIndices();
    return true;
}

void ZoneSky::draw(RenderContext *renderCtx, RenderProgram *prog, Zone *zone)
{
    uint32_t skyID = zone->info().skyID;
    if(!skyID || (skyID > m_skyDefs.size()) || !upload(renderCtx))
        return;
    
    // Restrict the movement of the camera so that we don't see the edges of the sky dome.
    vec3 camRot = vec3(0.0, 0.0, zone->playerOrient()) + zone->cameraOrient();
    matrix4 viewMat2 = matrix4::rotate(camRot.x * 0.25, 1.0, 0.0, 0.0) *
                       matrix4::rotate(camRot.y, 0.0, 1.0, 0.0) *
                       matrix4::rotate(camRot.z * 0.25, 0.0, 0.0, 1.0);
    vec3 focus = viewMat2.map(vec3(0.0, 1.0, 0.0));
    matrix4 viewMat = matrix4::lookAt(vec3(0.0, 0.0, 0.0), focus, vec3(0.0, 0.0, 1.0));
    
    renderCtx->setDepthWrite(false);
    
    // Import material groups from the visible layer.
    m_skyBuffer->matGroups.clear();
    /*const SkyDef &def = m_skyDefs[skyID - 1];
    if(def.secondLayer)
        m_skyBuffer->addMaterialGroups(def.secondLayer->data());
    if(def.mainLayer)
        m_skyBuffer->addMaterialGroups(def.mainLayer->data());*/
    
    // Draw the visible layer.
    prog->beginDrawMesh(m_skyBuffer, m_skyMaterials);
    prog->drawMeshBatch(&viewMat, NULL, 1);
    prog->endDrawMesh();
    
    renderCtx->setDepthWrite(true);
}
