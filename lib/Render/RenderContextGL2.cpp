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
#include <QString>
#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/FrameStat.h"

class RenderContextPrivate
{
public:
    RenderContextPrivate();
    
    bool initShader(RenderContext::Shader shader, QString vertexFile, QString fragmentFile);
    
    Frustum frustum;
    RenderContext::MatrixMode matrixMode;
    matrix4 matrix[2];
    std::vector<matrix4> matrixStack[2];
    RenderProgram *programs[3];
    uint32_t currentProgram;
    QVector<FrameStat *> stats;
    int gpuTimers;
    FrameStat *frameStat;
    FrameStat *clearStat;
    FrameStat *drawCallsStat;
    FrameStat *textureBindsStat;
};

RenderContextPrivate::RenderContextPrivate()
{
    matrixMode = RenderContext::ModelView;
    for(int i = 0; i < 3; i++)
        programs[i] = NULL;
    currentProgram = 0;
    gpuTimers = 0;
    frameStat = NULL;
    clearStat = NULL;
    drawCallsStat = NULL;
    textureBindsStat = NULL;
}

bool RenderContextPrivate::initShader(RenderContext::Shader shader, QString vertexFile, QString fragmentFile)
{
    RenderProgram *prog = programs[(int)shader];
    if(!prog->load(vertexFile, fragmentFile))
    {
        delete prog;
        programs[(int)shader] = NULL;
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

RenderContext::RenderContext()
{
    d = new RenderContextPrivate();
    d->matrix[(int)RenderContext::ModelView].setIdentity();
    d->matrix[(int)RenderContext::Projection].setIdentity();
    d->programs[(int)BasicShader] = new RenderProgram(this);
    d->programs[(int)SkinningUniformShader] = new UniformSkinningProgram(this);
    d->programs[(int)SkinningTextureShader] = new TextureSkinningProgram(this);
    d->drawCallsStat = createStat("Draw calls", FrameStat::Counter);
    d->textureBindsStat = createStat("Texture binds", FrameStat::Counter);
    d->frameStat = createStat("Frame (ms)", FrameStat::WallTime);
    d->clearStat = createStat("Clear (ms)", FrameStat::WallTime);
}

RenderContext::~RenderContext()
{
    delete d->programs[(int)BasicShader];
    delete d->programs[(int)SkinningUniformShader];
    delete d->programs[(int)SkinningTextureShader];
    delete d;
}

RenderProgram * RenderContext::programByID(Shader shaderID) const
{
    if((shaderID >= BasicShader) && (shaderID <= SkinningTextureShader))
        return d->programs[(int)shaderID];
    else
        return NULL;
}

void RenderContext::setCurrentProgram(RenderProgram *prog)
{
    uint32_t name = prog ? prog->program() : 0;
    if(d->currentProgram != name)
    {
        glUseProgram(name);
        d->currentProgram = name;
    }
}

void RenderContext::setMatrixMode(RenderContext::MatrixMode newMode)
{
    d->matrixMode = newMode;
}

bool RenderContext::beginFrame(const vec4 &clearColor)
{
    d->frameStat->beginTime();
    d->currentProgram = 0;
    RenderProgram *prog = programByID(BasicShader);
    bool shaderLoaded = prog && prog->loaded();
    glPushAttrib(GL_ENABLE_BIT);
    if(shaderLoaded)
        setCurrentProgram(prog);
    glEnable(GL_DEPTH_TEST);
    setMatrixMode(ModelView);
    pushMatrix();
    loadIdentity();
    if(shaderLoaded)
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    else
        glClearColor(0.6, 0.2, 0.2, 1.0);
    d->clearStat->beginTime();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    d->clearStat->endTime();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    return shaderLoaded;
}

void RenderContext::endFrame()
{
    // Count draw calls and texture binds made by all programs.
    int totalDrawCalls = 0;
    int totalTextureBinds = 0;
    for(int i = 0; i < 3; i++)
    {
        RenderProgram *prog = d->programs[i];
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
    setCurrentProgram(NULL);
    glPopAttrib();
    glCullFace(GL_BACK);

    // Wait for the GPU to finish rendering the scene if we are profiling it.
    if(d->gpuTimers > 0)
        glFinish();

    // Update the frame time and FPS.
    d->frameStat->endTime();

    // Move each frame stat to the next sample.
    foreach(FrameStat *stat, d->stats)
        stat->next();
}

Frustum & RenderContext::viewFrustum()
{
    return d->frustum;
}

void RenderContext::setupViewport(int w, int h)
{
    float r = (float)w / (float)h;
    d->frustum.setAspect(r);
    glViewport(0, 0, w, h);
    setMatrixMode(Projection);
    loadIdentity();
    multiplyMatrix(d->frustum.projection());
    setMatrixMode(ModelView);
}

void RenderContext::setDepthWrite(bool write)
{
    glDepthMask(write);
}

void RenderContext::loadIdentity()
{
    int i = (int)d->matrixMode;
    d->matrix[i].setIdentity();
}

void RenderContext::multiplyMatrix(const matrix4 &m)
{
    int i = (int)d->matrixMode;
    d->matrix[i] = d->matrix[i] * m;
}

void RenderContext::pushMatrix()
{
    int i = (int)d->matrixMode;
    d->matrixStack[i].push_back(d->matrix[i]);
}

void RenderContext::popMatrix()
{
    int i = (int)d->matrixMode;
    d->matrix[i] = d->matrixStack[i].back();
    d->matrixStack[i].pop_back();
}

void RenderContext::translate(const vec3 &v)
{
    multiplyMatrix(matrix4::translate(v.x, v.y, v.z));
}

void RenderContext::translate(float dx, float dy, float dz)
{
    multiplyMatrix(matrix4::translate(dx, dy, dz));
}

void RenderContext::rotate(float angle, float rx, float ry, float rz)
{
    multiplyMatrix(matrix4::rotate(angle, rx, ry, rz));
}

void RenderContext::scale(float sx, float sy, float sz)
{
    multiplyMatrix(matrix4::scale(sx, sy, sz));
}

void RenderContext::translate(const QVector3D &v)
{
    translate(v.x(), v.y(), v.z());
}

void RenderContext::rotate(const QQuaternion &q)
{
    QMatrix4x4 m;
    m.setToIdentity();
    m.rotate(q);
    matrix4 m2(m);
    multiplyMatrix(m2);
}

matrix4 RenderContext::currentMatrix() const
{
    return d->matrix[(int)d->matrixMode];
}

matrix4 & RenderContext::matrix(RenderContext::MatrixMode mode)
{
    return d->matrix[(int)mode];
}

const matrix4 & RenderContext::matrix(RenderContext::MatrixMode mode) const
{
    return d->matrix[(int)mode];
}

static void copyImageARGB32(const QImage &src, QImage &dst, size_t slicePitch, uint32_t z,
                          uint32_t repeatX, uint32_t repeatY, bool invertY)
{
    uint32_t width = src.width(), height = src.height();
    QRgb *dstPos = (QRgb *)dst.bits() + (slicePitch * z);
    for(uint32_t i = 0; i < repeatY; i++)
    {
        for(uint32_t y = 0; y < height; y++)
        {
            int scanIndex = invertY ? (height - y - 1) : y;
            const QRgb *srcPos = (const QRgb *)src.scanLine(scanIndex);
            for(uint32_t j = 0; j < repeatX; j++)
            {
                memcpy(dstPos, srcPos, width * sizeof(QRgb));
                dstPos += width;
            }
        }
    }
}

static void copyImageIndexed8(const QImage &src, QImage &dst, size_t slicePitch, uint32_t z,
                              uint32_t repeatX, uint32_t repeatY, bool invertY)
{
    uint32_t width = src.width(), height = src.height();
    QRgb *dstPos = (QRgb *)dst.bits() + (slicePitch * z);
    QVector<QRgb> colors = src.colorTable();
    for(uint32_t i = 0; i < repeatY; i++)
    {
        for(uint32_t y = 0; y < height; y++)
        {
            int scanIndex = invertY ? (height - y - 1) : y;
            const uint8_t *srcPos = (const uint8_t *)src.scanLine(scanIndex);
            for(uint32_t j = 0; j < repeatX; j++)
            {
                for(uint32_t x = 0; x < width; x++)
                {
                    uint8_t index = srcPos[x];
                    *dstPos++ = (index < colors.count()) ? colors.value(index) : 0;
                }
            }
        }
    }
}

static void copyImage(const QImage &src, QImage &dst, size_t slicePitch, uint32_t z,
                      uint32_t repeatX, uint32_t repeatY, bool invertY)
{
    Q_ASSERT((dst.format() == QImage::Format_ARGB32) && "Unsupported pixel format");
    switch(src.format())
    {
    case QImage::Format_ARGB32:
        copyImageARGB32(src, dst, slicePitch, z, repeatX, repeatY, invertY);
        break;
    case QImage::Format_Indexed8:
        copyImageIndexed8(src, dst, slicePitch, z, repeatX, repeatY, invertY);
        break;
    default:
        qDebug("Unsupported pixel format: %d", src.format());
        Q_ASSERT(0 && "Unsupported pixel format");
        break;
    }
}

static uint32_t maxMipmapLevel(uint32_t texWidth, uint32_t texHeight, uint32_t maxRepeat)
{
    int width = texWidth / maxRepeat, height = texHeight / maxRepeat;
    uint32_t level = 0;
    while(true)
    {
        width >>= 1;
        height >>= 1;
        if((width == 0) && (height == 0))
            return level;
        level++;
    }
    return 0;
}

texture_t RenderContext::loadTexture(const QImage &img)
{
    return loadTextures(&img, 1);
}

texture_t RenderContext::loadTextures(const QImage *images, size_t count)
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
    
    // Copy all images to a single image for each mipmap level.
    int maxGenLevel = useGenMipmaps ? 0 : maxLevel;
    int layerWidth = maxWidth, layerHeight = maxHeight;
    for(int level = 0; level <= maxGenLevel; level++)
    {
        QImage levelImg(layerWidth, layerHeight * count, QImage::Format_ARGB32);
        size_t slicePitch = layerWidth * layerHeight;
        levelImg.fill(0);
        for(size_t i = 0; i < count; i++)
        {
            // Repeat textures smaller than the texture array
            // so that we can easily use GL_REPEAT.
            QImage img = images[i];
            uint32_t repeatX = maxWidth / img.width(), repeatY = maxHeight / img.height();
            
            // Scale down the original image to generate the mipmap.
            int scaledWidth = (img.width() >> level), scaledHeight = (img.height() >> level);
            Q_ASSERT((scaledWidth > 0) || (scaledHeight > 0));
            scaledWidth = qMax(scaledWidth, 1);
            scaledHeight = qMax(scaledHeight, 1);
            if(level > 0)
                img = img.scaled(scaledWidth, scaledHeight);
            copyImage(img, levelImg, slicePitch, i, repeatX, repeatY, true);
        }
        glTexImage3D(target, level, GL_RGBA, layerWidth, layerHeight, count, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, levelImg.bits());
        layerWidth = qMax(layerWidth >> 1, 1);
        layerHeight = qMax(layerHeight >> 1, 1);
    }
    
    // Set texture parameters.
    GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
    glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
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

void RenderContext::freeTexture(texture_t tex)
{
    if(tex != 0)
        glDeleteTextures(1, &tex);
}

buffer_t RenderContext::createBuffer(const void *data, size_t size)
{
    buffer_t buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

void RenderContext::freeBuffers(buffer_t *buffers, int count)
{
    if(!buffers)
        return;
    glDeleteBuffers(count, buffers);
    memset(buffers, 0, sizeof(buffer_t) * count);
}

void RenderContext::init()
{
    d->initShader(BasicShader, "vertex.glsl", "fragment.glsl");
    d->initShader(SkinningUniformShader, "vertex_skinned_uniform.glsl", "fragment.glsl");
    d->initShader(SkinningTextureShader, "vertex_skinned_texture.glsl", "fragment.glsl");
}

const QVector<FrameStat *> & RenderContext::stats() const
{
    return d->stats;
}

FrameStat * RenderContext::createStat(QString name, FrameStat::Type type)
{
    FrameStat *stat = new FrameStat(name, 64, type);
    d->stats.append(stat);
    if(stat->type() == FrameStat::GPUTime)
        d->gpuTimers++;
    return stat;
}

void RenderContext::destroyStat(FrameStat *stat)
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
