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

#include <QObject>
#include <QList>
#include <QMap>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"
#include "EQuilibre/Render/RenderContext.h"

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
    Zone(QObject *parent = 0);
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

    bool showZone() const;
    bool showObjects() const;
    bool cullObjects() const;
    bool showSoundTriggers() const;
    bool frustumIsFrozen() const;
    
    void setShowZone(bool show);
    void setShowObjects(bool show);
    void setCullObjects(bool enabled);
    void setShowSoundTriggers(bool show);
    
    void freezeFrustum(RenderContext *renderCtx);
    void unFreezeFrustum();
    
    void currentSoundTriggers(QVector<SoundTrigger *> &triggers) const;

private:
    void setPlayerViewFrustum(Frustum &frustum) const;
    bool importLightSources(PFSArchive *archive);
    static void frustumCullingCallback(WLDActor *actor, void *user);

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
    bool m_showZone;
    bool m_showObjects;
    bool m_cullObjects;
    bool m_showSoundTriggers;
    bool m_frustumIsFrozen;
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
    QVector<WLDStaticActor *> & visibleZoneParts();

    bool load(PFSArchive *archive, WLDData *wld);
    void addTo(OctreeIndex *tree);
    void draw(RenderContext *renderCtx, ShaderProgramGL2 *prog);
    void clear(RenderContext *renderCtx);
    void resetVisible();

private:
    MeshBuffer * upload(RenderContext *renderCtx);

    Zone *m_zone;
    QVector<WLDStaticActor *> m_zoneParts;
    QVector<WLDStaticActor *> m_visibleZoneParts;
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
    void draw(RenderContext *renderCtx, ShaderProgramGL2 *prog);
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
    LightParams m_lightsInRange[8];
    FrameStat *m_objectsStat;
    FrameStat *m_objectsStatGPU;
    FrameStat *m_drawnObjectsStat;
};

/*!
  \brief Holds the resources needed to render static objects.
  */
class GAME_DLL ObjectPack
{
public:
    ObjectPack();
    virtual ~ObjectPack();
    
    const QMap<QString, WLDMesh *> & models() const;
    MeshBuffer * buffer() const;
    MaterialMap * materials() const;
    
    bool load(QString archivePath, QString wldName);
    MeshBuffer * upload(RenderContext *renderCtx);
    void clear(RenderContext *renderCtx);
    
private:
    PFSArchive *m_archive;
    WLDData *m_wld;
    QMap<QString, WLDMesh *> m_models;
    MeshBuffer *m_meshBuf;
    MaterialMap *m_materials;
};

/*!
  \brief Holds the resources needed to render characters.
  */
class GAME_DLL CharacterPack
{
public:
    CharacterPack();
    virtual ~CharacterPack();
    
    const QMap<QString, WLDCharActor *> models() const;
    
    bool load(QString archivePath, QString wldName);
    void upload(RenderContext *renderCtx);
    void clear(RenderContext *renderCtx);
    
private:
    void importSkeletons(WLDData *wld);
    void importCharacterPalettes(PFSArchive *archive, WLDData *wld);
    void importCharacters(PFSArchive *archive, WLDData *wld);
    void upload(RenderContext *renderCtx, WLDCharActor *actor);
    
    PFSArchive *m_archive;
    WLDData *m_wld;
    QMap<QString, WLDCharActor *> m_models;
};

#endif
