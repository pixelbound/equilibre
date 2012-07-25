#include <cstdio>
#include <GL/glew.h>
#include <QImage>
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
    m_renderMode = Basic;
    m_skinningMode = SoftwareSkinning;
    m_shader = BasicShader;
    m_programs[(int)BasicShader] = new ShaderProgramGL2(this);
    m_programs[(int)SkinningUniformShader] = new UniformSkinningProgram(this);
    m_programs[(int)SkinningTextureShader] = new TextureSkinningProgram(this);
    m_programs[(int)InstancedShader] = new InstancingProgram(this);
    createCube();
}

RenderStateGL2::~RenderStateGL2()
{
    delete m_programs[(int)BasicShader];
    delete m_programs[(int)SkinningUniformShader];
    delete m_programs[(int)SkinningTextureShader];
    delete m_programs[(int)InstancedShader];
    delete m_cube;
}

ShaderProgramGL2 * RenderStateGL2::program() const
{
    return m_programs[(int)m_shader];
}

void RenderStateGL2::beginDrawMesh(const VertexGroup *m, MaterialMap *materials,
                                   const BoneTransform *bones, int boneCount)
{
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->beginDrawMesh(m, materials, bones, boneCount);
}

void RenderStateGL2::drawMesh()
{
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->setModelViewMatrix(m_matrix[(int)ModelView]);
    prog->setProjectionMatrix(m_matrix[(int)Projection]);
    prog->drawMesh();
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
        {0, 3, 2, 1}, {2, 3, 7, 6},
        {0, 4, 7, 3}, {1, 2, 6, 5},
        {4, 5, 6, 7}, {0, 1, 5, 4}
    };
    
    m_cube = new VertexGroup(VertexGroup::Quad);
    m_cube->vertices.resize(24);
    VertexData *vd = m_cube->vertices.data();
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
    m_cube->matGroups.push_back(mg);
    
    m_cubeMats = new MaterialMap();
    Material *mat = new Material();
    mat->setAmbient(vec4(0.1, 0.1, 0.1, 0.4));
    mat->setDiffuse(vec4(0.2, 0.2, 0.2, 0.4));
    mat->setOpaque(false);
    m_cubeMats->setMaterial("cube", mat);
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
    case Instanced:
        return InstancedShader;
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
    int level = 0;
    while((width > 0) || (height > 0))
    {
        // The last mipmap level is 1x1, even when original width and height are different.
        width = qMax(width, 1);
        height = qMax(height, 1);
        switch(target)
        {
        default:
        case GL_TEXTURE_2D:
        case GL_TEXTURE_RECTANGLE:
            glTexImage2D(target, level, format, width, height, 0, format, type, NULL);
            break;
        case GL_TEXTURE_3D:
        case GL_TEXTURE_2D_ARRAY:
            glTexImage3D(target, level, format, width, height, depth, 0, format, type, NULL);
            break;
        }
        width >>= 1;
        height >>= 1;
        level++;
    }
}

static void uploadImage(uint32_t target, QImage img, uint32_t z, uint32_t repeatX, uint32_t repeatY)
{
    const GLenum format = GL_RGBA;
    const GLenum type = GL_UNSIGNED_BYTE;
    QImage levelImg = img;
    int width = img.width(), height = img.height();
    int level = 0;
    uint32_t i = 0, j = 0;
    while((width > 0) || (height > 0))
    {
        // The last mipmap level is 1x1, even when original width and height are different.
        width = qMax(width, 1);
        height = qMax(height, 1);
        if(level > 0)
        {
            // create mipmap image
            levelImg = img.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
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
                                    format, type, levelImg.bits());    
                    break;
                case GL_TEXTURE_3D:
                case GL_TEXTURE_2D_ARRAY:
                    glTexSubImage3D(target, level, i * width, j * height, z, width, height, 1,
                                    format, type, levelImg.bits());
                    break;
                }
            }
        }
        
        width >>= 1;
        height >>= 1;
        level++;
    }
}

texture_t RenderStateGL2::loadTexture(QImage img)
{
    GLuint target = GL_TEXTURE_2D_ARRAY;
    texture_t texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(target, texID);

    // Allocate the texture array and all its mipmap levels.
    defineImage(target, img.width(), img.height(), 1);
    
    // Copy image data.
    uploadImage(target, img, 0, 1, 1);

    // Set texture parameters.
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
    
    // Allocate the texture array and all its mipmap levels.
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
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
    popMatrix();
    ShaderProgramGL2 *prog = program();
    if(prog && prog->current())
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
    initShader(BasicShader, "vertex.glsl", "fragment.glsl");
    initShader(SkinningUniformShader, "vertex_skinned_uniform.glsl", "fragment.glsl");
    initShader(SkinningTextureShader, "vertex_skinned_texture.glsl", "fragment.glsl");
    initShader(InstancedShader, "vertex_instanced.glsl", "fragment.glsl");
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

            // Disable the old program.
            if(oldProg->current())
                oldProg->endFrame();

            // Make sure the new program is current.
            if(!newProg->current())
                newProg->beginFrame();
        }
    }
}
