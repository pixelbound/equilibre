#include <cmath>
#include <sstream>
#include "Scene.h"
#include "Mesh.h"
#include "Material.h"
#include "WLDData.h"
#include "WLDModel.h"

static Material debugMaterial(vec4(0.2, 0.2, 0.2, 1.0),
    vec4(1.0, 4.0/6.0, 0.0, 1.0), vec4(0.2, 0.2, 0.2, 1.0), 20.0);

static Material floorMaterial(vec4(0.5, 0.5, 0.5, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0), vec4(0.0, 0.0, 0.0, 1.0), 00.0);

static double currentTime();

Scene::Scene(RenderState *state) : StateObject(state)
{
    m_sigma = 1.0;
    m_wldData = 0;
    reset();
    animate();
}

Scene::~Scene()
{
}

WLDData * Scene::wldData() const
{
    return m_wldData;
}

QMap<QString, WLDModel *> & Scene::models()
{
    return m_models;
}

void Scene::init()
{
}

void Scene::reset()
{
    m_delta = vec3(-0.0, -0.0, -5.0);
    m_theta = vec3(-90.0, 00.0, 90.0);
    m_sigma = 0.10;
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

QString Scene::selectedModelName() const
{
    return m_meshName;
}

void Scene::setSelectedModelName(QString name)
{
    m_meshName = name;
}

WLDModel * Scene::selectedModel() const
{
    return m_models.value(m_meshName);
}

void Scene::draw()
{
    vec3 rot = m_theta;
    m_state->translate(m_delta.x, m_delta.y, m_delta.z);
    m_state->rotate(rot.x, 1.0, 0.0, 0.0);
    m_state->rotate(rot.y, 0.0, 1.0, 0.0);
    m_state->rotate(rot.z, 0.0, 0.0, 1.0);
    m_state->scale(m_sigma, m_sigma, m_sigma);
    WLDModel *model = selectedModel();
    if(model)
        model->draw();
}

void Scene::openWLD(QString path)
{
    if(m_wldData)
        delete m_wldData;
    m_wldData = WLDData::fromFile(path, this);
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
