#ifndef OPENEQ_SHADER_PROGRAM_GL2_H
#define OPENEQ_SHADER_PROGRAM_GL2_H

#include <vector>
#include <inttypes.h>
#include <QString>
#include "Vertex.h"

class RenderStateGL2;
class BoneTransform;
class Material;
class MaterialMap;

class MeshDataGL2
{
public:
    void clear();
    
    const VertexGroup *vg;
    const BoneTransform *bones;
    uint32_t boneCount;
    MaterialMap *materials;
    const uint32_t *indices;
    bool haveIndices;
    bool pending;
};

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
    
    virtual void beginDrawMesh(const VertexGroup *m, MaterialMap *materials,
                               const BoneTransform *bones, int boneCount);
    virtual void drawMesh();
    virtual void endDrawMesh();

    void setMatrices(const matrix4 &modelView, const matrix4 &projection);
    void setBoneTransforms(const BoneTransform *transforms, int count);

    void beginApplyMaterial(const Material &m);
    void endApplyMaterial(const Material &m);

    void enableVertexAttributes();
    void uploadVertexAttributes(const VertexGroup *vg);
    void disableVertexAttributes();

protected:
    bool compileProgram(QString vertexFile, QString fragmentFile);
    static uint32_t loadShader(QString path, uint32_t type);

    RenderStateGL2 *m_state;
    uint32_t m_vertexShader;
    uint32_t m_fragmentShader;
    uint32_t m_program;
    int m_attr[4];
    int m_uniform[6];
    vec4 *m_bones;
    MeshDataGL2 m_meshData;
};

class UniformSkinningProgram : public ShaderProgramGL2
{
public:
    UniformSkinningProgram(RenderStateGL2 *state);
    virtual bool init();
    //virtual void drawSkinned(const VertexGroup *m, WLDMaterialPalette *palette);

private:
    int m_bonesLoc;
};

class TextureSkinningProgram : public ShaderProgramGL2
{
public:
    TextureSkinningProgram(RenderStateGL2 *state);
    virtual ~TextureSkinningProgram();
    virtual bool init();
    //virtual void drawSkinned(const VertexGroup *m, WLDMaterialPalette *palette);

private:
    int m_bonesLoc;
    uint32_t m_boneTexture;
};

#endif
