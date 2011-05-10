#ifndef OPENEQ_MESH_H
#define OPENEQ_MESH_H

#include <cstdio>
#include <string>
#include <iostream>
#include <inttypes.h>
#include "Vertex.h"

using namespace std;

class RenderState;

class Mesh
{
public:
    Mesh();
    virtual ~Mesh();

    virtual int groupCount() const = 0;
    virtual uint32_t groupMode(int index) const = 0;
    virtual uint32_t groupSize(int index) const = 0;
    virtual void addGroup(VertexGroup *vg) = 0;
    virtual bool copyGroupTo(int index, VertexGroup *vg) const = 0;

    virtual void draw() = 0;
};

#endif
