#ifndef OPENEQ_ZONE_VIEWER_WINDOW_H
#define OPENEQ_ZONE_VIEWER_WINDOW_H

#include <QMainWindow>
#include "Scene.h"
#include "Vertex.h"

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
    QAction *m_showZoneObjectsAction;
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
    void showZoneObjects(bool show);

private:
    void step(double distForward, double distSideways);

    double m_started;
    // xyz position of the player in the world
    vec3 m_playerPos;
    // z angle that describes where the player is facing
    float m_playerOrient;
    // xyz angles that describe how the camera is oriented rel. to the player
    vec3 m_cameraOrient;
    // xyz position of the camera is rel. to the player
    vec3 m_cameraPos;
    float m_zoneScale;
    Zone *m_zone;
    bool m_showZoneObjects;
    // viewer settings
    MouseState m_rotState;
};

#endif
