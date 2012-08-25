#ifndef OPENEQ_RENDER_GEOMETRY_H
#define OPENEQ_RENDER_GEOMETRY_H

#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/LinearMath.h"

struct RENDER_DLL Plane
{
    Plane();
    Plane(vec3 point, vec3 normal);
    const vec3 & p() const;
    const vec3 & n() const;
    float distance(vec3 v) const;
    
private:
    vec3 m_p; // point on the plane
    vec3 m_n; // normal of the plane
    float m_dot_minus_n_p; // dot(-n, p)
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

#endif
