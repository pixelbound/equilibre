#include "Scene.h"
#include "WLDModel.h"
#include "Zone.h"

static double currentTime();

Scene::Scene(RenderState *state) : StateObject(state)
{
    m_sigma = 1.0;
    m_zone = new Zone(this);
    m_mode = CharacterViewer;
    reset();
}

Scene::~Scene()
{
}

const QMap<QString, WLDModel *> & Scene::models() const
{
    if(m_mode == CharacterViewer)
        return m_zone->charModels();
    else
        return m_zone->objectModels();
}

void Scene::init()
{
}

void Scene::reset()
{
    m_delta = vec3(-0.0, -0.0, -5.0);
    m_theta = vec3(-90.0, 00.0, 270.0);
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
    return models().value(m_meshName);
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
    switch(m_mode)
    {
    case CharacterViewer:
        if(model)
            model->draw(m_state, currentTime());
        break;
    case ObjectViewer:
        if(model)
            model->draw(m_state);
        break;
    case ZoneViewer:
        m_zone->drawGeometry(m_state);
        m_zone->drawObjects(m_state);
        break;
    }
}

bool Scene::openZone(QString path, QString zoneName)
{
    return m_zone->load(path, zoneName);
}

bool Scene::loadCharacters(QString archivePath, QString wldName)
{
    return m_zone->loadCharacters(archivePath, wldName);
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
