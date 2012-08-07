#ifndef OPENEQ_VERTEX_H
#define OPENEQ_VERTEX_H

#include <QString>
#include <QVector>
#include <QVector3D>
#include <QQuaternion>
#include "OpenEQ/Render/Platform.h"

bool fequal(double a, double b);

typedef unsigned int buffer_t;
typedef unsigned int texture_t;

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
    float d[16];

    matrix4();

    vec3 map(const vec3 &v) const;
    vec3 mapNormal(const vec3 &v) const;

    void clear();
    void setIdentity();

    void dump() const;

    static matrix4 translate(float dx, float dy, float dz);
    static matrix4 rotate(float angle, float rx, float ry, float rz);
    static matrix4 scale(float sx, float sy, float sz);
    static matrix4 perspective(float angle, float aspect, float nearPlane, float farPlane);
    static matrix4 ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    static matrix4 lookAt(vec3 eye, vec3 center, vec3 up);
};

matrix4 RENDER_DLL operator*(const matrix4 &a, const matrix4 &b);

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

struct RENDER_DLL Plane
{
    vec3 p; // point on the plane
    vec3 n; // normal of the plane

    float distance(vec3 v) const;
};

struct RENDER_DLL AABox
{
    vec3 low, high;

    AABox();
    AABox(const vec3 &low, const vec3 &high);
    vec3 center() const;
    vec3 posVertex(const vec3 &normal) const;
    vec3 negVertex(const vec3 &normal) const;
    bool contains(const vec3 &p) const;
    bool contains(const AABox &b) const;
    void cornersTo(vec3 *corners) const;
    void extendTo(const vec3 &p);
    void extendTo(const AABox &b);
    void translate(const vec3 &trans);
    void rotate(const vec3 &rot);
    void scale(const vec3 &scale);
    void scaleCenter(float s);
};

#ifdef _WIN32
#undef NEAR
#undef FAR
#endif

class RENDER_DLL Frustum
{
public:
    enum PlaneIndex
    {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        NEAR,
        FAR
    };

    enum TestResult
    {
        INSIDE,
        INTERSECTING,
        OUTSIDE
    };

    Frustum();

    float aspect() const;
    void setAspect(float aspect);

    const vec3 & eye() const;
    void setEye(vec3 eye);

    const vec3 & focus() const;
    void setFocus(vec3 focus);

    const vec3 & up() const;
    void setUp(vec3 up);

    matrix4 projection() const;
    matrix4 camera() const;
    
    const vec3 * corners() const;
    const Plane * planes() const;
    
    void update();

    TestResult containsPoint(vec3 v) const;
    TestResult containsAABox(const AABox &b) const;
    TestResult containsBox(const vec3 *corners) const;

private:
    float m_angle, m_aspect, m_nearPlane, m_farPlane;
    vec3 m_eye, m_focus, m_up;
    Plane m_planes[6];
    vec3 m_corners[8];
    bool m_dirty;
};

class RENDER_DLL VertexData
{
public:
    vec3 position;
    vec3 normal;
    vec3 texCoords;
    uint32_t bone;
    uint32_t padding[2]; // align on 16-bytes boundaries
};

class RENDER_DLL MaterialGroup
{
public:
    uint32_t id;
    uint32_t offset;
    uint32_t count;
    uint32_t matID;
};

struct RENDER_DLL BufferSegment
{
    buffer_t buffer;
    uint32_t elementSize;
    uint32_t offset;
    uint32_t count;
    
    BufferSegment();
    size_t size() const;
    size_t address() const;
};

class RENDER_DLL VertexGroup
{
public:
    enum Primitive
    {
        Triangle,
        Quad
    };
    
    VertexGroup(Primitive mode);
    virtual ~VertexGroup();

    Primitive mode;
    QVector<VertexData> vertices;
    QVector<uint32_t> indices;
    QVector<MaterialGroup> matGroups;
    BufferSegment vertexBuffer;
    BufferSegment indexBuffer;
};

#endif
