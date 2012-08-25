#include <cmath>
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
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "CharacterViewerWindow.h"
#include "OpenEQ/Render/SceneViewport.h"
#include "OpenEQ/Render/RenderState.h"
#include "OpenEQ/Render/Scene.h"
#include "OpenEQ/Game/WLDModel.h"
#include "OpenEQ/Game/WLDActor.h"
#include "OpenEQ/Game/WLDSkeleton.h"
#include "OpenEQ/Game/Zone.h"

CharacterViewerWindow::CharacterViewerWindow(RenderState *state, QWidget *parent) : QMainWindow(parent)
{
    m_scene = new CharacterScene(state);
    m_state = state;
    m_lastDir = m_scene->assetPath();
    setWindowTitle("OpenEQ Character Viewer");
    m_viewport = new SceneViewport(m_scene, state);
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
    connect(m_viewport, SIGNAL(initialized()), this, SLOT(initialized()));
}

CharacterScene * CharacterViewerWindow::scene() const
{
    return m_scene;
}

void CharacterViewerWindow::initialized()
{
    updateLists();
    m_viewport->setAnimation(true);
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
    m_hardwareSkinningUniformAction = new QAction("Hardware Skinning (Uniform)", this);
    m_hardwareSkinningTextureAction = new QAction("Hardware Skinning (Texture)", this);
    m_softwareSkinningAction->setCheckable(true);
    m_hardwareSkinningUniformAction->setCheckable(true);
    m_hardwareSkinningTextureAction->setCheckable(true);
    QActionGroup *skinningActions = new QActionGroup(this);
    skinningActions->addAction(m_softwareSkinningAction);
    skinningActions->addAction(m_hardwareSkinningUniformAction);
    skinningActions->addAction(m_hardwareSkinningTextureAction);

    m_showFpsAction = new QAction("Show stats", this);
    m_showFpsAction->setCheckable(true);

    renderMenu->addAction(m_softwareSkinningAction);
    renderMenu->addAction(m_hardwareSkinningUniformAction);
    renderMenu->addAction(m_hardwareSkinningTextureAction);
    renderMenu->addAction(m_showFpsAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(copyAnimationsAction, SIGNAL(triggered()), this, SLOT(copyAnimations()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clear()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_softwareSkinningAction, SIGNAL(triggered()), this, SLOT(setSoftwareSkinning()));
    connect(m_hardwareSkinningUniformAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinningUniform()));
    connect(m_hardwareSkinningTextureAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinningTexture()));
    connect(m_showFpsAction, SIGNAL(toggled(bool)), m_viewport, SLOT(setShowStats(bool)));
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
    if(m_scene->loadCharacters(archivePath))
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
    WLDSkeleton *charSkel = charModel->complexModel()->skeleton();
    if(!charSkel)
        return;
    QDialog d;
    d.setWindowTitle("Select a character to copy animations from");
    QComboBox *charList = new QComboBox();
    foreach(CharacterPack *pack, m_scene->zone()->characterPacks())
    {
        const QMap<QString, WLDActor *> &actors = pack->models();
        foreach(QString charName, actors.keys())
        {
            WLDSkeleton *skel = actors.value(charName)->complexModel()->skeleton();
            if(skel)
                charList->addItem(charName);
        }
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
    WLDActor *actor = m_scene->zone()->findCharacter(fromChar);
    if(actor)
    {
        WLDSkeleton *fromSkel = actor->complexModel()->skeleton();
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
    foreach(CharacterPack *pack, m_scene->zone()->characterPacks())
    {
        const QMap<QString, WLDActor *> &actors = pack->models();
        foreach(QString name, actors.keys())
            m_actorText->addItem(name);
    }

    if(m_scene->selectedModelName().isEmpty() && m_actorText->count() > 0)
        m_scene->setSelectedModelName(m_actorText->itemText(0));

    WLDActor *charModel = m_scene->selectedCharacter();
    if(charModel)
    {
        WLDSkeleton *skel = charModel->complexModel()->skeleton();
        if(skel)
        {
            foreach(QString animName, skel->animations().keys())
                m_animationText->addItem(animName);
        }
        foreach(WLDModelSkin *skin, charModel->complexModel()->skins())
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
    case RenderState::SoftwareSkinning:
        m_softwareSkinningAction->setChecked(true);
        break;
    case RenderState::HardwareSkinningUniform:
        m_hardwareSkinningUniformAction->setChecked(true);
        break;
    case RenderState::HardwareSkinningTexture:
        m_hardwareSkinningTextureAction->setChecked(true);
        break;
    }
    m_showFpsAction->setChecked(m_viewport->showStats());
}

void CharacterViewerWindow::clear()
{
    m_scene->zone()->clear();
    updateLists();
}

void CharacterViewerWindow::setSoftwareSkinning()
{
    m_state->setSkinningMode(RenderState::SoftwareSkinning);
    updateMenus();
}

void CharacterViewerWindow::setHardwareSkinningUniform()
{
    m_state->setSkinningMode(RenderState::HardwareSkinningUniform);
    updateMenus();
}

void CharacterViewerWindow::setHardwareSkinningTexture()
{
    m_state->setSkinningMode(RenderState::HardwareSkinningTexture);
    updateMenus();
}

////////////////////////////////////////////////////////////////////////////////

CharacterScene::CharacterScene(RenderState *state) : Scene(state)
{
    m_state = state;
    m_sigma = 1.0;
    m_zone = new Zone(this);
    m_transState.last = vec3();
    m_rotState.last = vec3();
    m_transState.active = false;
    m_rotState.active = false;
    m_delta = vec3(-0.0, -0.0, -5.0);
    m_theta = vec3(-90.0, 00.0, 270.0);
    m_sigma = 0.5;
}

Zone * CharacterScene::zone() const
{
    return m_zone;
}

QString CharacterScene::selectedModelName() const
{
    return m_meshName;
}

void CharacterScene::setSelectedModelName(QString name)
{
    m_meshName = name;
}

WLDActor * CharacterScene::selectedCharacter() const
{
    return m_zone->findCharacter(m_meshName);
}

void CharacterScene::init()
{
    m_started = currentTime();
    foreach(CharacterPack *charPack, m_zone->characterPacks())
        charPack->upload(m_state);
    foreach(ObjectPack *objPack, m_zone->objectPacks())
        objPack->upload(m_state);
}

CharacterPack * CharacterScene::loadCharacters(QString archivePath)
{
    CharacterPack *charPack = m_zone->loadCharacters(archivePath);
    if(charPack)
    {
        charPack->upload(m_state);
        return charPack;
    }
    return NULL;
}

void CharacterScene::draw()
{
    vec3 rot = m_theta;
    m_state->translate(m_delta.x, m_delta.y, m_delta.z);
    m_state->rotate(rot.x, 1.0, 0.0, 0.0);
    m_state->rotate(rot.y, 0.0, 1.0, 0.0);
    m_state->rotate(rot.z, 0.0, 0.0, 1.0);
    m_state->scale(m_sigma, m_sigma, m_sigma);
    
    WLDActor *charModel = selectedCharacter();
    if(charModel)
    {
        charModel->setAnimTime(currentTime());
        charModel->draw(m_state);
    }
}

void CharacterScene::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if(key == Qt::Key_Q)
        m_theta.y += 5.0;
    else if(key == Qt::Key_D)
        m_theta.y -= 5.0;
    else if(key == Qt::Key_2)
        m_theta.x += 5.0;
    else if(key == Qt::Key_8)
        m_theta.x -= 5.0;
    else if(key == Qt::Key_4)
        m_theta.z += 5.0;
    else if(key == Qt::Key_6)
        m_theta.z -= 5.0;
}

void CharacterScene::mouseMoveEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();

    if(m_transState.active)
    {
        int dx = m_transState.x0 - x;
        int dy = m_transState.y0 - y;
        m_delta.x = (m_transState.last.x - (dx / 100.0));
        m_delta.z = (m_transState.last.y + (dy / 100.0));
    }

    if(m_rotState.active)
    {
        int dx = m_rotState.x0 - x;
        int dy = m_rotState.y0 - y;
        m_theta.x = (m_rotState.last.x + (dy * 2.0));
        m_theta.z = (m_rotState.last.z + (dx * 2.0));
    }
}

void CharacterScene::mousePressEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();
    if(e->button() & Qt::MidButton)       // middle button pans the scene
    {
        m_transState.active = true;
        m_transState.x0 = x;
        m_transState.y0 = y;
        m_transState.last = m_delta;
    }
    else if(e->button() & Qt::LeftButton)   // left button rotates the scene
    {
        m_rotState.active = true;
        m_rotState.x0 = x;
        m_rotState.y0 = y;
        m_rotState.last = m_theta;
    }
}

void CharacterScene::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::MidButton)
        m_transState.active = false;
    else if(e->button() & Qt::LeftButton)
        m_rotState.active = false;
}

void CharacterScene::wheelEvent(QWheelEvent *e)
{
    // mouse wheel up zooms towards the scene
    // mouse wheel down zooms away from scene
    m_sigma *= pow(1.01, e->delta() / 8);
}
