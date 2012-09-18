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

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDData.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"

Game::Game()
{
    m_zone = NULL;
    m_sky = NULL;
    m_showZone = true;
    m_showObjects = true;
    m_showFog = false;
    m_cullObjects = true;
    m_showSoundTriggers = false;
    m_frustumIsFrozen = false;
}

Game::~Game()
{
    clear(NULL);
}

void Game::clear(RenderContext *renderCtx)
{
    clearZone(renderCtx);
    if(m_sky)
    {
        m_sky->clear(renderCtx);
        delete m_sky;
        m_sky = NULL;
    }
    foreach(ObjectPack *pack, m_objectPacks)
    {
        pack->clear(renderCtx);
        delete pack;
    }
    foreach(CharacterPack *pack, m_charPacks)
    {
        pack->clear(renderCtx);
        delete pack;
    }
    m_objectPacks.clear();
    m_charPacks.clear();
}

void Game::clearZone(RenderContext *renderCtx)
{
    if(m_zone)
    {
        m_zone->clear(renderCtx);
        delete m_zone;
        m_zone = NULL;
    }
}

bool Game::showZone() const
{
    return m_showZone;
}

void Game::setShowZone(bool show)
{
    m_showZone = show;
}

bool Game::showObjects() const
{
    return m_showObjects;
}

void Game::setShowObjects(bool show)
{
    m_showObjects = show;
}

bool Game::showFog() const
{
    return m_showFog;
}

void Game::setShowFog(bool show)
{
    m_showFog = show;
}

bool Game::cullObjects() const
{
    return m_cullObjects;
}

void Game::setCullObjects(bool enabled)
{
    m_cullObjects = enabled;
}

bool Game::frustumIsFrozen() const
{
    return m_frustumIsFrozen;
}

void Game::freezeFrustum(RenderContext *renderCtx)
{
    if(m_zone && !m_frustumIsFrozen)
    {
        m_zone->freezeFrustum(renderCtx);
        m_frustumIsFrozen = true;    
    }
}

void Game::unFreezeFrustum()
{
    m_frustumIsFrozen = false;
}

bool Game::showSoundTriggers() const
{
    return m_showSoundTriggers;
}

void Game::setShowSoundTriggers(bool show)
{
    m_showSoundTriggers = show;
}

Zone * Game::zone() const
{
    return m_zone;
}

ZoneSky * Game::sky() const
{
    return m_sky;
}

float Game::fogDensity() const
{
    return m_showFog ? 0.003f : 0.0f;
}

QList<ObjectPack *> Game::objectPacks() const
{
    return m_objectPacks;
}

QList<CharacterPack *> Game::characterPacks() const
{
    return m_charPacks;
}

Zone * Game::loadZone(QString path, QString name)
{
    if(m_zone)
        return NULL;
    Zone *zone = new Zone(this);
    if(!zone->load(path, name))
    {
        delete zone;
        return NULL;
    }
    m_zone = zone;
    if(m_zoneInfo.contains(name))
    {
        const ZoneInfo &info = m_zoneInfo[name];
        m_zone->setInfo(info);
        m_zone->setPlayerPos(info.safePos);
    }
    else
    {
        m_zone->setPlayerPos(vec3(0.0, 0.0, 0.1));
    }
    return zone;
}

bool Game::loadZoneInfo(QString filePath)
{
    QFile file(filePath);
    if(!file.open(QFile::ReadOnly))
        return false;
    QTextStream s(&file);
    while(!s.atEnd())
    {
        QString line = s.readLine();
        if(line.isEmpty() || line.startsWith("//"))
            continue;
        QStringList fields = line.split(",");
        if(fields.count() < 14)
            continue;
        
        ZoneInfo zi;
        float fogFactor = 0.6f / 255.0f;
        int idx = 0;
        zi.name = fields.value(idx++);
        zi.skyID = fields.value(idx++).toInt();
        zi.fogColor.x = (fields.value(idx++).toInt() * fogFactor);
        zi.fogColor.y = (fields.value(idx++).toInt() * fogFactor);
        zi.fogColor.z = (fields.value(idx++).toInt() * fogFactor);
        zi.fogColor.w = 1.0f;
        zi.fogMinClip = fields.value(idx++).toFloat();
        zi.fogMaxClip = fields.value(idx++).toFloat();
        zi.minClip = fields.value(idx++).toFloat();
        zi.maxClip = fields.value(idx++).toFloat();
        zi.safePos.y = fields.value(idx++).toFloat();
        zi.safePos.z = fields.value(idx++).toFloat();
        zi.safePos.z = fields.value(idx++).toFloat();
        zi.underworldZ = fields.value(idx++).toFloat();
        zi.flags = fields.value(idx++).toInt();
        m_zoneInfo.insert(zi.name, zi);
    }
    m_showFog = true;
    return true;
}

bool Game::loadSky(QString path)
{
    ZoneSky *sky = new ZoneSky();
    if(!sky->load(path))
    {
        delete sky;
        return false;
    }
    m_sky = sky;
    return true;
}

ObjectPack * Game::loadObjects(QString archivePath, QString wldName)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        wldName = baseName + ".wld";
    }
    
    ObjectPack *objPack = new ObjectPack();
    if(!objPack->load(archivePath, wldName))
    {
        delete objPack;
        return NULL;
    }
    m_objectPacks.append(objPack);
    return objPack;
}

CharacterPack * Game::loadCharacters(QString archivePath, QString wldName, bool own)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        if(baseName == "global_chr1")
            baseName = "global_chr";
        wldName = baseName + ".wld";
    }
    
    CharacterPack *charPack = new CharacterPack();
    if(!charPack->load(archivePath, wldName))
    {
        delete charPack;
        return NULL;
    }
    if(own)
        m_charPacks.append(charPack);
    return charPack;
}

WLDCharActor * Game::findCharacter(QString name) const
{
    foreach(CharacterPack *pack, m_charPacks)
    {
        if(pack->models().contains(name))
            return pack->models().value(name);
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

ObjectPack::ObjectPack()
{
    m_archive = NULL;
    m_wld = NULL;
    m_meshBuf = NULL;
}

ObjectPack::~ObjectPack()
{
    clear(NULL);
}

const QMap<QString, WLDMesh *> & ObjectPack::models() const
{
    return m_models;
}

MeshBuffer * ObjectPack::buffer() const
{
    return m_meshBuf;
}

MaterialMap * ObjectPack::materials() const
{
    return m_materials;
}

void ObjectPack::clear(RenderContext *renderCtx)
{
    foreach(WLDMesh *model, m_models)
        delete model;
    m_models.clear();
    if(m_meshBuf)
    {
        m_meshBuf->clear(renderCtx);
        delete m_meshBuf;
        m_meshBuf = NULL;
    }
    delete m_wld;
    m_wld = NULL;
    delete m_archive;
    m_archive = NULL;
}

bool ObjectPack::load(QString archivePath, QString wldName)
{
    m_archive = new PFSArchive(archivePath);
    m_wld = WLDData::fromArchive(m_archive, wldName);
    if(!m_wld)
        return false;

    // Import models through ActorDef fragments.
    WLDMaterialPalette palette(m_archive);
    WLDFragmentArray<ActorDefFragment> actorDefs = m_wld->table()->byKind<ActorDefFragment>();
    for(uint32_t i = 0; i < actorDefs.count(); i++)
    {
        ActorDefFragment *actorDef = actorDefs[i];
        WLDFragment *subModel = actorDef->m_models.value(0);
        if(!subModel)
            continue;
        MeshFragment *mesh = subModel->cast<MeshFragment>();
        if(!mesh)
        {
            if(subModel->kind() == HierSpriteFragment::ID)
                qDebug("Hierarchical model in zone objects (%s)",
                       actorDef->name().toLatin1().constData());
            continue;
        }
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        WLDMesh *model = new WLDMesh(mesh->m_def, 0);
        palette.addPaletteDef(mesh->m_def->m_palette);
        m_models.insert(actorName, model);
    }
    m_materials = palette.loadMaterials();
    return true;
}

MeshBuffer * ObjectPack::upload(RenderContext *renderCtx)
{
    m_materials->uploadArray(renderCtx);
    
    // Import vertices and indices for each mesh.
    m_meshBuf = new MeshBuffer();
    foreach(WLDMesh *mesh, m_models.values())
    {
        MeshData *meshData = mesh->importFrom(m_meshBuf);
        meshData->updateTexCoords(m_materials);
    }
    
    // Create the GPU buffers.
    m_meshBuf->upload(renderCtx);
    m_meshBuf->clearVertices();
    m_meshBuf->clearIndices();
    return m_meshBuf;
}

////////////////////////////////////////////////////////////////////////////////

CharacterPack::CharacterPack()
{
    m_archive = NULL;
    m_wld = NULL;
}

CharacterPack::~CharacterPack()
{
    clear(NULL);
}

const QMap<QString, WLDCharActor *> CharacterPack::models() const
{
    return m_models;
}

void CharacterPack::clear(RenderContext *renderCtx)
{
    foreach(WLDCharActor *actor, m_models)
    {
        WLDModel *model = actor->model();
        MeshBuffer *meshBuf = model->buffer();
        if(meshBuf)
        {
            meshBuf->clear(renderCtx);
            delete meshBuf;
        }
        delete model->skeleton();
        delete model;
        delete actor;
    }
    m_models.clear();
    delete m_wld;
    delete m_archive;
    m_wld = 0;
    m_archive = 0;
}

bool CharacterPack::load(QString archivePath, QString wldName)
{
    m_archive = new PFSArchive(archivePath);
    m_wld = WLDData::fromArchive(m_archive, wldName);
    if(!m_wld)
    {
        delete m_archive;
        m_archive = 0;
        return false;
    }

    importCharacters(m_archive, m_wld);
    importCharacterPalettes(m_archive, m_wld);
    importSkeletons(m_wld);
    return true;
}

void CharacterPack::importSkeletons(WLDData *wld)
{
    // XXX add a actorName+animName -> WLDAnimation map that contains all animations in the pack (no duplicate).
    // Makes it easier to free the resources even if some animations are shared between actors.
    
    // import skeletons which contain the pose animation
    WLDFragmentArray<HierSpriteDefFragment> skelDefs = wld->table()->byKind<HierSpriteDefFragment>();
    for(uint32_t i = 0; i < skelDefs.count(); i++)
    {
        HierSpriteDefFragment *skelDef = skelDefs[i];
        QString actorName = skelDef->name().replace("_HS_DEF", "");
        WLDCharActor *actor = m_models.value(actorName);
        if(!actor)
            continue;
        actor->model()->setSkeleton(new WLDSkeleton(skelDef));
    }

    // import other animations
    WLDFragmentArray<TrackFragment> tracks = wld->table()->byKind<TrackFragment>();
    for(uint32_t i = 0; i < tracks.count(); i++)
    {
        TrackFragment *track = tracks[i];
        QString animName = track->name().left(3);
        QString actorName = track->name().mid(3, 3);
        WLDCharActor *actor = m_models.value(actorName);
        if(!actor)
            continue;
        WLDSkeleton *skel = actor->model()->skeleton();
        if(skel && track->m_def)
            skel->addTrack(animName, track->m_def);
    }
}

void CharacterPack::importCharacterPalettes(PFSArchive *archive, WLDData *wld)
{
    WLDFragmentArray<MaterialDefFragment> matDefs = wld->table()->byKind<MaterialDefFragment>();
    for(uint32_t i = 0; i < matDefs.count(); i++)
    {
        MaterialDefFragment *matDef = matDefs[i];
        QString charName, palName, partName;
        if(WLDMaterialPalette::explodeName(matDef, charName, palName, partName))
        {
            WLDCharActor *actor = m_models.value(charName);
            if(!actor)
                continue;
            WLDModel *model = actor->model();
            WLDModelSkin *skin = model->skins().value(palName);
            if(!skin)
            {
                skin = model->newSkin(palName, archive);
                skin->palette()->copyFrom(model->skin()->palette());
            }
            skin->palette()->addMaterialDef(matDef);
        }
    }

    // look for alternate meshes (e.g. heads)
    WLDFragmentArray<MeshDefFragment> meshDefs = wld->table()->byKind<MeshDefFragment>();
    for(uint32_t i = 0; i < meshDefs.count(); i++)
    {
        MeshDefFragment *meshDef = meshDefs[i];
        QString actorName, meshName, skinName;
        WLDModelSkin::explodeMeshName(meshDef->name(), actorName, meshName, skinName);
        WLDCharActor *actor = m_models.value(actorName);
        if(!actor || meshName.isEmpty())
            continue;
        WLDModel *model = actor->model();
        WLDModelSkin *skin = model->skins().value(skinName);
        if(!skin)
            continue;
        foreach(WLDMesh *part, model->skin()->parts())
        {
            QString actorName2, meshName2, skinName2;
            WLDModelSkin::explodeMeshName(part->def()->name(), actorName2, meshName2, skinName2);
            if((meshName2 == meshName) && (skinName2 != skinName))
                skin->replacePart(part, meshDef);
        }
    }
}

void CharacterPack::importCharacters(PFSArchive *archive, WLDData *wld)
{
    WLDFragmentArray<ActorDefFragment> actorDefs = wld->table()->byKind<ActorDefFragment>();
    for(uint32_t i = 0; i < actorDefs.count(); i++)
    {
        ActorDefFragment *actorDef = actorDefs[i];
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        WLDModel *model = new WLDModel(archive);
        WLDCharActor *actor = new WLDCharActor(model);
        WLDModelSkin *skin = model->skin();
        foreach(MeshDefFragment *meshDef, WLDModel::listMeshes(actorDef))
        {
            skin->addPart(meshDef);
            skin->palette()->addPaletteDef(meshDef->m_palette);
        }
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

        m_models.insert(actorName, actor);
    }
}

void CharacterPack::upload(RenderContext *renderCtx)
{
    foreach(WLDCharActor *actor, m_models)
        upload(renderCtx, actor);
}

void CharacterPack::upload(RenderContext *renderCtx, WLDCharActor *actor)
{
    // Make sure we haven't uploaded this character before.
    WLDModel *model = actor->model();
    if(model->buffer())
        return;

    // Import mesh geometry.
    MeshBuffer *meshBuf = new MeshBuffer();
    model->setBuffer(meshBuf);
    foreach(WLDModelSkin *skin, model->skins())
    {
        foreach(WLDMesh *mesh, skin->parts())
            mesh->importFrom(meshBuf);

        // Upload materials (textures).
        MaterialMap *materials = skin->materials();
        if(!materials)
        {
            materials = skin->palette()->loadMaterials();
            skin->setMaterials(materials);
        }
        materials->upload(renderCtx);
    }

    // Create the GPU buffers.
    meshBuf->upload(renderCtx);

    // Free the memory used for indices. We need to keep the vertices around for software skinning.
    meshBuf->clearIndices();
}
