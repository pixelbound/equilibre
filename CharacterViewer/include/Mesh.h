#ifndef OPENEQ_MESH_H
#define OPENEQ_MESH_H

#include <cstdio>
#include <string>
#include <iostream>
#include <inttypes.h>
#include "Vertex.h"

using namespace std;

class RenderState;
class BoneTransform;

class Mesh
{
public:
    Mesh();
    virtual ~Mesh();

    virtual VertexGroup *group() const = 0;
    virtual void setGroup(VertexGroup *vg) = 0;
    virtual void draw(const BoneTransform *bone, int boneCount) = 0;
};

#endif
