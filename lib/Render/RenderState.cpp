#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include "OpenEQ/Render/RenderState.h"

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
    matrix4 m2;
    for(int i = 0; i < 16; i++)
        m2.d[i] = (float)m.constData()[i];
    multiplyMatrix(m2);
}

void RenderState::scale(const vec3 &v)
{
    scale(v.x, v.y, v.z);
}
