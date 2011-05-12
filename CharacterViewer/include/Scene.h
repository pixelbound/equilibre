#ifndef OPENEQ_SCENE_H
#define OPENEQ_SCENE_H

#include <QObject>
#include <QMap>
#include "RenderState.h"
#include "Vertex.h"

class WLDData;
class WLDModel;

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

    WLDData * wldData() const;
    QMap<QString, WLDModel *> & models();
    WLDModel * selectedModel() const;
    QString selectedModelName() const;

    void draw();

    void selectNext();
    void selectPrevious();

    void topView();
    void sideView();
    void frontView();

    void reset();
    void animate();

    void openWLD(QString path);

public slots:
    void setSelectedModelName(QString name);

private:
    double m_started;
    int m_selected;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    QString m_meshName;
    WLDData *m_wldData;
    QMap<QString, WLDModel *> m_models;
};

#endif
