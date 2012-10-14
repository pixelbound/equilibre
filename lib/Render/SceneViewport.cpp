// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <cmath>
#include <GL/glew.h>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include "EQuilibre/Render/SceneViewport.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Scene.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/FrameStat.h"

SceneViewport::SceneViewport(Scene *scene, RenderContext *renderCtx, QWidget *parent) : QGLWidget(parent)
{
    setMinimumSize(640, 480);
    m_scene = scene;
    m_renderCtx = renderCtx;
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(0);
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(1000);
#ifdef USE_VTUNE_PROFILER
    m_traceDomain = __itt_domain_create("SceneViewport");
#endif
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
    
    m_renderCtx->init();
    m_scene->init();
    emit initialized();
}

void SceneViewport::resizeGL(int w, int h)
{
    m_renderCtx->setupViewport(w, h);
}

void SceneViewport::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
#ifdef USE_VTUNE_PROFILER
    __itt_frame_begin_v3(m_traceDomain, NULL); 
#endif
    m_scene->draw();
#ifdef USE_VTUNE_PROFILER
    __itt_frame_end_v3(m_traceDomain, NULL); 
#endif
    paintOverlay(painter); // XXX fix QPainter overlay artifacts on Windows.
}

void SceneViewport::paintOverlay(QPainter &painter)
{
    if(m_statsTimer->isActive())
        paintStats(&painter);
    paintFrameLog(&painter);
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
    foreach(FrameStat *stat, m_renderCtx->stats())
        stat->clear();
}

void SceneViewport::updateStats()
{
    const QVector<FrameStat *> stats = m_renderCtx->stats();
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
    const QVector<FrameStat *> stats = m_renderCtx->stats();
    for(int i = 0; i < stats.count(); i++)
    {
        FrameStat *stat = stats[i];
        float val = m_lastStats.value(i);
        text += QString("%1: %2\n").arg(stat->name()).arg(val, 0, 'g', 4);
    }
    p->setPen(QPen(Qt::white));
    p->drawText(QRectF(QPointF(10, 5), QSizeF(800, 400)), text);
}

void SceneViewport::paintFrameLog(QPainter *p)
{
    QFont f;
    f.setPointSizeF(16.0);
    f.setWeight(QFont::Bold);
    p->setFont(f);
    p->setPen(QPen(Qt::white));
    QString text = m_scene->frameLog();
    QFontMetrics fm(f);
    QSizeF size(fm.size(0, text));
    QPointF pos(width() - size.width() - 10, 5);
    if(!text.isEmpty())
        p->drawText(QRectF(pos, size), text);
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
