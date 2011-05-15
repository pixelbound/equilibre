#include <QComboBox>
#include <QVBoxLayout>
#include "CharacterViewerWindow.h"
#include "SceneViewport.h"
#include "Scene.h"

CharacterViewerWindow::CharacterViewerWindow(Scene *scene, RenderState *state,
                                             QWidget *parent) : QWidget(parent)
{
    m_scene = scene;
    m_state = state;
    setWindowTitle("OpenEQ Character Viewer");
    m_viewport = new SceneViewport(scene, state);
    m_actorText = new QComboBox();

    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(m_actorText);
    l->addWidget(m_viewport);

    connect(m_actorText, SIGNAL(activated(QString)), this, SLOT(loadActor(QString)));
}

bool CharacterViewerWindow::loadZone(QString path, QString name)
{
    m_actorText->clear();
    m_viewport->makeCurrent();
    if(m_scene->openZone(path, name))
    {
        foreach(QString name, m_scene->models().keys())
            m_actorText->addItem(name);
        loadActor(m_actorText->itemText(0));
        return true;
    }
    return false;
}

void CharacterViewerWindow::loadActor(QString name)
{
    m_actorText->setCurrentIndex(m_actorText->findText(name));
    m_scene->setSelectedModelName(name);
}
