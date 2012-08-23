#ifndef OPENEQ_VERTEX_H
#define OPENEQ_VERTEX_H

#include <QVector>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/LinearMath.h"

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

class RENDER_DLL VertexGroup
{
public:
    enum Primitive
    {
        Triangle,
        Quad
    };
    
    VertexGroup(Primitive mode);
    virtual ~VertexGroup();

    Primitive mode;
    QVector<Vertex> vertices;
    QVector<uint32_t> indices;
    QVector<MaterialGroup> matGroups;
    BufferSegment vertexBuffer;
    BufferSegment indexBuffer;
};

#endif
