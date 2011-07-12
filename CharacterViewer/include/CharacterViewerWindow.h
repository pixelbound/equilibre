#ifndef OPENEQ_CHARACTER_VIEWER_WINDOW_H
#define OPENEQ_CHARACTER_VIEWER_WINDOW_H

#include <QMainWindow>
#include "Scene.h"

class QComboBox;
class QVBoxLayout;
class QAction;
class RenderState;
class SceneViewport;
class WLDSkeleton;
class CharacterScene;

class CharacterViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    CharacterViewerWindow(RenderState *state, QWidget *parent = 0);

    CharacterScene *scene() const;

    bool loadZone(QString path, QString name);
    bool loadCharacters(QString archivePath);

private slots:
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
    RenderState *m_state;
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
    CharacterScene(RenderState *state);

    void init();

    Zone * zone() const;
    const QMap<QString, WLDActor *> & charModels() const;
    WLDActor * selectedCharacter() const;
    QString selectedModelName() const;

    virtual void draw();

    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

public slots:
    void setSelectedModelName(QString name);

private:
    QString m_meshName;
    double m_started;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    Zone *m_zone;
    // viewer settings
    MouseState m_transState;
    MouseState m_rotState;
};

#endif
