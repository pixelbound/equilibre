#ifndef OPENEQ_CHARACTER_VIEWER_WINDOW_H
#define OPENEQ_CHARACTER_VIEWER_WINDOW_H

#include <QMainWindow>

class QComboBox;
class QVBoxLayout;
class Scene;
class RenderState;
class SceneViewport;
class WLDSkeleton;

class CharacterViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    CharacterViewerWindow(Scene *scene, RenderState *state, QWidget *parent = 0);

    bool loadZone(QString path, QString name);
    bool loadCharacters(QString archivePath);

private slots:
    void loadActor(QString name);
    void loadPalette(QString name);
    void loadAnimation(QString animName);
    void openArchive();
    void copyAnimations();
    void clear();

private:
    void copyAnimations(WLDSkeleton *toSkel, QString fromChar);
    void updateLists();

    SceneViewport *m_viewport;
    Scene *m_scene;
    RenderState *m_state;
    QComboBox *m_actorText;
    QComboBox *m_paletteText;
    QComboBox *m_animationText;
    QString m_lastDir;
};

#endif
