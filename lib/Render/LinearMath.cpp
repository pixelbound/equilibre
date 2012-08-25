#include <cmath>
#include <QMatrix4x4>
#include "OpenEQ/Render/LinearMath.h"

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

matrix4::matrix4(const QMatrix4x4 &m)
{
    const qreal *md = m.constData();
    for(int i = 0; i < 16; i++)
        d[i] = md[i];
}

const float * matrix4::data() const
{
    return d;
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

matrix4 matrix4::operator*(const matrix4 &b)
{
    matrix4 m;
    m.d[0] = d[0] * b.d[0] + d[4] * b.d[1] + d[8] * b.d[2] + d[12] * b.d[3];
    m.d[1] = d[1] * b.d[0] + d[5] * b.d[1] + d[9] * b.d[2] + d[13] * b.d[3];
    m.d[2] = d[2] * b.d[0] + d[6] * b.d[1] + d[10] * b.d[2] + d[14] * b.d[3];
    m.d[3] = d[3] * b.d[0] + d[7] * b.d[1] + d[11] * b.d[2] + d[15] * b.d[3];
    m.d[4] = d[0] * b.d[4] + d[4] * b.d[5] + d[8] * b.d[6] + d[12] * b.d[7];
    m.d[5] = d[1] * b.d[4] + d[5] * b.d[5] + d[9] * b.d[6] + d[13] * b.d[7];
    m.d[6] = d[2] * b.d[4] + d[6] * b.d[5] + d[10] * b.d[6] + d[14] * b.d[7];
    m.d[7] = d[3] * b.d[4] + d[7] * b.d[5] + d[11] * b.d[6] + d[15] * b.d[7];
    m.d[8] = d[0] * b.d[8] + d[4] * b.d[9] + d[8] * b.d[10] + d[12] * b.d[11];
    m.d[9] = d[1] * b.d[8] + d[5] * b.d[9] + d[9] * b.d[10] + d[13] * b.d[11];
    m.d[10] = d[2] * b.d[8] + d[6] * b.d[9] + d[10] * b.d[10] + d[14] * b.d[11];
    m.d[11] = d[3] * b.d[8] + d[7] * b.d[9] + d[11] * b.d[10] + d[15] * b.d[11];
    m.d[12] = d[0] * b.d[12] + d[4] * b.d[13] + d[8] * b.d[14] + d[12] * b.d[15];
    m.d[13] = d[1] * b.d[12] + d[5] * b.d[13] + d[9] * b.d[14] + d[13] * b.d[15];
    m.d[14] = d[2] * b.d[12] + d[6] * b.d[13] + d[10] * b.d[14] + d[14] * b.d[15];
    m.d[15] = d[3] * b.d[12] + d[7] * b.d[13] + d[11] * b.d[14] + d[15] * b.d[15];
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

////////////////////////////////////////////////////////////////////////////////

BoneTransform::BoneTransform()
{
}

BoneTransform::BoneTransform(const vec4 &loc, const vec4 &rot)
{
    location = QVector4D(loc.x, loc.y, loc.z, loc.w);
    rotation = QQuaternion(rot.w, rot.x, rot.y, rot.z);
}

vec3 BoneTransform::map(const vec3 &v)
{
    QVector3D v2(v.x, v.y, v.z);
    v2 = rotation.rotatedVector(v2) + location.toVector3D();
    return vec3(v2.x(), v2.y(), v2.z());
}

QVector4D BoneTransform::map(const QVector4D &v)
{
    return QVector4D(rotation.rotatedVector(v.toVector3D()) + location.toVector3D(), 1.0);
}

BoneTransform BoneTransform::interpolate(BoneTransform a, BoneTransform b, double f)
{
    BoneTransform c;
    c.rotation = QQuaternion::slerp(a.rotation, b.rotation, f);
    c.location = (a.location * (1.0 - f)) + (b.location * f);
    return c;
}

void BoneTransform::toDualQuaternion(vec4 &d0, vec4 &d1) const
{
    const QVector4D &tran(location);
    d0.x = rotation.x();
    d0.y = rotation.y();
    d0.z = rotation.z();
    d0.w = rotation.scalar();
    d1.x = 0.5f * (tran.x() * d0.w + tran.y() * d0.z - tran.z() * d0.y);
    d1.y = 0.5f * (-tran.x() * d0.z + tran.y() * d0.w + tran.z() * d0.x);
    d1.z = 0.5f * (tran.x() * d0.y - tran.y() * d0.x + tran.z() * d0.w);
    d1.w = -0.5f * (tran.x() * d0.x + tran.y() * d0.y + tran.z() * d0.z);
}
