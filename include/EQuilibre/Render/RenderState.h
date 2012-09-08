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
class MaterialMap;
class FrameStat;
class MeshBuffer;
struct RenderStateData;
class RenderState;
typedef RenderState RenderStateGL2;

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

class RENDER_DLL RenderState
{
public:
    RenderState();
    virtual ~RenderState();

    void init();

    /**
     * @brief Prepare the GPU for drawing one or more mesh with the same geometry.
     * For example, send the geometry to the GPU if it isn't there already.
     * @param geom Geometry of the mesh (vertices and indices).
     * @param materials Mesh materials or NULL if the mesh has no material.
     * @param bones Array of bone transformations or NULL if the mesh is not skinned.
     * @param boneCount Number of bone transformations.
     */
    void beginDrawMesh(const MeshBuffer *geom, MaterialMap *materials,
                       const BoneTransform *bones = 0, int boneCount = 0);
    /**
     * @brief Draw a mesh whose geometry was passed to @ref beginDrawMesh.
     * Multiple instances of the same mesh can be drawn by calling @ref drawMesh
     * multiple times with different transformations.
     */
    void drawMesh();
    /**
     * @brief Draw several instances of a mesh whose geometry was passed to
     * @ref beginDrawMesh, each with a different model-view matrix.
     */
    void drawMeshBatch(const matrix4 *mvMatrices, const BufferSegment *colorSegments, uint32_t instances) ;
    /**
     * @brief Clean up the resources used by @ref beginDrawMesh and allow it to be
     * called again.
     */
    void endDrawMesh();
    
    // debug operations
    void drawBox(const AABox &box);
    void drawFrustum(const Frustum &frustum);

    enum SkinningMode
    {
        SoftwareSkinning = 0,
        HardwareSkinningUniform = 1,
        HardwareSkinningTexture = 2
    };

    SkinningMode skinningMode() const;
    void setSkinningMode(SkinningMode newMode);
    
    enum LightingMode
    {
        NoLighting = 0,
        BakedLighting = 1,
        DebugVertexColor = 2,
        DebugTextureFactor = 3
    };
    
    LightingMode lightingMode() const;
    void setLightingMode(LightingMode newMode);

    enum RenderMode
    {
        Basic = 0,
        Skinning = 1
    };

    RenderMode renderMode() const;
    void setRenderMode(RenderMode newMode);

    // matrix operations

    enum MatrixMode
    {
        ModelView,
        Projection,
        Texture
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
    matrix4 matrix(RenderState::MatrixMode mode) const;

    // general state operations

    Frustum & viewFrustum();
    void setupViewport(int width, int heigth);
    bool beginFrame();
    void endFrame();

    // material operations

    texture_t loadTexture(QImage img);
    texture_t loadTextures(const QImage *images, size_t count);
    void freeTexture(texture_t tex);
    void setAmbientLight(vec4 lightColor);
    void setLightSources(const LightParams *sources, int count);
    void setFogParams(const FogParams &fogParams);
    
    // buffer operations
    
    buffer_t createBuffer(const void *data, size_t size);
    void freeBuffers(buffer_t *buffers, int count);

    // Performance measurement

    FrameStat * createStat(QString name, FrameStat::Type type);
    void destroyStat(FrameStat *stat);
    const QVector<FrameStat *> &stats() const;
    
protected:
    RenderStateData *d;
};

#endif
