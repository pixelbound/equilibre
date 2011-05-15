#ifndef OPENEQ_SCENE_H
#define OPENEQ_SCENE_H

#include <QObject>
#include <QMap>
#include "RenderState.h"
#include "Vertex.h"

class WLDModel;
class Zone;

class Scene : public QObject, StateObject
{
    Q_OBJECT

public:
    Scene(RenderState *state);
    virtual ~Scene();

    void init();

    vec3 & theta();
    float & sigma();
    vec3 & delta();

    const QMap<QString, WLDModel *> & models() const;
    WLDModel * selectedModel() const;
    QString selectedModelName() const;

    void draw();

    void selectNext();
    void selectPrevious();

    void topView();
    void sideView();
    void frontView();

    void reset();

    bool openZone(QString path, QString zoneName);

    enum Mode
    {
        CharacterViewer,
        ObjectViewer,
        ZoneViewer
    };

public slots:
    void setSelectedModelName(QString name);

private:
    double m_started;
    int m_selected;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    QString m_meshName;
    Zone *m_zone;
    Mode m_mode;
};

#endif
