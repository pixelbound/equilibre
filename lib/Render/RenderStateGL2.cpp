#include <cstdio>
#include <GL/glew.h>
#include <QImage>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/RenderStateGL2.h"
#include "OpenEQ/Render/ShaderProgramGL2.h"
#include "OpenEQ/Render/Material.h"
#include "OpenEQ/Render/FrameStat.h"

RenderStateGL2::RenderStateGL2() : RenderState()
{
    m_matrixMode = ModelView;
    m_matrix[(int)ModelView].setIdentity();
    m_matrix[(int)Projection].setIdentity();
    m_matrix[(int)Texture].setIdentity();
    m_renderMode = Basic;
    m_skinningMode = SoftwareSkinning;
    m_shader = BasicShader;
    m_programs[(int)BasicShader] = new ShaderProgramGL2(this);
    m_programs[(int)SkinningUniformShader] = new UniformSkinningProgram(this);
    m_programs[(int)SkinningTextureShader] = new TextureSkinningProgram(this);
    m_gpuTimers = 0;
    createCube();
    m_drawCallsStat = createStat("Draw calls", FrameStat::Counter);
    m_textureBindsStat = createStat("Texture binds", FrameStat::Counter);
    m_frameStat = createStat("Frame (ms)", FrameStat::WallTime);
    m_clearStat = createStat("Clear (ms)", FrameStat::WallTime);
}

RenderStateGL2::~RenderStateGL2()
{
    delete m_programs[(int)BasicShader];
    delete m_programs[(int)SkinningUniformShader];
    delete m_programs[(int)SkinningTextureShader];
    delete m_cube;
}

ShaderProgramGL2 * RenderStateGL2::program() const
{
    return m_programs[(int)m_shader];
}

void RenderStateGL2::beginDrawMesh(const MeshBuffer *m, MaterialMap *materials,
                                   const BoneTransform *bones, int boneCount)
{
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->beginDrawMesh(m, materials, bones, boneCount);
}

void RenderStateGL2::drawMesh()
{
    drawMeshBatch(&m_matrix[(int)ModelView], 1);
}

void RenderStateGL2::drawMeshBatch(const matrix4 *mvMatrices, uint32_t instances)
{
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->setProjectionMatrix(m_matrix[(int)Projection]);
    prog->drawMeshBatch(mvMatrices, instances);
}

void RenderStateGL2::endDrawMesh()
{
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->endDrawMesh();
}

void RenderStateGL2::createCube()
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
        {0, 3, 1}, {1, 3, 2}, {2, 3, 6}, {6, 3, 7},
        {0, 4, 3}, {3, 4, 7}, {1, 2, 5}, {5, 2, 6},
        {4, 5, 7}, {7, 5, 6}, {0, 1, 4}, {4, 1, 5}
    };
    
    m_cube = new MeshBuffer();
    m_cube->vertices.resize(36);
    Vertex *v = m_cube->vertices.data();
    for(uint32_t i = 0; i < 12; i++)
    {
        const GLuint *face = faces_indices[i];
        const GLfloat *normal = faces_normals[i/2];
        for(uint32_t j = 0; j < 3; j++, v++)
        {
            const GLfloat *vertex = vertices[face[j]];
            v->position = vec3(vertex[0], vertex[1], vertex[2]);
            v->normal = vec3(normal[0], normal[1], normal[2]);
        }
    }
    MaterialGroup mg;
    mg.count = 36;
    mg.offset = 0;
    mg.id = 0;
    mg.matID = 1;
    m_cube->matGroups.push_back(mg);
    
    m_cubeMats = new MaterialMap();
    Material *mat = new Material();
    mat->setAmbient(vec4(0.1, 0.1, 0.1, 0.4));
    mat->setDiffuse(vec4(0.2, 0.2, 0.2, 0.4));
    //mat->setOpaque(false);
    m_cubeMats->setMaterial(mg.matID, mat);
}

void RenderStateGL2::drawBox(const AABox &box)
{
    vec3 size = box.high - box.low;
    pushMatrix();
    translate(box.low.x, box.low.y, box.low.z);
    scale(size.x, size.y, size.z);
    translate(0.5, 0.5, 0.5);
    beginDrawMesh(m_cube, m_cubeMats, NULL, 0);
    drawMesh();
    endDrawMesh();
    popMatrix();
}

RenderStateGL2::Shader RenderStateGL2::shaderFromModes(RenderState::RenderMode render,
                                                       RenderState::SkinningMode skinning) const
{
    switch(render)
    {
    default:
    case Basic:
        return BasicShader;
    case Skinning:
      switch(skinning)
      {
      default:
      case SoftwareSkinning:
          return BasicShader;
      case HardwareSkinningUniform:
          return SkinningUniformShader;
      case HardwareSkinningTexture:
          return SkinningTextureShader;
      }
    }
}

RenderStateGL2::SkinningMode RenderStateGL2::skinningMode() const
{
    return m_skinningMode;
}

void RenderStateGL2::setSkinningMode(RenderStateGL2::SkinningMode newMode)
{
    m_skinningMode = newMode;
    setShader(shaderFromModes(m_renderMode, newMode));
}

void RenderStateGL2::setMatrixMode(RenderStateGL2::MatrixMode newMode)
{
    m_matrixMode = newMode;
}

RenderState::RenderMode RenderStateGL2::renderMode() const
{
    return m_renderMode;
}

void RenderStateGL2::setRenderMode(RenderState::RenderMode newMode)
{
    m_renderMode = newMode;
    setShader(shaderFromModes(newMode, m_skinningMode));
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

matrix4 RenderStateGL2::matrix(RenderState::MatrixMode mode) const
{
    return m_matrix[(int)mode];
}

static void setTextureParams(GLenum target, bool mipmaps)
{
    if(mipmaps)
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static void defineImage(uint32_t target, int width, int height, uint32_t depth)
{
    const GLenum format = GL_RGBA;
    const GLenum type = GL_UNSIGNED_BYTE;
    switch(target)
    {
    default:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_RECTANGLE:
        glTexImage2D(target, 0, format, width, height, 0, format, type, NULL);
        break;
    case GL_TEXTURE_3D:
    case GL_TEXTURE_2D_ARRAY:
        glTexImage3D(target, 0, format, width, height, depth, 0, format, type, NULL);
        break;
    }
}

static void uploadImage(uint32_t target, QImage img, uint32_t z, uint32_t repeatX, uint32_t repeatY)
{
    const GLenum format = GL_RGBA;
    const GLenum type = GL_UNSIGNED_BYTE;
    int width = img.width(), height = img.height();
    int level = 0;
    uint32_t i = 0, j = 0;
    for(i = 0; i < repeatX; i++)
    {
        for(j = 0; j < repeatY; j++)
        {
            switch(target)
            {
            default:
            case GL_TEXTURE_2D:
            case GL_TEXTURE_RECTANGLE:
                glTexSubImage2D(target, level, i * width, j * height, width, height,
                                format, type, img.bits());
                break;
            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
                glTexSubImage3D(target, level, i * width, j * height, z, width, height, 1,
                                format, type, img.bits());
                break;
            }
        }
    }
}

static uint32_t maxMipmapLevel(uint32_t texWidth, uint32_t texHeight, uint32_t maxRepeat)
{
    int width = texWidth / maxRepeat, height = texHeight / maxRepeat;
    uint32_t level = 0;
    while((width > 0) || (height > 0))
    {
        width >>= 1;
        height >>= 1;
        level++;
    }
    return level;
}

texture_t RenderStateGL2::loadTexture(QImage img)
{
    GLuint target = GL_TEXTURE_2D_ARRAY;
    texture_t texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(target, texID);

    // Allocate the texture array.
    defineImage(target, img.width(), img.height(), 1);
    
    // Copy image data.
    uploadImage(target, img, 0, 1, 1);

    // Set texture parameters.
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmapEXT(GL_TEXTURE_2D_ARRAY);
    glBindTexture(target, 0);
    return texID;
}

texture_t RenderStateGL2::loadTextures(const QImage *images, size_t count)
{
    GLuint target = GL_TEXTURE_2D_ARRAY;
    texture_t texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(target, texID);
    
    // Figure out what's the maximum texture dimensions of the images.
    int maxWidth = 0, maxHeight = 0;
    for(size_t i = 0; i < count; i++)
    {
        maxWidth = qMax(maxWidth, images[i].width());
        maxHeight = qMax(maxHeight, images[i].height());
    }

    // Figure out what's the maximum mipmap level we can use.
    int maxRepeat = 1, maxLevel = 0;
    for(size_t i = 0; i < count; i++)
    {
        QImage img = images[i];
        int repeatX = maxWidth / img.width();
        int repeatY = maxHeight / img.height();
        maxRepeat = qMax(maxRepeat, qMax(repeatX, repeatY));
    }
    maxLevel = maxMipmapLevel(maxWidth, maxHeight, maxRepeat);
    
    // Allocate the texture array.
    defineImage(target, maxWidth, maxHeight, count);
    
    // Copy image data.
    for(size_t i = 0; i < count; i++)
    {
        QImage img = images[i];
        // Repeat textures smaller than the texture array
        // so that we can easily use GL_REPEAT.
        uint32_t repeatX = maxWidth / img.width();
        uint32_t repeatY = maxHeight / img.height();
        uploadImage(target, img, i, repeatX, repeatY);
    }
    
    // Set texture parameters.
    glTexParameterf(target, GL_TEXTURE_MAX_LEVEL, maxLevel);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmapEXT(GL_TEXTURE_2D_ARRAY);
    glBindTexture(target, 0);
    return texID;
}

void RenderStateGL2::freeTexture(texture_t tex)
{
    if(tex != 0)
        glDeleteTextures(1, &tex);
}

bool RenderStateGL2::beginFrame()
{
    m_frameStat->beginTime();
    ShaderProgramGL2 *prog = program();
    bool shaderLoaded = prog && prog->loaded();
    glPushAttrib(GL_ENABLE_BIT);
    if(shaderLoaded)
        glUseProgram(prog->program());
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
    m_clearStat->beginTime();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_clearStat->endTime();
    return shaderLoaded;
}

void RenderStateGL2::endFrame()
{
    // Count draw calls and texture binds made by all programs.
    int totalDrawCalls = 0;
    int totalTextureBinds = 0;
    for(int i = 0; i < 3; i++)
    {
        ShaderProgramGL2 *prog = m_programs[i];
        if(prog)
        {
            totalDrawCalls += prog->drawCalls();
            totalTextureBinds += prog->textureBinds();
            prog->resetFrameStats();
        }
    }
    m_drawCallsStat->setCurrent(totalDrawCalls);
    m_textureBindsStat->setCurrent(totalTextureBinds);
    
    // Reset state.
    setMatrixMode(ModelView);
    popMatrix();
    glUseProgram(0);
    glPopAttrib();

    // Wait for the GPU to finish rendering the scene if we are profiling it.
    if(m_gpuTimers > 0)
        glFinish();

    // Update the frame time and FPS.
    m_frameStat->endTime();

    // Move each frame stat to the next sample.
    foreach(FrameStat *stat, m_stats)
        stat->next();
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
    initShader(BasicShader, "vertex.glsl", "fragment.glsl");
    initShader(SkinningUniformShader, "vertex_skinned_uniform.glsl", "fragment.glsl");
    initShader(SkinningTextureShader, "vertex_skinned_texture.glsl", "fragment.glsl");
}

bool RenderStateGL2::initShader(RenderStateGL2::Shader shader,
                                 QString vertexFile, QString fragmentFile)
{
    ShaderProgramGL2 *prog = m_programs[(int)shader];
    if(!prog->load(vertexFile, fragmentFile))
    {
        delete prog;
        m_programs[(int)shader] = NULL;
        return false;
    }
    return true;
}

void RenderStateGL2::setShader(RenderStateGL2::Shader newShader)
{
    ShaderProgramGL2 *oldProg = program();
    ShaderProgramGL2 *newProg = m_programs[(int)newShader];
    if(oldProg != newProg)
    {
        // Change to the new program only if the program loaded correctly.
        if(newProg && newProg->loaded())
        {
            m_shader = newShader;
            glUseProgram(newProg->program());
        }
    }
}

const QVector<FrameStat *> & RenderStateGL2::stats() const
{
    return m_stats;
}

FrameStat * RenderStateGL2::createStat(QString name, FrameStat::Type type)
{
    FrameStat *stat = new FrameStat(name, 64, type);
    m_stats.append(stat);
    if(stat->type() == FrameStat::GPUTime)
        m_gpuTimers++;
    return stat;
}

void RenderStateGL2::destroyStat(FrameStat *stat)
{
    int pos = m_stats.indexOf(stat);
    if(pos >= 0)
    {
        if(stat->type() == FrameStat::GPUTime)
            m_gpuTimers--;
        m_stats.remove(pos);
        delete stat;
    }
}
