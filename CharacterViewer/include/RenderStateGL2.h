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

    ShaderProgramGL2 *program() const;

    virtual Mesh * createMesh();
    virtual void drawMesh(Mesh *m, const BoneTransform *bones, int boneCount);

    virtual void startSkinning();
    virtual void stopSkinning();
    virtual void setBoneTransforms(const BoneTransform *transforms, int count);

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
    virtual bool beginFrame(int width, int heigth);
    virtual void setupViewport(int width, int heigth);
    virtual void endFrame();

    // material operations
    virtual void pushMaterial(const Material &m);
    virtual void popMaterial();

private:
    void beginApplyMaterial(const Material &m);
    void endApplyMaterial(const Material &m);
    bool loadShaders();
    void uploadBoneTransformsUniform();
    void uploadBoneTransformsTexture();

    vec4 m_ambient0;
    vec4 m_diffuse0;
    vec4 m_specular0;
    vec4 m_light0_pos;
    std::vector<Material> m_materialStack;
    RenderState::MatrixMode m_matrixMode;
    matrix4 m_matrix[3];
    std::vector<matrix4> m_matrixStack[3];
    bool m_shaderLoaded;
    ShaderProgramGL2 *m_program;
    SkinningMode m_skinningMode;
    uint32_t m_boneTexture;
    vec4 *m_bones;
};

class ShaderProgramGL2
{
public:
    ShaderProgramGL2();
    virtual ~ShaderProgramGL2();

    bool loaded() const;

    bool load(QString vertexFile, QString fragmentFile);

    void beginFrame();
    void endFrame();

    void setUniformValue(QString name, const vec4 &v);
    void setUniformValue(QString name, float f);
    void setUniformValue(QString name, int i);

    void setMatrices(const matrix4 &modelView, const matrix4 &projection);

    void enableVertexAttributes();
    void uploadVertexAttributes(VertexGroup *vg);
    void disableVertexAttributes();

private:
    bool compileProgram(QString vertexFile, QString fragmentFile);
    uint32_t loadShader(QString path, uint32_t type) const;

    uint32_t m_vertexShader;
    uint32_t m_fragmentShader;
    uint32_t m_program;
    int m_modelViewMatrixLoc;
    int m_projMatrixLoc;
    int m_positionAttr;
    int m_normalAttr;
    int m_texCoordsAttr;
    int m_boneAttr;
};

#endif
