#include <cmath>
#include <sstream>
#include "Scene.h"
#include "Mesh.h"
#include "Material.h"

static Material debugMaterial(vec4(0.2, 0.2, 0.2, 1.0),
    vec4(1.0, 4.0/6.0, 0.0, 1.0), vec4(0.2, 0.2, 0.2, 1.0), 20.0);

static Material floorMaterial(vec4(0.5, 0.5, 0.5, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0), vec4(0.0, 0.0, 0.0, 1.0), 00.0);

static double currentTime();

Scene::Scene(RenderState *state) : StateObject(state)
{
    m_sigma = 1.0;

    reset();
    animate();
}

Scene::~Scene()
{
}

void Scene::init()
{
}

void Scene::reset()
{
    m_delta = vec3(-0.0, -0.0, -5.0);
    m_theta = vec3();
    m_sigma = 0.01;
    m_started = currentTime();
}

vec3 & Scene::theta()
{
    return m_theta;
}

float & Scene::sigma()
{
    return m_sigma;
}

vec3 & Scene::delta()
{
    return m_delta;
}

void Scene::draw()
{
    vec3 rot = m_theta;
    m_state->translate(m_delta.x, m_delta.y, m_delta.z);
    m_state->rotate(rot.x, 1.0, 0.0, 0.0);
    m_state->rotate(rot.y, 0.0, 1.0, 0.0);
    m_state->rotate(rot.z, 0.0, 0.0, 1.0);
    m_state->scale(m_sigma, m_sigma, m_sigma);
    m_state->pushMaterial(debugMaterial);
    m_state->drawMesh("tree");
    m_state->popMaterial();
}

void Scene::topView()
{
    m_theta = vec3(0.0, 0.0, 0.0);
}

void Scene::sideView()
{
    m_theta = vec3(-90.0, 0.0, -90.0);
}

void Scene::frontView()
{
    m_theta = vec3(-90.0, 0.0, 0.0);
}

void Scene::animate()
{
    //double t = currentTime() - m_started;
}

#ifdef WIN32
#include <windows.h>
double currentTime()
{
    return (double)GetTickCount() * 10e-3;
}
#else
#include <sys/time.h>
double currentTime()
{
    timeval tv;
    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + ((double)tv.tv_usec * 10e-7);
}
#endif
