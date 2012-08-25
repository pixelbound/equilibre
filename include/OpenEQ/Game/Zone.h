#ifndef OPENEQ_ZONE_H
#define OPENEQ_ZONE_H

#include <QObject>
#include <QList>
#include <QMap>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/Vertex.h"
#include "OpenEQ/Render/Geometry.h"

class PFSArchive;
class WLDData;
class WLDModel;
class WLDMesh;
class WLDActor;
class ActorIndex;
class ActorIndexNode;
class OctreeIndex;
class WLDSkeleton;
class WLDMaterialPalette;
class MaterialMap;
class ActorDefFragment;
class SoundTrigger;
class RenderState;
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
    QList<CharacterPack *> characterPacks() const;
    QList<ObjectPack *> objectPacks() const;
    
    bool load(QString path, QString name);
    CharacterPack * loadCharacters(QString archivePath, QString wldName = QString::null);
    ObjectPack * loadObjects(QString archivePath, QString wldName = QString::null);
    WLDActor * findCharacter(QString name) const;

    void clear();

    void draw(RenderState *state);

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
    
    void freezeFrustum(RenderState *state);
    void unFreezeFrustum();
    
    void currentSoundTriggers(QVector<SoundTrigger *> &triggers) const;

private:
    void setPlayerViewFrustum(Frustum &frustum) const;

    QString m_name;
    ZoneTerrain *m_terrain;
    ZoneObjects *m_objects;
    QList<CharacterPack *> m_charPacks;
    QList<ObjectPack *> m_objectPacks; // XXX this shouldn't be in Zone, maybe Game.
    PFSArchive *m_mainArchive;
    WLDData *m_mainWld;
    QVector<buffer_t> m_gpuBuffers;
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

    bool load(PFSArchive *archive, WLDData *wld);
    void draw(RenderState *state, Frustum &frustum);
    void clear();

private:
    MeshBuffer * upload(RenderState *state);

    Zone *m_zone;
    QVector<WLDMesh *> m_zoneParts;
    MeshBuffer *m_zoneBuffer;
    OctreeIndex *m_zoneTree;
    MaterialMap *m_zoneMaterials;
    AABox m_zoneBounds;
    FrameStat *m_zoneStat;
    FrameStat *m_zoneStatGPU;
    QVector<WLDActor *> m_visibleZoneParts;
};

/*!
  \brief Holds the resources needed to render a zone's static objects.
  */
class GAME_DLL ZoneObjects
{
public:
    ZoneObjects(Zone *zone);
    virtual ~ZoneObjects();
    
    const QMap<QString, WLDMesh *> & models() const;

    bool load(QString path, QString name, PFSArchive *mainArchive);
    void draw(RenderState *state, Frustum &frustum);
    void clear();

private:
    void importMeshes();
    void importActors();
    MeshBuffer * upload(RenderState *state);
    
    Zone *m_zone;
    ObjectPack *m_pack;
    WLDData *m_objDefWld;
    QVector<WLDActor *> m_objects;
    QVector<WLDActor *> m_visibleObjects;
    OctreeIndex *m_objectTree;
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
    
    const QMap<QString, WLDMesh *> models() const;
    MeshBuffer * buffer() const;
    MaterialMap * materials() const;
    
    bool load(QString archivePath, QString wldName);
    MeshBuffer * upload(RenderState *state);
    void clear();
    
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
    
    const QMap<QString, WLDActor *> models() const;
    
    bool load(QString archivePath, QString wldName);
    void upload(RenderState *state);
    void clear();
    
private:
    void importSkeletons(WLDData *wld);
    void importCharacterPalettes(PFSArchive *archive, WLDData *wld);
    void importCharacters(PFSArchive *archive, WLDData *wld);
    void upload(RenderState *state, WLDActor *actor);
    
    PFSArchive *m_archive;
    WLDData *m_wld;
    QMap<QString, WLDActor *> m_models;
};

#endif
