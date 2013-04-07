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

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Game/StreamReader.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDData.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"

uint32_t Game::SHAPE_TERRAIN = 1;
uint32_t Game::SHAPE_STATIC_OBJECT = 2;
uint32_t Game::SHAPE_CHARACTER = 4;

uint32_t Game::COLLIDES_TERRAIN = 0;
uint32_t Game::COLLIDES_STATIC_OBJECT = 0;
uint32_t Game::COLLIDES_CHARACTER =
        Game::SHAPE_TERRAIN |
        Game::SHAPE_STATIC_OBJECT |
        Game::SHAPE_CHARACTER;

Game::Game()
{
    m_player = new WLDCharActor(NULL);
    m_playerIdleAnim = NULL;
    m_playerRunningAnim = NULL;
    m_startAnimationTime = 0.0;
    m_zone = NULL;
    m_sky = NULL;
    m_builtinObjects = NULL;
    m_builtinMats = NULL;
    m_capsule = NULL;
    m_collisionWorld = NewtonCreate();
    m_showZone = true;
    m_showObjects = true;
    m_showFog = false;
    m_cullObjects = true;
    m_showSoundTriggers = false;
    m_frustumIsFrozen = false;
    m_minDistanceToShowCharacter = 1.0;
    m_movementAheadTime = 0.0;
    m_movementStateX = m_movementStateY = 0;
    
    float min[3] = {-5000.0, -5000.0, -5000.0};
    float max[3] = { 5000.0,  5000.0,  5000.0};
    NewtonSetWorldSize(m_collisionWorld, min, max);
}

Game::~Game()
{
    clear(NULL);
    delete m_player;
    NewtonDestroy(m_collisionWorld);
}

void Game::clear(RenderContext *renderCtx)
{
    clearZone(renderCtx);
    if(m_sky)
    {
        m_sky->clear(renderCtx);
        delete m_sky;
        m_sky = NULL;
    }
    if(m_builtinObjects)
    {
        m_builtinObjects->clear(renderCtx);
        delete m_builtinObjects;
        m_builtinObjects = NULL;
    }
    delete m_builtinMats;
    m_builtinMats = NULL;
    foreach(ObjectPack *pack, m_objectPacks)
    {
        pack->clear(renderCtx);
        delete pack;
    }
    m_player->setModel(NULL);
    m_player->setHasCamera(false);
    m_playerIdleAnim = NULL;
    m_playerRunningAnim = NULL;
    foreach(CharacterPack *pack, m_charPacks)
    {
        pack->clear(renderCtx);
        delete pack;
    }
    m_objectPacks.clear();
    m_charPacks.clear();
}

void Game::clearZone(RenderContext *renderCtx)
{
    if(m_zone)
    {
        m_zone->clear(renderCtx);
        delete m_zone;
        m_zone = NULL;
    }
}

bool Game::showZone() const
{
    return m_showZone;
}

void Game::setShowZone(bool show)
{
    m_showZone = show;
}

bool Game::showObjects() const
{
    return m_showObjects;
}

void Game::setShowObjects(bool show)
{
    m_showObjects = show;
}

bool Game::showFog() const
{
    return m_showFog;
}

void Game::setShowFog(bool show)
{
    m_showFog = show;
}

bool Game::cullObjects() const
{
    return m_cullObjects;
}

void Game::setCullObjects(bool enabled)
{
    m_cullObjects = enabled;
}

bool Game::frustumIsFrozen() const
{
    return m_frustumIsFrozen;
}

void Game::freezeFrustum(RenderContext *renderCtx)
{
    if(m_zone && !m_frustumIsFrozen)
    {
        m_zone->freezeFrustum(renderCtx);
        m_frustumIsFrozen = true;    
    }
}

void Game::unFreezeFrustum()
{
    m_frustumIsFrozen = false;
}

bool Game::showSoundTriggers() const
{
    return m_showSoundTriggers;
}

void Game::setShowSoundTriggers(bool show)
{
    m_showSoundTriggers = show;
}

void Game::setMovementX(int movementX)
{
    m_movementStateX = movementX;
}

void Game::setMovementY(int movementY)
{
    m_movementStateY = movementY;
}

WLDCharActor *  Game::player() const
{
    return m_player;   
}

Zone * Game::zone() const
{
    return m_zone;
}

ZoneSky * Game::sky() const
{
    return m_sky;
}

float Game::fogDensity() const
{
    return m_showFog ? 0.003f : 0.0f;
}

float Game::minDistanceToShowCharacter() const
{
    return m_minDistanceToShowCharacter;
}

QList<ObjectPack *> Game::objectPacks() const
{
    return m_objectPacks;
}

QList<CharacterPack *> Game::characterPacks() const
{
    return m_charPacks;
}

MeshBuffer * Game::builtinOjectBuffer() const
{
    return m_builtinObjects;
}

MaterialArray * Game::builtinMaterials() const
{
    return m_builtinMats;
}

MeshData * Game::capsule() const
{
    return m_capsule;
}

NewtonWorld * Game::collisionWorld()
{
    return m_collisionWorld;
}

Zone * Game::loadZone(QString path, QString name)
{
    if(m_zone)
        return NULL;
    Zone *zone = new Zone(this);
    if(!zone->load(path, name))
    {
        delete zone;
        return NULL;
    }
    m_zone = zone;
    
    vec3 initialPos(0.0, 0.0, 0.1);
    if(m_zoneInfo.contains(name))
    {
        const ZoneInfo &info = m_zoneInfo[name];
        m_zone->setInfo(info);
        initialPos = info.safePos;
    }
    m_player->createShape(m_zone->collisionWorld());
    m_player->setLocation(initialPos);
    m_player->setHasCamera(true);
    m_currentState.position = m_previousState.position = initialPos;
    return zone;
}

bool Game::loadZoneInfo(QString filePath)
{
    QFile file(filePath);
    if(!file.open(QFile::ReadOnly))
        return false;
    QTextStream s(&file);
    while(!s.atEnd())
    {
        QString line = s.readLine();
        if(line.isEmpty() || line.startsWith("//"))
            continue;
        QStringList fields = line.split(",");
        if(fields.count() < 14)
            continue;
        
        ZoneInfo zi;
        float fogFactor = 0.6f / 255.0f;
        int idx = 0;
        zi.name = fields.value(idx++);
        zi.skyID = fields.value(idx++).toInt();
        zi.fogColor.x = (fields.value(idx++).toInt() * fogFactor);
        zi.fogColor.y = (fields.value(idx++).toInt() * fogFactor);
        zi.fogColor.z = (fields.value(idx++).toInt() * fogFactor);
        zi.fogColor.w = 1.0f;
        zi.fogMinClip = fields.value(idx++).toFloat();
        zi.fogMaxClip = fields.value(idx++).toFloat();
        zi.minClip = fields.value(idx++).toFloat();
        zi.maxClip = fields.value(idx++).toFloat();
        zi.safePos.y = fields.value(idx++).toFloat();
        zi.safePos.z = fields.value(idx++).toFloat();
        zi.safePos.z = fields.value(idx++).toFloat();
        zi.underworldZ = fields.value(idx++).toFloat();
        zi.flags = fields.value(idx++).toInt();
        m_zoneInfo.insert(zi.name, zi);
    }
    m_showFog = true;
    return true;
}

bool Game::loadSky(QString path)
{
    ZoneSky *sky = new ZoneSky();
    if(!sky->load(path))
    {
        delete sky;
        return false;
    }
    m_sky = sky;
    return true;
}

bool Game::loadBuiltinOjects(QString path)
{
    QFile capsuleFile(QString("%1/capsule.stl").arg(path));
    if(!capsuleFile.exists())
    {
        return false;
    }
    
    m_builtinObjects = new MeshBuffer();
    m_capsule = loadBuiltinSTLMesh(capsuleFile.fileName());
    
    Material *mat = new Material();
    mat->setOpaque(false);
    m_builtinMats = new MaterialArray();
    m_builtinMats->setMaterial(1, mat);
    return true;
}

ObjectPack * Game::loadObjects(QString archivePath, QString wldName)
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

CharacterPack * Game::loadCharacters(QString archivePath, QString wldName, bool own)
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
    if(own)
        m_charPacks.append(charPack);
    return charPack;
}

WLDModel * Game::findCharacter(QString name)
{
    return findCharacter(name, NULL);
}

WLDModel * Game::findCharacter(QString name, RenderContext *renderCtx)
{
    foreach(CharacterPack *pack, m_charPacks)
    {
        if(pack->models().contains(name))
        {
            WLDModel *model = pack->models().value(name);
            if(model && renderCtx)
            {
                pack->upload(renderCtx, model);
            }
            return model;
        }
    }
    return NULL;
}

MeshData * Game::loadBuiltinSTLMesh(QString path)
{
    QFile file(path);
    if(!file.open(QFile::ReadOnly))
    {
        return NULL;
    }
    
    StreamReader reader(&file);
    uint32_t numTriangles = 0;
    vec3 normal;
    uint16_t attribSize = 0;
    uint32_t defaultColorABGR = 0xffffffff; // A=1, B=1, G=1, R=1
    
    // Skip header.
    file.seek(80);
    reader.unpackField('I', &numTriangles);
    
    MeshBuffer *meshBuf = m_builtinObjects;
    MeshData *data = meshBuf->createMesh(1);
    MaterialGroup *group = &data->matGroups[0];
    QVector<Vertex> &vertices(meshBuf->vertices);
    QVector<uint32_t> &indices(meshBuf->indices);
    BufferSegment &dataLoc = data->vertexSegment;
    uint32_t vertexCount = numTriangles * 3;
    uint32_t vertexIndex = vertices.count();
    dataLoc.offset = vertexIndex;
    dataLoc.count = vertexCount;
    dataLoc.elementSize = sizeof(Vertex);
    group->id = 0;
    group->offset = 0;
    group->count = vertexCount;
    group->matID = 1;
    for(uint32_t i = 0; i < numTriangles; i++)
    {
        reader.unpackArray("f", 3, &normal);
        for(int j = 0; j < 3; j++)
        {
            Vertex v;
            reader.unpackArray("f", 3, &v.position);
            v.normal = normal;
            v.texCoords = vec3();
            v.color = defaultColorABGR;
            v.bone = 0;
            v.padding[0] = 0;
            vertices.append(v);
            indices.append((i * 3) + j);
        }
        reader.unpackField('H', &attribSize);
    }
    return data;
}

void Game::drawBuiltinObject(MeshData *object, RenderContext *renderCtx,
                             RenderProgram *prog)
{
    if(!object || !renderCtx || !prog)
    {
        return;
    }
    
    if(m_builtinObjects->vertexBuffer == 0)
    {
        m_builtinObjects->upload(renderCtx);
    }
    m_builtinObjects->matGroups.clear();
    m_builtinObjects->addMaterialGroups(object);
    prog->beginDrawMesh(m_builtinObjects, m_builtinMats);
    prog->drawMesh();
    prog->endDrawMesh();
}

void Game::drawPlayer(RenderContext *renderCtx, RenderProgram *prog)
{
    if(m_player->cameraDistance() > m_minDistanceToShowCharacter)
    {
        if(m_player->model())
        {
            m_player->draw(renderCtx, prog);
        }
        if(m_capsule) //XXX
        {
            vec3 loc = m_player->location();
            const float modelHeight = 4.0;
            const float modelRadius = 1.0;
            float scaleXY = (m_player->capsuleRadius() / modelRadius);
            float scaleZ = (m_player->capsuleHeight() / modelHeight);
            float offsetZ = (m_player->capsuleHeight() * 0.5f);
            renderCtx->pushMatrix();
            renderCtx->translate(loc.x, loc.y, loc.z + offsetZ);
            renderCtx->rotate(-m_player->lookOrient().z + 90.0f, 0.0, 0.0, 1.0);
            renderCtx->scale(scaleXY, scaleXY, scaleZ);
            drawBuiltinObject(m_capsule, renderCtx, prog);
            renderCtx->popMatrix();
        }
    }
}

void Game::setPlayerModel(WLDModel *model)
{
    m_player->setModel(model);
    if(model)
    {
        m_playerIdleAnim = m_player->findAnimation("P01");
        m_playerRunningAnim = m_player->findAnimation("L02");
    }
}

void Game::update(RenderContext *renderCtx, double currentTime,
                  double sinceLastUpdate)
{
    updateMovement(sinceLastUpdate);
    
    // Update the player's animation.
    if(m_player->model())
    {
        WLDAnimation *oldAnimation = m_player->animation();
        WLDAnimation *newAnimation = oldAnimation;
        if(m_movementStateX || m_movementStateY)
        {
            newAnimation = m_playerRunningAnim;
        }
        else
        {
            newAnimation = m_playerIdleAnim;
        }
        
        if(oldAnimation != newAnimation)
        {
            m_player->setAnimation(newAnimation);
            m_startAnimationTime = currentTime;
        }
        m_player->setAnimTime(currentTime - m_startAnimationTime);
    }
    
    if(m_zone)
    {
        m_zone->update(renderCtx, currentTime);
    }
}

void Game::updateMovement(double sinceLastUpdate)
{
    // Calculate the next player position using fixed-duration ticks.
    const double tick = (1.0 / 30.0); // 30 movement ticks per second
    m_movementAheadTime += sinceLastUpdate;
    while(m_movementAheadTime > tick)
    {
        m_previousState = m_currentState;
        updatePlayerPosition(m_player, m_currentState, tick);
        m_movementAheadTime -= tick;
    }
    
    // Interpolate the position since we calculated it too far in the future.
    double alpha = (m_movementAheadTime / tick);
    vec3 newPosition = (m_currentState.position * alpha) +
            (m_previousState.position * (1.0 - alpha));
    m_player->setLocation(newPosition);
}

void Game::updatePlayerPosition(WLDCharActor *player, ActorState &state, double dt)
{
    const float playerVelocity = 25.0;
    float dist = (playerVelocity * dt);
    vec3 &pos = state.position;
    float deltaX = dist * m_movementStateX;
    float deltaY = dist * m_movementStateY;
    bool ghost = (player->cameraDistance() < m_minDistanceToShowCharacter);
    player->calculateStep(pos, deltaX, deltaY, ghost);
    
    // Collision detection if the player is in a zone.
    if(!m_zone)
    {
        return;
    }
    
    const vec3 gravity(0, 0, -1.0);
    state.velocity = state.velocity + (gravity * dt);
    pos = pos + state.velocity;
    
    vec3 responseVelocity;
    const int MAX_CONTACTS = 1;
    vec3 contacts[MAX_CONTACTS];
    vec3 normals[MAX_CONTACTS];
    float penetration[MAX_CONTACTS] = {0};
    float offsetZ = (m_player->capsuleHeight() * 0.5f);
    matrix4 playerTransform = matrix4::translate(pos.x, pos.y, pos.z + offsetZ);
    // Make the capsule upright.
    playerTransform = playerTransform * matrix4::rotate(90.0, 0.0, 1.0, 0.0);
    matrix4 groundTransform = matrix4::translate(0.0, 0.0, 0.0);
    int hits = NewtonCollisionCollide(m_collisionWorld, MAX_CONTACTS,
        m_player->shape(), (const float *)playerTransform.columns(),
        m_zone->groundShape(), (const float *)groundTransform.columns(),
        (float *)contacts, (float *)normals, penetration, 0);
    if(hits > 0)
    {
        /*qDebug("Collision with player (%f, %f, %f) at (%f, %f, %f) normal (%f, %f, %f) depth %f",
               pos.x, pos.y, pos.z,
               contacts[0].x, contacts[0].y, contacts[0].z,
               normals[0].x, normals[0].y, normals[0].z,
                penetration[0]);*/
        responseVelocity = responseVelocity - (normals[0] * penetration[0]);
    }
    
    matrix4 wallTransform = matrix4::translate(0.0, 265.0, 0.0);
    hits = NewtonCollisionCollide(m_collisionWorld, MAX_CONTACTS,
            m_player->shape(), (const float *)playerTransform.columns(),
            m_zone->wallShape(), (const float *)wallTransform.columns(),
            (float *)contacts, (float *)normals, penetration, 0);
    if(hits > 0)
    {
        /*qDebug("Collision with player (%f, %f, %f) at (%f, %f, %f) normal (%f, %f, %f) depth %f",
               pos.x, pos.y, pos.z,
               contacts[0].x, contacts[0].y, contacts[0].z,
               normals[0].x, normals[0].y, normals[0].z,
                penetration[0]);*/
        responseVelocity = responseVelocity - (normals[0] * penetration[0]);
    }
    
#if 0
    const int MAX_CONTACTS = 1;
    dContactGeom contacts[MAX_CONTACTS];
    std::vector<dGeomID> &geomList = player->collidingShapes();
    dGeomID playerGeom = player->shape();
    dGeomID geom = NULL;
    vec3 responseVelocity;
    dGeomSetPosition(playerGeom, pos.x, pos.y, pos.z);
    dSpaceCollide(m_zone->collisionIndex(), this, collisionNearCallback);
    for(size_t i = 0; i < geomList.size(); i++)
    {
        geom = geomList[i];
        memset(contacts, 0, MAX_CONTACTS * sizeof(dContactGeom));
        int collision = dCollide(playerGeom, geom, MAX_CONTACTS, contacts,
                                 sizeof(dContactGeom));
        if(collision > 0)
        {
            // Only deal with player/plane collisions for now.
            dContactGeom &c = contacts[0];
            if((c.depth < 1e-4) || (dGeomGetClass(geom) != dPlaneClass))
            {
                continue;
            }
            
            // Make sure the player is actually crossing the plane.
            vec3 normal(c.normal[0], c.normal[1], c.normal[2]);
            dReal d1 = dGeomPlanePointDepth(geom, pos.x, pos.y, pos.z);
            dReal d2 = dGeomPlanePointDepth(geom, c.pos[0], c.pos[1], c.pos[2]);
            dReal depthSign = d1 * d2;
            if(depthSign >= 0.0)
            {
                continue;
            }
            
            // Resolve the collision by pushing the playing to the other
            // side of the plane.
            /*qDebug("Collision with player (%f, %f, %f) at (%f, %f, %f) normal (%f, %f, %f) depth %f d1 %f d2 %f",
                   pos.x, pos.y, pos.z,
                   c.pos[0], c.pos[1], c.pos[2],
                   c.normal[0], c.normal[1], c.normal[2],
                   c.depth, d1, d2);*/
            responseVelocity = responseVelocity + (normal * c.depth);
        }
    }
    geomList.clear();
#endif
    
    // Apply the collision response to the player.
    if(responseVelocity.z > 1e-4)
    {
        // Clear the player's velocity when the ground is hit.
        state.velocity = vec3(0, 0, 0);
    }
    pos = pos + responseVelocity;
    
    //qDebug("Player now at (%f, %f, %f)", pos.x, pos.y, pos.z);
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
    clear(NULL);
}

const QMap<QString, WLDMesh *> & ObjectPack::models() const
{
    return m_models;
}

MeshBuffer * ObjectPack::buffer() const
{
    return m_meshBuf;
}

void ObjectPack::clear(RenderContext *renderCtx)
{
    foreach(WLDMesh *model, m_models)
        delete model;
    m_models.clear();
    if(m_meshBuf)
    {
        m_meshBuf->clear(renderCtx);
        delete m_meshBuf;
        m_meshBuf = NULL;
    }
    delete m_wld;
    m_wld = NULL;
    delete m_archive;
    m_archive = NULL;
}

bool ObjectPack::load(QString archivePath, QString wldName)
{
    m_archive = new PFSArchive(archivePath);
    m_wld = WLDData::fromArchive(m_archive, wldName);
    if(!m_wld)
        return false;

    // Import models through ActorDef fragments.
    WLDFragmentArray<ActorDefFragment> actorDefs = m_wld->table()->byKind<ActorDefFragment>();
    for(uint32_t i = 0; i < actorDefs.count(); i++)
    {
        ActorDefFragment *actorDef = actorDefs[i];
        WLDFragment *subModel = actorDef->m_models.value(0);
        if(!subModel)
            continue;
        MeshFragment *mesh = subModel->cast<MeshFragment>();
        if(!mesh)
        {
            if(subModel->kind() == HierSpriteFragment::KIND)
                qDebug("Hierarchical model in zone objects (%s)",
                       actorDef->name().toLatin1().constData());
            continue;
        }
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        WLDMesh *model = new WLDMesh(mesh->m_def, 0);
        WLDMaterialPalette *pal = model->importPalette(m_archive);
        pal->createArray();
        pal->createMap();
        m_models.insert(actorName, model);
    }
    
    return true;
}

MeshBuffer * ObjectPack::upload(RenderContext *renderCtx)
{
    // Import vertices and indices for each mesh.
    m_meshBuf = new MeshBuffer();
    foreach(WLDMesh *mesh, m_models.values())
    {
        MaterialArray *materials = mesh->palette()->array();
        materials->uploadArray(renderCtx);
        MeshData *meshData = mesh->importFrom(m_meshBuf);
        meshData->updateTexCoords(materials, true);
    }
    
    // Create the GPU buffers.
    m_meshBuf->upload(renderCtx);
    m_meshBuf->clearVertices();
    m_meshBuf->clearIndices();
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
    clear(NULL);
}

const QMap<QString, WLDModel *> CharacterPack::models() const
{
    return m_models;
}

void CharacterPack::clear(RenderContext *renderCtx)
{
    foreach(WLDModel *model, m_models)
    {
        MeshBuffer *meshBuf = model->buffer();
        if(meshBuf)
        {
            meshBuf->clear(renderCtx);
            delete meshBuf;
        }
        delete model->skeleton();
        delete model;
    }
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
    WLDFragmentArray<HierSpriteDefFragment> skelDefs = wld->table()->byKind<HierSpriteDefFragment>();
    for(uint32_t i = 0; i < skelDefs.count(); i++)
    {
        HierSpriteDefFragment *skelDef = skelDefs[i];
        QString actorName = skelDef->name().replace("_HS_DEF", "");
        WLDModel *model = m_models.value(actorName);
        if(!model)
            continue;
        model->setSkeleton(new WLDSkeleton(skelDef));
    }

    // import other animations
    WLDFragmentArray<TrackFragment> tracks = wld->table()->byKind<TrackFragment>();
    for(uint32_t i = 0; i < tracks.count(); i++)
    {
        TrackFragment *track = tracks[i];
        QString animName = track->name().left(3);
        QString actorName = track->name().mid(3, 3);
        WLDModel *model = m_models.value(actorName);
        if(!model)
            continue;
        WLDSkeleton *skel = model->skeleton();
        if(skel && track->m_def)
            skel->addTrack(animName, track->m_def);
    }
}

void CharacterPack::importCharacterPalettes(PFSArchive *archive, WLDData *wld)
{
    WLDFragmentArray<MaterialDefFragment> matDefs = wld->table()->byKind<MaterialDefFragment>();
    for(uint32_t i = 0; i < matDefs.count(); i++)
    {
        MaterialDefFragment *matDef = matDefs[i];
        if(matDef->handled())
            continue;
        QString charName, skinName, partName;
        if(WLDMaterialPalette::explodeName(matDef, charName, skinName, partName))
        {
            WLDModel *model = m_models.value(charName);
            if(!model)
                continue;
            WLDMaterialPalette *palette = model->mainMesh()->palette();
            WLDModelSkin *skin = model->skins().value(skinName);
            if(!skin)
                skin = model->newSkin(skinName);
            int skinID = skinName.toInt();
            WLDMaterialSlot *matSlot = palette->slotByName(partName);
            if(matSlot)
                matSlot->addSkinMaterial(skinID, matDef);
        }
    }
}

static bool findMainMesh(ActorDefFragment *actorDef, const QString &actorName,
                         MeshDefFragment *&meshDefOut, MaterialPaletteFragment *&palDefOut)
{
    if(!actorDef->m_models.size())
        return false;

    QString mainMeshName = actorName + "_DMSPRITEDEF";
    foreach(MeshDefFragment *meshDef, WLDModel::listMeshes(actorDef))
    {
        if(meshDef->name() == mainMeshName)
        {
            meshDefOut = meshDef;
            palDefOut = meshDef->m_palette;
            return true;
        }
    }
    return false;
}

void CharacterPack::importCharacters(PFSArchive *archive, WLDData *wld)
{
    WLDFragmentArray<ActorDefFragment> actorDefs = wld->table()->byKind<ActorDefFragment>();
    for(uint32_t i = 0; i < actorDefs.count(); i++)
    {
        ActorDefFragment *actorDef = actorDefs[i];
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        MeshDefFragment *mainMeshDef = NULL;
        MaterialPaletteFragment *mainPalette = NULL;
        if(!findMainMesh(actorDef, actorName, mainMeshDef, mainPalette))
        {
            qDebug("Warning: could not find main mesh for actor '%s'.", actorName.toLatin1().constData());
            continue;
        }

        // Create the main mesh.
        WLDMesh *mainMesh = new WLDMesh(mainMeshDef, 0);
        WLDMaterialPalette *pal = mainMesh->importPalette(archive);
        WLDModel *model = new WLDModel(mainMesh);
        WLDModelSkin *defaultSkin = model->skin();
        foreach(MeshDefFragment *meshDef, WLDModel::listMeshes(actorDef))
        {
            if(meshDef == mainMeshDef)
                continue;
            Q_ASSERT(mainPalette == meshDef->m_palette);
            defaultSkin->addPart(meshDef);
            pal->addMeshMaterials(meshDef, 0);
        }
        foreach(WLDFragment *frag, actorDef->m_models)
        {
            switch(frag->kind())
            {
            case HierSpriteFragment::KIND:
            case MeshFragment::KIND:
                break;
            default:
                qDebug("Unknown model fragment kind (0x%02x) %s",
                       frag->kind(), actorName.toLatin1().constData());
                break;
            }
        }

        m_models.insert(actorName, model);
    }
    
    // look for alternate meshes (e.g. heads)
    WLDFragmentArray<MeshDefFragment> meshDefs = wld->table()->byKind<MeshDefFragment>();
    for(uint32_t i = 0; i < meshDefs.count(); i++)
    {
        MeshDefFragment *meshDef = meshDefs[i];
        if(meshDef->handled())
            continue;
        QString actorName, meshName, skinName;
        WLDModelSkin::explodeMeshName(meshDef->name(), actorName, meshName, skinName);
        bool skinIsInt = false;
        uint32_t skinID = skinName.toUInt(&skinIsInt);
       WLDModel *model = m_models.value(actorName);
        if(!model || meshName.isEmpty() || !skinIsInt)
            continue;
        WLDModelSkin *skin = model->skins().value(skinName);
        if(!skin)
            skin = model->newSkin(skinName);
        WLDMaterialPalette *pal = model->mainMesh()->palette();
        foreach(WLDMesh *part, model->skin()->parts())
        {
            QString actorName2, meshName2, skinName2;
            WLDModelSkin::explodeMeshName(part->def()->name(), actorName2, meshName2, skinName2);
            if((meshName2 == meshName) && (skinName2 != skinName))
            {
                skin->replacePart(part, meshDef);
                pal->addMeshMaterials(meshDef, skinID);
            }
        }
    }
}

void CharacterPack::upload(RenderContext *renderCtx)
{
    foreach(WLDModel *model, m_models)
        upload(renderCtx, model);
}

void CharacterPack::upload(RenderContext *renderCtx, WLDModel *model)
{
    // Make sure we haven't uploaded this character before.
    if(!model || model->buffer())
        return;

    // Import materials.
    MaterialArray *materials = model->mainMesh()->palette()->createArray();
    materials->uploadArray(renderCtx);

    // Import mesh geometry.
    MeshBuffer *meshBuf = new MeshBuffer();
    model->setBuffer(meshBuf);
    foreach(WLDMesh *mesh, model->meshes())
    {
        mesh->importFrom(meshBuf);
        mesh->data()->updateTexCoords(materials, true);
    }

    // Create the GPU buffers.
    meshBuf->upload(renderCtx);

    // Free the memory used for indices. We need to keep the vertices around for software skinning.
    meshBuf->clearIndices();
}
