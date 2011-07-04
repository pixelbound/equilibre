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

class Scene : public QObject
{
    Q_OBJECT

public:
    Scene(RenderState *state);
    virtual ~Scene();

    void init();

    vec3 & theta();
    float & sigma();
    vec3 & delta();

    Zone * zone() const;
    const QMap<QString, WLDActor *> & charModels() const;
    WLDActor * selectedCharacter() const;
    QString selectedModelName() const;

    QString assetPath() const;
    void setAssetPath(QString path);

    void draw();

    void selectNext();
    void selectPrevious();

    void topView();
    void sideView();
    void frontView();

    void reset();

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
    int m_selected;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    QString m_meshName;
    Zone *m_zone;
    Mode m_mode;
    bool m_showZoneObjects;
    QSettings *m_settings;
};

#endif
