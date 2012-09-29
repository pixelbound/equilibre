// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef EQUILIBRE_WLD_MODEL_H
#define EQUILIBRE_WLD_MODEL_H

#include <QList>
#include <QMap>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"

class MeshDefFragment;
class HierSpriteDefFragment;
class MaterialDefFragment;
class MaterialPaletteFragment;
class ActorDefFragment;
class Material;
class MaterialArray;
class MaterialMap;
class PFSArchive;
class RenderContext;
class RenderProgram;
class MeshData;
class MeshBuffer;
class WLDMesh;
class WLDModelSkin;
class WLDSkeleton;
class BoneTransform;
class WLDAnimation;
class WLDMaterialPalette;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class GAME_DLL WLDModel
{
public:
    WLDModel(WLDMesh *mainMesh);
    virtual ~WLDModel();

    static QList<MeshDefFragment *> listMeshes(ActorDefFragment *def);
    const QList<WLDMesh *> & meshes() const;

    WLDMesh *mainMesh() const;

    MeshBuffer * buffer() const;
    void setBuffer(MeshBuffer *newBuffer);

    WLDSkeleton *skeleton() const;
    void setSkeleton(WLDSkeleton *skeleton);

    WLDModelSkin *skin() const;
    const QMap<QString, WLDModelSkin *> & skins() const;

    WLDModelSkin * newSkin(QString name);

private:
    friend class WLDModelSkin;
    MeshBuffer *m_buffer;
    WLDSkeleton *m_skel;
    WLDModelSkin *m_skin;
    QMap<QString, WLDModelSkin *> m_skins;
    WLDMesh *m_mainMesh;
    QList<WLDMesh *> m_meshes;
};

/*!
  \brief Describes part of a model.
  */
class GAME_DLL WLDMesh
{
public:
    WLDMesh(MeshDefFragment *meshDef, uint32_t partID);
    virtual ~WLDMesh();

    MeshData * data() const;
    void setData(MeshData *data);
    MeshDefFragment *def() const;
    MaterialMap * materialMap() const;
    void setMaterialMap(MaterialMap *map);
    WLDMaterialPalette * palette() const;
    uint32_t partID() const;
    const AABox & boundsAA() const;

    WLDMaterialPalette * importPalette(PFSArchive *archive);
    MeshData * importFrom(MeshBuffer *meshBuf, uint32_t paletteOffset = 0);
    static MeshBuffer *combine(const QVector<WLDMesh *> &meshes);

private:
    void importVertexData(MeshBuffer *buffer, BufferSegment &dataLoc);
    void importIndexData(MeshBuffer *buffer, BufferSegment &indexLoc,
                         const BufferSegment &dataLoc, uint32_t offset, uint32_t count);
    MeshData * importMaterialGroups(MeshBuffer *buffer, uint32_t paletteOffset);
    
    uint32_t m_partID;
    MeshData *m_data;
    MeshDefFragment *m_meshDef;
    WLDMaterialPalette *m_palette;
    MaterialMap *m_materialMap;
    AABox m_boundsAA;
};

/*!
  \brief Describes one way an actor can be rendered. A skin can use alternative
  materials and meshes (e.g.\ for a character's head).
  */
class GAME_DLL WLDModelSkin
{
public:
    WLDModelSkin(QString name, WLDModel *model);
    virtual ~WLDModelSkin();

    QString name() const;
    
    const AABox & boundsAA() const;
    
    const QList<WLDMesh *> & parts() const;

    WLDMesh * addPart(MeshDefFragment *frag);
    void replacePart(WLDMesh *basePart, MeshDefFragment *frag);

    static bool explodeMeshName(QString defName, QString &actorName,
                                QString &meshName, QString &skinName);

    void draw(RenderProgram *prog, const QVector<BoneTransform> &bones,
              MaterialMap *materialMap);

private:
    void updateBounds();
    
    QString m_name;
    WLDModel *m_model;
    QList<WLDMesh *> m_parts;
    AABox m_boundsAA;
};

#endif
