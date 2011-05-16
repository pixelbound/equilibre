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
        wldName = QFileInfo(archivePath).baseName() + ".wld";
    m_charArchive = new PFSArchive(archivePath, this);
    m_charWld = WLDData::fromArchive(m_charArchive, wldName, this);
    if(!m_charWld)
        return false;
    importCharacters(m_charArchive, m_charWld);
    importCharacterPalettes(m_charArchive, m_charWld);
    importSkeletons(m_charArchive, m_charWld);
    return true;
}

void Zone::importGeometry()
{
    m_geometry = new WLDModel(0, this);
    foreach(MeshDefFragment *meshDef, m_mainWld->fragmentsByType<MeshDefFragment>())
        m_geometry->addPart(meshDef);
    WLDMaterialPalette *palette = new WLDMaterialPalette(m_mainArchive, m_geometry);
    WLDModelSkin *skin = new WLDModelSkin("00", m_geometry, palette, m_geometry);
    foreach(MaterialDefFragment *matDef, m_mainWld->fragmentsByType<MaterialDefFragment>())
        palette->addMaterialDef(matDef);
    m_geometry->skins().insert(skin->name(), skin);
}

void Zone::importObjects()
{
    // import models through ActorDef fragments
    foreach(ActorDefFragment *actorDef, m_objMeshWld->fragmentsByType<ActorDefFragment>())
        m_objModels.insert(actorDef->name(), new WLDModel(actorDef, this));

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

void Zone::importSkeletons(PFSArchive *archive, WLDData *wld)
{
    // import skeletons which contain the pose animation
    foreach(HierSpriteDefFragment *skelDef, wld->fragmentsByType<HierSpriteDefFragment>())
    {
        QString actorName = skelDef->name().left(3);
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
            WLDMaterialPalette *pal;
            if(!skin)
            {
                pal = new WLDMaterialPalette(archive, model);
                skin = new WLDModelSkin(palName, model, pal, model);
                model->skins().insert(palName, skin);
            }
            skin->palette()->addMaterialDef(matDef);
        }
    }

    // look for alternate meshes (e.g. heads)
    foreach(MeshDefFragment *meshDef, wld->fragmentsByType<MeshDefFragment>())
    {
        QString actorName = meshDef->name().left(3);
        QString meshName = meshDef->name().mid(3).replace("_DMSPRITEDEF", "");
        QString skinName = meshName.right(2);
        meshName = meshName.left(meshName.length() - 2);
        WLDActor *actor = m_charModels.value(actorName);
        if(!actor || meshName.isEmpty())
            continue;
        meshName = actorName + meshName;
        WLDModel *model = actor->model();
        WLDModelSkin *skin = model->skins().value(skinName);
        if(!skin)
            continue;
        foreach(WLDModelPart *part, model->parts())
        {
            if(part->name().startsWith(meshName))
                skin->addPart(meshDef);
        }
    }
}

void Zone::importCharacters(PFSArchive *archive, WLDData *wld)
{
    foreach(ActorDefFragment *actorDef, wld->fragmentsByType<ActorDefFragment>())
    {
        QString actorName = actorDef->name().left(3);
        WLDModel *model = new WLDModel(actorDef, this);
        WLDActor *actor = new WLDActor(model, this);
        m_charModels.insert(actorName, actor);
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
