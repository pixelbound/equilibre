#ifndef OPENEQ_WLD_MODEL_H
#define OPENEQ_WLD_MODEL_H

#include <QObject>
#include <QList>
#include <QMap>
#include "Platform.h"
#include "Vertex.h"

class Mesh;
class MeshDefFragment;
class HierSpriteDefFragment;
class MaterialDefFragment;
class MaterialPaletteFragment;
class ActorDefFragment;
class Material;
class MaterialMap;
class PFSArchive;
class RenderState;
class VertexGroup;
class WLDMesh;
class WLDModelSkin;
class WLDSkeleton;
class BoneTransform;
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
class WLDMesh : public QObject
{
public:
    WLDMesh(MeshDefFragment *meshDef, uint32_t partID, QObject *parent = 0);
    virtual ~WLDMesh();

    VertexGroup * data() const;
    MeshDefFragment *def() const;
    MaterialMap *materials() const;
    void setMaterials(MaterialMap *materials);
    const AABox & boundsAA() const;

    void importVertexData();
    void importIndexData();
    void importMaterialGroups();
    void importVertexData(VertexGroup *vg, BufferSegment &dataLoc);
    void importIndexData(VertexGroup *vg, BufferSegment &indexLoc,
                         const BufferSegment &dataLoc, uint32_t offset, uint32_t count);
    void importMaterialGroups(VertexGroup *vg);

    static VertexGroup * combine(const QList<WLDMesh *> &meshes);

private:
    uint32_t m_partID;
    VertexGroup *m_data;
    MeshDefFragment *m_meshDef;
    MaterialMap *m_materials;
    AABox m_boundsAA;
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
    
    MaterialMap * loadMaterials();

    /*!
      \brief Return the canonical name of a material. This strips out the palette ID.
      */
    static QString materialName(QString defName);
    static QString materialName(MaterialDefFragment *def);

    static bool explodeName(QString defName, QString &charName,
                            QString &palName, QString &partName);
    static bool explodeName(MaterialDefFragment *def, QString &charName,
                            QString &palName, QString &partName);

private:
    Material * loadMaterial(MaterialDefFragment *frag);

    QMap<QString, MaterialDefFragment *> m_materialDefs;
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
    virtual ~WLDModelSkin();

    QString name() const;

    WLDMaterialPalette *palette() const;
    void setPalette(WLDMaterialPalette *newPal);
    
    MaterialMap *materials() const;
    void setMaterials(MaterialMap *newMaterials);
    
    const QList<WLDMesh *> & parts() const;

    void addPart(MeshDefFragment *frag, bool importPalette = true);

    static bool explodeMeshName(QString defName, QString &actorName,
                                QString &meshName, QString &skinName);

    void combineParts();

    void draw(RenderState *state, const BoneTransform *bones = 0, uint32_t boneCount = 0);

private:
    QString m_name;
    WLDModel *m_model;
    WLDMaterialPalette *m_palette;
    MaterialMap *m_materials;
    QList<WLDMesh *> m_parts;
};

#endif
