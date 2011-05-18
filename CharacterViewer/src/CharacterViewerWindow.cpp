#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
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
    menuBar()->addMenu(fileMenu);

    QWidget *centralWidget = new QWidget();

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_actorText);
    hbox->addWidget(m_paletteText);
    hbox->addWidget(m_animationText);

    QVBoxLayout *l = new QVBoxLayout(centralWidget);
    l->addLayout(hbox);
    l->addWidget(m_viewport);

    setCentralWidget(centralWidget);

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(copyAnimationsAction, SIGNAL(triggered()), this, SLOT(copyAnimations()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clear()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_actorText, SIGNAL(activated(QString)), this, SLOT(loadActor(QString)));
    connect(m_paletteText, SIGNAL(activated(QString)), this, SLOT(loadPalette(QString)));
    connect(m_animationText, SIGNAL(activated(QString)), this, SLOT(loadAnimation(QString)));
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
        m_actorText->setEnabled(m_actorText->count() > 1);
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
        foreach(WLDModelSkin *skin, charModel->model()->skins())
            m_paletteText->addItem(skin->name());
        loadPalette(charModel->paletteName());
    }
    m_animationText->setEnabled(m_animationText->count() > 1);
    m_paletteText->setEnabled(m_paletteText->count() > 1);
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
        if(!fromSkel)
            return;
        foreach(QString animName, fromSkel->animations().keys())
        {
            if(!toSkel->animations().contains(animName))
                toSkel->copyFrom(fromSkel, animName);
        }
        m_animationText->clear();
        foreach(QString animName, toSkel->animations().keys())
            m_animationText->addItem(animName);
        m_animationText->setEnabled(m_animationText->count() > 1);
        loadAnimation(actor->animName());
    }
}

void CharacterViewerWindow::clear()
{
    m_scene->zone()->clear();
    m_actorText->clear();
    m_paletteText->clear();
    m_animationText->clear();
    m_actorText->setEnabled(false);
    m_paletteText->setEnabled(false);
    m_animationText->setEnabled(false);
}
