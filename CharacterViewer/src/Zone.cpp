#include <QFileInfo>
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

const QMap<QString, WLDActor *> & Zone::charModels() const
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
    importCharacters(m_charArchive, m_charWld);
    importCharacterPalettes(m_charArchive, m_charWld);
    importSkeletons(m_charArchive, m_charWld);
    return true;
}

bool Zone::loadCharacters(QString archivePath, QString wldName)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        if(baseName == "global_chr1")
            baseName = "global_chr";
        wldName = baseName + ".wld";
    }
    m_charArchive = new PFSArchive(archivePath, this);
    m_charWld = WLDData::fromArchive(m_charArchive, wldName, this);
    if(!m_charWld)
        return false;
    importCharacters(m_charArchive, m_charWld);
    importCharacterPalettes(m_charArchive, m_charWld);
    importSkeletons(m_charArchive, m_charWld);
    return true;
}

void Zone::clear()
{
    foreach(WLDModel *model, m_objModels)
        delete model;
    foreach(WLDActor *actor, m_charModels)
        delete actor;
    foreach(WLDActor *actor, m_actors)
        delete actor;
    m_objModels.clear();
    m_charModels.clear();
    m_actors.clear();
    delete m_geometry;
    delete m_mainWld;
    delete m_objMeshWld;
    delete m_objDefWld;
    delete m_charWld;
    delete m_mainArchive;
    delete m_objMeshArchive;
    delete m_charArchive;
    m_geometry = 0;
    m_mainWld = 0;
    m_objMeshWld = 0;
    m_objDefWld = 0;
    m_charWld = 0;
    m_mainArchive = 0;
    m_objMeshArchive = 0;
    m_charArchive = 0;
}

void Zone::importGeometry()
{
    m_geometry = new WLDModel(m_mainArchive, this);
    WLDModelSkin *skin = m_geometry->skin();
    foreach(MeshDefFragment *meshDef, m_mainWld->fragmentsByType<MeshDefFragment>())
        skin->addPart(meshDef, false);
    WLDMaterialPalette *palette = skin->palette();
    foreach(MaterialDefFragment *matDef, m_mainWld->fragmentsByType<MaterialDefFragment>())
        palette->addMaterialDef(matDef);
    skin->combineParts();
}

void Zone::importObjects()
{
    importObjectsMeshes(m_objMeshArchive, m_objMeshWld);

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

void Zone::importObjectsMeshes(PFSArchive *archive, WLDData *wld)
{
    // import models through ActorDef fragments
    foreach(ActorDefFragment *actorDef, wld->fragmentsByType<ActorDefFragment>())
    {
        WLDModel *model = new WLDModel(archive, this);
        WLDModelSkin *skin = model->skin();
        foreach(MeshDefFragment *meshDef, WLDModel::listMeshes(actorDef))
            skin->addPart(meshDef, true);
        m_objModels.insert(actorDef->name(), model);
    }
}

void Zone::importSkeletons(PFSArchive *archive, WLDData *wld)
{
    // import skeletons which contain the pose animation
    foreach(HierSpriteDefFragment *skelDef, wld->fragmentsByType<HierSpriteDefFragment>())
    {
        QString actorName = skelDef->name().replace("_HS_DEF", "");
        WLDActor *actor = m_charModels.value(actorName);
        if(!actor)
            continue;
        actor->model()->setSkeleton(new WLDSkeleton(skelDef, this));
    }

    // import other animations
    foreach(TrackFragment *track, wld->fragmentsByType<TrackFragment>())
    {
        QString animName = track->name().left(3);
        QString actorName = track->name().mid(3, 3);
        WLDActor *actor = m_charModels.value(actorName);
        if(!actor)
            continue;
        WLDSkeleton *skel = actor->model()->skeleton();
        if(skel && track->m_def)
            skel->addTrack(animName, track->m_def);
    }
}

void Zone::importCharacterPalettes(PFSArchive *archive, WLDData *wld)
{
    foreach(MaterialDefFragment *matDef, wld->fragmentsByType<MaterialDefFragment>())
    {
        QString charName, palName, partName;
        if(WLDMaterialPalette::explodeName(matDef, charName, palName, partName))
        {
            WLDActor *actor = m_charModels.value(charName);
            if(!actor)
                continue;
            WLDModel *model = actor->model();
            WLDModelSkin *skin = model->skins().value(palName);
            if(!skin)
                skin = model->newSkin(palName, archive);
            skin->palette()->addMaterialDef(matDef);
        }
    }

    // look for alternate meshes (e.g. heads)
    foreach(MeshDefFragment *meshDef, wld->fragmentsByType<MeshDefFragment>())
    {
        QString actorName, meshName, skinName;
        WLDModelSkin::explodeMeshName(meshDef->name(), actorName, meshName, skinName);
        WLDActor *actor = m_charModels.value(actorName);
        if(!actor || meshName.isEmpty())
            continue;
        WLDModel *model = actor->model();
        WLDModelSkin *skin = model->skins().value(skinName);
        if(!skin)
            continue;
        foreach(WLDModelPart *part, model->skin()->parts())
        {
            QString actorName2, meshName2, skinName2;
            WLDModelSkin::explodeMeshName(part->def()->name(), actorName2, meshName2, skinName2);
            if(meshName2 == meshName)
                skin->addPart(meshDef, false);
        }
    }
}

void Zone::importCharacters(PFSArchive *archive, WLDData *wld)
{
    foreach(ActorDefFragment *actorDef, wld->fragmentsByType<ActorDefFragment>())
    {
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        WLDModel *model = new WLDModel(archive, this);
        WLDActor *actor = new WLDActor(model, this);
        WLDModelSkin *skin = model->skin();
        foreach(MeshDefFragment *meshDef, WLDModel::listMeshes(actorDef))
            skin->addPart(meshDef);
        foreach(WLDFragment *frag, actorDef->m_models)
        {
            switch(frag->kind())
            {
            case HierSpriteFragment::ID:
            case MeshFragment::ID:
                break;
            default:
                qDebug("Unknown model fragment kind (0x%02x) %s",
                       frag->kind(), actorName.toLatin1().constData());
                break;
            }
        }

        m_charModels.insert(actorName, actor);
    }
}

void Zone::drawGeometry(RenderState *state)
{
    if(m_geometry)
        m_geometry->skin()->draw(state);
}

void Zone::drawObjects(RenderState *state)
{
    foreach(WLDActor *actor, m_actors)
        actor->draw(state);
}
