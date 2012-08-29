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
#include "EQuilibre/Render/FrameStat.h"

class QImage;
class QVector3D;
class QQuaternion;
class BoneTransform;
class Frustum;
class AABox;
class Material;
class MaterialMap;
class FrameStat;
class MeshBuffer;

struct RENDER_DLL LightParams
{
    vec3 color;
    vec3 position;
    float radius;
};

struct RENDER_DLL FogParams
{
    float start;
    float end;
    float density;
    vec4 color;
};

class RENDER_DLL RenderState
{
public:
    RenderState();
    virtual ~RenderState();

    virtual void init();

    /**
     * @brief Prepare the GPU for drawing one or more mesh with the same geometry.
     * For example, send the geometry to the GPU if it isn't there already.
     * @param geom Geometry of the mesh (vertices and indices).
     * @param materials Mesh materials or NULL if the mesh has no material.
     * @param bones Array of bone transformations or NULL if the mesh is not skinned.
     * @param boneCount Number of bone transformations.
     */
    virtual void beginDrawMesh(const MeshBuffer *geom, MaterialMap *materials,
                               const BoneTransform *bones = 0, int boneCount = 0) = 0;
    /**
     * @brief Draw a mesh whose geometry was passed to @ref beginDrawMesh.
     * Multiple instances of the same mesh can be drawn by calling @ref drawMesh
     * multiple times with different transformations.
     */
    virtual void drawMesh() = 0;
    /**
     * @brief Draw several instances of a mesh whose geometry was passed to
     * @ref beginDrawMesh, each with a different model-view matrix.
     */
    virtual void drawMeshBatch(const matrix4 *mvMatrices, const BufferSegment *colorSegments, uint32_t instances) = 0;
    /**
     * @brief Clean up the resources used by @ref beginDrawMesh and allow it to be
     * called again.
     */
    virtual void endDrawMesh() = 0;
    
    // debug operations
    virtual void drawBox(const AABox &box) = 0;
    virtual void drawFrustum(const Frustum &frustum) = 0;

    enum SkinningMode
    {
        SoftwareSkinning = 0,
        HardwareSkinningUniform = 1,
        HardwareSkinningTexture = 2
    };

    virtual SkinningMode skinningMode() const = 0;
    virtual void setSkinningMode(SkinningMode newMode) = 0;

    enum RenderMode
    {
        Basic = 0,
        Skinning = 1
    };

    virtual RenderMode renderMode() const = 0;
    virtual void setRenderMode(RenderMode newMode) = 0;

    // matrix operations

    enum MatrixMode
    {
        ModelView,
        Projection,
        Texture
    };

    virtual void setMatrixMode(MatrixMode newMode) = 0;

    virtual void loadIdentity() = 0;
    virtual void multiplyMatrix(const matrix4 &m) = 0;
    virtual void pushMatrix() = 0;
    virtual void popMatrix() = 0;

    void translate(const QVector3D &v);
    void translate(const vec3 &v);
    void rotate(const QQuaternion &q);
    void scale(const vec3 &v);
    virtual void translate(float dx, float dy, float dz) = 0;
    virtual void rotate(float angle, float rx, float ry, float rz) = 0;
    virtual void scale(float sx, float sy, float sz) = 0;

    virtual matrix4 currentMatrix() const = 0;
    virtual matrix4 matrix(RenderState::MatrixMode mode) const = 0;

    // general state operations

    virtual Frustum & viewFrustum() = 0;
    virtual void setupViewport(int width, int heigth) = 0;
    virtual bool beginFrame() = 0;
    virtual void endFrame() = 0;

    // material operations

    virtual texture_t loadTexture(QImage img) = 0;
    virtual texture_t loadTextures(const QImage *images, size_t count) = 0;
    virtual void freeTexture(texture_t tex) = 0;
    virtual void setAmbientLight(vec4 lightColor) = 0;
    virtual void setLightSources(const LightParams *sources, int count) = 0;
    virtual void setFogParams(const FogParams &fogParams) = 0;
    
    // buffer operations
    
    virtual buffer_t createBuffer(const void *data, size_t size) = 0;
    virtual void freeBuffers(buffer_t *buffers, int count) = 0;

    // Performance measurement

    virtual FrameStat * createStat(QString name, FrameStat::Type type) = 0;
    virtual void destroyStat(FrameStat *stat) = 0;
    virtual const QVector<FrameStat *> &stats() const = 0;
};

#endif
