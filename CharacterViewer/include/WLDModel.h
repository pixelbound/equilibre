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
class WLDModelSkin;
class WLDSkeleton;
class WLDMaterialPalette;
class WLDAnimation;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class WLDModel : public QObject
{
public:
    WLDModel(PFSArchive *archive, QObject *parent = 0);
    virtual ~WLDModel();

    static QList<MeshDefFragment *> listMeshes(ActorDefFragment *def);

    WLDSkeleton *skeleton() const;
    void setSkeleton(WLDSkeleton *skeleton);

    WLDModelSkin *skin() const;
    QMap<QString, WLDModelSkin *> & skins();
    const QMap<QString, WLDModelSkin *> & skins() const;

    WLDModelSkin * newSkin(QString name, PFSArchive *archive);

private:
    WLDSkeleton *m_skel;
    WLDModelSkin *m_skin;
    QMap<QString, WLDModelSkin *> m_skins;
};

/*!
  \brief Describes part of a model.
  */
class WLDModelPart : public QObject
{
public:
    WLDModelPart(MeshDefFragment *meshDef, QObject *parent = 0);

    MeshDefFragment *def() const;

    void draw(RenderState *state, WLDModelSkin *skin, WLDAnimation *anim,
            double currentTime);

private:
    void importMaterialGroups(Mesh *m, WLDModelSkin *skin, WLDAnimation *anim,
            double currentTime);

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
    WLDMaterialPalette(PFSArchive *archive, QObject *parent = 0);

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

    Material * material(QString name);

private:
    Material * loadMaterial(MaterialDefFragment *frag);

    QMap<QString, MaterialDefFragment *> m_materialDefs;
    QMap<QString, Material *> m_materials;
    PFSArchive *m_archive;
};

/*!
  \brief Describes one way an actor can be rendered. A skin can include a texture
  palette and alternative meshes (e.g. for a character's head).
  */
class WLDModelSkin : public QObject
{
public:
    WLDModelSkin(QString name, WLDModel *model, PFSArchive *archive, QObject *parent = 0);

    QString name() const;

    WLDMaterialPalette *palette() const;
    const QMap<QString, WLDModelPart *> & parts() const;

    void addPart(MeshDefFragment *frag, bool importPalette = true);

    static bool explodeMeshName(QString defName, QString &actorName,
                                QString &meshName, QString &skinName);

    void draw(RenderState *state, WLDAnimation *anim = 0, double currentTime = 0.0);

private:
    QString m_name;
    WLDModel *m_model;
    WLDMaterialPalette *m_palette;
    QMap<QString, WLDModelPart *> m_parts;
};

#endif
