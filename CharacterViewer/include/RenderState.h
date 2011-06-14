#ifndef OPENEQ_RENDER_STATE_H
#define OPENEQ_RENDER_STATE_H

#include <map>
#include <string>
#include "Mesh.h"
#include "Material.h"
#include "Vertex.h"

using namespace std;

class QVector3D;
class QQuaternion;
class BoneTransform;

class RenderState
{
public:
    RenderState();
    virtual ~RenderState();

    virtual void init();

    virtual void toggleWireframe();
    virtual void toggleProjection();

    virtual void reset();

    // mesh operations
    virtual void drawMesh(Mesh *m, const BoneTransform *bones = 0, int boneCount = 0) = 0;
    virtual Mesh * createMesh() = 0;

    virtual void setBoneTransforms(const BoneTransform *transforms, int count) = 0;

    enum SkinningMode
    {
        SoftwareSingleQuaternion = 0,
        HardwareSingleQuaternion = 1,
        HardwareDualQuaternion = 2
    };

    virtual SkinningMode skinningMode() const = 0;
    virtual void setSkinningMode(SkinningMode newMode) = 0;

    // matrix operations

    enum MatrixMode
    {
        ModelView,
        Projection,
        Texture
    };

    virtual void setMatrixMode(MatrixMode newMode) = 0;

    virtual void loadIdentity() = 0;
    virtual void multiplyMatrix(const matrix4 &m) = 0;
    virtual void pushMatrix() = 0;
    virtual void popMatrix() = 0;

    void translate(const QVector3D &v);
    void translate(const vec3 &v);
    void rotate(const QQuaternion &q);
    void scale(const vec3 &v);
    virtual void translate(float dx, float dy, float dz) = 0;
    virtual void rotate(float angle, float rx, float ry, float rz) = 0;
    virtual void scale(float sx, float sy, float sz) = 0;

    virtual matrix4 currentMatrix() const = 0;

    // general state operations
    virtual bool beginFrame(int width, int heigth) = 0;
    virtual void setupViewport(int width, int heigth) = 0;
    virtual void endFrame() = 0;

    // material operations

    virtual void pushMaterial(const Material &m) = 0;
    virtual void popMaterial() = 0;

protected:
    bool m_projection;
    bool m_wireframe;
    vec4 m_bgColor;
};

class StateObject
{
public:
    StateObject(RenderState *s);

    void loadIdentity();
    void pushMatrix();
    void popMatrix();

    void translate(float dx, float dy, float dz);
    void rotate(float angle, float rx, float ry, float rz);
    void scale(float sx, float sy, float sz);

    void pushMaterial(const Material &m);
    void popMaterial();

protected:
    RenderState *m_state;
};

#endif
