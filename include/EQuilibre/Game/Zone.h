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

#ifndef EQUILIBRE_ZONE_H
#define EQUILIBRE_ZONE_H

#include <vector>
#include <QList>
#include <QMap>
#include "Newton.h"
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"
#include "EQuilibre/Render/RenderContext.h"

class Game;
class PFSArchive;
class WLDData;
class WLDModel;
class WLDMesh;
class WLDActor;
class WLDStaticActor;
class WLDCharActor;
class WLDLightActor;
class ActorIndex;
class ActorIndexNode;
class OctreeIndex;
class WLDSkeleton;
class WLDMaterialPalette;
class MaterialArray;
class MaterialMap;
struct ActorState;
class ActorDefFragment;
class RegionTreeFragment;
class RegionFragment;
struct RegionTreeNode;
class SoundTrigger;
class RenderContext;
class FrameStat;
class MeshBuffer;
class ZoneTerrain;
class ZoneObjects;
class CharacterPack;
class ObjectPack;

/*!
  \brief Contains some rendering information about a zone.
  */
class GAME_DLL ZoneInfo
{
public:
    ZoneInfo();
    QString name;
    int skyID;
    vec4 fogColor;
    float fogMinClip;
    float fogMaxClip;
    float minClip;
    float maxClip;
    vec3 safePos;
    float underworldZ;
    int flags;
};

/*!
  \brief Describes a zone of the world.
  */
class GAME_DLL Zone
{
public:
    Zone(Game *game);
    virtual ~Zone();

    ZoneTerrain * terrain() const;
    ZoneObjects * objects() const;
    const QVector<WLDLightActor *> & lights() const;
    QList<CharacterPack *> characterPacks() const;
    OctreeIndex * actorIndex() const;
    NewtonWorld * collisionWorld();
    const ZoneInfo & info() const;
    void setInfo(const ZoneInfo &info);
    
    Game * game() const;
    // XXX PlayerActor class.
    WLDCharActor * player() const;
    
    bool load(QString path, QString name);
    bool loadSky(PFSArchive *archive, WLDData *wld);
    CharacterPack * loadCharacters(QString archivePath, QString wldName = QString::null);

    void clear(RenderContext *renderCtx);
    void draw(RenderContext *renderCtx, RenderProgram *prog);
    void update(RenderContext *renderCtx, double currentTime,
                double sinceLastUpdate);
    
    void acceptPlayer(WLDCharActor *player, const vec3 &initialPos);
    void playerJumped();
    
    void freezeFrustum(RenderContext *renderCtx);
    void unFreezeFrustum();
    
    void currentSoundTriggers(QVector<SoundTrigger *> &triggers) const;

private:
    bool importLightSources(PFSArchive *archive);
    static void frustumCullingCallback(WLDActor *actor, void *user);
    void updateMovement(double sinceLastUpdate);
    void handlePlayerCollisions(ActorState &state);

    Game *m_game;
    WLDCharActor *m_player;
    ZoneInfo m_info;
    ZoneTerrain *m_terrain;
    ZoneObjects *m_objects;
    QList<CharacterPack *> m_charPacks;
    PFSArchive *m_mainArchive;
    WLDData *m_mainWld;
    OctreeIndex *m_actorTree;
    QVector<WLDLightActor *> m_lights;
    QVector<SoundTrigger *> m_soundTriggers;
    Frustum m_frustum;
    Frustum m_frozenFrustum;
    
    // Duration between the newest movement tick and the current frame.
    double m_movementAheadTime;
    FrameStat *m_collisionChecksStat;
    int m_collisionChecks;
    NewtonWorld *m_collisionWorld;
};

/*!
  \brief Holds the resources needed to render a zone's terrain.
  */
class GAME_DLL ZoneTerrain
{
public:
    ZoneTerrain(Zone *zone);
    virtual ~ZoneTerrain();
    
    const AABox & bounds() const;
    uint32_t currentRegion() const;
    NewtonCollision * currentRegionShape() const;

    bool load(PFSArchive *archive, WLDData *wld);
    void update(double currentTime);
    void draw(RenderContext *renderCtx, RenderProgram *prog);
    void clear(RenderContext *renderCtx);
    void resetVisible();
    void showAllRegions(const Frustum &frustum);
    void showNearbyRegions(const Frustum &frustum);
    void showCurrentRegion(const Frustum &frustum);
    uint32_t findRegionShapes(Sphere sphere, NewtonCollision **regions,
                              uint32_t maxRegions);
    uint32_t findNearbyRegionShapes(NewtonCollision **firstRegion,
                                    uint32_t maxRegions);
    uint32_t findAllRegionShapes(NewtonCollision **firstRegion,
                                 uint32_t maxRegions);
    uint32_t findCurrentRegion(const vec3 &cameraPos);
    NewtonCollision * loadRegionShape(uint32_t regionID);

private:
    uint32_t findRegion(const vec3 &pos, const RegionTreeNode *nodes, uint32_t nodeIdx);
    void findRegionShapes(const Sphere &sphere, const RegionTreeNode *nodes,
                          uint32_t nodeIdx, NewtonCollision **regions,
                          uint32_t maxRegions, uint32_t &found);
    void upload(RenderContext *renderCtx);

    WLDData *m_zoneWld;
    uint32_t m_regionCount;
    uint32_t m_currentRegion;
    Zone *m_zone;
    std::vector<WLDStaticActor *> m_regionActors;
    std::vector<WLDStaticActor *> m_visibleRegions;
    RegionTreeFragment *m_regionTree;
    MeshBuffer *m_zoneBuffer;
    WLDMaterialPalette *m_palette;
    bool m_uploaded;
    AABox m_zoneBounds;
    std::vector<NewtonCollision *> m_regionShapes;
    FrameStat *m_zoneStat;
    FrameStat *m_zoneStatGPU;
};

/*!
  \brief Holds the resources needed to render a zone's static objects.
  */
class GAME_DLL ZoneObjects
{
public:
    ZoneObjects(Zone *zone);
    virtual ~ZoneObjects();
    
    const AABox & bounds() const;
    const QMap<QString, WLDMesh *> & models() const;
    QVector<WLDStaticActor *> & visibleObjects();

    bool load(QString path, QString name, PFSArchive *mainArchive);
    void addTo(OctreeIndex *tree);
    void update(double currentTime);
    void draw(RenderContext *renderCtx, RenderProgram *prog);
    void clear(RenderContext *renderCtx);
    void resetVisible();

private:
    void importMeshes();
    void importActors();
    void upload(RenderContext *renderCtx);
    
    Zone *m_zone;
    AABox m_bounds;
    ObjectPack *m_pack;
    WLDData *m_objDefWld;
    QVector<WLDStaticActor *> m_objects;
    QVector<WLDStaticActor *> m_visibleObjects;
    FrameStat *m_objectsStat;
    FrameStat *m_objectsStatGPU;
    FrameStat *m_drawnObjectsStat;
};

class GAME_DLL SkyDef
{
public:
    SkyDef();

    WLDMesh *mainLayer;
    WLDMesh *secondLayer;
};

/*!
  \brief Holds the resources needed to render a zone's sky dome.
  */
class GAME_DLL ZoneSky
{
public:
    ZoneSky();
    virtual ~ZoneSky();
    void clear(RenderContext *renderCtx);
    bool upload(RenderContext *renderCtx);
    bool load(QString path);
    void draw(RenderContext *renderCtx, RenderProgram *prog, Zone *zone);
    
private:
    PFSArchive *m_skyArchive;
    WLDData *m_skyWld;
    MaterialArray *m_skyMaterials;
    std::vector<SkyDef> m_skyDefs;
    MeshBuffer *m_skyBuffer;
};

#endif
