#include <QSettings>
#include "Scene.h"
#include "RenderState.h"
#include "WLDModel.h"
#include "WLDActor.h"
#include "Zone.h"

Scene::Scene(RenderState *state)
{
    m_state = state;
    m_sigma = 1.0;
    m_zone = new Zone(this);
    m_mode = CharacterViewer;
    m_showZoneObjects = false;
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
        "OpenEQ", QString(), this);
    reset();
}

Scene::~Scene()
{
}

const QMap<QString, WLDActor *> & Scene::charModels() const
{
    return m_zone->charModels();
}

QString Scene::assetPath() const
{
    return m_settings->value("assetPath", ".").toString();
}

void Scene::setAssetPath(QString path)
{
     m_settings->setValue("assetPath", path);
}

void Scene::init()
{
}

void Scene::reset()
{
    m_delta = vec3(-0.0, -0.0, -5.0);
    m_theta = vec3(-90.0, 00.0, 270.0);
    m_sigma = 0.5;
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

WLDActor * Scene::selectedCharacter() const
{
    return charModels().value(m_meshName);
}

Zone * Scene::zone() const
{
    return m_zone;
}

Scene::Mode Scene::mode() const
{
    return m_mode;
}

void Scene::setMode(Scene::Mode mode)
{
    m_mode = mode;
}

void Scene::draw()
{
    vec3 rot = m_theta;
    m_state->translate(m_delta.x, m_delta.y, m_delta.z);
    m_state->rotate(rot.x, 1.0, 0.0, 0.0);
    m_state->rotate(rot.y, 0.0, 1.0, 0.0);
    m_state->rotate(rot.z, 0.0, 0.0, 1.0);
    m_state->scale(m_sigma, m_sigma, m_sigma);

    WLDActor *charModel = selectedCharacter();
    switch(m_mode)
    {
    case CharacterViewer:
        if(charModel)
        {
            charModel->setAnimTime(currentTime());
            charModel->draw(m_state);
        }
        break;
    case ZoneViewer:
        m_zone->drawGeometry(m_state);
        if(m_showZoneObjects)
            m_zone->drawObjects(m_state);
        break;
    }
}

void Scene::showZoneObjects(bool show)
{
    m_showZoneObjects = show;
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
