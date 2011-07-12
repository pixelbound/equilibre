#include <QSettings>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <cmath>
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
    m_transState.last = vec3();
    m_rotState.last = vec3();
    m_transState.active = false;
    m_rotState.active = false;
    m_delta = vec3(-0.0, -0.0, -5.0);
    m_theta = vec3(-90.0, 00.0, 270.0);
    m_sigma = 0.5;
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
    m_started = currentTime();
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

void Scene::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if(key == Qt::Key_Q)
        m_theta.y += 5.0;
    else if(key == Qt::Key_D)
        m_theta.y -= 5.0;
    else if(key == Qt::Key_2)
        m_theta.x += 5.0;
    else if(key == Qt::Key_8)
        m_theta.x -= 5.0;
    else if(key == Qt::Key_4)
        m_theta.z += 5.0;
    else if(key == Qt::Key_6)
        m_theta.z -= 5.0;
    else if(key == Qt::Key_7)
        topView();
    else if(key == Qt::Key_3)
        sideView();
    else if(key == Qt::Key_1)
        frontView();
}

void Scene::mouseMoveEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();

    if(m_transState.active)
    {
        int dx = m_transState.x0 - x;
        int dy = m_transState.y0 - y;
        m_delta.x = (m_transState.last.x - (dx / 100.0));
        m_delta.z = (m_transState.last.y + (dy / 100.0));
    }

    if(m_rotState.active)
    {
        int dx = m_rotState.x0 - x;
        int dy = m_rotState.y0 - y;
        m_theta.x = (m_rotState.last.x + (dy * 2.0));
        m_theta.z = (m_rotState.last.z + (dx * 2.0));
    }
}

void Scene::mousePressEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();
    if(e->button() & Qt::MiddleButton)       // middle button pans the scene
    {
        m_transState.active = true;
        m_transState.x0 = x;
        m_transState.y0 = y;
        m_transState.last = m_delta;
    }
    else if(e->button() & Qt::LeftButton)   // left button rotates the scene
    {
        m_rotState.active = true;
        m_rotState.x0 = x;
        m_rotState.y0 = y;
        m_rotState.last = m_theta;
        //setFocus();
    }
}

void Scene::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::MiddleButton)
        m_transState.active = false;
    else if(e->button() & Qt::LeftButton)
        m_rotState.active = false;
}

void Scene::wheelEvent(QWheelEvent *e)
{
    // mouse wheel up zooms towards the scene
    // mouse wheel down zooms away from scene
    m_sigma *= pow(1.01, e->delta() / 8);
}
