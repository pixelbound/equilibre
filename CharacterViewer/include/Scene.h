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

    void init();

    Zone * zone() const;
    const QMap<QString, WLDActor *> & charModels() const;
    WLDActor * selectedCharacter() const;
    QString selectedModelName() const;

    QString assetPath() const;
    void setAssetPath(QString path);

    void draw();

    void topView();
    void sideView();
    void frontView();

    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

    enum Mode
    {
        CharacterViewer,
        ZoneViewer
    };

    Mode mode() const;
    void setMode(Mode mode);

public slots:
    void setSelectedModelName(QString name);
    void showZoneObjects(bool show);

private:
    RenderState *m_state;
    double m_started;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    QString m_meshName;
    Zone *m_zone;
    Mode m_mode;
    bool m_showZoneObjects;
    QSettings *m_settings;
    // viewer settings
    MouseState m_transState;
    MouseState m_rotState;
};

#endif
