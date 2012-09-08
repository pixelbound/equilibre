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

#ifndef EQUILIBRE_ZONE_VIEWER_WINDOW_H
#define EQUILIBRE_ZONE_VIEWER_WINDOW_H

#include <QMainWindow>
#include "EQuilibre/Render/Scene.h"
#include "EQuilibre/Render/Vertex.h"

class QComboBox;
class QVBoxLayout;
class QAction;
class ZoneScene;
class RenderState;
class SceneViewport;
class WLDSkeleton;
class Zone;

class ZoneViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    ZoneViewerWindow(RenderState *state, QWidget *parent = 0);

    ZoneScene * scene() const;

    bool loadZone(QString path, QString name);
    bool loadCharacters(QString archivePath);

private slots:
    void openArchive();
    void selectAssetDir();
    void setSoftwareSkinning();
    void setHardwareSkinningUniform();
    void setHardwareSkinningTexture();
    void setNoLighting();
    void setBakedLighting();
    void setDebugVertexColor();

private:
    void initMenus();
    void updateMenus();

    SceneViewport *m_viewport;
    ZoneScene *m_scene;
    RenderState *m_state;
    QAction *m_softwareSkinningAction;
    QAction *m_hardwareSkinningUniformAction;
    QAction *m_hardwareSkinningTextureAction;
    QAction *m_noLightingAction;
    QAction *m_bakedLightingAction;
    QAction *m_debugVertexColorAction;
    QAction *m_showFpsAction;
    QAction *m_showZoneAction;
    QAction *m_showZoneObjectsAction;
    QAction *m_cullZoneObjectsAction;
    QAction *m_showSoundTriggersAction;
};

class ZoneScene : public Scene
{
    Q_OBJECT

public:
    ZoneScene(RenderState *state);

    Zone * zone() const;

    virtual void init();
    virtual void draw();
    
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

public slots:
    void showZone(bool show);
    void showZoneObjects(bool show);
    void setFrustumCulling(bool enabled);
    void showSoundTriggers(bool show);

private:
    double m_started;
    Zone *m_zone;
    MouseState m_rotState;
};

#endif
