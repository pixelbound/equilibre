#ifndef OPENEQ_SCENE_H
#define OPENEQ_SCENE_H

#include <QObject>
#include <QMap>
#include "Vertex.h"

class WLDModel;
class WLDActor;
class Zone;
class RenderState;
class QSettings;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

struct MouseState
{
    bool active;
    int x0;
    int y0;
    vec3 last;        // value of delta/theta when the user last clicked
};

class Scene : public QObject
{
    Q_OBJECT

public:
    Scene(RenderState *state);
    virtual ~Scene();

    QString assetPath() const;
    void setAssetPath(QString path);

    virtual void init();
    virtual void draw() = 0;

    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

protected:
    RenderState *m_state;
    QSettings *m_settings;
};

#endif
