#include <cstdio>
#include "Platform.h"
#include "RenderStateGL2.h"
#include "ShaderProgramGL2.h"
#include "Material.h"

RenderStateGL2::RenderStateGL2() : RenderState()
{
    m_matrixMode = ModelView;
    m_matrix[(int)ModelView].setIdentity();
    m_matrix[(int)Projection].setIdentity();
    m_matrix[(int)Texture].setIdentity();
    m_ambient0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_diffuse0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_specular0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_light0_pos = vec4(0.0, 1.0, 1.0, 0.0);
    m_shaderLoaded = false;
    m_program = new ShaderProgramGL2(this);
    //m_program = new UniformSkinningProgram(this);
    //m_program = new TextureSkinningProgram(this);
    m_skinningMode = SoftwareSkinning;
}

RenderStateGL2::~RenderStateGL2()
{
    delete m_program;
}

void RenderStateGL2::drawMesh(VertexGroup *vg, const BoneTransform *bones, int boneCount)
{
    if(!vg)
        return;
    m_program->setMatrices(m_matrix[(int)ModelView], m_matrix[(int)Projection]);
    m_program->setBoneTransforms(bones, boneCount);
    m_program->drawSkinned(vg);
}

RenderStateGL2::SkinningMode RenderStateGL2::skinningMode() const
{
    return m_skinningMode;
}

void RenderStateGL2::setSkinningMode(RenderStateGL2::SkinningMode newMode)
{
    m_skinningMode = newMode;
}

void RenderStateGL2::setMatrixMode(RenderStateGL2::MatrixMode newMode)
{
    m_matrixMode = newMode;
}

void RenderStateGL2::loadIdentity()
{
    int i = (int)m_matrixMode;
    m_matrix[i].setIdentity();
}

void RenderStateGL2::multiplyMatrix(const matrix4 &m)
{
    int i = (int)m_matrixMode;
    m_matrix[i] = m_matrix[i] * m;
}

void RenderStateGL2::pushMatrix()
{
    int i = (int)m_matrixMode;
    m_matrixStack[i].push_back(m_matrix[i]);
}

void RenderStateGL2::popMatrix()
{
    int i = (int)m_matrixMode;
    m_matrix[i] = m_matrixStack[i].back();
    m_matrixStack[i].pop_back();
}

void RenderStateGL2::translate(float dx, float dy, float dz)
{
    multiplyMatrix(matrix4::translate(dx, dy, dz));
}

void RenderStateGL2::rotate(float angle, float rx, float ry, float rz)
{
    multiplyMatrix(matrix4::rotate(angle, rx, ry, rz));
}

void RenderStateGL2::scale(float sx, float sy, float sz)
{
    multiplyMatrix(matrix4::scale(sx, sy, sz));
}

matrix4 RenderStateGL2::currentMatrix() const
{
    return m_matrix[(int)m_matrixMode];
}

void RenderStateGL2::pushMaterial(const Material &m)
{
    m_materialStack.push_back(m);
    m_program->beginApplyMaterial(m);
}

void RenderStateGL2::popMaterial()
{
    Material m = m_materialStack.back();
    m_materialStack.pop_back();
    m_program->endApplyMaterial(m);
    if(m_materialStack.size() > 0)
        m_program->beginApplyMaterial(m_materialStack.back());
}

bool RenderStateGL2::beginFrame(int w, int h)
{
    glPushAttrib(GL_ENABLE_BIT);
    if(m_shaderLoaded)
        m_program->beginFrame();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setupViewport(w, h);
    setMatrixMode(ModelView);
    pushMatrix();
    loadIdentity();
    if(m_shaderLoaded)
        glClearColor(m_bgColor.x, m_bgColor.y, m_bgColor.z, m_bgColor.w);
    else
        glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return m_shaderLoaded;
}

void RenderStateGL2::endFrame()
{
    setMatrixMode(ModelView);
    popMatrix();
    if(m_shaderLoaded)
        m_program->endFrame();
    glPopAttrib();
    glFlush();
}

void RenderStateGL2::setupViewport(int w, int h)
{
    glViewport(0, 0, w, h);
    setMatrixMode(Projection);
    loadIdentity();
    float r = (float)w / (float)h;
    if(m_projection)
        multiplyMatrix(matrix4::perspective(45.0f, r, 0.1f, 100.0f));
    else if (w <= h)
        multiplyMatrix(matrix4::ortho(-1.0, 1.0, -1.0 / r, 1.0 / r, -10.0, 10.0));
    else
        multiplyMatrix(matrix4::ortho(-1.0 * r, 1.0 * r, -1.0, 1.0, -10.0, 10.0));
    setMatrixMode(ModelView);
}

void RenderStateGL2::init()
{
    m_shaderLoaded = loadShaders();
}

bool RenderStateGL2::loadShaders()
{
    return m_program->load("vertex.glsl", "fragment.glsl");
    //return m_program->load("vertex_skinned_uniform.glsl", "fragment.glsl");
    //return m_program->load("vertex_skinned_texture.glsl", "fragment.glsl");
}
