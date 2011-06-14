#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include "RenderState.h"

RenderState::RenderState()
{
    m_bgColor = vec4(0.6, 0.6, 1.0, 1.0);
    reset();
}

RenderState::~RenderState()
{
}

void RenderState::toggleWireframe()
{
    m_wireframe = !m_wireframe;
}

void RenderState::toggleProjection()
{
    m_projection = !m_projection;
}

void RenderState::reset()
{
    m_projection = true;
    m_wireframe = false;
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

////////////////////////////////////////////////////////////////////////////////

StateObject::StateObject(RenderState *s)
{
    m_state = s;
}

void StateObject::loadIdentity()
{
    m_state->loadIdentity();
}

void StateObject::pushMatrix()
{
    m_state->pushMatrix();
}

void StateObject::popMatrix()
{
    m_state->popMatrix();
}

void StateObject::translate(float dx, float dy, float dz)
{
    m_state->translate(dx, dy, dz);
}

void StateObject::rotate(float angle, float rx, float ry, float rz)
{
    m_state->rotate(angle, rx, ry, rz);
}

void StateObject::scale(float sx, float sy, float sz)
{
    m_state->scale(sx, sy, sz);
}

void StateObject::pushMaterial(const Material &m)
{
    m_state->pushMaterial(m);
}

void StateObject::popMaterial()
{
    m_state->popMaterial();
}
