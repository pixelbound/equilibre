#include <cmath>
#include <cstring>
#include <stddef.h>
#include "Platform.h"
#include "MeshGL2.h"
#include "Material.h"
#include "RenderState.h"
#include "RenderStateGL2.h"
#include "WLDModel.h"
#include "WLDSkeleton.h"

#define BUFFER_OFFSET(type, field) ((char *)NULL + (offsetof(type, field)))

MeshGL2::MeshGL2(RenderStateGL2 *state) : Mesh()
{
    m_state = state;
    m_vg = 0;
}

MeshGL2::~MeshGL2()
{
    delete m_vg;
}

VertexGroup *MeshGL2::group() const
{
    return m_vg;
}

void MeshGL2::setGroup(VertexGroup *vg)
{
    m_vg = vg;
}

void MeshGL2::draw(const BoneTransform *bones, int boneCount)
{
    if(!m_vg)
        return;

    int position = m_state->positionAttr();
    int normal = m_state->normalAttr();
    int texCoords = m_state->texCoordsAttr();
    int bone = m_state->boneAttr();
    glEnableVertexAttribArray(position);
    if(normal >= 0)
        glEnableVertexAttribArray(normal);
    if(texCoords >= 0)
        glEnableVertexAttribArray(texCoords);
    if(bone >= 0)
        glEnableVertexAttribArray(bone);
    //m_state->setBoneTransforms(bones, boneCount);
    m_state->startSkinning();
    if(m_state->skinningMode() == RenderState::SoftwareSkinning)
    {
        VertexGroup skinnedVg(m_vg->mode, m_vg->count);
        skinnedVg.indices = m_vg->indices;
        skinnedVg.matGroups = m_vg->matGroups;

        VertexData *src = m_vg->data, *dst = skinnedVg.data;
        for(uint32_t i = 0; i < m_vg->count; i++, src++, dst++)
        {
            BoneTransform transform;
            if((int)src->bone < boneCount)
                transform = bones[src->bone];
            dst->position = transform.map(src->position);
            dst->normal = src->normal;
            dst->bone = src->bone;
            dst->texCoords = src->texCoords;
        }
        drawArray(&skinnedVg, position, normal, texCoords, bone);
    }
    else
    {
        drawArray(m_vg, position, normal, texCoords, bone);
    }
    m_state->stopSkinning();

    glDisableVertexAttribArray(position);
    if(normal >= 0)
        glDisableVertexAttribArray(normal);
    if(texCoords >= 0)
        glDisableVertexAttribArray(texCoords);
    if(bone >= 0)
        glDisableVertexAttribArray(bone);
}

void MeshGL2::drawArray(VertexGroup *vg, int position, int normal, int texCoords, int bone)
{
    Material *mat = 0;
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), &vg->data->position);
    if(normal >= 0)
        glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE,
            sizeof(VertexData), &vg->data->normal);
    if(texCoords >= 0)
        glVertexAttribPointer(texCoords, 2, GL_FLOAT, GL_FALSE,
            sizeof(VertexData), &vg->data->texCoords);
    if(bone >= 0)
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
