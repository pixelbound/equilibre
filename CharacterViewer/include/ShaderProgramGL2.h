#ifndef OPENEQ_SHADER_PROGRAM_GL2_H
#define OPENEQ_SHADER_PROGRAM_GL2_H

#include <vector>
#include <inttypes.h>
#include <QString>
#include "Vertex.h"

class RenderStateGL2;
class BoneTransform;

class ShaderProgramGL2
{
public:
    ShaderProgramGL2(RenderStateGL2 *state);
    virtual ~ShaderProgramGL2();

    bool loaded() const;

    bool load(QString vertexFile, QString fragmentFile);

    virtual bool init();
    virtual void beginFrame();
    virtual void endFrame();

    virtual void drawSkinned(VertexGroup *m);

    void setMatrices(const matrix4 &modelView, const matrix4 &projection);
    void setBoneTransforms(const BoneTransform *transforms, int count);

    void beginApplyMaterial(const Material &m);
    void endApplyMaterial(const Material &m);

    void enableVertexAttributes();
    void uploadVertexAttributes(VertexGroup *vg);
    void disableVertexAttributes();

protected:
    bool compileProgram(QString vertexFile, QString fragmentFile);
    static uint32_t loadShader(QString path, uint32_t type);
    void drawArray(VertexGroup *vg);

    RenderStateGL2 *m_state;
    uint32_t m_vertexShader;
    uint32_t m_fragmentShader;
    uint32_t m_program;
    int m_attr[4];
    int m_uniform[6];
    vec4 *m_bones;
};

class UniformSkinningProgram : public ShaderProgramGL2
{
public:
    UniformSkinningProgram(RenderStateGL2 *state);
    virtual bool init();
    virtual void drawSkinned(VertexGroup *m);

private:
    int m_bonesLoc;
};

class TextureSkinningProgram : public ShaderProgramGL2
{
public:
    TextureSkinningProgram(RenderStateGL2 *state);
    virtual ~TextureSkinningProgram();
    virtual bool init();
    virtual void drawSkinned(VertexGroup *m);

private:
    int m_bonesLoc;
    uint32_t m_boneTexture;
};

#endif
