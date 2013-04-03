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

#ifndef EQUILIBRE_CHARACTER_VIEWER_WINDOW_H
#define EQUILIBRE_CHARACTER_VIEWER_WINDOW_H

#include <QMainWindow>
#include "EQuilibre/Render/Scene.h"

class QComboBox;
class QVBoxLayout;
class QAction;
class RenderContext;
class RenderProgram;
class SceneViewport;
class WLDSkeleton;
class WLDCharActor;
class WLDModel;
class CharacterScene;
class CharacterPack;
class Game;
class Zone;

class CharacterViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    CharacterViewerWindow(RenderContext *renderCtx, QWidget *parent = 0);

    CharacterScene *scene() const;

    bool loadCharacters(QString archivePath);

private slots:
    void initialized();
    void loadActor(QString name);
    void loadPalette(QString name);
    void loadAnimation(QString animName);
    void openArchive();
    void copyAnimations();
    void clear();
    void setSoftwareSkinning();
    void setHardwareSkinningUniform();
    void setHardwareSkinningTexture();

private:
    void initMenus();
    void copyAnimations(WLDSkeleton *toSkel, QString fromChar);
    void updateLists();
    void updateMenus();

    SceneViewport *m_viewport;
    CharacterScene *m_scene;
    RenderContext *m_renderCtx;
    QComboBox *m_actorText;
    QComboBox *m_paletteText;
    QComboBox *m_animationText;
    QString m_lastDir;
    QAction *m_softwareSkinningAction;
    QAction *m_hardwareSkinningUniformAction;
    QAction *m_hardwareSkinningTextureAction;
    QAction *m_showFpsAction;
};

class CharacterScene : public Scene
{
    Q_OBJECT

public:
    CharacterScene(RenderContext *renderCtx);
    virtual ~CharacterScene();
    
    enum SkinningMode
    {
        SoftwareSkinning = 0,
        HardwareSkinningUniform = 1,
        HardwareSkinningTexture = 2
    };
    
    enum RenderMode
    {
        Basic = 0,
        Skinning = 1
    };

    void init();

    Game * game() const;
    SkinningMode skinningMode() const;
    void setSkinningMode(SkinningMode newMode);
    
    QString selectedModelName() const;
    
    WLDCharActor * actor() const;
    
    CharacterPack * loadCharacters(QString archivePath);

    virtual void draw();

    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

public slots:
    void setSelectedModelName(QString name);

private:
    RenderProgram * program(RenderMode renderMode);
    void drawFrame();
    
    WLDCharActor *m_actor;
    QString m_meshName;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    Game *m_game;
    SkinningMode m_skinningMode;
    // viewer settings
    MouseState m_transState;
    MouseState m_rotState;
};

#endif
