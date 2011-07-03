#ifndef OPENEQ_ZONE_VIEWER_WINDOW_H
#define OPENEQ_ZONE_VIEWER_WINDOW_H

#include <QMainWindow>

class QComboBox;
class QVBoxLayout;
class QAction;
class Scene;
class RenderState;
class SceneViewport;
class WLDSkeleton;

class ZoneViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    ZoneViewerWindow(Scene *scene, RenderState *state, QWidget *parent = 0);

    bool loadZone(QString path, QString name);
    bool loadCharacters(QString archivePath);

private slots:
    void openArchive();
    void setSoftwareSkinning();
    void setHardwareSkinningUniform();
    void setHardwareSkinningTexture();

private:
    void initMenus();
    void updateMenus();

    SceneViewport *m_viewport;
    Scene *m_scene;
    RenderState *m_state;
    QString m_lastDir;
    QAction *m_softwareSkinningAction;
    QAction *m_hardwareSkinningUniformAction;
    QAction *m_hardwareSkinningTextureAction;
    QAction *m_showFpsAction;
};

#endif
