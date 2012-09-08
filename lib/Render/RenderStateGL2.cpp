// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <cstdio>
#include <GL/glew.h>
#include <QImage>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/RenderStateGL2.h"
#include "EQuilibre/Render/ShaderProgramGL2.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/FrameStat.h"

struct RenderStateGL2Data
{
    RenderStateGL2Data();
    
    Frustum frustum;
    vec4 clearColor;
    vec4 ambientLightColor;
    RenderState::MatrixMode matrixMode;
    matrix4 matrix[3];
    std::vector<matrix4> matrixStack[3];
    ShaderProgramGL2 *programs[3];
    RenderState::RenderMode renderMode;
    RenderState::SkinningMode skinningMode;
    RenderState::LightingMode lightingMode;
    RenderStateGL2::Shader shader;
    MeshBuffer *cube;
    MaterialMap *cubeMats;
    QVector<FrameStat *> stats;
    int gpuTimers;
    FrameStat *frameStat;
    FrameStat *clearStat;
    FrameStat *drawCallsStat;
    FrameStat *textureBindsStat;
};

RenderStateGL2Data::RenderStateGL2Data()
{
    matrixMode = RenderState::ModelView;
    renderMode = RenderState::Basic;
    skinningMode = RenderState::SoftwareSkinning;
    lightingMode = RenderState::NoLighting;
    shader = RenderStateGL2::BasicShader;
    for(int i = 0; i < 3; i++)
        programs[i] = NULL;
    cube = NULL;
    cubeMats = NULL;
    gpuTimers = 0;
    frameStat = NULL;
    clearStat = NULL;
    drawCallsStat = NULL;
    textureBindsStat = NULL;
}

RenderStateGL2::RenderStateGL2() : RenderState()
{
    d = new RenderStateGL2Data();
    d->clearColor = vec4(0.6, 0.6, 0.9, 1.0);
    d->ambientLightColor = vec4(1.0, 1.0, 1.0, 1.0);
    d->matrix[(int)RenderState::ModelView].setIdentity();
    d->matrix[(int)RenderState::Projection].setIdentity();
    d->matrix[(int)RenderState::Texture].setIdentity();
    d->programs[(int)BasicShader] = new ShaderProgramGL2(this);
    d->programs[(int)SkinningUniformShader] = new UniformSkinningProgram(this);
    d->programs[(int)SkinningTextureShader] = new TextureSkinningProgram(this);
    createCube();
    d->drawCallsStat = createStat("Draw calls", FrameStat::Counter);
    d->textureBindsStat = createStat("Texture binds", FrameStat::Counter);
    d->frameStat = createStat("Frame (ms)", FrameStat::WallTime);
    d->clearStat = createStat("Clear (ms)", FrameStat::WallTime);
}

RenderStateGL2::~RenderStateGL2()
{
    delete d->programs[(int)BasicShader];
    delete d->programs[(int)SkinningUniformShader];
    delete d->programs[(int)SkinningTextureShader];
    delete d->cubeMats;
    delete d->cube;
    delete d;
}

ShaderProgramGL2 * RenderStateGL2::program() const
{
    return d->programs[(int)d->shader];
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
    drawMeshBatch(&d->matrix[(int)ModelView], NULL, 1);
}

void RenderStateGL2::drawMeshBatch(const matrix4 *mvMatrices, const BufferSegment *colorSegments, uint32_t instances)
{
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->setProjectionMatrix(d->matrix[(int)Projection]);
    prog->drawMeshBatch(mvMatrices, colorSegments, instances);
}

void RenderStateGL2::endDrawMesh()
{
    ShaderProgramGL2 *prog = program();
    if(!prog || !prog->loaded())
        return;
    prog->endDrawMesh();
}

static const vec3 cubeVertices[] =
{
    vec3(-0.5, -0.5,  0.5), vec3(  0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5), vec3( -0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3(  0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5), vec3( -0.5,  0.5, -0.5)
};

static void fromEightCorners(MeshBuffer *meshBuffer, const vec3 *corners)
{
    static const uint8_t faces_indices[] =
    {
        0, 1, 3,   3, 1, 2,   2, 1, 6,   6, 1, 5,
        0, 4, 1,   1, 4, 5,   3, 2, 7,   7, 2, 6,
        4, 7, 5,   5, 7, 6,   0, 3, 4,   4, 3, 7
    };
    
    Vertex *v = meshBuffer->vertices.data();
    for(uint32_t i = 0; i < 12; i++)
    {
        uint8_t idx1 = faces_indices[(i * 3) + 0];
        uint8_t idx2 = faces_indices[(i * 3) + 1];
        uint8_t idx3 = faces_indices[(i * 3) + 2];
        Vertex &v1 = v[(i * 3) + 0];
        Vertex &v2 = v[(i * 3) + 1];
        Vertex &v3 = v[(i * 3) + 2];
        v1.position = corners[idx1];
        v2.position = corners[idx2];
        v3.position = corners[idx3];
        vec3 u = (v2.position - v1.position), v = (v3.position - v1.position);
        v1.normal = v2.normal = v3.normal = vec3::cross(u, v).normalized();
    }
}

void RenderStateGL2::createCube()
{   
    MaterialGroup mg;
    mg.count = 36;
    mg.offset = 0;
    mg.id = 0;
    mg.matID = 1;
    
    d->cube = new MeshBuffer();
    d->cube->vertices.resize(36);
    d->cube->matGroups.push_back(mg);
    
    Material *mat = new Material();
    mat->setOpaque(false);
    
    d->cubeMats = new MaterialMap();
    d->cubeMats->setMaterial(mg.matID, mat);
}

void RenderStateGL2::drawBox(const AABox &box)
{
    const vec4 boxColor(0.4, 0.2, 0.2, 0.4);
    vec3 size = box.high - box.low;
    pushMatrix();
    translate(box.low.x, box.low.y, box.low.z);
    scale(size.x, size.y, size.z);
    translate(0.5, 0.5, 0.5);
    fromEightCorners(d->cube, cubeVertices);
    program()->setAmbientLight(boxColor);
    beginDrawMesh(d->cube, d->cubeMats, NULL, 0);
    drawMesh();
    endDrawMesh();
    program()->setAmbientLight(d->ambientLightColor);
    popMatrix();
}

void RenderStateGL2::drawFrustum(const Frustum &frustum)
{
    const vec4 frustumColor(0.2, 0.4, 0.2, 0.4);
    fromEightCorners(d->cube, frustum.corners());
    program()->setAmbientLight(frustumColor);
    beginDrawMesh(d->cube, d->cubeMats, NULL, 0);
    drawMesh();
    endDrawMesh();
    program()->setAmbientLight(d->ambientLightColor);
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
    return d->skinningMode;
}

void RenderStateGL2::setSkinningMode(RenderStateGL2::SkinningMode newMode)
{
    d->skinningMode = newMode;
    setShader(shaderFromModes(d->renderMode, newMode));
}

RenderStateGL2::LightingMode RenderStateGL2::lightingMode() const
{
    return d->lightingMode;
}

void RenderStateGL2::setLightingMode(RenderStateGL2::LightingMode newMode)
{
    d->lightingMode = newMode;
    if(program())
        program()->setLightingMode(newMode);
}

void RenderStateGL2::setMatrixMode(RenderStateGL2::MatrixMode newMode)
{
    d->matrixMode = newMode;
}

RenderState::RenderMode RenderStateGL2::renderMode() const
{
    return d->renderMode;
}

void RenderStateGL2::setRenderMode(RenderState::RenderMode newMode)
{
    d->renderMode = newMode;
    setShader(shaderFromModes(newMode, d->skinningMode));
}

void RenderStateGL2::loadIdentity()
{
    int i = (int)d->matrixMode;
    d->matrix[i].setIdentity();
}

void RenderStateGL2::multiplyMatrix(const matrix4 &m)
{
    int i = (int)d->matrixMode;
    d->matrix[i] = d->matrix[i] * m;
}

void RenderStateGL2::pushMatrix()
{
    int i = (int)d->matrixMode;
    d->matrixStack[i].push_back(d->matrix[i]);
}

void RenderStateGL2::popMatrix()
{
    int i = (int)d->matrixMode;
    d->matrix[i] = d->matrixStack[i].back();
    d->matrixStack[i].pop_back();
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
    return d->matrix[(int)d->matrixMode];
}

matrix4 RenderStateGL2::matrix(RenderState::MatrixMode mode) const
{
    return d->matrix[(int)mode];
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

static void defineImage(uint32_t target, int width, int height, uint32_t depth, int maxLevel)
{
    const GLenum format = GL_RGBA;
    const GLenum type = GL_UNSIGNED_BYTE;
    int level = 0;
    while((level <= maxLevel) && ((width > 0) || (height > 0)))
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

static void uploadImage(uint32_t target, QImage img, uint32_t z, uint32_t repeatX, uint32_t repeatY, int maxLevel)
{
    const GLenum format = GL_RGBA;
    const GLenum type = GL_UNSIGNED_BYTE;
    QImage levelImg = img;
    int width = img.width(), height = img.height();
    int level = 0;
    uint32_t i = 0, j = 0;
    while((level <= maxLevel) && ((width > 0) || (height > 0)))
    {
        // The last mipmap level is 1x1, even when original width and height are different.
        width = qMax(width, 1);
        height = qMax(height, 1);
        if(level > 0)
        {
            // create mipmap image
            levelImg = levelImg.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
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
    const bool useGenMipmaps = false;
    uint32_t maxLevel = maxMipmapLevel(img.width(), img.height(), 1);
    defineImage(target, img.width(), img.height(), 1, useGenMipmaps ? 0 : maxLevel);
    
    // Copy image data.
    uploadImage(target, img, 0, 1, 1, useGenMipmaps ? 0 : maxLevel);

    // Set texture parameters.
    glTexParameterf(target, GL_TEXTURE_MAX_LEVEL, maxLevel);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if(useGenMipmaps)
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
    const bool useGenMipmaps = false;
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
    defineImage(target, maxWidth, maxHeight, count, useGenMipmaps ? 0 : maxLevel);
    
    // Copy image data.
    for(size_t i = 0; i < count; i++)
    {
        QImage img = images[i];
        // Repeat textures smaller than the texture array
        // so that we can easily use GL_REPEAT.
        uint32_t repeatX = maxWidth / img.width();
        uint32_t repeatY = maxHeight / img.height();
        uploadImage(target, img, i, repeatX, repeatY, useGenMipmaps ? 0 : maxLevel);
    }
    
    // Set texture parameters.
    glTexParameterf(target, GL_TEXTURE_MAX_LEVEL, maxLevel);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if(useGenMipmaps)
        glGenerateMipmapEXT(GL_TEXTURE_2D_ARRAY);
    glBindTexture(target, 0);
    return texID;
}

void RenderStateGL2::freeTexture(texture_t tex)
{
    if(tex != 0)
        glDeleteTextures(1, &tex);
}

void RenderStateGL2::setAmbientLight(vec4 lightColor)
{
    d->ambientLightColor = lightColor;
    if(program())
        program()->setAmbientLight(lightColor);
}

void RenderStateGL2::setLightSources(const LightParams *sources, int count)
{
    if(program())
        program()->setLightSources(sources, count);
}

void RenderStateGL2::setFogParams(const FogParams &fogParams)
{
    d->clearColor = fogParams.color;
    if(program())
        program()->setFogParams(fogParams);
}

bool RenderStateGL2::beginFrame()
{
    d->frameStat->beginTime();
    ShaderProgramGL2 *prog = program();
    bool shaderLoaded = prog && prog->loaded();
    glPushAttrib(GL_ENABLE_BIT);
    if(shaderLoaded)
    {
        glUseProgram(prog->program());
        prog->setLightingMode(d->lightingMode);
    }
    glEnable(GL_DEPTH_TEST);
    setMatrixMode(ModelView);
    pushMatrix();
    loadIdentity();
    if(shaderLoaded)
        glClearColor(d->clearColor.x, d->clearColor.y, d->clearColor.z, d->clearColor.w);
    else
        glClearColor(0.6, 0.2, 0.2, 1.0);
    d->clearStat->beginTime();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    d->clearStat->endTime();
    return shaderLoaded;
}

void RenderStateGL2::endFrame()
{
    // Count draw calls and texture binds made by all programs.
    int totalDrawCalls = 0;
    int totalTextureBinds = 0;
    for(int i = 0; i < 3; i++)
    {
        ShaderProgramGL2 *prog = d->programs[i];
        if(prog)
        {
            totalDrawCalls += prog->drawCalls();
            totalTextureBinds += prog->textureBinds();
            prog->resetFrameStats();
        }
    }
    d->drawCallsStat->setCurrent(totalDrawCalls);
    d->textureBindsStat->setCurrent(totalTextureBinds);
    
    // Reset state.
    setMatrixMode(ModelView);
    popMatrix();
    glUseProgram(0);
    glPopAttrib();

    // Wait for the GPU to finish rendering the scene if we are profiling it.
    if(d->gpuTimers > 0)
        glFinish();

    // Update the frame time and FPS.
    d->frameStat->endTime();

    // Move each frame stat to the next sample.
    foreach(FrameStat *stat, d->stats)
        stat->next();
}

Frustum & RenderStateGL2::viewFrustum()
{
    return d->frustum;
}

void RenderStateGL2::setupViewport(int w, int h)
{
    float r = (float)w / (float)h;
    d->frustum.setAspect(r);
    glViewport(0, 0, w, h);
    setMatrixMode(Projection);
    loadIdentity();
    multiplyMatrix(d->frustum.projection());
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

void RenderStateGL2::freeBuffers(buffer_t *buffers, int count)
{
    if(!buffers)
        return;
    glDeleteBuffers(count, buffers);
    memset(buffers, 0, sizeof(buffer_t) * count);
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
    ShaderProgramGL2 *prog = d->programs[(int)shader];
    if(!prog->load(vertexFile, fragmentFile))
    {
        delete prog;
        d->programs[(int)shader] = NULL;
        return false;
    }
    return true;
}

void RenderStateGL2::setShader(RenderStateGL2::Shader newShader)
{
    ShaderProgramGL2 *oldProg = program();
    ShaderProgramGL2 *newProg = d->programs[(int)newShader];
    if(oldProg != newProg)
    {
        // Change to the new program only if the program loaded correctly.
        if(newProg && newProg->loaded())
        {
            d->shader = newShader;
            glUseProgram(newProg->program());
        }
    }
}

const QVector<FrameStat *> & RenderStateGL2::stats() const
{
    return d->stats;
}

FrameStat * RenderStateGL2::createStat(QString name, FrameStat::Type type)
{
    FrameStat *stat = new FrameStat(name, 64, type);
    d->stats.append(stat);
    if(stat->type() == FrameStat::GPUTime)
        d->gpuTimers++;
    return stat;
}

void RenderStateGL2::destroyStat(FrameStat *stat)
{
    int pos = d->stats.indexOf(stat);
    if(pos >= 0)
    {
        if(stat->type() == FrameStat::GPUTime)
            d->gpuTimers--;
        d->stats.remove(pos);
        delete stat;
    }
}
