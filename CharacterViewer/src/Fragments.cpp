#include "Fragments.h"

MeshFragment::MeshFragment(QString name) : WLDFragment(ID, name)
{
}

bool MeshFragment::unpack(WLDFragmentStream *s)
{
    s->unpackField('I', &m_flags);
    s->unpackArray("i", 4, m_ref);
    s->unpackArray("f", 3, &m_center);
    s->unpackArray("I", 3, &m_param2);
    s->unpackField('f', &m_maxDist);
    s->unpackArray("f", 3, &m_min);
    s->unpackArray("f", 3, &m_max);
    s->unpackFields("hhhhhhhhhh", &m_vertexCount, &m_texCoordsCount, &m_normalCount,
                 &m_colorCount, &m_polyCount, &m_vertexPieceCount, &m_polyTexCount,
                 &m_vertexTexCount, &m_size9, &m_scale);
    return true;
}
