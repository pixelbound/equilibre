#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "CharacterViewerWindow.h"
#include "SceneViewport.h"
#include "Scene.h"
#include "WLDModel.h"
#include "WLDSkeleton.h"

CharacterViewerWindow::CharacterViewerWindow(Scene *scene, RenderState *state,
                                             QWidget *parent) : QWidget(parent)
{
    m_scene = scene;
    m_state = state;
    setWindowTitle("OpenEQ Character Viewer");
    m_viewport = new SceneViewport(scene, state);
    m_actorText = new QComboBox();
    m_animationText = new QComboBox();

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_actorText);
    hbox->addWidget(m_animationText);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->addLayout(hbox);
    l->addWidget(m_viewport);

    connect(m_actorText, SIGNAL(activated(QString)), this, SLOT(loadActor(QString)));
    connect(m_animationText, SIGNAL(activated(QString)), this, SLOT(loadAnimation(QString)));
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

bool CharacterViewerWindow::loadCharacters(QString archivePath, QString wldName)
{
    m_actorText->clear();
    m_viewport->makeCurrent();
    if(m_scene->loadCharacters(archivePath, wldName))
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
    m_animationText->clear();
    WLDModel *model = m_scene->selectedModel();
    if(model && model->skeleton())
    {
        foreach(QString animName, model->skeleton()->animations().keys())
            m_animationText->addItem(animName);
        loadAnimation(model->animName());
    }
}

void CharacterViewerWindow::loadAnimation(QString animName)
{
    m_animationText->setCurrentIndex(m_animationText->findText(animName));
    WLDModel *model = m_scene->selectedModel();
    if(model)
        model->setAnimName(animName);
}
