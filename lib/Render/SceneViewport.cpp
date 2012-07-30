#include <cmath>
#include <GL/glew.h>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include "OpenEQ/Render/SceneViewport.h"
#include "OpenEQ/Render/Vertex.h"
#include "OpenEQ/Render/Scene.h"
#include "OpenEQ/Render/Material.h"
#include "OpenEQ/Render/RenderState.h"
#include "OpenEQ/Render/FrameStat.h"

SceneViewport::SceneViewport(Scene *scene, RenderState *state, QWidget *parent) : QGLWidget(parent)
{
    setMinimumSize(640, 480);
    m_scene = scene;
    m_state = state;
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(0);
    m_frameStat = state->createStat("Frame (ms)");
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(1000);
    setAutoFillBackground(false);
    connect(m_statsTimer, SIGNAL(timeout()), this, SLOT(updateStats()));
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
        fprintf(stderr, "OpenGL 2.0 features not available.");
        return;
    }
    else if(!GL_EXT_texture_array)
    {
        fprintf(stderr, "'GL_EXT_texture_array' extension not available.");
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
    if(m_statsTimer->isActive())
        paintStats(&painter);
}

void SceneViewport::paintGL()
{
    double frameStart = currentTime();
    if(m_state->beginFrame())
    {
        m_scene->draw();
        m_state->endFrame();
    }
    double frameEnd = currentTime();
    float frameDur = (float)(frameEnd - frameStart);
    //float fps = (frameDur == 0.0f) ? 0.0f : (1.0f / frameDur);
    m_frameStat->addSample(frameDur * 1000.0f);
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

bool SceneViewport::showStats() const
{
    return m_statsTimer->isActive();
}

void SceneViewport::setShowStats(bool show)
{
    if(show)
    {
        startStats();
        m_statsTimer->start();
    }
    else
    {
        m_statsTimer->stop();
    }
}

void SceneViewport::startStats()
{
    m_frameStat->clear();
}

void SceneViewport::updateStats()
{
    const QVector<FrameStat *> stats = m_state->stats();
    for(int i = 0; i < stats.count(); i++)
    {
        if(i >= m_lastStats.count())
            m_lastStats.append(0.0f);
        m_lastStats[i] = stats[i]->average();
    }
}

void SceneViewport::paintStats(QPainter *p)
{
    QFont f;
    f.setPointSizeF(16.0);
    f.setWeight(QFont::Bold);
    p->setFont(f);
    QString text;
    const QVector<FrameStat *> stats = m_state->stats();
    for(int i = 0; i < stats.count(); i++)
    {
        FrameStat *stat = stats[i];
        float val = m_lastStats.value(i);
        text += QString("%1: %2\n").arg(stat->name()).arg(val, 0, 'g', 4);
    }
    p->setPen(QPen(Qt::white));
    p->drawText(QRectF(QPointF(10, 5), QSizeF(800, 400)), text);
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
