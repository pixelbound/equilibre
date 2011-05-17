#include <cmath>
#include <cstring>
#include <stddef.h>
#include "Platform.h"
#include "MeshGL2.h"
#include "Material.h"
#include "RenderState.h"
#include "RenderStateGL2.h"
#include "WLDModel.h"

#define BUFFER_OFFSET(type, field) ((char *)NULL + (offsetof(type, field)))

MeshGL2::MeshGL2(RenderStateGL2 *state) : Mesh()
{
    m_state = state;
}

MeshGL2::~MeshGL2()
{
    for(uint32_t i = 0; i < m_groups.size(); i++)
    {
        VertexGroup *vg = m_groups[i];
        if(vg->id != 0)
            glDeleteBuffers(1, &vg->id);
        delete vg;
    }
    m_groups.clear();
}

int MeshGL2::groupCount() const
{
    return m_groups.size();
}

uint32_t MeshGL2::groupMode(int index) const
{
    if((index < 0) || ((uint32_t)index > groupCount()))
        return 0;
    else
        return m_groups[index]->mode;
}

uint32_t MeshGL2::groupSize(int index) const
{
    if((index < 0) || ((uint32_t)index > groupCount()))
        return 0;
    else
        return m_groups[index]->count;
}

void MeshGL2::addGroup(VertexGroup *vg)
{
    m_groups.push_back(vg);
}

bool MeshGL2::copyGroupTo(int index, VertexGroup *vg) const
{
    if((index < 0) || (index >= groupCount()))
        return false;
    VertexGroup *source = m_groups[index];
    if(source->count > vg->count)
        return false;
    memcpy(vg->data, source->data, source->count * sizeof(VertexData));
    return true;
}

void MeshGL2::draw()
{
    int position = m_state->positionAttr();
    int normal = m_state->normalAttr();
    int texCoords = m_state->texCoordsAttr();
    int bone = m_state->boneAttr();
    glEnableVertexAttribArray(position);
    glEnableVertexAttribArray(normal);
    glEnableVertexAttribArray(texCoords);
    glEnableVertexAttribArray(bone);
    for(uint32_t i = 0; i < m_groups.size(); i++)
    {
        VertexGroup *vg = m_groups[i];
        //if(vg->count > 100)
        //    drawVBO(vg, position, normal, texCoords);
        //else
        drawArray(vg, position, normal, texCoords, bone);
    }
    glDisableVertexAttribArray(position);
    glDisableVertexAttribArray(normal);
    glDisableVertexAttribArray(texCoords);
    glDisableVertexAttribArray(bone);
}

void MeshGL2::drawArray(VertexGroup *vg, int position, int normal, int texCoords, int bone)
{
    Material *mat = 0;
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), &vg->data->position);
    glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), &vg->data->normal);
    glVertexAttribPointer(texCoords, 2, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), &vg->data->texCoords);
    glVertexAttribPointer(bone, 1, GL_INT, GL_FALSE,
        sizeof(VertexData), &vg->data->bone);
    if(vg->indices.count() > 0)
    {
        const uint16_t *indices = vg->indices.constData();
        foreach(MaterialGroup mg, vg->matGroups)
        {
            if(mg.palette)
                mat = mg.palette->material(mg.matName);
            if(mat)
                m_state->pushMaterial(*mat);
            glDrawElements(vg->mode, mg.count, GL_UNSIGNED_SHORT, indices + mg.offset);
            if(mat)
                m_state->popMaterial();
        }
    }
    else
    {
        foreach(MaterialGroup mg, vg->matGroups)
        {
            if(mg.palette)
                mat = mg.palette->material(mg.matName);
            if(mat)
                m_state->pushMaterial(*mat);
            glDrawArrays(vg->mode, mg.offset, mg.count);
            if(mat)
                m_state->popMaterial();
        }
    }
}

void MeshGL2::drawVBO(VertexGroup *vg, int position, int normal, int texCoords, int bone)
{
    if(vg->id == 0)
    {
        uint32_t size = vg->count * sizeof(VertexData);
        glGenBuffers(1, &vg->id);
        glBindBuffer(GL_ARRAY_BUFFER, vg->id);
        glBufferData(GL_ARRAY_BUFFER, size, vg->data, GL_STATIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, vg->id);
    }
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), BUFFER_OFFSET(VertexData, position));
    glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), BUFFER_OFFSET(VertexData, normal));
    glVertexAttribPointer(texCoords, 2, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), BUFFER_OFFSET(VertexData, texCoords));
    glVertexAttribPointer(bone, 1, GL_INT, GL_FALSE,
        sizeof(VertexData), BUFFER_OFFSET(VertexData, bone));
    glDrawArrays(vg->mode, 0, vg->count);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
