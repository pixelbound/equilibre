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

class PFSArchive;
class Zone;
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

    Zone * zone() const;

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
    
    void freezeFrustum(RenderContext *renderCtx);
    void unFreezeFrustum();
    
    Zone * loadZone(QString path, QString name);
    void clear(RenderContext *renderCtx);
    
private:
    //QList<ObjectPack *> m_objectPacks; // XXX move from Zone.
    Zone *m_zone;
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
