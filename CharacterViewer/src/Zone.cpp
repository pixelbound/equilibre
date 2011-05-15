#include "Zone.h"
#include "PFSArchive.h"
#include "WLDData.h"
#include "WLDModel.h"
#include "WLDActor.h"
#include "WLDSkeleton.h"
#include "Fragments.h"

Zone::Zone(QObject *parent) : QObject(parent)
{
    m_mainArchive = 0;
    m_objMeshArchive = 0;
    m_charArchive = 0;
    m_mainWld = 0;
    m_objMeshWld = m_objDefWld = 0;
    m_charWld = 0;
    m_geometry = 0;
}

Zone::~Zone()
{
}

const QMap<QString, WLDModel *> & Zone::objectModels() const
{
    return m_objModels;
}

const QMap<QString, WLDModel *> & Zone::charModels() const
{
    return m_charModels;
}

const QList<WLDActor *> & Zone::actors() const
{
    return m_actors;
}

bool Zone::load(QString path, QString name)
{
    m_name = name;

    // load archives and WLD files
    QString zonePath = QString("%1/%2.s3d").arg(path).arg(name);
    QString zoneFile = QString("%1.wld").arg(name);
    m_mainArchive = new PFSArchive(zonePath, this);
    m_mainWld = WLDData::fromArchive(m_mainArchive, zoneFile, this);
    m_objDefWld = WLDData::fromArchive(m_mainArchive, "objects.wld", this);
    if(!m_mainWld || !m_objDefWld)
        return false;
    QString objMeshPath = QString("%1/%2_obj.s3d").arg(path).arg(name);
    QString objMeshFile = QString("%1_obj.wld").arg(name);
    m_objMeshArchive = new PFSArchive(objMeshPath, this);
    m_objMeshWld = WLDData::fromArchive(m_objMeshArchive, objMeshFile, this);
    if(!m_objMeshWld)
        return false;
    QString charPath = QString("%1/%2_chr.s3d").arg(path).arg(name);
    QString charFile = QString("%1_chr.wld").arg(name);
    m_charArchive = new PFSArchive(charPath, this);
    m_charWld = WLDData::fromArchive(m_charArchive, charFile, this);
    if(!m_charWld)
        return false;

    // import geometry, objects, characters
    importGeometry();
    importObjects();
    importSkeletons();
    importCharacters();
    return true;
}

bool Zone::loadCharacters(QString archivePath, QString wldName)
{
    m_charArchive = new PFSArchive(archivePath, this);
    m_charWld = WLDData::fromArchive(m_charArchive, wldName, this);
    if(!m_charWld)
        return false;
    importSkeletons();
    importCharacters();
    return true;
}

void Zone::importGeometry()
{
    m_geometry = new WLDModel(m_mainArchive, 0, 0, this);
    foreach(MeshDefFragment *meshDef, m_mainWld->fragmentsByType<MeshDefFragment>())
        m_geometry->importMesh(meshDef);
}

void Zone::importObjects()
{
    // import models through ActorDef fragments
    foreach(ActorDefFragment *actorDef, m_objMeshWld->fragmentsByType<ActorDefFragment>())
        m_objModels.insert(actorDef->name(), new WLDModel(m_objMeshArchive, actorDef, 0, this));

    // import actors through Actor fragments
    foreach(ActorFragment *actorFrag, m_objDefWld->fragmentsByType<ActorFragment>())
    {
        WLDModel *model = m_objModels.value(actorFrag->m_def.name());
        if(model)
        {
            WLDActor *actor = new WLDActor(actorFrag, model, this);
            m_actors.append(actor);
        }
        else
        {
            qDebug("Actor '%s' not found", actorFrag->m_def.name().toLatin1().constData());
        }
    }
}

void Zone::importSkeletons()
{
    // import skeletons which contain the pose animation
    foreach(HierSpriteDefFragment *skelDef, m_charWld->fragmentsByType<HierSpriteDefFragment>())
        m_skeletons.insert(skelDef->name().left(3), new WLDSkeleton(skelDef, this));

    // import other animations
    foreach(TrackFragment *track, m_charWld->fragmentsByType<TrackFragment>())
    {
        QString animName = track->name().left(3);
        QString skelName = track->name().mid(3, 3);
        WLDSkeleton *skel = m_skeletons.value(skelName);
        if(skel && track->m_def)
            skel->addTrack(animName, track->m_def);
    }
}

void Zone::importCharacters()
{
    foreach(ActorDefFragment *actorDef, m_charWld->fragmentsByType<ActorDefFragment>())
    {
        QString actorName = actorDef->name().left(3);
        WLDSkeleton *skel = m_skeletons.value(actorName);
        m_charModels.insert(actorDef->name(), new WLDModel(m_charArchive, actorDef, skel, this));
    }
}

void Zone::drawGeometry(RenderState *state)
{
    if(m_geometry)
        m_geometry->draw(state);
}

void Zone::drawObjects(RenderState *state)
{
    foreach(WLDActor *actor, m_actors)
        actor->draw(state);
}
