#include <cstdio>
#include <GL/glew.h>
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
    m_programs[(int)SoftwareSkinning] = new ShaderProgramGL2(this);
    m_programs[(int)HardwareSkinningUniform] = new UniformSkinningProgram(this);
    m_programs[(int)HardwareSkinningTexture] = new TextureSkinningProgram(this);
    m_skinningMode = SoftwareSkinning;
    m_cube = createCube();
}

RenderStateGL2::~RenderStateGL2()
{
    delete m_programs[(int)SoftwareSkinning];
    delete m_programs[(int)HardwareSkinningUniform];
    delete m_programs[(int)HardwareSkinningTexture];
    delete m_cube;
}

ShaderProgramGL2 * RenderStateGL2::program() const
{
    if((m_skinningMode >= SoftwareSkinning) && (m_skinningMode <= HardwareSkinningTexture))
        return m_programs[(int)m_skinningMode];
    else
        return 0;
}

void RenderStateGL2::drawMesh(const VertexGroup *vg, WLDMaterialPalette *palette,
    const BoneTransform *bones, int boneCount)
{
    ShaderProgramGL2 *prog = program();
    if(!vg || !prog || !prog->loaded())
        return;
    prog->setMatrices(m_matrix[(int)ModelView], m_matrix[(int)Projection]);
    if(bones && boneCount > 0)
    {
        prog->setBoneTransforms(bones, boneCount);
        prog->drawSkinned(vg, palette);
    }
    else
    {
        prog->draw(vg, palette);
    }
}

VertexGroup * RenderStateGL2::createCube()
{
    static const GLfloat vertices[][3] =
    {
        {-0.5, -0.5,  0.5}, {-0.5,  0.5,  0.5},
        { 0.5,  0.5,  0.5}, { 0.5, -0.5,  0.5},
        {-0.5, -0.5, -0.5}, {-0.5,  0.5, -0.5},
        { 0.5,  0.5, -0.5}, { 0.5, -0.5, -0.5}
    };

    static const GLfloat faces_normals[][3] =
    {
        { 0.0,  0.0,  1.0}, { 1.0,  0.0,  0.0},
        { 0.0, -1.0,  0.0}, { 0.0,  1.0,  0.0},
        { 0.0,  0.0, -1.0}, {-1.0,  0.0,  0.0},
    };

    static const GLuint faces_indices[][4] =
    {
        {0, 3, 2, 1}, {2, 3, 7, 6},
        {0, 4, 7, 3}, {1, 2, 6, 5},
        {4, 5, 6, 7}, {0, 1, 5, 4}
    };
    
    VertexGroup *vg = new VertexGroup(VertexGroup::Quad);
    vg->vertices.resize(24);
    VertexData *vd = vg->vertices.data();
    for(uint32_t i = 0; i < 6; i++)
    {
        const GLuint *face = faces_indices[i];
        const GLfloat *normal = faces_normals[i];
        for(uint32_t j = 0; j < 4; j++, vd++)
        {
            const GLfloat *vertex = vertices[face[j]];
            vd->position = vec3(vertex[0], vertex[1], vertex[2]);
            vd->normal = vec3(normal[0], normal[1], normal[2]);
        }
    }
    MaterialGroup mg;
    mg.count = 24;
    mg.offset = 0;
    mg.id = 0;
    mg.matName = "cube";
    vg->matGroups.push_back(mg);
    return vg;
}

void RenderStateGL2::drawBox(const AABox &box)
{
    vec3 size = box.high - box.low;
    pushMatrix();
    translate(box.low.x, box.low.y, box.low.z);
    scale(size.x, size.y, size.z);
    translate(0.5, 0.5, 0.5);
    
    Material mat;
    mat.setAmbient(vec4(0.1, 0.1, 0.1, 0.4));
    mat.setDiffuse(vec4(0.2, 0.2, 0.2, 0.4));
    mat.setOpaque(false);
    program()->beginApplyMaterial(mat);
    drawMesh(m_cube, NULL, NULL, 0);
    program()->endApplyMaterial(mat);
    
    popMatrix();
}

RenderStateGL2::SkinningMode RenderStateGL2::skinningMode() const
{
    return m_skinningMode;
}

void RenderStateGL2::setSkinningMode(RenderStateGL2::SkinningMode newMode)
{
    ShaderProgramGL2 *newProg = m_programs[(int)newMode];
    if((m_skinningMode != newMode) && newProg->loaded())
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
    ShaderProgramGL2 *prog = program();
    if(prog && prog->loaded())
        prog->beginApplyMaterial(m);
}

void RenderStateGL2::popMaterial()
{
    Material m = m_materialStack.back();
    m_materialStack.pop_back();
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->endApplyMaterial(m);
    if(m_materialStack.size() > 0)
        prog->beginApplyMaterial(m_materialStack.back());
}

bool RenderStateGL2::beginFrame()
{
    ShaderProgramGL2 *prog = program();
    bool shaderLoaded = prog && prog->loaded();
    glPushAttrib(GL_ENABLE_BIT);
    if(shaderLoaded)
        prog->beginFrame();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setMatrixMode(ModelView);
    pushMatrix();
    loadIdentity();
    if(shaderLoaded)
        glClearColor(m_bgColor.x, m_bgColor.y, m_bgColor.z, m_bgColor.w);
    else
        glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return shaderLoaded;
}

void RenderStateGL2::endFrame()
{
    setMatrixMode(ModelView);
    popMatrix();ShaderProgramGL2 *prog = program();
    if(prog && prog->loaded())
        prog->endFrame();
    glPopAttrib();
    glFlush();
}

Frustum & RenderStateGL2::viewFrustum()
{
    return m_frustum;
}

void RenderStateGL2::setupViewport(int w, int h)
{
    float r = (float)w / (float)h;
    m_frustum.setAspect(r);
    glViewport(0, 0, w, h);
    setMatrixMode(Projection);
    loadIdentity();
    multiplyMatrix(m_frustum.projection());
    setMatrixMode(ModelView);
}

buffer_t RenderStateGL2::createBuffer(const void *data, size_t size)
{
    buffer_t buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

void RenderStateGL2::init()
{
    m_programs[(int)SoftwareSkinning]->load("vertex.glsl", "fragment.glsl");
    m_programs[(int)HardwareSkinningUniform]->load("vertex_skinned_uniform.glsl", "fragment.glsl");
    m_programs[(int)HardwareSkinningTexture]->load("vertex_skinned_texture.glsl", "fragment.glsl");
}
