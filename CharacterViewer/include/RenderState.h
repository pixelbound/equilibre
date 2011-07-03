#ifndef OPENEQ_RENDER_STATE_H
#define OPENEQ_RENDER_STATE_H

#include "Mesh.h"
#include "Vertex.h"

class QVector3D;
class QQuaternion;
class BoneTransform;
class Material;
class WLDMaterialPalette;

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
    virtual void drawMesh(VertexGroup *m, WLDMaterialPalette *palette,
        const BoneTransform *bones = 0, int boneCount = 0) = 0;

    enum SkinningMode
    {
        SoftwareSkinning = 0,
        HardwareSkinningUniform = 1,
        HardwareSkinningTexture = 2
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

#endif
