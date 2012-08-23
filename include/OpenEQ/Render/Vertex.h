#ifndef OPENEQ_RENDER_VERTEX_H
#define OPENEQ_RENDER_VERTEX_H

#include <QVector>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/LinearMath.h"

class MeshBuffer;
class MaterialMap;
class RenderState;

class RENDER_DLL Vertex
{
public:
    vec3 position;
    vec3 normal;
    vec3 texCoords;
    uint32_t bone;
    uint32_t padding[2]; // align on 16-bytes boundaries
};

class RENDER_DLL MaterialGroup
{
public:
    uint32_t id;
    uint32_t offset;
    uint32_t count;
    uint32_t matID;
};

struct RENDER_DLL BufferSegment
{
    buffer_t buffer;
    uint32_t elementSize;
    uint32_t offset;
    uint32_t count;
    
    BufferSegment();
    size_t size() const;
    size_t address() const;
};

class RENDER_DLL MeshData
{
public:
    MeshData(MeshBuffer *buffer, uint32_t groups);
    ~MeshData();
    void updateTexCoords(MaterialMap *map);
    
    MeshBuffer *buffer;
    BufferSegment vertexSegment;
    BufferSegment indexSegment;
    MaterialGroup *matGroups;
    uint32_t groupCount;
};

class RENDER_DLL MeshBuffer
{
public:
    MeshBuffer();
    ~MeshBuffer();
    MeshData *createMesh(uint32_t groups);
    void addMaterialGroups(MeshData *mesh);
    void upload(RenderState *state, bool clearVertices);
    
    QVector<Vertex> vertices;
    QVector<uint32_t> indices;
    QVector<MaterialGroup> matGroups;
    QVector<MeshData *> meshes;
    buffer_t vertexBuffer;
    buffer_t indexBuffer;
    uint32_t vertexBufferSize;
    uint32_t indexBufferSize;
};

class RENDER_DLL VertexGroup
{
public:
    QVector<Vertex> vertices;
    QVector<uint32_t> indices;
    QVector<MaterialGroup> matGroups;
    BufferSegment vertexBuffer;
    BufferSegment indexBuffer;
};

#endif
