#ifndef OPENEQ_WLD_MODEL_H
#define OPENEQ_WLD_MODEL_H

#include <QObject>
#include <QList>
#include <QMap>
#include "Platform.h"

class Mesh;
class MeshDefFragment;
class HierSpriteDefFragment;
class MaterialDefFragment;
class ActorDefFragment;
class Material;
class PFSArchive;
class RenderState;
class WLDModelPart;
class WLDSkeleton;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class WLDModel : public QObject
{
public:
    WLDModel(PFSArchive *archive, ActorDefFragment *def = 0, WLDSkeleton *skel = 0, QObject *parent = 0);
    virtual ~WLDModel();

    WLDSkeleton *skeleton() const;

    void importMesh(MeshDefFragment *frag);
    Material * importMaterial(MaterialDefFragment *frag);

    void draw(RenderState *state, double currentTime = 0.0);

private:
    void importDefinition(ActorDefFragment *def);
    void importHierMesh(HierSpriteDefFragment *def);

    QList<WLDModelPart *> m_parts;
    WLDSkeleton *m_skel;
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

    void draw(RenderState *state, double currentTime = 0.0);

private:
    void importMaterialGroups(Mesh *m, double currentTime);

    WLDModel *m_model;
    Mesh *m_mesh;
    MeshDefFragment *m_meshDef;
};

#endif
