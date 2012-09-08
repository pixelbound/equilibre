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

#ifndef EQUILIBRE_RENDER_STATE_GL2_H
#define EQUILIBRE_RENDER_STATE_GL2_H

#include <vector>
#include <string>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/RenderState.h"
#include "EQuilibre/Render/Geometry.h"

class ShaderProgramGL2;
class MaterialMap;
struct RenderStateGL2Data;

class RENDER_DLL RenderStateGL2 : public RenderState
{
public:
    RenderStateGL2();
    virtual ~RenderStateGL2();

    virtual void init();
    
    virtual void beginDrawMesh(const MeshBuffer *m, MaterialMap *materials,
                               const BoneTransform *bones, int boneCount);
    virtual void drawMesh();
    virtual void drawMeshBatch(const matrix4 *mvMatrices, const BufferSegment *colorSegments, uint32_t instances);
    virtual void endDrawMesh();
    
    virtual void drawBox(const AABox &box);
    virtual void drawFrustum(const Frustum &frustum);
    ShaderProgramGL2 * program() const;

    virtual SkinningMode skinningMode() const;
    virtual void setSkinningMode(SkinningMode newMode);
    
    virtual LightingMode lightingMode() const;
    virtual void setLightingMode(LightingMode newMode);

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
    virtual void freeTexture(texture_t tex);
    virtual void setAmbientLight(vec4 lightColor);
    virtual void setLightSources(const LightParams *sources, int count);
    virtual void setFogParams(const FogParams &fogParams);
    
    virtual buffer_t createBuffer(const void *data, size_t size);
    virtual void freeBuffers(buffer_t *buffers, int count);

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

    RenderStateGL2Data *d;
};

#endif
