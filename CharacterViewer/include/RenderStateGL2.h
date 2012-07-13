#ifndef OPENEQ_RENDER_STATE_GL2_H
#define OPENEQ_RENDER_STATE_GL2_H

#include <vector>
#include <string>
#include <inttypes.h>
#include "RenderState.h"

class ShaderProgramGL2;

class RenderStateGL2 : public RenderState
{
public:
    RenderStateGL2();
    virtual ~RenderStateGL2();

    virtual void init();

    virtual void drawMesh(VertexGroup *m, WLDMaterialPalette *palette,
        const BoneTransform *bones, int boneCount);
    virtual void drawBox(const AABox &box);
    ShaderProgramGL2 * program() const;

    virtual SkinningMode skinningMode() const;
    virtual void setSkinningMode(SkinningMode newMode);

    // matrix operations
    virtual void setMatrixMode(MatrixMode newMode);

    virtual void loadIdentity();
    virtual void multiplyMatrix(const matrix4 &m);
    virtual void pushMatrix();
    virtual void popMatrix();

    virtual void translate(float dx, float dy, float dz);
    virtual void rotate(float angle, float rx, float ry, float rz);
    virtual void scale(float sx, float sy, float sz);

    virtual matrix4 currentMatrix() const;

    // general state operations
    virtual Frustum & viewFrustum();
    virtual void setupViewport(int width, int heigth);
    virtual bool beginFrame();
    virtual void endFrame();

    // material operations
    virtual void pushMaterial(const Material &m);
    virtual void popMaterial();
    
    virtual buffer_t createBuffer(const void *data, size_t size);

private:
    bool loadShaders();
    static VertexGroup * createCube();

    vec4 m_ambient0;
    vec4 m_diffuse0;
    vec4 m_specular0;
    vec4 m_light0_pos;
    Frustum m_frustum;
    std::vector<Material> m_materialStack;
    RenderState::MatrixMode m_matrixMode;
    matrix4 m_matrix[3];
    std::vector<matrix4> m_matrixStack[3];
    ShaderProgramGL2 *m_programs[3];
    SkinningMode m_skinningMode;
    VertexGroup *m_cube;
};

#endif
