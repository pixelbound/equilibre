#ifndef OPENEQ_ZONE_H
#define OPENEQ_ZONE_H

#include <QObject>
#include <QList>
#include <QMap>
#include "Platform.h"
#include "Vertex.h"

class Mesh;
class PFSArchive;
class WLDData;
class WLDModel;
class WLDModelPart;
class WLDActor;
class WLDZoneActor;
class ActorIndex;
class ActorIndexNode;
class WLDSkeleton;
class WLDMaterialPalette;
class ActorDefFragment;
class RenderState;

/*!
  \brief Describes a zone of the world.
  */
class Zone : public QObject
{
public:
    Zone(QObject *parent = 0);
    virtual ~Zone();

    const QMap<QString, WLDModelPart *> & objectModels() const;
    const QMap<QString, WLDActor *> & charModels() const;
    const QList<WLDZoneActor> & actors() const;

    bool load(QString path, QString name);
    bool loadCharacters(QString archivePath, QString wldName = QString::null);

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

    bool showObjects() const;
    bool cullObjects() const;

    void setShowObjects(bool show);
    void setCullObjects(bool enabled);

private:
    void importGeometry();
    void importObjects();
    void importObjectsMeshes(PFSArchive *archive, WLDData *wld);
    void importSkeletons(PFSArchive *archive, WLDData *wld);
    void importCharacterPalettes(PFSArchive *archive, WLDData *wld);
    void importCharacters(PFSArchive *archive, WLDData *wld);
    void drawVisibleObjects(RenderState *state, ActorIndexNode *node, Frustum &f);
    void drawObjects(RenderState *state, ActorIndexNode *node);

    //TODO refactor this into data container classes
    QString m_name;
    VertexGroup *m_zoneGeometry;
    WLDMaterialPalette *m_zonePalette;
    PFSArchive *m_mainArchive;
    PFSArchive *m_objMeshArchive;
    PFSArchive *m_charArchive;
    WLDData *m_mainWld;
    WLDData *m_objMeshWld, *m_objDefWld;
    WLDData *m_charWld;
    QMap<QString, WLDModelPart *> m_objModels;
    QMap<QString, WLDMaterialPalette *> m_objPalettes;
    QMap<QString, WLDActor *> m_charModels;
    ActorIndex *m_index;
    bool m_showObjects;
    bool m_cullObjects;
    // player and camera settings
    vec3 m_playerPos;
    float m_playerOrient;
    vec3 m_cameraOrient;
    vec3 m_cameraPos;
};

#endif
