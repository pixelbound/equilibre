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

#ifndef EQUILIBRE_RENDER_STATE_H
#define EQUILIBRE_RENDER_STATE_H

#include <QVector>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"
#include "EQuilibre/Render/FrameStat.h"

class QImage;
class QVector3D;
class QQuaternion;
class BoneTransform;
class Frustum;
class AABox;
class Material;
class MaterialArray;
class FrameStat;
class MeshBuffer;
class RenderContextPrivate;
class RenderProgram;

struct RENDER_DLL LightParams
{
    vec3 color;
    Sphere bounds;
};

struct RENDER_DLL FogParams
{
    float start;
    float end;
    float density;
    vec4 color;
};

class RENDER_DLL RenderContext
{
public:
    RenderContext();
    virtual ~RenderContext();

    void init();
    
    enum Shader
    {
        BasicShader = 0,
        SkinningUniformShader = 1,
        SkinningTextureShader = 2
    };
    
    RenderProgram * programByID(Shader shaderID) const;
    void setCurrentProgram(RenderProgram *prog);

    // matrix operations

    enum MatrixMode
    {
        ModelView,
        Projection
    };

    void setMatrixMode(MatrixMode newMode);

    void loadIdentity();
    void multiplyMatrix(const matrix4 &m);
    void pushMatrix();
    void popMatrix();

    void translate(const QVector3D &v);
    void translate(const vec3 &v);
    void rotate(const QQuaternion &q);
    void scale(const vec3 &v);
    void translate(float dx, float dy, float dz);
    void rotate(float angle, float rx, float ry, float rz);
    void scale(float sx, float sy, float sz);

    matrix4 currentMatrix() const;
    matrix4 & matrix(RenderContext::MatrixMode mode);
    const matrix4 & matrix(RenderContext::MatrixMode mode) const;

    // general state operations

    Frustum & viewFrustum();
    void setupViewport(int width, int heigth);
    bool beginFrame(const vec4 &clearColor);
    void endFrame();
    
    void setDepthWrite(bool write);

    // material operations

    texture_t loadTexture(const QImage &img, bool isDDS);
    texture_t loadTextures(const QImage *images, size_t count, const bool *isDDS);
    void freeTexture(texture_t tex);
    
    // buffer operations
    
    buffer_t createBuffer(const void *data, size_t size);
    void freeBuffers(buffer_t *buffers, int count);

    // Performance measurement

    FrameStat * createStat(QString name, FrameStat::Type type);
    void destroyStat(FrameStat *stat);
    const QVector<FrameStat *> &stats() const;
    
protected:
    RenderContextPrivate *d;
};

#endif
