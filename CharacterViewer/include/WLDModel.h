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
class WLDModelPart;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class WLDModel : public QObject
{
public:
    WLDModel(PFSArchive *archive, ActorDefFragment *def = 0, QObject *parent = 0);
    virtual ~WLDModel();

    void importMesh(MeshDefFragment *frag);
    Material * importMaterial(MaterialDefFragment *frag);

    void draw(RenderState *state);

private:
    void importDefinition(ActorDefFragment *def);

    QList<WLDModelPart *> m_parts;
    QMap<QString, Material *> m_materials;
    PFSArchive *m_archive;
};

/*!
  \brief Describes part of a model.
  */
class WLDModelPart : public QObject
{
public:
    WLDModelPart(WLDModel *model, MeshDefFragment *meshDef, QObject *parent = 0);

    void draw(RenderState *state);

private:
    void importMaterialGroups(Mesh *m);

    WLDModel *m_model;
    Mesh *m_mesh;
    MeshDefFragment *m_meshDef;
};

#endif
