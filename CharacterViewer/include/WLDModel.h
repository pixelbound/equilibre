#ifndef OPENEQ_WLD_MODEL_H
#define OPENEQ_WLD_MODEL_H

#include <QObject>
#include <QList>
#include <QMap>
#include "Platform.h"

class Mesh;
class MeshDefFragment;
class MaterialDefFragment;
class ActorDefFragment;
class Material;
class PFSArchive;
class RenderState;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class WLDModel : public QObject
{
public:
    WLDModel(ActorDefFragment *def, PFSArchive *archive, QObject *parent = 0);
    virtual ~WLDModel();

    void draw(RenderState *state);

private:
    void importDefinition(ActorDefFragment *def);
    void importMesh(MeshDefFragment *frag);
    void importMaterialGroups(MeshDefFragment *frag, Mesh *m);
    Material * importMaterial(MaterialDefFragment *frag);

    QList<Mesh *> m_meshes;
    QList<MeshDefFragment *> m_meshFrags;
    QMap<QString, Material *> m_materials;
    ActorDefFragment *m_def;
    PFSArchive *m_archive;
};

#endif
