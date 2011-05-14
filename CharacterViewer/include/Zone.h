#ifndef OPENEQ_ZONE_H
#define OPENEQ_ZONE_H

#include <QObject>
#include <QList>
#include <QMap>
#include "Platform.h"
#include "RenderState.h"

class Mesh;
class PFSArchive;
class WLDData;
class WLDModel;
class WLDActor;
class ActorDefFragment;

/*!
  \brief Describes a zone of the world.
  */
class Zone : public QObject
{
public:
    Zone(QObject *parent = 0);
    virtual ~Zone();

    const QMap<QString, WLDModel *> & models() const;

    bool load(QString path, QString name);

private:
    void importGeometry();
    void importObjects();
    void importSkeletons();
    void importCharacters();

    QString m_name;
    QList<Mesh *> m_regionMeshes;
    PFSArchive *m_mainArchive;
    PFSArchive *m_objMeshArchive;
    PFSArchive *m_charArchive;
    WLDData *m_mainWld;
    WLDData *m_objMeshWld, *m_objDefWld;
    WLDData *m_charWld;
    QMap<QString, WLDModel *> m_models;
    QList<WLDActor *> m_actors;
};

#endif
