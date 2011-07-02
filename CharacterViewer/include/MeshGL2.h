#ifndef OPENEQ_MESH_GL2_H
#define OPENEQ_MESH_GL2_H

#include <vector>
#include <inttypes.h>
#include "Mesh.h"
#include "Vertex.h"

class RenderState;
class RenderStateGL2;
class ShaderProgramGL2;

class MeshGL2 : public Mesh
{
public:
    MeshGL2(RenderStateGL2 *state);
    virtual ~MeshGL2();

    virtual VertexGroup *group() const;
    virtual void setGroup(VertexGroup *vg);
    virtual void draw(const BoneTransform *bones, int boneCount);

private:
    void drawArray(VertexGroup *vg, ShaderProgramGL2 *prog);

    RenderStateGL2 *m_state;
    VertexGroup * m_vg;
};

#endif
