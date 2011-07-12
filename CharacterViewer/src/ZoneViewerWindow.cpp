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
#include "SceneViewport.h"
#include "RenderState.h"
#include "Scene.h"
#include "WLDModel.h"
#include "WLDActor.h"
#include "WLDSkeleton.h"
#include "Zone.h"

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

    m_showFpsAction = new QAction("Show FPS", this);
    m_showFpsAction->setCheckable(true);
    m_showZoneObjectsAction = new QAction("Show Zone Objects", this);
    m_showZoneObjectsAction->setCheckable(true);

    renderMenu->addAction(m_softwareSkinningAction);
    renderMenu->addAction(m_hardwareSkinningUniformAction);
    renderMenu->addAction(m_hardwareSkinningTextureAction);
    renderMenu->addAction(m_showFpsAction);
    renderMenu->addAction(m_showZoneObjectsAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(selectDirAction, SIGNAL(triggered()), this, SLOT(selectAssetDir()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_softwareSkinningAction, SIGNAL(triggered()), this, SLOT(setSoftwareSkinning()));
    connect(m_hardwareSkinningUniformAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinningUniform()));
    connect(m_hardwareSkinningTextureAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinningTexture()));
    connect(m_showFpsAction, SIGNAL(toggled(bool)), m_viewport, SLOT(setShowFps(bool)));
    connect(m_showZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZoneObjects(bool)));
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
    m_showFpsAction->setChecked(m_viewport->showFps());
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
    m_showZoneObjects = false;
    m_transState.last = vec3();
    m_rotState.last = vec3();
    m_transState.active = false;
    m_rotState.active = false;
    m_playerPos = vec3(0.0, 0.0, 0.0);
    m_playerOrient = 0.0;
    m_zoneScale = 0.5;
}

Zone * ZoneScene::zone() const
{
    return m_zone;
}

void ZoneScene::showZoneObjects(bool show)
{
    m_showZoneObjects = show;
}

void ZoneScene::init()
{
    m_started = currentTime();
}

void ZoneScene::draw()
{
    vec3 rot = vec3(-90.0, 00.0, m_playerOrient);
    m_state->pushMatrix();
    m_state->rotate(rot.x, 1.0, 0.0, 0.0);
    m_state->rotate(rot.y, 0.0, 1.0, 0.0);
    m_state->rotate(rot.z, 0.0, 0.0, 1.0);
    m_state->translate(m_playerPos.x, m_playerPos.y, m_playerPos.z);
    m_state->scale(m_zoneScale, m_zoneScale, m_zoneScale);
    m_zone->drawGeometry(m_state);
    if(m_showZoneObjects)
        m_zone->drawObjects(m_state);
    m_state->popMatrix();
}

void ZoneScene::step(double distance)
{
    matrix4 m = matrix4::rotate(m_playerOrient, 0.0, 0.0, 1.0);
    m_playerPos = m_playerPos + m.map(vec3(0.0, -distance, 0.0));
}

void ZoneScene::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if(key == Qt::Key_Q)
        m_playerOrient -= 5.0;
    else if(key == Qt::Key_D)
        m_playerOrient += 5.0;
    else if(key == Qt::Key_Z)
        step(5.0);
    else if(key == Qt::Key_S)
        step(-5.0);
    else if(key == Qt::Key_E)
        m_playerPos.z -= 5.0;
    else if(key == Qt::Key_A)
        m_playerPos.z += 5.0;
}
