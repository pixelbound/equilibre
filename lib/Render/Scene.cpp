// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <QSettings>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include "EQuilibre/Render/Scene.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/Zone.h"

Scene::Scene(RenderContext *renderCtx)
{
    m_renderCtx = renderCtx;
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
        "EQuilibre", QString(), this);
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

void Scene::update(double /*timestamp*/)
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
