#include <QSettings>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include "OpenEQ/Render/Scene.h"
#include "OpenEQ/Render/RenderState.h"
#include "OpenEQ/Game/WLDModel.h"
#include "OpenEQ/Game/WLDActor.h"
#include "OpenEQ/Game/Zone.h"

Scene::Scene(RenderState *state)
{
    m_state = state;
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
        "OpenEQ", QString(), this);
}

Scene::~Scene()
{
}

QString Scene::assetPath() const
{
    return m_settings->value("assetPath", ".").toString();
}

void Scene::setAssetPath(QString path)
{
     m_settings->setValue("assetPath", path);
}

QString Scene::frameLog() const
{
    return m_frameLog;
}

void Scene::log(QString text)
{
    m_frameLog += text;
    m_frameLog += "\n";
}

void Scene::clearLog()
{
    m_frameLog.clear();
}

void Scene::init()
{
}

void Scene::keyReleaseEvent(QKeyEvent *e)
{
    e->ignore();
}

void Scene::mouseMoveEvent(QMouseEvent *e)
{
    e->ignore();
}

void Scene::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
}

void Scene::mouseReleaseEvent(QMouseEvent *e)
{
    e->ignore();
}

void Scene::wheelEvent(QWheelEvent *e)
{
    e->ignore();
}
