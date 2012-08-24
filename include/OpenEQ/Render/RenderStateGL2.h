#ifndef OPENEQ_RENDER_STATE_GL2_H
#define OPENEQ_RENDER_STATE_GL2_H

#include <vector>
#include <string>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/RenderState.h"
#include "OpenEQ/Render/Geometry.h"

class ShaderProgramGL2;
class MaterialMap;

class RENDER_DLL RenderStateGL2 : public RenderState
{
public:
    RenderStateGL2();
    virtual ~RenderStateGL2();

    virtual void init();
    
    virtual void beginDrawMesh(const MeshBuffer *m, MaterialMap *materials,
                               const BoneTransform *bones, int boneCount);
    virtual void drawMesh();
    virtual void drawMeshBatch(const matrix4 *mvMatrices, uint32_t instances);
    virtual void endDrawMesh();
    
    virtual void drawBox(const AABox &box);
    virtual void drawFrustum(const Frustum &frustum);
    ShaderProgramGL2 * program() const;

    virtual SkinningMode skinningMode() const;
    virtual void setSkinningMode(SkinningMode newMode);

    virtual RenderMode renderMode() const;
    virtual void setRenderMode(RenderMode newMode);

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
    virtual matrix4 matrix(RenderState::MatrixMode mode) const;

    // general state operations
    virtual Frustum & viewFrustum();
    virtual void setupViewport(int width, int heigth);
    virtual bool beginFrame();
    virtual void endFrame();

    // material operations
    virtual texture_t loadTexture(QImage img);
    virtual texture_t loadTextures(const QImage *images, size_t count);
    virtual void freeTexture(texture_t tex) ;
    
    virtual buffer_t createBuffer(const void *data, size_t size);

    // Performance measurement

    virtual FrameStat * createStat(QString name, FrameStat::Type type);
    virtual void destroyStat(FrameStat *stat);
    virtual const QVector<FrameStat *> &stats() const;

    enum Shader
    {
        BasicShader = 0,
        SkinningUniformShader = 1,
        SkinningTextureShader = 2
    };

private:
    bool initShader(RenderStateGL2::Shader shader, QString vertexFile,
                    QString fragmentFile);
    Shader shaderFromModes(RenderMode render, SkinningMode skinning) const;
    void setShader(Shader newShader);
    void createCube();

    Frustum m_frustum;
    RenderState::MatrixMode m_matrixMode;
    matrix4 m_matrix[3];
    std::vector<matrix4> m_matrixStack[3];
    ShaderProgramGL2 *m_programs[3];
    RenderMode m_renderMode;
    SkinningMode m_skinningMode;
    Shader m_shader;
    MeshBuffer *m_cube;
    MaterialMap *m_cubeMats;
    QVector<FrameStat *> m_stats;
    int m_gpuTimers;
    FrameStat *m_frameStat;
    FrameStat *m_clearStat;
    FrameStat *m_drawCallsStat;
    FrameStat *m_textureBindsStat;
};

#endif
