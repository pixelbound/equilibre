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
#include "ZoneViewerWindow.h"
#include "SceneViewport.h"
#include "RenderState.h"
#include "Scene.h"
#include "WLDModel.h"
#include "WLDActor.h"
#include "WLDSkeleton.h"
#include "Zone.h"

ZoneViewerWindow::ZoneViewerWindow(Scene *scene, RenderState *state,
                                             QWidget *parent) : QMainWindow(parent)
{
    m_scene = scene;
    m_state = state;
    setWindowTitle("OpenEQ Zone Viewer");
    m_viewport = new SceneViewport(scene, state);
    setCentralWidget(m_viewport);
    initMenus();
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
