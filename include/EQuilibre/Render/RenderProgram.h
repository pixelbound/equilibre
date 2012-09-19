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

#ifndef EQUILIBRE_SHADER_PROGRAM_GL2_H
#define EQUILIBRE_SHADER_PROGRAM_GL2_H

#include <vector>
#include <QString>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/RenderContext.h"

const int MAX_TRANSFORMS = 256;
const int MAX_LIGHTS = 8;

const int A_POSITION = 0;
const int A_NORMAL = 1;
const int A_TEX_COORDS = 2;
const int A_COLOR = 3;
const int A_BONE_INDEX = 4;
const int A_MODEL_VIEW_0 = 5;
const int A_MAX = A_MODEL_VIEW_0;

const int U_MODELVIEW_MATRIX = 0;
const int U_PROJECTION_MATRIX = 1;
const int U_AMBIENT_LIGHT = 2;
const int U_MAT_HAS_TEXTURE = 3;
const int U_MAT_TEXTURE = 4;
const int U_LIGHTING_MODE = 5;
const int U_FOG_START = 6;
const int U_FOG_END = 7;
const int U_FOG_DENSITY = 8;
const int U_FOG_COLOR = 9;
const int U_MAX = U_FOG_COLOR;

struct RENDER_DLL ShaderSymbolInfo
{
    uint32_t ID;
    const char *Name;
};

class BoneTransform;
class Material;
class MaterialMap;

class RENDER_DLL MeshDataGL2
{
public:
    void clear();
    
    const MeshBuffer *meshBuf;
    const BoneTransform *bones;
    uint32_t boneCount;
    MaterialMap *materials;
    const uint32_t *indices;
    bool haveIndices;
    bool pending;
};

class RENDER_DLL RenderProgram
{
public:
    RenderProgram(RenderContext *renderCtx);
    virtual ~RenderProgram();

    bool loaded() const;
    bool current() const;
    uint32_t program() const;
    int drawCalls() const;
    int textureBinds() const;
    void resetFrameStats();

    bool load(QString vertexFile, QString fragmentFile);

    virtual bool init();
    
    /**
     * @brief Prepare the GPU for drawing one or more mesh with the same geometry.
     * For example, send the geometry to the GPU if it isn't there already.
     * @param geom Geometry of the mesh (vertices and indices).
     * @param materials Mesh materials or NULL if the mesh has no material.
     * @param bones Array of bone transformations or NULL if the mesh is not skinned.
     * @param boneCount Number of bone transformations.
     */
    virtual void beginDrawMesh(const MeshBuffer *geom, MaterialMap *materials,
                               const BoneTransform *bones = 0, uint32_t boneCount = 0);
    /**
     * @brief Draw a mesh whose geometry was passed to @ref beginDrawMesh.
     * Multiple instances of the same mesh can be drawn by calling @ref drawMesh
     * multiple times with different transformations.
     */
    virtual void drawMesh();
    /**
     * @brief Draw several instances of a mesh whose geometry was passed to
     * @ref beginDrawMesh, each with a different model-view matrix.
     */
    virtual void drawMeshBatch(const matrix4 *mvMatrices, const BufferSegment *colorSegments, uint32_t instances) ;
    /**
     * @brief Clean up the resources used by @ref beginDrawMesh and allow it to be
     * called again.
     */
    virtual void endDrawMesh();
    
    // debug operations
    void drawBox(const AABox &box);
    void drawFrustum(const Frustum &frustum);
    
    enum LightingMode
    {
        NoLighting = 0,
        BakedLighting = 1,
        DebugVertexColor = 2,
        DebugTextureFactor = 3,
        DebugDiffuse = 4
    };

    void setModelViewMatrix(const matrix4 &modelView);
    void setProjectionMatrix(const matrix4 &projection);
    void setBoneTransforms(const BoneTransform *transforms, int count);
    void setAmbientLight(vec4 lightColor);
    void setLightingMode(LightingMode newMode);
    void setFogParams(const FogParams &fogParams);

protected:
    bool compileProgram(QString vertexFile, QString fragmentFile);
    static uint32_t loadShader(QString path, uint32_t type);
    void enableVertexAttribute(int attr, int index = 0);
    void disableVertexAttribute(int attr, int index = 0);
    void uploadVertexAttributes(const MeshBuffer *meshBuf);
    void beginApplyMaterial(MaterialMap *map, Material *m);
    void endApplyMaterial(MaterialMap *map, Material *m);
    void drawMaterialGroup(const MaterialGroup &mg);
    void bindColorBuffer(const BufferSegment *colorSegments, int instanceID, bool &enabledColor);
    virtual void beginSkinMesh();
    virtual void endSkinMesh();
    void createCube();
    void uploadCube();

    RenderContext *m_renderCtx;
    uint32_t m_vertexShader;
    uint32_t m_fragmentShader;
    uint32_t m_program;
    int m_attr[A_MAX+1];
    int m_uniform[U_MAX+1];
    vec4 *m_bones;
    MeshDataGL2 m_meshData;
    int m_drawCalls;
    int m_textureBinds;
    bool m_projectionSent;
    bool m_blendingEnabled;
    bool m_currentMatNeedsBlending;
    MeshBuffer *m_cube;
    MaterialMap *m_cubeMats;
};

class UniformSkinningProgram : public RenderProgram
{
public:
    UniformSkinningProgram(RenderContext *renderCtx);
    virtual bool init();
    virtual void beginSkinMesh();
    virtual void endSkinMesh();

private:
    int m_bonesLoc;
};

class TextureSkinningProgram : public RenderProgram
{
public:
    TextureSkinningProgram(RenderContext *renderCtx);
    virtual ~TextureSkinningProgram();
    virtual bool init();
    virtual void beginSkinMesh();
    virtual void endSkinMesh();

private:
    int m_bonesLoc;
    uint32_t m_boneTexture;
};

#endif
