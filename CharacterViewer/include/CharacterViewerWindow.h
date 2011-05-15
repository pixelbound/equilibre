#ifndef OPENEQ_CHARACTER_VIEWER_WINDOW_H
#define OPENEQ_CHARACTER_VIEWER_WINDOW_H

#include <QWidget>
#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include "Vertex.h"

class QComboBox;
class QVBoxLayout;
class Scene;
class RenderState;
class SceneViewport;

class CharacterViewerWindow : public QWidget
{
    Q_OBJECT

public:
    CharacterViewerWindow(Scene *scene, RenderState *state, QWidget *parent = 0);

    bool loadZone(QString path, QString name);
    bool loadCharacters(QString archivePath, QString wldName);

private slots:
    void loadActor(QString name);
    void loadAnimation(QString animName);

private:
    SceneViewport *m_viewport;
    Scene *m_scene;
    RenderState *m_state;
    QComboBox *m_actorText;
    QComboBox *m_animationText;
};

#endif
