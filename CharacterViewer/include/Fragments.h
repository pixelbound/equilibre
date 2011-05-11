#ifndef OPENEQ_FRAGMENTS_H
#define OPENEQ_FRAGMENTS_H

#include <QString>
#include <QVector>
#include <QColor>
#include <QPair>
#include "Platform.h"
#include "Vertex.h"
#include "WLDFragment.h"

typedef QPair<uint16_t, uint16_t> vec2us;

/*!
  \brief This type of fragment describes a mesh.
  */
class MeshFragment : public WLDFragment
{
public:
    MeshFragment(QString name);
    virtual bool unpack(WLDFragmentStream *s);

    VertexGroup *toGroup() const;

    const static uint32_t ID = 0x36;

    uint32_t m_flags;
    int32_t m_ref[4];
    vec3 m_center;
    uint32_t m_param2[3];
    float m_maxDist;
    vec3 m_min, m_max;
    uint16_t m_size9;
    QVector<vec3> m_vertices;
    QVector<vec2> m_texCoords;
    QVector<vec3> m_normals;
    QVector<QRgb> m_colors;
    QVector<uint16_t> m_indices;
    QVector<vec2us> m_vertexPieces;
    QVector<vec2us> m_polygonsByTex;
    QVector<vec2us> m_verticesByTex;
};

#endif

