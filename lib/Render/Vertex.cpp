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

#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"

BufferSegment::BufferSegment()
{
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

void MeshData::updateTexCoords(MaterialArray *array, bool useMap)
{
    buffer->updateTexCoords(array, matGroups, groupCount, indexSegment.offset, useMap);
}

////////////////////////////////////////////////////////////////////////////////

MeshBuffer::MeshBuffer()
{
    vertexBuffer = 0;
    indexBuffer = 0;
    colorBuffer = 0;
    vertexBufferSize = 0;
    indexBufferSize = 0;
    colorBufferSize = 0;
}

MeshBuffer::~MeshBuffer()
{
    clear(NULL);
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

void MeshBuffer::updateTexCoords(MaterialArray *array, const MaterialGroup *matGroups,
                                 uint32_t groupCount, uint32_t startIndex, bool useMap)
{
    Vertex *vertices = this->vertices.data();
    const uint32_t *indices = this->indices.constData() + startIndex;
    
    int maxWidth = array->maxWidth(), maxHeight = array->maxHeight();
    for(uint32_t i = 0; i < groupCount; i++)
    {
        const MaterialGroup &mg(matGroups[i]);
        Material *mat = array->material(mg.matID);
        float matScalingX = 1.0, matScalingY = 1.0, z = (mg.matID + 1);
        if(!useMap && mat)
        { 
            // XXX put the scaling info in the uniform array.
            matScalingX = (float)mat->width() / (float)maxWidth;
            matScalingY = (float)mat->height() / (float)maxHeight;
            z = mat->subTexture();
        }
        
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

void MeshBuffer::upload(RenderContext *renderCtx)
{
    // Create the GPU buffers.
    vertexBufferSize = vertices.count() * sizeof(Vertex);
    indexBufferSize = indices.count() * sizeof(uint32_t);
    vertexBuffer = renderCtx->createBuffer(vertices.constData(), vertexBufferSize);
    indexBuffer = renderCtx->createBuffer(indices.constData(), indexBufferSize);
}

void MeshBuffer::clear(RenderContext *renderCtx)
{
    foreach(MeshData *m, meshes)
        delete m;
    meshes.clear();
    clearVertices();
    clearIndices();
    clearColors();
    if(renderCtx)
    {
        renderCtx->freeBuffers(&vertexBuffer, 1);
        renderCtx->freeBuffers(&indexBuffer, 1);
        renderCtx->freeBuffers(&colorBuffer, 1);
    }
}

void MeshBuffer::clearVertices()
{
    vertices.clear();
    vertices.squeeze();
}

void MeshBuffer::clearIndices()
{
    indices.clear();
    indices.squeeze();
}

void MeshBuffer::clearColors()
{
    colors.clear();
    colors.squeeze();
}
