#include <cmath>
#include "OpenEQ/Render/Geometry.h"

using namespace std;

Plane::Plane()
{
    m_dot_minus_n_p = 0.0f;
}

Plane::Plane(vec3 point, vec3 normal)
{
    m_p = point;
    m_n = normal;
    m_dot_minus_n_p = vec3::dot(-m_n, m_p);
}

const vec3 & Plane::p() const
{
    return m_p;
}

const vec3 & Plane::n() const
{
    return m_n;
}

float Plane::distance(vec3 v) const
{
    return vec3::dot(m_n, v) + m_dot_minus_n_p;
}

////////////////////////////////////////////////////////////////////////////////

AABox::AABox()
{
}

AABox::AABox(const vec3 &low, const vec3 &high)
{
    this->low = low;
    this->high = high;
}

vec3 AABox::center() const
{
    return low + (high - low) * 0.5;
}

vec3 AABox::posVertex(const vec3 &normal) const
{
    vec3 res = low;
    if(normal.x > 0)
        res.x = high.x;
    if(normal.y > 0)
        res.y = high.y;
    if(normal.z > 0)
        res.z = high.z;
    return res;
}

vec3 AABox::negVertex(const vec3 &normal) const
{
    vec3 res = low;
    if(normal.x < 0)
        res.x = high.x;
    if(normal.y < 0)
        res.y = high.y;
    if(normal.z < 0)
        res.z = high.z;
    return res;
}

bool AABox::contains(const AABox &b) const
{
    return contains(b.low) && contains(b.high);
}

bool AABox::contains(const vec3 &p) const
{
    return (low.x <= p.x) && (p.x <= high.x)
        && (low.y <= p.y) && (p.y <= high.y)
        && (low.z <= p.z) && (p.z <= high.z);
}

void AABox::cornersTo(vec3 *corners) const
{
    vec3 size = high - low;
    corners[0] = low + vec3(0.0, 0.0, 0.0);
    corners[1] = low + vec3(size.x, 0.0, 0.0);
    corners[2] = low + vec3(size.x, size.y, 0.0);
    corners[3] = low + vec3(0.0, size.y, 0.0);
    corners[4] = low + vec3(0.0, 0.0, size.z);
    corners[5] = low + vec3(size.x, 0.0, size.z);
    corners[6] = low + vec3(size.x, size.y, size.z);
    corners[7] = low + vec3(0.0, size.y, size.z);
}

void AABox::extendTo(const vec3 &p)
{
    low.x = min(low.x, p.x);
    low.y = min(low.y, p.y);
    low.z = min(low.z, p.z);
    high.x = max(high.x, p.x);
    high.y = max(high.y, p.y);
    high.z = max(high.z, p.z);
}

void AABox::extendTo(const AABox &b)
{
    extendTo(b.low);
    extendTo(b.high);
}

static vec3 rotate_by_quat(vec3 v, QQuaternion q)
{
    QVector3D res = q.rotatedVector(QVector3D(v.x, v.y, v.z));
    return vec3(res.x(), res.y(), res.z());
}

void AABox::translate(const vec3 &trans)
{
    low = low + trans;
    high = high + trans;
}

void AABox::rotate(const vec3 &rot)
{
    QQuaternion qRot = QQuaternion::fromAxisAndAngle(1.0, 0.0, 0.0, rot.x)
        * QQuaternion::fromAxisAndAngle(0.0, 1.0, 0.0, rot.y)
        * QQuaternion::fromAxisAndAngle(0.0, 0.0, 1.0, rot.z);
    vec3 corners[8];
    cornersTo(corners);
    low = high = rotate_by_quat(corners[0], qRot);
    for(uint32_t i = 1; i < 8; i++)
        extendTo(rotate_by_quat(corners[i], qRot));
}

void AABox::scale(const vec3 &scale)
{
    low.x = low.x * scale.x;
    low.y = low.y * scale.y;
    low.z = low.z * scale.z;
    high.x = high.x * scale.x;
    high.y = high.y * scale.y;
    high.z = high.z * scale.z;
}

void AABox::scaleCenter(float s)
{
    low = ((low + high) + (low - high) * s) * 0.5f;
    high = ((low + high) + (high - low) * s) * 0.5f;
}

////////////////////////////////////////////////////////////////////////////////

Frustum::Frustum()
{
    m_angle = 45.0;
    m_aspect = 1.0;
    m_nearPlane = 0.1;
    m_farPlane = 5000.0;
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

const vec3 * Frustum::corners() const
{
    return m_corners;
}

const Plane * Frustum::planes() const
{
    return m_planes;
}

void Frustum::update()
{
    if(!m_dirty)
        return;
    float ratio = (float)tan(m_angle * 0.5 * M_PI / 180.0);
    float nearHeight = m_nearPlane * ratio;
    float nearWidth = nearHeight * m_aspect;
    float farHeight = m_farPlane * ratio;
    float farWidth = farHeight * m_aspect;

    // compute the three axes
    vec3 zAxis = (m_eye - m_focus).normalized();
    vec3 xAxis = vec3::cross(m_up, zAxis).normalized();
    vec3 yAxis = vec3::cross(zAxis, xAxis);

    // compute the centers of the near and far planes
    vec3 nearCenter = m_eye - zAxis * m_nearPlane;
    vec3 farCenter = m_eye - zAxis * m_farPlane;
    
    // compute the corners of the frustum
    vec3 halfNearX = xAxis * nearWidth;
    vec3 halfNearY = yAxis * nearHeight;
    m_corners[0] = nearCenter - halfNearX - halfNearY;
    m_corners[1] = nearCenter + halfNearX - halfNearY;
    m_corners[2] = nearCenter + halfNearX + halfNearY;
    m_corners[3] = nearCenter - halfNearX + halfNearY;
    
    vec3 halfFarX = xAxis * farWidth;
    vec3 halfFarY = yAxis * farHeight;
    m_corners[4] = farCenter - halfFarX - halfFarY;
    m_corners[5] = farCenter + halfFarX - halfFarY;
    m_corners[6] = farCenter + halfFarX + halfFarY;
    m_corners[7] = farCenter - halfFarX + halfFarY;

    // compute the plane "positions" and normals
    vec3 topN = vec3::cross((nearCenter + halfNearY - m_eye).normalized(), xAxis);
    vec3 bottomN = vec3::cross(xAxis, (nearCenter - halfNearY - m_eye).normalized());
    vec3 leftN = vec3::cross((nearCenter - halfNearX - m_eye).normalized(), yAxis);
    vec3 rightN = vec3::cross(yAxis, (nearCenter + halfNearX - m_eye).normalized());
    m_planes[NEAR] = Plane(nearCenter, -zAxis);
    m_planes[FAR] = Plane(farCenter, zAxis);
    m_planes[TOP] = Plane(nearCenter + halfNearY, topN);
    m_planes[BOTTOM] = Plane(nearCenter - halfNearY, bottomN);
    m_planes[LEFT] = Plane(nearCenter - halfNearX, leftN);
    m_planes[RIGHT] = Plane(nearCenter + halfNearX, rightN);
    
    // the planes and corners are up-to-date now
    m_dirty = false;
}

Frustum::TestResult Frustum::containsPoint(vec3 v) const
{
    for(int i = 0; i < 6; i++)
    {
        if(m_planes[i].distance(v) < 0)
            return OUTSIDE;
    }
    return INSIDE;
}

Frustum::TestResult Frustum::containsBox(const vec3 *corners) const
{
    int allInCount = 0;
    for(int i = 0; i < 6; i++)
    {
        // Count how many points are on the wrong side of the plane ('out').
        const Plane &plane = m_planes[i];
        int outCount = 0;
        for(int j = 0; j < 8; j++)
        {
            if(plane.distance(corners[j]) < 0.0)
            {
                outCount++;
            }
        }
        
        // When all points are outside one plane, the box is outside the frustum.
        if(outCount == 8)
        {
            return OUTSIDE;
        }
        allInCount += (outCount == 0);
    }
    return (allInCount == 6) ? INSIDE : INTERSECTING;
}

Frustum::TestResult Frustum::containsAABox(const AABox &b) const
{
    TestResult result = INSIDE;
    for(int i = 0; i < 6; i++)
    {
        const Plane &plane = m_planes[i];
        // is the positive vertex outside?
        if(plane.distance(b.posVertex(plane.n())) < 0)
            return OUTSIDE;
        // is the negative vertex outside?
        else if(plane.distance(b.negVertex(plane.n())) < 0)
            result = INTERSECTING;
    }
    return result;
}
