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

#ifndef EQUILIBRE_GAME_GAME_H
#define EQUILIBRE_GAME_GAME_H

#include <vector>
#include <QObject>
#include <QList>
#include <QMap>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Game/Zone.h"

class CharacterPack;
class ObjectPack;
class PFSArchive;
class Zone;
class ZoneSky;
class WLDCharActor;
class WLDMesh;
class WLDData;

/*!
  \brief Contains the global state of the game.
  */
class GAME_DLL Game
{
public:
    Game();
    virtual ~Game();
    
    void clearZone(RenderContext *renderCtx);
    void clear(RenderContext *renderCtx);

    bool showZone() const;
    bool showObjects() const;
    bool showFog() const;
    bool cullObjects() const;
    bool showSoundTriggers() const;
    bool frustumIsFrozen() const;
    
    void setShowZone(bool show);
    void setShowObjects(bool show);
    void setShowFog(bool show);
    void setCullObjects(bool enabled);
    void setShowSoundTriggers(bool show);
    
    float fogDensity() const;
    
    void freezeFrustum(RenderContext *renderCtx);
    void unFreezeFrustum();
    
    Zone * zone() const;
    ZoneSky * sky() const;
    QList<ObjectPack *> objectPacks() const;
    QList<CharacterPack *> characterPacks() const;
    
    Zone * loadZone(QString path, QString name);
    bool loadZoneInfo(QString file);
    bool loadSky(QString path);
    ObjectPack * loadObjects(QString archivePath, QString wldName = QString::null);
    CharacterPack * loadCharacters(QString archivePath, QString wldName = QString::null, bool own = true);
    WLDCharActor * findCharacter(QString name) const;
    
private:
    QList<ObjectPack *> m_objectPacks;
    QList<CharacterPack *> m_charPacks;
    QMap<QString, ZoneInfo> m_zoneInfo;
    Zone *m_zone;
    ZoneSky *m_sky;
    bool m_showZone;
    bool m_showObjects;
    bool m_showFog;
    bool m_cullObjects;
    bool m_showSoundTriggers;
    bool m_frustumIsFrozen;
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
    
    bool load(QString archivePath, QString wldName);
    MeshBuffer * upload(RenderContext *renderCtx);
    void clear(RenderContext *renderCtx);
    
private:
    PFSArchive *m_archive;
    WLDData *m_wld;
    QMap<QString, WLDMesh *> m_models;
    MeshBuffer *m_meshBuf;
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
