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

#ifndef EQUILIBRE_WLD_ACTOR_H
#define EQUILIBRE_WLD_ACTOR_H

#include <QObject>
#include <QMap>
#include <QPair>
#include "Newton.h"
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"
#include "EQuilibre/Render/RenderContext.h"

class PFSArchive;
class ActorFragment;
class LightSourceFragment;
class WLDModel;
class WLDMesh;
class WLDActor;
class WLDAnimation;
class Game;
class MaterialMap;
class RenderContext;
class Octree;
class OctreeIndex;
class Zone;

class ActorEquip
{
public:
    int TrackID;
    WLDMesh *Mesh;
    MaterialArray *Materials;
};

/*!
  \brief Describes an instance of an entity that can be referenced by a spatial index.
  */
class GAME_DLL WLDActor
{
public:
    enum ActorType
    {
        Static = 0,
        Character,
        LightSource
    };
    
    WLDActor(ActorType type);
    virtual ~WLDActor();

    const vec3 & location() const;
    const AABox & boundsAA() const;
    ActorType type() const;
    
    template<typename T>
    T * cast()
    {
        if(m_type == T::Kind)
            return static_cast<T *>(this);
        else
            return 0;
    }
    
    template<typename T>
    const T * cast() const 
    {
        if(m_type == T::Kind)
            return static_cast<const T *>(this);
        else
            return 0;
    }
    
protected:
    vec3 m_location;
    AABox m_boundsAA;
    ActorType m_type;
};

/*!
  \brief Describes an instance of a static model, like a placeable object or terrain part.
  */
class GAME_DLL WLDStaticActor : public WLDActor
{
public:
    WLDStaticActor(ActorFragment *frag, WLDMesh *mesh);
    virtual ~WLDStaticActor();
    const static ActorType Kind = Static;

    const matrix4 & modelMatrix() const;
    WLDMesh * mesh() const;
    ActorFragment * frag() const;
    const BufferSegment & colorSegment() const;

    void update();
    void importColorData(MeshBuffer *meshBuf);
    
private:
    ActorFragment *m_frag;
    vec3 m_rotation, m_scale;
    matrix4 m_modelMatrix;
    WLDMesh *m_mesh;
    BufferSegment m_colorSegment;
};

/*!
  \brief Describes an instance of a character model.
  */
class GAME_DLL WLDCharActor : public WLDActor
{
public:
    WLDCharActor(Game *game, WLDModel *model);
    virtual ~WLDCharActor();
    const static ActorType Kind = Character;
    
    Game * game() const;
    Zone * zone() const;
    
    WLDModel * model() const;
    void setModel(WLDModel *newModel);
    
    float capsuleHeight() const;
    float capsuleRadius() const;
    NewtonCollision * shape() const;
    
    float runSpeed() const;
    
    void setLocation(const vec3 &newLocation);
    
    // x and z angles that describe where the character is looking at.
    vec3 lookOrient() const;
    void setLookOrientX(float newOrientation);
    void setLookOrientZ(float newOrientation);
    
    float cameraDistance() const;
    void setCameraDistance(float dist);
    
    bool hasCamera() const;
    void setHasCamera(bool camera);
    
    MaterialMap * materialMap() const;
    
    WLDAnimation * animation() const;
    void setAnimation(WLDAnimation *newAnim);

    double animTime() const;
    void setAnimTime(double newTime);

    // XXX use integer
    QString paletteName() const;
    void setPaletteName(QString palName);

    enum EquipSlot
    {
        Head,
        Guild,
        Left,
        Right,
        Shield
    };

    bool addEquip(EquipSlot slot, WLDMesh *actor, MaterialArray *materials);
    WLDAnimation * findAnimation(QString animName);
    void setSkin(uint32_t skinID);
    void draw(RenderContext *renderCtx, RenderProgram *prog);
    
    void enterZone(Zone *newZone, const vec3 &initialPos);
    void update(double currentTime);
    
    void calculateStep(vec3 &position, float distSideways, float distForward,
                       bool ghost);
    void calculateViewFrustum(Frustum &frustum) const;

private:
    static QString slotName(EquipSlot slot);

    vec3 m_rotation, m_scale;
    bool m_hasCamera;
    float m_lookOrientX;
    float m_lookOrientZ;
    float m_cameraDistance;
    float m_runSpeed;
    Game *m_game;
    Zone *m_zone;
    WLDModel *m_model;
    WLDAnimation *m_animation;
    WLDAnimation *m_idleAnim;
    WLDAnimation *m_runningAnim;
    double m_startAnimationTime;
    double m_animTime;
    QString m_palName;
    MaterialMap *m_materialMap; // Slot ID -> Material ID in MaterialArray
    QMap<EquipSlot, ActorEquip> m_equip;
    NewtonCollision *m_shape;
    float m_capsuleHeight;
    float m_capsuleRadius;
};

/*!
  \brief Describes an instance of a light source.
  */
class GAME_DLL WLDLightActor : public WLDActor
{
public:
    WLDLightActor(LightSourceFragment *frag, uint16_t lightID);
    const static ActorType Kind = LightSource;
   
    uint16_t lightID() const;
    const LightParams & params() const;
    
private:
    LightSourceFragment *m_frag;
    uint16_t m_lightID;
    LightParams m_params;
};

typedef void (*OctreeCallback)(WLDActor *actor, void *user);

class GAME_DLL OctreeIndex
{
public:
    OctreeIndex(AABox bounds, int maxDepth=5);
    Octree * add(WLDActor *actor);
    void findVisible(const Frustum &f, OctreeCallback callback, void *user, bool cull);
    void findVisible(const Sphere &s, OctreeCallback callback, void *user, bool cull);
    void findIdealInsertion(AABox bb, int &x, int &y, int &z, int &depth);
    Octree * findBestFittingOctant(int x, int y, int z, int depth);
    
private:
    void findVisible(const Frustum &f, Octree *octant, OctreeCallback callback, void *user, bool cull);
    void findVisible(const Sphere &f, Octree *octant, OctreeCallback callback, void *user, bool cull);
    
    Octree *m_root;
    int m_maxDepth;
};

class GAME_DLL Octree
{
public:
    Octree(AABox bounds, OctreeIndex *index);
    const AABox & strictBounds() const;
    AABox looseBounds() const;
    const QVector<WLDActor *> & actors() const;
    QVector<WLDActor *> & actors();
    Octree *child(int index) const;
    Octree *createChild(int index);
    
private:
    AABox m_bounds;
    OctreeIndex *m_index;
    Octree *m_children[8];
    QVector<WLDActor *> m_actors;
};

#endif
