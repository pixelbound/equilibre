#ifndef OPENEQ_SCENE_H
#define OPENEQ_SCENE_H

#include "RenderState.h"
#include "Vertex.h"

class Scene : public StateObject
{
public:
    Scene(RenderState *state);
    virtual ~Scene();

    void init();

    vec3 & theta();
    float & sigma();
    vec3 & delta();

    void draw();

    void selectNext();
    void selectPrevious();

    void topView();
    void sideView();
    void frontView();

    void reset();
    void animate();

private:
    double m_started;
    int m_selected;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
};

#endif
