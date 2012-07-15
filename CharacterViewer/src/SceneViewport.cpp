#include <cmath>
#include <GL/glew.h>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include "SceneViewport.h"
#include "Vertex.h"
#include "Scene.h"
#include "Material.h"
#include "RenderState.h"

SceneViewport::SceneViewport(Scene *scene, RenderState *state, QWidget *parent) : QGLWidget(parent)
{
    setMinimumSize(640, 480);
    m_scene = scene;
    m_state = state;
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(0);
    m_frames = 0;
    m_lastFPS = 0;
    m_fpsTimer = new QTimer(this);
    m_fpsTimer->setInterval(1000);
    setAutoFillBackground(false);
    connect(m_fpsTimer, SIGNAL(timeout()), this, SLOT(updateFPS()));
    connect(m_renderTimer, SIGNAL(timeout()), this, SLOT(update()));
}

SceneViewport::~SceneViewport()
{
}

void SceneViewport::initializeGL()
{
    GLenum err = glewInit();
    if(GLEW_OK != err)
    {
        fprintf(stderr, "GLEW Error: %s", glewGetErrorString(err));
        return;
    }
    else if(!GLEW_VERSION_2_0)
    {
        fprintf(stderr, "OpenGL 2.0 features not available");
        return;
    }
    
    m_state->init();
    m_scene->init();
}

void SceneViewport::resizeGL(int w, int h)
{
    m_state->setupViewport(w, h);
}

void SceneViewport::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    paintGL();
    m_frames++;
    if(m_fpsTimer->isActive())
        paintFPS(&painter, m_lastFPS);
}

void SceneViewport::paintGL()
{
    if(m_state->beginFrame())
    {
        m_scene->draw();
        m_state->endFrame();
    }
}

void SceneViewport::toggleAnimation()
{
    m_animate = !m_animate;
    if(m_animate)
        m_renderTimer->start();
    else
        m_renderTimer->stop();
}

void SceneViewport::setAnimation(bool enabled)
{
    m_animate = enabled;
    updateAnimationState();
}

bool SceneViewport::showFps() const
{
    return m_fpsTimer->isActive();
}

void SceneViewport::setShowFps(bool show)
{
    if(show)
    {
        startFPS();
        m_fpsTimer->start();
    }
    else
    {
        m_fpsTimer->stop();
    }
}

void SceneViewport::startFPS()
{
    m_start = QTime::currentTime();
}

void SceneViewport::updateFPS()
{
    qint64 elapsedMillis = m_start.msecsTo(QTime::currentTime());
    m_lastFPS = m_frames / ((float)elapsedMillis / 1000.0);
    m_frames = 0;
    m_start = QTime::currentTime();
}

void SceneViewport::paintFPS(QPainter *p, float fps)
{
    QFont f;
    f.setPointSizeF(16.0);
    f.setWeight(QFont::Bold);
    p->setFont(f);
    QString text = QString("%1 FPS").arg(fps, 0, 'g', 4);
    p->setPen(QPen(Qt::white));
    p->drawText(QRectF(QPointF(10, 5), QSizeF(200, 100)), text);
}

void SceneViewport::updateAnimationState()
{
    if(m_animate)
    {
        m_renderTimer->start();
    }
    else
    {
        m_renderTimer->stop();
    }
}

void SceneViewport::keyReleaseEvent(QKeyEvent *e)
{
    m_scene->keyReleaseEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::keyReleaseEvent(e);
        //return;
    }
    update();
}

void SceneViewport::mouseMoveEvent(QMouseEvent *e)
{
    m_scene->mouseMoveEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::mouseMoveEvent(e);
        //return;
    }
    update();
}

void SceneViewport::mousePressEvent(QMouseEvent *e)
{
    m_scene->mousePressEvent(e);
    if(e->button() & Qt::LeftButton)
        setFocus();
    //if(!e->isAccepted())
    {
        QGLWidget::mousePressEvent(e);
        //return;
    }
    update();
}

void SceneViewport::mouseReleaseEvent(QMouseEvent *e)
{
    m_scene->mouseReleaseEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::mouseReleaseEvent(e);
        //return;
    }
    update();
}

void SceneViewport::wheelEvent(QWheelEvent *e)
{
    m_scene->wheelEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::wheelEvent(e);
        //return;
    }
    update();
}
