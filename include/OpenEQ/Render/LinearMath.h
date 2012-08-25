#ifndef OPENEQ_RENDER_LINEAR_MATH_H
#define OPENEQ_RENDER_LINEAR_MATH_H

#include <QVector3D>
#include <QQuaternion>
#include "OpenEQ/Render/Platform.h"

bool fequal(double a, double b);

class RENDER_DLL vec2
{
public:
    float x;
    float y;

    inline vec2()
    {
        x = y = 0.0;
    }

    inline vec2(float x, float y)
    {
        this->x = x;
        this->y = y;
    }
};

class RENDER_DLL vec3
{
public:
    float x;
    float y;
    float z;

    inline vec3()
    {
        x = y = z = 0.0;
    }

    inline vec3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    
    inline vec3(vec2 xy, float z)
    {
        this->x = xy.x;
        this->y = xy.y;
        this->z = z;
    }

    vec3 normalized() const;

    static float dot(const vec3 &a, const vec3 &b);
    static vec3 cross(const vec3 &a, const vec3 &b);
    static vec3 normal(const vec3 &a, const vec3 &b, const vec3 &c);
};

vec3 RENDER_DLL operator-(const vec3 &a);
vec3 RENDER_DLL operator+(const vec3 &a, const vec3 &b);
vec3 RENDER_DLL operator-(const vec3 &a, const vec3 &b);
vec3 RENDER_DLL operator*(const vec3 &a, float scalar);

class RENDER_DLL vec4
{
public:
    float x;
    float y;
    float z;
    float w;

    inline vec4()
    {
        x = y = z = w = 0.0;
    }

    inline vec4(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }
};

class RENDER_DLL matrix4
{
public:

    matrix4();
    matrix4(const QMatrix4x4 &m);
    
    const float * data() const;

    vec3 map(const vec3 &v) const;
    vec3 mapNormal(const vec3 &v) const;

    void clear();
    void setIdentity();

    static matrix4 translate(float dx, float dy, float dz);
    static matrix4 rotate(float angle, float rx, float ry, float rz);
    static matrix4 scale(float sx, float sy, float sz);
    static matrix4 perspective(float angle, float aspect, float nearPlane, float farPlane);
    static matrix4 ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    static matrix4 lookAt(vec3 eye, vec3 center, vec3 up);
    
    matrix4 operator*(const matrix4 &b);
    
private:
    float d[16];
};

class RENDER_DLL BoneTransform
{
public:
    QVector4D location;
    QQuaternion rotation;

    BoneTransform();
    BoneTransform(const vec4 &loc, const vec4 &rot);

    vec3 map(const vec3 &v);
    QVector4D map(const QVector4D &v);
    void toDualQuaternion(vec4 &d0, vec4 &d1) const;

    static BoneTransform interpolate(BoneTransform a, BoneTransform b, double c);
};

#endif
