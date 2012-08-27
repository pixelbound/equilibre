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

#ifndef EQUILIBRE_RENDER_GEOMETRY_H
#define EQUILIBRE_RENDER_GEOMETRY_H

#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/LinearMath.h"

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
    
    float farPlane() const;
    void setFarPlane(float distance);

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
