#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QDialog>
#include <QDialogButtonBox>
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
                                             QWidget *parent) : QMainWindow(parent)
{
    m_scene = scene;
    m_state = state;
    m_lastDir = "../Data";
    setWindowTitle("OpenEQ Character Viewer");
    m_viewport = new SceneViewport(scene, state);
    m_actorText = new QComboBox();
    m_paletteText = new QComboBox();
    m_animationText = new QComboBox();

    QWidget *centralWidget = new QWidget();

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_actorText);
    hbox->addWidget(m_paletteText);
    hbox->addWidget(m_animationText);

    QVBoxLayout *l = new QVBoxLayout(centralWidget);
    l->addLayout(hbox);
    l->addWidget(m_viewport);

    setCentralWidget(centralWidget);

    initMenus();
    updateLists();

    connect(m_actorText, SIGNAL(activated(QString)), this, SLOT(loadActor(QString)));
    connect(m_paletteText, SIGNAL(activated(QString)), this, SLOT(loadPalette(QString)));
    connect(m_animationText, SIGNAL(activated(QString)), this, SLOT(loadAnimation(QString)));
}

void CharacterViewerWindow::initMenus()
{
    QMenu *fileMenu = new QMenu();
    fileMenu->setTitle("&File");

    QAction *openAction = new QAction("&Open S3D archive...", this);
    openAction->setShortcut(QKeySequence::Open);
    QAction *copyAnimationsAction = new QAction("Copy Animations From Character...", this);
    QAction *clearAction = new QAction("Clear", this);
    clearAction->setShortcut(QKeySequence::Delete);
    QAction *quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);

    fileMenu->addAction(openAction);
    fileMenu->addAction(copyAnimationsAction);
    fileMenu->addAction(clearAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu *renderMenu = new QMenu();
    renderMenu->setTitle("&Render");

    m_softwareSkinningAction = new QAction("Software Skinning", this);
    m_hardwareSkinningAction = new QAction("Hardware Skinning", this);
    m_hardwareSkinningDQAction = new QAction("Hardware Skinning (Dual Quaternion)", this);
    m_softwareSkinningAction->setCheckable(true);
    m_hardwareSkinningAction->setCheckable(true);
    m_hardwareSkinningDQAction->setCheckable(true);
    QActionGroup *skinningActions = new QActionGroup(this);
    skinningActions->addAction(m_softwareSkinningAction);
    skinningActions->addAction(m_hardwareSkinningAction);
    skinningActions->addAction(m_hardwareSkinningDQAction);

    renderMenu->addAction(m_softwareSkinningAction);
    renderMenu->addAction(m_hardwareSkinningAction);
    renderMenu->addAction(m_hardwareSkinningDQAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(copyAnimationsAction, SIGNAL(triggered()), this, SLOT(copyAnimations()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clear()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_softwareSkinningAction, SIGNAL(triggered()), this, SLOT(setSoftwareSkinning()));
    connect(m_hardwareSkinningAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinning()));
    connect(m_hardwareSkinningDQAction, SIGNAL(triggered()), this, SLOT(setHardwareDQSkinning()));
}

void CharacterViewerWindow::openArchive()
{
    QString filePath;
    // OpenGL rendering interferes wtih QFileDialog
    m_viewport->setAnimation(false);
    filePath = QFileDialog::getOpenFileName(0, "Select a S3D file to open",
        m_lastDir, "S3D Archive (*.s3d)");
    if(!filePath.isEmpty())
    {
        m_lastDir = QFileInfo(filePath).dir().absolutePath();
        loadCharacters(filePath);
    }
    m_viewport->setAnimation(true);
}

bool CharacterViewerWindow::loadZone(QString path, QString name)
{
    m_viewport->makeCurrent();
    if(m_scene->zone()->load(path, name))
    {
        updateLists();
        return true;
    }
    return false;
}

bool CharacterViewerWindow::loadCharacters(QString archivePath)
{
    m_viewport->makeCurrent();
    if(m_scene->zone()->loadCharacters(archivePath))
    {
        updateLists();
        return true;
    }
    return false;
}

void CharacterViewerWindow::loadActor(QString name)
{
    m_scene->setSelectedModelName(name);
    updateLists();
}

void CharacterViewerWindow::loadPalette(QString name)
{
    WLDActor *charModel = m_scene->selectedCharacter();
    if(charModel)
    {
        charModel->setPaletteName(name);
        updateLists();
    }
}

void CharacterViewerWindow::loadAnimation(QString animName)
{
    WLDActor *charModel = m_scene->selectedCharacter();
    if(charModel)
    {
        charModel->setAnimName(animName);
        updateLists();
    }
}

void CharacterViewerWindow::copyAnimations()
{
    WLDActor *charModel = m_scene->selectedCharacter();
    if(!charModel)
        return;
    WLDSkeleton *charSkel = charModel->model()->skeleton();
    if(!charSkel)
        return;
    QDialog d;
    d.setWindowTitle("Select a character to copy animations from");
    QComboBox *charList = new QComboBox();
    const QMap<QString, WLDActor *> &actors = m_scene->zone()->charModels();
    foreach(QString charName, actors.keys())
    {
        WLDSkeleton *skel = actors.value(charName)->model()->skeleton();
        if(skel)
            charList->addItem(charName);
    }
    QDialogButtonBox *buttons = new QDialogButtonBox();
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), &d, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), &d, SLOT(reject()));

    QVBoxLayout *vb = new QVBoxLayout(&d);
    vb->addWidget(charList);
    vb->addWidget(buttons);

    if(d.exec() == QDialog::Accepted)
        copyAnimations(charSkel, charList->currentText());
}

void CharacterViewerWindow::copyAnimations(WLDSkeleton *toSkel, QString fromChar)
{
    WLDActor *actor = m_scene->zone()->charModels().value(fromChar);
    if(actor)
    {
        WLDSkeleton *fromSkel = actor->model()->skeleton();
        if(fromSkel)
        {
            toSkel->copyAnimationsFrom(fromSkel);
            updateLists();
        }
    }
}

void CharacterViewerWindow::updateLists()
{
    m_actorText->clear();
    m_paletteText->clear();
    m_animationText->clear();
    foreach(QString name, m_scene->charModels().keys())
        m_actorText->addItem(name);

    if(m_scene->selectedModelName().isEmpty() && m_actorText->count() > 0)
        m_scene->setSelectedModelName(m_actorText->itemText(0));

    WLDActor *charModel = m_scene->selectedCharacter();
    if(charModel)
    {
        WLDSkeleton *skel = charModel->model()->skeleton();
        if(skel)
        {
            foreach(QString animName, skel->animations().keys())
                m_animationText->addItem(animName);
        }
        foreach(WLDModelSkin *skin, charModel->model()->skins())
            m_paletteText->addItem(skin->name());
        m_actorText->setCurrentIndex(m_actorText->findText(m_scene->selectedModelName()));
        m_paletteText->setCurrentIndex(m_paletteText->findText(charModel->paletteName()));
        m_animationText->setCurrentIndex(m_animationText->findText(charModel->animName()));
    }
    m_actorText->setEnabled(m_actorText->count() > 1);
    m_animationText->setEnabled(m_animationText->count() > 1);
    m_paletteText->setEnabled(m_paletteText->count() > 1);
}

void CharacterViewerWindow::updateMenus()
{
    switch(m_state->skinningMode())
    {
    default:
    case RenderState::SoftwareSingleQuaternion:
        m_softwareSkinningAction->setChecked(true);
        break;
    case RenderState::HardwareSingleQuaternion:
        m_hardwareSkinningAction->setChecked(true);
        break;
    case RenderState::HardwareDualQuaternion:
        m_hardwareSkinningDQAction->setChecked(true);
        break;
    }
}

void CharacterViewerWindow::clear()
{
    m_scene->zone()->clear();
    updateLists();
}

void CharacterViewerWindow::setSoftwareSkinning()
{
    m_state->setSkinningMode(RenderState::SoftwareSingleQuaternion);
    updateMenus();
}

void CharacterViewerWindow::setHardwareSkinning()
{
    m_state->setSkinningMode(RenderState::HardwareSingleQuaternion);
    updateMenus();
}

void CharacterViewerWindow::setHardwareDQSkinning()
{
    m_state->setSkinningMode(RenderState::HardwareDualQuaternion);
    updateMenus();
}
