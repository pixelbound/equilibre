#include <cmath>
#include <iostream>
#include <vector>
#include <cstring>
#include "Vertex.h"

using namespace std;

bool fequal(double a, double b)
{
    return fabs(a - b) < 1e-16;
}

vec3 vec3::normalized() const
{
    float w = (float)sqrt(x * x + y * y + z * z);
    return vec3(x / w, y / w, z / w);
}

float vec3::dot(const vec3 &a, const vec3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3::cross(const vec3 &u, const vec3 &v)
{
    vec3 n;
    n.x = (u.y * v.z - u.z * v.y);
    n.y = (u.z * v.x - u.x * v.z);
    n.z = (u.x * v.y - u.y * v.x);
    return n;
}

vec3 vec3::normal(const vec3 &a, const vec3 &b, const vec3 &c)
{
    // the cross-product of AB and AC is the normal of ABC
    return cross(b - a, c - a).normalized();
}

vec3 operator-(const vec3 &a)
{
    return vec3(-a.x, -a.y, -a.z);
}

vec3 operator+(const vec3 &a, const vec3 &b)
{
    vec3 u;
    u.x = a.x + b.x;
    u.y = a.y + b.y;
    u.z = a.z + b.z;
    return u;
}

vec3 operator-(const vec3 &a, const vec3 &b)
{
    vec3 u;
    u.x = a.x - b.x;
    u.y = a.y - b.y;
    u.z = a.z - b.z;
    return u;
}

vec3 operator*(const vec3 &a, float scalar)
{
    vec3 u;
    u.x = a.x * scalar;
    u.y = a.y * scalar;
    u.z = a.z * scalar;
    return u;
}

////////////////////////////////////////////////////////////////////////////////

matrix4::matrix4()
{
    clear();
}

vec3 matrix4::map(const vec3 &v) const
{
    float v_w = 1.0;
    float x = d[0] * v.x + d[1] * v.y + d[2] * v.z + d[3] * v_w;
    float y = d[4] * v.x + d[5] * v.y + d[6] * v.z + d[7] * v_w;
    float z = d[8] * v.x + d[9] * v.y + d[10] * v.z + d[11] * v_w;
    float w = d[12] * v.x + d[13] * v.y + d[14] * v.z + d[15] * v_w;
    return vec3(x / w, y / w, z / w);
}

vec3 matrix4::mapNormal(const vec3 &v) const
{
    float v_w = 1.0;
    float x = d[0] * v.x + d[1] * v.y + d[2] * v.z;
    float y = d[4] * v.x + d[5] * v.y + d[6] * v.z;
    float z = d[8] * v.x + d[9] * v.y + d[10] * v.z;
    float w = d[12] * v.x + d[13] * v.y + d[14] * v.z + v_w;
    return vec3(x / w, y / w, z / w);
}

void matrix4::clear()
{
    for(int i = 0; i < 16; i++)
        d[i] = 0.0;
}

void matrix4::setIdentity()
{
    clear();
    d[0] = d[5] = d[10] = d[15] = 1.0;
}

matrix4 matrix4::translate(float dx, float dy, float dz)
{
    matrix4 m;
    m.setIdentity();
    m.d[12] = dx;
    m.d[13] = dy;
    m.d[14] = dz;
    return m;
}

matrix4 matrix4::rotate(float angle, float x, float y, float z)
{
    float theta = angle / 180.0 * M_PI;
    float c = cos(theta);
    float s = sin(theta);
    float t = 1 - c;
    matrix4 m;
    m.d[0] = t * x * x + c;
    m.d[4] = t * x * y - s * z;
    m.d[8] = t * x * z + s * y;
    m.d[12] = 0.0;

    m.d[1] = t * x * y + s * z;
    m.d[5] = t * y * y + c;
    m.d[9] = t * y * z - s * x;
    m.d[13] = 0.0;

    m.d[2] = t * x * z - s * y;
    m.d[6] = t * y * z + s * x;
    m.d[10] = t * z * z + c;
    m.d[14] = 0.0;

    m.d[3] = 0.0;
    m.d[7] = 0.0;
    m.d[11] = 0.0;
    m.d[15] = 1.0;
    return m;
}

matrix4 matrix4::scale(float sx, float sy, float sz)
{
    matrix4 m;
    m.d[0] = sx;
    m.d[5] = sy;
    m.d[10] = sz;
    m.d[15] = 1.0;
    return m;
}

// The following three functions have been adapted from Qt:
// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qmatrix4x4.h
// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qmatrix4x4.cpp

matrix4 matrix4::perspective(float angle, float aspect, float nearPlane, float farPlane)
{
    matrix4 m;
    if (nearPlane == farPlane || aspect == 0.0)
        return m;
    float radians = (angle / 2.0) * M_PI / 180.0;
    float sine = sin(radians);
    if(fequal(sine, 0.0))
        return m;
    float cotan = cos(radians) / sine;
    float clip = farPlane - nearPlane;
    m.d[0] = cotan / aspect;
    m.d[4] = 0.0;
    m.d[8] = 0.0;
    m.d[12] = 0.0;
    m.d[1] = 0.0;
    m.d[5] = cotan;
    m.d[9] = 0.0;
    m.d[13] = 0.0;
    m.d[2] = 0.0;
    m.d[6] = 0.0;
    m.d[10] = -(nearPlane + farPlane) / clip;
    m.d[14] = -(2.0 * nearPlane * farPlane) / clip;
    m.d[3] = 0.0;
    m.d[7] = 0.0;
    m.d[11] = -1.0;
    m.d[15] = 0.0;
    return m;
}

matrix4 matrix4::ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    matrix4 m;
    if (left == right || bottom == top || nearPlane == farPlane)
        return m;
    float width = right - left;
    float invheight = top - bottom;
    float clip = farPlane - nearPlane;
    m.d[0] = 2.0f / width;
    m.d[4] = 0.0f;
    m.d[8] = 0.0f;
    m.d[12] = -(left + right) / width;
    m.d[1] = 0.0f;
    m.d[5] = 2.0f / invheight;
    m.d[9] = 0.0f;
    m.d[13] = -(top + bottom) / invheight;
    m.d[2] = 0.0f;
    m.d[6] = 0.0f;
    m.d[10] = -2.0f / clip;
    m.d[14] = -(nearPlane + farPlane) / clip;
    m.d[3] = 0.0f;
    m.d[7] = 0.0f;
    m.d[11] = 0.0f;
    m.d[15] = 1.0f;
    return m;
}

matrix4 operator*(const matrix4 &a, const matrix4 &b)
{
    matrix4 m;
    m.d[0] = a.d[0] * b.d[0] + a.d[4] * b.d[1] + a.d[8] * b.d[2] + a.d[12] * b.d[3];
    m.d[1] = a.d[1] * b.d[0] + a.d[5] * b.d[1] + a.d[9] * b.d[2] + a.d[13] * b.d[3];
    m.d[2] = a.d[2] * b.d[0] + a.d[6] * b.d[1] + a.d[10] * b.d[2] + a.d[14] * b.d[3];
    m.d[3] = a.d[3] * b.d[0] + a.d[7] * b.d[1] + a.d[11] * b.d[2] + a.d[15] * b.d[3];
    m.d[4] = a.d[0] * b.d[4] + a.d[4] * b.d[5] + a.d[8] * b.d[6] + a.d[12] * b.d[7];
    m.d[5] = a.d[1] * b.d[4] + a.d[5] * b.d[5] + a.d[9] * b.d[6] + a.d[13] * b.d[7];
    m.d[6] = a.d[2] * b.d[4] + a.d[6] * b.d[5] + a.d[10] * b.d[6] + a.d[14] * b.d[7];
    m.d[7] = a.d[3] * b.d[4] + a.d[7] * b.d[5] + a.d[11] * b.d[6] + a.d[15] * b.d[7];
    m.d[8] = a.d[0] * b.d[8] + a.d[4] * b.d[9] + a.d[8] * b.d[10] + a.d[12] * b.d[11];
    m.d[9] = a.d[1] * b.d[8] + a.d[5] * b.d[9] + a.d[9] * b.d[10] + a.d[13] * b.d[11];
    m.d[10] = a.d[2] * b.d[8] + a.d[6] * b.d[9] + a.d[10] * b.d[10] + a.d[14] * b.d[11];
    m.d[11] = a.d[3] * b.d[8] + a.d[7] * b.d[9] + a.d[11] * b.d[10] + a.d[15] * b.d[11];
    m.d[12] = a.d[0] * b.d[12] + a.d[4] * b.d[13] + a.d[8] * b.d[14] + a.d[12] * b.d[15];
    m.d[13] = a.d[1] * b.d[12] + a.d[5] * b.d[13] + a.d[9] * b.d[14] + a.d[13] * b.d[15];
    m.d[14] = a.d[2] * b.d[12] + a.d[6] * b.d[13] + a.d[10] * b.d[14] + a.d[14] * b.d[15];
    m.d[15] = a.d[3] * b.d[12] + a.d[7] * b.d[13] + a.d[11] * b.d[14] + a.d[15] * b.d[15];
    return m;
}

matrix4 matrix4::lookAt(vec3 eye, vec3 center, vec3 up)
{
    vec3 forward = (center - eye).normalized();
    vec3 side = vec3::cross(forward, up).normalized();
    up = vec3::cross(side, forward);

    matrix4 m;
    m.setIdentity();
    m.d[0] = side.x;
    m.d[4] = side.y;
    m.d[8] = side.z;
    m.d[1] = up.x;
    m.d[5] = up.y;
    m.d[9] = up.z;
    m.d[2] = -forward.x;
    m.d[6] = -forward.y;
    m.d[10] = -forward.z;
    return m * matrix4::translate(-eye.x, -eye.y, -eye.z);
}

void matrix4::dump() const
{
    cout << d[0] << d[1] << d[2] << d[3] << endl;
    cout << d[4] << d[5] << d[6] << d[7] << endl;
    cout << d[8] << d[9] << d[10] << d[11] << endl;
    cout << d[12] << d[13] << d[14] << d[15] << endl;
}

////////////////////////////////////////////////////////////////////////////////

float Plane::distance(vec3 v) const
{
    return vec3::dot(n, v) + vec3::dot(-n, p);
}

////////////////////////////////////////////////////////////////////////////////

Frustum::Frustum()
{
    m_angle = 45.0;
    m_aspect = 1.0;
    m_nearPlane = 0.1;
    m_farPlane = 1000.0;
    m_dirty = true;
}

float Frustum::aspect() const
{
    return m_aspect;
}

void Frustum::setAspect(float aspect)
{
    m_aspect = aspect;
    m_dirty = true;
}

const vec3 & Frustum::eye() const
{
    return m_eye;
}

void Frustum::setEye(vec3 eye)
{
    m_eye = eye;
    m_dirty = true;
}

const vec3 & Frustum::focus() const
{
    return m_focus;
}

void Frustum::setFocus(vec3 focus)
{
    m_focus = focus;
    m_dirty = true;
}

const vec3 & Frustum::up() const
{
    return m_up;
}

void Frustum::setUp(vec3 up)
{
    m_up = up;
    m_dirty = true;
}

matrix4 Frustum::projection() const
{
    return matrix4::perspective(m_angle, m_aspect, m_nearPlane, m_farPlane);
}

matrix4 Frustum::camera() const
{
    return matrix4::lookAt(m_eye, m_focus, m_up);
}

void Frustum::computePlanes()
{
    float nearHeight = m_nearPlane * (float)tan(m_angle * 0.5 * M_PI / 180.0);
    float nearWidth = nearHeight * m_aspect;

    // compute the three axes
    vec3 zAxis = (m_eye - m_focus).normalized();
    vec3 xAxis = vec3::cross(m_up, zAxis).normalized();
    vec3 yAxis = vec3::cross(zAxis, xAxis);

    // compute the centers of the near and far planes
    vec3 nearCenter = m_eye - zAxis * m_nearPlane;
    vec3 farCenter = m_eye - zAxis * m_farPlane;

    // compute the plane "positions" and normals
    m_planes[NEAR].p = nearCenter;
    m_planes[NEAR].n = -zAxis;

    m_planes[FAR].p = farCenter;
    m_planes[FAR].n = zAxis;

    m_planes[TOP].p = nearCenter + (yAxis * nearHeight);
    m_planes[TOP].n = vec3::cross(
        ((nearCenter + (yAxis * nearHeight)) - m_eye).normalized(), xAxis);

    m_planes[BOTTOM].p = nearCenter - (yAxis * nearHeight);
    m_planes[BOTTOM].n = vec3::cross(xAxis,
        ((nearCenter - (yAxis * nearHeight)) - m_eye).normalized());

    m_planes[LEFT].p = nearCenter - (xAxis * nearWidth);
    m_planes[LEFT].n = vec3::cross(
        ((nearCenter - (xAxis * nearWidth)) - m_eye).normalized(), yAxis);

    m_planes[RIGHT].p = nearCenter + (xAxis * nearWidth);
    m_planes[RIGHT].n = vec3::cross(yAxis,
        ((nearCenter + (xAxis * nearWidth)) - m_eye).normalized());
}

Frustum::TestResult Frustum::contains(vec3 v)
{
    if(m_dirty)
    {
        computePlanes();
        m_dirty = false;
    }
    for(int i = 0; i < 6; i++)
    {
        if(m_planes[i].distance(v) < 0)
            return OUTSIDE;
    }
    return INSIDE;
}

////////////////////////////////////////////////////////////////////////////////

VertexGroup::VertexGroup(uint32_t mode, uint32_t count)
{
    this->mode = mode;
    this->count = count;
    this->data = new VertexData[count];
    this->dataBuffer = 0;
    this->indicesBuffer = 0;
    memset(this->data, 0, sizeof(VertexData) * count);
}

VertexGroup::VertexGroup(uint32_t mode, const vector<VertexData> &data)
{
    this->mode = mode;
    this->count = data.size();
    this->data = new VertexData[this->count];
    this->dataBuffer = 0;
    this->indicesBuffer = 0;
    for(uint32_t i = 0; i < this->count; i++)
        this->data[i] = data[i];
}

VertexGroup::~VertexGroup()
{
    delete [] data;
}
