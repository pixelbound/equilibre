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

#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include "EQuilibre/Render/RenderState.h"

RenderState::RenderState()
{
    m_bgColor = vec4(0.6, 0.6, 1.0, 1.0);
}

RenderState::~RenderState()
{
}

void RenderState::init()
{
}

void RenderState::translate(const QVector3D &v)
{
    translate(v.x(), v.y(), v.z());
}

void RenderState::translate(const vec3 &v)
{
    translate(v.x, v.y, v.z);
}

void RenderState::rotate(const QQuaternion &q)
{
    QMatrix4x4 m;
    m.setToIdentity();
    m.rotate(q);
    matrix4 m2(m);
    multiplyMatrix(m2);
}

void RenderState::scale(const vec3 &v)
{
    scale(v.x, v.y, v.z);
}
