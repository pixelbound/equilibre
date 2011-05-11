#ifndef OPENEQ_SCENE_H
#define OPENEQ_SCENE_H

#include <QObject>
#include "RenderState.h"
#include "Vertex.h"

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

    QString meshName() const;


    void draw();

    void selectNext();
    void selectPrevious();

    void topView();
    void sideView();
    void frontView();

    void reset();
    void animate();

public slots:
    void setMeshName(QString name);

private:
    double m_started;
    int m_selected;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    QString m_meshName;
};

#endif
