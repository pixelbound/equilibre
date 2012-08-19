#ifndef OPENEQ_ZONE_VIEWER_WINDOW_H
#define OPENEQ_ZONE_VIEWER_WINDOW_H

#include <QMainWindow>
#include "OpenEQ/Render/Scene.h"
#include "OpenEQ/Render/Vertex.h"

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

private:
    void initMenus();
    void updateMenus();

    SceneViewport *m_viewport;
    ZoneScene *m_scene;
    RenderState *m_state;
    QAction *m_softwareSkinningAction;
    QAction *m_hardwareSkinningUniformAction;
    QAction *m_hardwareSkinningTextureAction;
    QAction *m_showFpsAction;
    QAction *m_showZoneAction;
    QAction *m_showZoneObjectsAction;
    QAction *m_cullZoneObjectsAction;
};

class ZoneScene : public Scene
{
    Q_OBJECT

public:
    ZoneScene(RenderState *state);

    Zone * zone() const;

    virtual void init();
    virtual void draw();
    
    virtual vec3 cameraPos() const;

    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

public slots:
    void showZone(bool show);
    void showZoneObjects(bool show);
    void setFrustumCulling(bool enabled);

private:
    double m_started;
    Zone *m_zone;
    MouseState m_rotState;
};

#endif
