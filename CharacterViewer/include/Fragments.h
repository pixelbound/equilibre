#ifndef OPENEQ_FRAGMENTS_H
#define OPENEQ_FRAGMENTS_H

#include <QString>
#include "Platform.h"
#include "Vertex.h"
#include "WLDFragment.h"

/*!
  \brief This type of fragment describes a mesh.
  */
class MeshFragment : public WLDFragment
{
public:
    MeshFragment(QString name);
    virtual bool unpack(WLDFragmentStream *s);

    const static uint32_t ID = 0x36;

    uint32_t m_flags;
    int32_t m_ref[4];
    vec3 m_center;
    uint32_t m_param2[3];
    float m_maxDist;
    vec3 m_min, m_max;
    uint16_t m_vertexCount, m_texCoordsCount, m_normalCount, m_colorCount, m_polyCount;
    uint16_t m_vertexPieceCount, m_polyTexCount, m_vertexTexCount, m_size9, m_scale;
};

#endif

