#ifndef OPENEQ_SCENE_VIEWPORT_H
#define OPENEQ_SCENE_VIEWPORT_H

#include <QGLWidget>
#include <QDateTime>
#include "Vertex.h"

class QTimer;
class QPainter;
class QGLFormat;
class Scene;
class RenderState;

typedef struct
{
    bool active;
    int x0;
    int y0;
    vec3 last;        // value of delta/theta when the user last clicked
} MouseState;

class SceneViewport : public QGLWidget
{
    Q_OBJECT

public:
    SceneViewport(Scene *scene, RenderState *state, const QGLFormat &format, QWidget *parent = 0);
    virtual ~SceneViewport();

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
    void updateFPS();
    void animateScene();

private:
    void paintFPS(QPainter *p, float fps);
    void startFPS();
    void updateAnimationState();
    void toggleAnimation();
    void resetCamera();

    Scene *m_scene;
    RenderState *m_state;
    QTimer *m_renderTimer;

    // viewer settings
    MouseState m_transState;
    MouseState m_rotState;
    bool m_animate;

    // FPS settings
    QTimer *m_fpsTimer;
    QDateTime m_start;
    uint m_frames;
    float m_lastFPS;
};

#endif
