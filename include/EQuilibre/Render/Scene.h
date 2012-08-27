#ifndef EQUILIBRE_SCENE_H
#define EQUILIBRE_SCENE_H

#include <QObject>
#include <QMap>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"

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

class RENDER_DLL Scene : public QObject
{
    Q_OBJECT

public:
    Scene(RenderState *state);
    virtual ~Scene();

    QString assetPath() const;
    void setAssetPath(QString path);
    
    virtual QString frameLog() const;
    virtual void log(QString text);
    virtual void clearLog();

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
    QString m_frameLog;
};

#endif
