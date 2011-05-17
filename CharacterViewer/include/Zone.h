#ifndef OPENEQ_ZONE_H
#define OPENEQ_ZONE_H

#include <QObject>
#include <QList>
#include <QMap>
#include "Platform.h"

class Mesh;
class PFSArchive;
class WLDData;
class WLDModel;
class WLDActor;
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

    const QMap<QString, WLDModel *> & objectModels() const;
    const QMap<QString, WLDActor *> & charModels() const;
    const QList<WLDActor *> & actors() const;

    bool load(QString path, QString name);
    bool loadCharacters(QString archivePath, QString wldName = QString::null);

    void clear();

    void drawGeometry(RenderState *state);
    void drawObjects(RenderState *state);

private:
    void importGeometry();
    void importObjects();
    void importSkeletons(PFSArchive *archive, WLDData *wld);
    void importCharacterPalettes(PFSArchive *archive, WLDData *wld);
    void importCharacters(PFSArchive *archive, WLDData *wld);

    QString m_name;
    WLDModel *m_geometry;
    PFSArchive *m_mainArchive;
    PFSArchive *m_objMeshArchive;
    PFSArchive *m_charArchive;
    WLDData *m_mainWld;
    WLDData *m_objMeshWld, *m_objDefWld;
    WLDData *m_charWld;
    QMap<QString, WLDModel *> m_objModels;
    QMap<QString, WLDActor *> m_charModels;
    QList<WLDActor *> m_actors;
};

#endif
