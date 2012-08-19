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
#include "ZoneViewerWindow.h"
#include "OpenEQ/Render/SceneViewport.h"
#include "OpenEQ/Render/RenderState.h"
#include "OpenEQ/Render/Scene.h"
#include "OpenEQ/Game/WLDModel.h"
#include "OpenEQ/Game/WLDActor.h"
#include "OpenEQ/Game/WLDSkeleton.h"
#include "OpenEQ/Game/Zone.h"

ZoneViewerWindow::ZoneViewerWindow(RenderState *state, QWidget *parent) : QMainWindow(parent)
{
    m_scene = new ZoneScene(state);
    m_state = state;
    setWindowTitle("OpenEQ Zone Viewer");
    m_viewport = new SceneViewport(m_scene, state);
    setCentralWidget(m_viewport);
    initMenus();
}

ZoneScene * ZoneViewerWindow::scene() const
{
    return m_scene;
}

void ZoneViewerWindow::initMenus()
{
    QMenu *fileMenu = new QMenu(this);
    fileMenu->setTitle("&File");

    QAction *openAction = new QAction("&Open S3D archive...", this);
    openAction->setShortcut(QKeySequence::Open);
    QAction *selectDirAction = new QAction("Select Asset Directory...", this);

    QAction *quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);

    fileMenu->addAction(openAction);
    fileMenu->addAction(selectDirAction);
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

    m_showFpsAction = new QAction("Show Stats", this);
    m_showFpsAction->setCheckable(true);
    m_showZoneAction = new QAction("Show Zone", this);
    m_showZoneAction->setCheckable(true);
    m_showZoneAction->setChecked(m_scene->zone()->showZone());
    m_showZoneObjectsAction = new QAction("Show Zone Objects", this);
    m_showZoneObjectsAction->setCheckable(true);
    m_showZoneObjectsAction->setChecked(m_scene->zone()->showObjects());
    m_cullZoneObjectsAction = new QAction("Frustum Culling of Zone Objects", this);
    m_cullZoneObjectsAction->setCheckable(true);
    m_cullZoneObjectsAction->setChecked(m_scene->zone()->cullObjects());
    m_showSoundTriggersAction = new QAction("Show Sound Triggers", this);
    m_showSoundTriggersAction->setCheckable(true);

    renderMenu->addAction(m_softwareSkinningAction);
    renderMenu->addAction(m_hardwareSkinningUniformAction);
    renderMenu->addAction(m_hardwareSkinningTextureAction);
    renderMenu->addAction(m_showFpsAction);
    renderMenu->addAction(m_showZoneAction);
    renderMenu->addAction(m_showZoneObjectsAction);
    renderMenu->addAction(m_cullZoneObjectsAction);
    renderMenu->addAction(m_showSoundTriggersAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(selectDirAction, SIGNAL(triggered()), this, SLOT(selectAssetDir()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_softwareSkinningAction, SIGNAL(triggered()), this, SLOT(setSoftwareSkinning()));
    connect(m_hardwareSkinningUniformAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinningUniform()));
    connect(m_hardwareSkinningTextureAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinningTexture()));
    connect(m_showFpsAction, SIGNAL(toggled(bool)), m_viewport, SLOT(setShowStats(bool)));
    connect(m_showZoneAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZone(bool)));
    connect(m_showZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZoneObjects(bool)));
    connect(m_cullZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(setFrustumCulling(bool)));
    connect(m_showSoundTriggersAction, SIGNAL(toggled(bool)), m_scene, SLOT(showSoundTriggers(bool)));
}

void ZoneViewerWindow::openArchive()
{
    QString filePath;
    // OpenGL rendering interferes wtih QFileDialog
    m_viewport->setAnimation(false);
    filePath = QFileDialog::getOpenFileName(0, "Select a S3D file to open",
        m_scene->assetPath(), "S3D Archive (*.s3d)");
    if(!filePath.isEmpty())
    {
        QFileInfo info(filePath);
        loadZone(info.absolutePath(), info.baseName());
        m_viewport->setFocus();
    }
    m_viewport->setAnimation(true);
}

void ZoneViewerWindow::selectAssetDir()
{
    QFileDialog fd;
    fd.setFileMode(QFileDialog::Directory);
    fd.setDirectory(m_scene->assetPath());
    fd.setWindowTitle("Select an asset directory");
    // OpenGL rendering interferes wtih QFileDialog
    m_viewport->setAnimation(false);
    if((fd.exec() == QDialog::Accepted) && (fd.selectedFiles().count() > 0))
        m_scene->setAssetPath(fd.selectedFiles().value(0));
    m_viewport->setAnimation(true);
}

bool ZoneViewerWindow::loadZone(QString path, QString name)
{
    m_viewport->makeCurrent();
    m_scene->zone()->clear();
    return m_scene->zone()->load(path, name);
}

bool ZoneViewerWindow::loadCharacters(QString archivePath)
{
    m_viewport->makeCurrent();
    return m_scene->zone()->loadCharacters(archivePath);
}

void ZoneViewerWindow::updateMenus()
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

void ZoneViewerWindow::setSoftwareSkinning()
{
    m_state->setSkinningMode(RenderState::SoftwareSkinning);
    updateMenus();
}

void ZoneViewerWindow::setHardwareSkinningUniform()
{
    m_state->setSkinningMode(RenderState::HardwareSkinningUniform);
    updateMenus();
}

void ZoneViewerWindow::setHardwareSkinningTexture()
{
    m_state->setSkinningMode(RenderState::HardwareSkinningTexture);
    updateMenus();
}

////////////////////////////////////////////////////////////////////////////////

ZoneScene::ZoneScene(RenderState *state) : Scene(state)
{
    m_zone = new Zone(this);
    m_rotState.last = vec3();
    m_rotState.active = false;
}

Zone * ZoneScene::zone() const
{
    return m_zone;
}

void ZoneScene::showZone(bool show)
{
    m_zone->setShowZone(show);
}

void ZoneScene::showZoneObjects(bool show)
{
    m_zone->setShowObjects(show);
}

void ZoneScene::setFrustumCulling(bool enabled)
{
    m_zone->setCullObjects(enabled);
}

void ZoneScene::showSoundTriggers(bool show)
{
    m_zone->setShowSoundTriggers(show);
}

void ZoneScene::init()
{
    m_started = currentTime();
}

vec3 ZoneScene::cameraPos() const
{
    return m_zone->playerPos() + m_zone->cameraPos();
}

void ZoneScene::draw()
{
    m_zone->draw(m_state);
}

void ZoneScene::keyReleaseEvent(QKeyEvent *e)
{
    float dist = 10.0;
    int key = e->key();
    if(key == Qt::Key_Q)
        m_zone->step(0.0, dist, 0.0);
    else if(key == Qt::Key_D)
        m_zone->step(0.0, -dist, 0.0);
    else if(key == Qt::Key_Z)
        m_zone->step(dist, 0.0, 0.0);
    else if(key == Qt::Key_S)
        m_zone->step(-dist, 0.0, 0.0);
}

void ZoneScene::mouseMoveEvent(QMouseEvent *e)
{
    if(m_rotState.active)
    {
        int dx = m_rotState.x0 - e->x();
        int dy = m_rotState.y0 - e->y();
        vec3 camOrient = m_zone->cameraOrient();
        camOrient.x = (m_rotState.last.x - (dy * 1.0));
        m_zone->setCameraOrient(camOrient);
        m_zone->setPlayerOrient(m_rotState.last.z - (dx * 1.0));
    }
}

void ZoneScene::mousePressEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
    {
        m_rotState.active = true;
        m_rotState.x0 = e->x();
        m_rotState.y0 = e->y();
        m_rotState.last = vec3(0.0, 0.0, m_zone->playerOrient()) + m_zone->cameraOrient();
    }
}

void ZoneScene::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
        m_rotState.active = false;
}
