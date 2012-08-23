#include "OpenEQ/Render/Vertex.h"
#include "OpenEQ/Render/Material.h"
#include "OpenEQ/Render/RenderState.h"

using namespace std;

BufferSegment::BufferSegment()
{
    buffer = 0;
    elementSize = 0;
    offset = 0;
    count = 0;
}

size_t BufferSegment::size() const
{
    return count * elementSize;
}

size_t BufferSegment::address() const
{
    return offset * elementSize;
}

////////////////////////////////////////////////////////////////////////////////

MeshData::MeshData(MeshBuffer *buffer, uint32_t groups)
{
    this->buffer = buffer;
    matGroups = new MaterialGroup[groups];
    groupCount = groups;
}

MeshData::~MeshData()
{
    delete [] matGroups;
}

void MeshData::updateTexCoords(MaterialMap *map)
{
    Vertex *vertices = buffer->vertices.data();
    const uint32_t *indices = buffer->indices.constData() + indexSegment.offset;
    
    // Get texture array info from the material map.
    int maxWidth, maxHeight;
    size_t totalMem, usedMem;
    map->textureArrayInfo(maxWidth, maxHeight, totalMem, usedMem);
    
    for(uint32_t i = 0; i < groupCount; i++)
    {
        MaterialGroup &mg(matGroups[i]);
        Material *mat = map->material(mg.matID);
        if(!mat)
            continue;
        float matScalingX = (float)mat->image().width() / (float)maxWidth;
        float matScalingY = (float)mat->image().height() / (float)maxHeight;
        float z = mat->subTexture();
        const uint32_t *mgIndices = indices + mg.offset;
        for(uint32_t i = 0; i < mg.count; i++)
        {
            uint32_t vertexID = mgIndices[i];
            if(vertices[vertexID].texCoords.z == 0)
            {
                vertices[vertexID].texCoords.x *= matScalingX;
                vertices[vertexID].texCoords.y *= matScalingY;
                vertices[vertexID].texCoords.z = z;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

MeshBuffer::MeshBuffer()
{
    vertexBuffer = 0;
    indexBuffer = 0;
    vertexBufferSize = 0;
    indexBufferSize = 0;
}

MeshBuffer::~MeshBuffer()
{
    foreach(MeshData *m, meshes)
        delete m;
}

MeshData * MeshBuffer::createMesh(uint32_t groups)
{
    MeshData *m = new MeshData(this, groups);
    meshes.append(m);
    return m;
}

void MeshBuffer::addMaterialGroups(MeshData *mesh)
{
    for(uint32_t i = 0; i < mesh->groupCount; i++)
    {
        MaterialGroup mg = mesh->matGroups[i];
        mg.offset += mesh->indexSegment.offset;
        matGroups.append(mg);
    }
}

void MeshBuffer::upload(RenderState *state, bool clearVertices)
{
    // Create the GPU buffers.
    vertexBufferSize = vertices.count() * sizeof(Vertex);
    indexBufferSize = indices.count() * sizeof(uint32_t);
    vertexBuffer = state->createBuffer(vertices.constData(), vertexBufferSize);
    indexBuffer = state->createBuffer(indices.constData(), indexBufferSize);
    
    // Free the memory used for vertices and indices.
    if(clearVertices)
    {
        vertices.clear();
        vertices.squeeze();
    }
    indices.clear();
    indices.squeeze();
}
