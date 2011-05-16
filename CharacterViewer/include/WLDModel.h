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
class MaterialPaletteFragment;
class ActorDefFragment;
class Material;
class PFSArchive;
class RenderState;
class WLDModelPart;
class WLDSkeleton;
class WLDMaterialPalette;
class WLDAnimation;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class WLDModel : public QObject
{
public:
    WLDModel(PFSArchive *archive, ActorDefFragment *def = 0, WLDSkeleton *skel = 0, QObject *parent = 0);
    virtual ~WLDModel();

    WLDSkeleton *skeleton() const;
    const QMap<QString, WLDMaterialPalette *> & palettes() const;
    void addPalette(WLDMaterialPalette *palette);

    void addPart(MeshDefFragment *frag);

    void draw(RenderState *state, WLDMaterialPalette *palette = 0, WLDAnimation *anim = 0,
              double currentTime = 0.0);

private:
    void importDefinition(ActorDefFragment *def);
    void importHierMesh(HierSpriteDefFragment *def);

    QList<WLDModelPart *> m_parts;
    WLDSkeleton *m_skel;
    QMap<QString, WLDMaterialPalette *> m_palettes;
    PFSArchive *m_archive;
};

/*!
  \brief Describes part of a model.
  */
class WLDModelPart : public QObject
{
public:
    WLDModelPart(WLDModel *model, MeshDefFragment *meshDef, QObject *parent = 0);

    void draw(RenderState *state, WLDMaterialPalette *palette, WLDAnimation *anim,
            double currentTime);

private:
    void importMaterialGroups(Mesh *m, WLDMaterialPalette *palette, WLDAnimation *anim,
            double currentTime);

    WLDModel *m_model;
    Mesh *m_mesh;
    MeshDefFragment *m_meshDef;
};

/*!
  \brief Defines a set of materials (e.g. textures) that can be used for a model.
  One model can have multiple palettes but can only use one at the same time.
  */
class WLDMaterialPalette : public QObject
{
public:
    WLDMaterialPalette(QString name, PFSArchive *archive, QObject *parent = 0);

    QString name() const;

    void addPaletteDef(MaterialPaletteFragment *def);
    QString addMaterialDef(MaterialDefFragment *def);

    /*!
      \brief Return the canonical name of a material. This strips out the palette ID.
      */
    QString materialName(QString defName) const;
    QString materialName(MaterialDefFragment *def) const;

    static bool explodeName(QString defName, QString &charName,
                            QString &palName, QString &partName);
    static bool explodeName(MaterialDefFragment *def, QString &charName,
                            QString &palName, QString &partName);

    Material * material(QString name) const;

private:
    void importMaterial(QString key, MaterialDefFragment *frag);

    QString m_name;
    QMap<QString, Material *> m_materials;
    PFSArchive *m_archive;
};

#endif
