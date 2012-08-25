#ifndef OPENEQ_SCENE_VIEWPORT_H
#define OPENEQ_SCENE_VIEWPORT_H

#include <QGLWidget>
#include <QTime>
#include <QVector>
#include "OpenEQ/Render/Platform.h"

#ifdef USE_VTUNE_PROFILER
#include <ittnotify.h>
#endif

class QTimer;
class QPainter;
class QGLFormat;
class Scene;
class RenderState;
class FrameStat;

class RENDER_DLL SceneViewport : public QGLWidget
{
    Q_OBJECT

public:
    SceneViewport(Scene *scene, RenderState *state, QWidget *parent = 0);
    virtual ~SceneViewport();

    void setAnimation(bool enabled);
    bool showStats() const;
    
signals:
    void initialized();

public slots:
    void setShowStats(bool show);

protected:
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintGL();
    virtual void paintEvent(QPaintEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

private slots:
    void updateStats();

private:
    void paintStats(QPainter *p);
    void paintFrameLog(QPainter *p);
    void startStats();
    void updateAnimationState();
    void toggleAnimation();

    Scene *m_scene;
    RenderState *m_state;
    QTimer *m_renderTimer;
    bool m_animate;

    // Stats
    QTimer *m_statsTimer;
    QVector<float> m_lastStats;
    
#ifdef USE_VTUNE_PROFILER
    __itt_domain *m_traceDomain;
#endif
};

#endif
