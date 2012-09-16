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
#include <QObject>
#include <QList>
#include <QMap>
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
class MaterialMap;
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
  \brief Describes a zone of the world.
  */
class GAME_DLL Zone : public QObject
{
public:
    Zone(Game *game);
    virtual ~Zone();

    ZoneTerrain * terrain() const;
    ZoneObjects * objects() const;
    const QVector<WLDLightActor *> & lights() const;
    QList<CharacterPack *> characterPacks() const;
    QList<ObjectPack *> objectPacks() const;
    OctreeIndex * actorIndex() const;
    const FogParams & fogParams() const;
    
    bool load(QString path, QString name);
    CharacterPack * loadCharacters(QString archivePath, QString wldName = QString::null);
    ObjectPack * loadObjects(QString archivePath, QString wldName = QString::null);
    WLDCharActor * findCharacter(QString name) const;

    void clear(RenderContext *renderCtx);
    void draw(RenderContext *renderCtx);

    // xyz position of the player in the zone
    const vec3 & playerPos() const;
    // z angle that describes where the player is facing
    float playerOrient() const;
    // xyz angles that describe how the camera is oriented rel. to the player
    const vec3 & cameraOrient() const;
    // xyz position of the camera is rel. to the player
    const vec3 & cameraPos() const;

    void setPlayerOrient(float rot);
    void setCameraOrient(const vec3 &rot);
    void step(float x, float y, float z);
    
    void freezeFrustum(RenderContext *renderCtx);
    void unFreezeFrustum();
    
    void currentSoundTriggers(QVector<SoundTrigger *> &triggers) const;

private:
    void setPlayerViewFrustum(Frustum &frustum) const;
    bool importLightSources(PFSArchive *archive);
    static void frustumCullingCallback(WLDActor *actor, void *user);

    Game *m_game;
    QString m_name;
    ZoneTerrain *m_terrain;
    ZoneObjects *m_objects;
    QList<CharacterPack *> m_charPacks;
    QList<ObjectPack *> m_objectPacks; // XXX this shouldn't be in Zone, maybe Game.
    PFSArchive *m_mainArchive;
    WLDData *m_mainWld;
    OctreeIndex *m_actorTree;
    QVector<WLDLightActor *> m_lights;
    QVector<SoundTrigger *> m_soundTriggers;
    Frustum m_frozenFrustum;
    // player and camera settings XXX replace by a WLDActor player
    vec3 m_playerPos;
    float m_playerOrient;
    vec3 m_cameraOrient;
    vec3 m_cameraPos;
    FogParams m_fogParams;
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

    bool load(PFSArchive *archive, WLDData *wld);
    void draw(RenderContext *renderCtx, RenderProgram *prog);
    void clear(RenderContext *renderCtx);
    void resetVisible();
    void showAllRegions(const Frustum &frustum);
    void showNearbyRegions(const Frustum &frustum);
    uint32_t findCurrentRegion(const vec3 &cameraPos);

private:
    uint32_t findCurrentRegion(const vec3 &cameraPos, const RegionTreeNode *nodes, uint32_t nodeIdx);
    MeshBuffer * upload(RenderContext *renderCtx);

    WLDData *m_zoneWld;
    uint32_t m_regionCount;
    uint32_t m_currentRegion;
    Zone *m_zone;
    std::vector<WLDStaticActor *> m_regionActors;
    std::vector<WLDStaticActor *> m_visibleRegions;
    RegionTreeFragment *m_regionTree;
    MeshBuffer *m_zoneBuffer;
    MaterialMap *m_zoneMaterials;
    AABox m_zoneBounds;
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

#endif
