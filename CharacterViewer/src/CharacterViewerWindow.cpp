#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "CharacterViewerWindow.h"
#include "SceneViewport.h"
#include "Scene.h"
#include "WLDModel.h"
#include "WLDActor.h"
#include "WLDSkeleton.h"
#include "Zone.h"

CharacterViewerWindow::CharacterViewerWindow(Scene *scene, RenderState *state,
                                             QWidget *parent) : QWidget(parent)
{
    m_scene = scene;
    m_state = state;
    setWindowTitle("OpenEQ Character Viewer");
    m_viewport = new SceneViewport(scene, state);
    m_actorText = new QComboBox();
    m_paletteText = new QComboBox();
    m_animationText = new QComboBox();

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_actorText);
    hbox->addWidget(m_paletteText);
    hbox->addWidget(m_animationText);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->addLayout(hbox);
    l->addWidget(m_viewport);

    connect(m_actorText, SIGNAL(activated(QString)), this, SLOT(loadActor(QString)));
    connect(m_paletteText, SIGNAL(activated(QString)), this, SLOT(loadPalette(QString)));
    connect(m_animationText, SIGNAL(activated(QString)), this, SLOT(loadAnimation(QString)));
}

bool CharacterViewerWindow::loadZone(QString path, QString name)
{
    m_actorText->clear();
    m_viewport->makeCurrent();
    if(m_scene->zone()->load(path, name))
    {
        foreach(QString name, m_scene->charModels().keys())
            m_actorText->addItem(name);
        loadActor(m_actorText->itemText(0));
        return true;
    }
    return false;
}

bool CharacterViewerWindow::loadCharacters(QString archivePath)
{
    m_actorText->clear();
    m_viewport->makeCurrent();
    if(m_scene->zone()->loadCharacters(archivePath))
    {
        foreach(QString name, m_scene->charModels().keys())
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
    m_paletteText->clear();
    m_animationText->clear();
    WLDActor *charModel = m_scene->selectedCharacter();
    if(charModel)
    {
        WLDSkeleton *skel = charModel->model()->skeleton();
        if(skel)
        {
            foreach(QString animName, skel->animations().keys())
                m_animationText->addItem(animName);
            loadAnimation(charModel->animName());
        }
        foreach(WLDMaterialPalette *pal, charModel->model()->palettes())
            m_paletteText->addItem(pal->name());
        loadPalette(charModel->paletteName());
    }
}

void CharacterViewerWindow::loadPalette(QString name)
{
    m_paletteText->setCurrentIndex(m_paletteText->findText(name));
    WLDActor *charModel = m_scene->selectedCharacter();
    if(charModel)
        charModel->setPaletteName(name);
}

void CharacterViewerWindow::loadAnimation(QString animName)
{
    m_animationText->setCurrentIndex(m_animationText->findText(animName));
    WLDActor *charModel = m_scene->selectedCharacter();
    if(charModel)
        charModel->setAnimName(animName);
}
