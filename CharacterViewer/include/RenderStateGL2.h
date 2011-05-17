#ifndef OPENEQ_RENDER_STATE_GL2_H
#define OPENEQ_RENDER_STATE_GL2_H

#include <vector>
#include <string>
#include <inttypes.h>
#include "RenderState.h"

class RenderStateGL2 : public RenderState
{
public:
    RenderStateGL2();
    virtual ~RenderStateGL2();

    virtual void init();

    virtual Mesh * createMesh();
    virtual void drawMesh(Mesh *m);

    virtual void setBoneTransforms(const BoneTransform *transforms, int count);
    virtual void clearBoneTransforms();

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
    virtual void beginFrame(int width, int heigth);
    virtual void setupViewport(int width, int heigth);
    virtual void endFrame();

    // material operations
    virtual void pushMaterial(const Material &m);
    virtual void popMaterial();

    int positionAttr() const;
    int normalAttr() const;
    int texCoordsAttr() const;
    int boneAttr() const;

private:
    void beginApplyMaterial(const Material &m);
    void endApplyMaterial(const Material &m);
    char * loadShaderSource(string path) const;
    uint32_t loadShader(string path, uint32_t type) const;
    bool loadShaders();
    void initShaders();
    void setUniformValue(QString name, const vec4 &v);
    void setUniformValue(QString name, float f);
    void setUniformValue(QString name, int i);

    vec4 m_ambient0;
    vec4 m_diffuse0;
    vec4 m_specular0;
    vec4 m_light0_pos;
    std::vector<Material> m_materialStack;
    RenderState::MatrixMode m_matrixMode;
    matrix4 m_matrix[3];
    std::vector<matrix4> m_matrixStack[3];
    uint32_t m_vertexShader;
    uint32_t m_pixelShader;
    uint32_t m_program;
    int m_modelViewMatrixLoc;
    int m_projMatrixLoc;
    int m_positionAttr;
    int m_normalAttr;
    int m_texCoordsAttr;
    int m_boneAttr;
};

#endif
