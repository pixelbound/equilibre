#ifndef OPENEQ_MESH_GL2_H
#define OPENEQ_MESH_GL2_H

#include <vector>
#include <inttypes.h>
#include "Mesh.h"
#include "Vertex.h"

class RenderState;
class RenderStateGL2;

class MeshGL2 : public Mesh
{
public:
    MeshGL2(RenderStateGL2 *state);
    virtual ~MeshGL2();

    virtual int groupCount() const;
    virtual uint32_t groupMode(int index) const;
    virtual uint32_t groupSize(int index) const;
    virtual void addGroup(VertexGroup *vg);
    virtual bool copyGroupTo(int index, VertexGroup *vg) const;
    virtual void draw();

private:
    void drawArray(VertexGroup *vg, int position, int normal, int texCoords);
    void drawVBO(VertexGroup *vg, int position, int normal, int texCoords);

    RenderStateGL2 *m_state;
    std::vector<VertexGroup *> m_groups;
};

#endif
