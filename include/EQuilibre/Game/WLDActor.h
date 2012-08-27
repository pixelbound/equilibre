#ifndef EQUILIBRE_WLD_ACTOR_H
#define EQUILIBRE_WLD_ACTOR_H

#include <QObject>
#include <QMap>
#include <QPair>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"

class PFSArchive;
class ActorFragment;
class WLDModel;
class WLDMesh;
class WLDActor;
class RenderState;

class ActorEquip
{
public:
    int TrackID;
    WLDMesh *Mesh;
    MaterialMap *Materials;
};

/*!
  \brief Describes an instance of a model (such as an object or a character).
  */
class GAME_DLL WLDActor : public QObject
{
public:
    WLDActor(ActorFragment *frag, WLDMesh *simpleModel, QObject *parent = 0);
    WLDActor(WLDModel *complexModel, QObject *parent = 0);
    WLDActor(ActorFragment *frag, WLDModel *complexModel, QObject *parent = 0);
    virtual ~WLDActor();

    const vec3 & location() const;
    
    const AABox & boundsAA() const;
    const matrix4 & modelMatrix() const;
    
    enum ModelType
    {
        Simple = 0,
        Complex
    };

    WLDModel * complexModel() const;
    WLDMesh * simpleModel() const;
    ModelType type() const;

    QString animName() const;
    void setAnimName(QString name);

    double animTime() const;
    void setAnimTime(double newTime);

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

    bool addEquip(EquipSlot slot, WLDMesh *actor, MaterialMap *materials);
    
    void update();

    void draw(RenderState *state);

private:
    static QString slotName(EquipSlot slot);

    ActorFragment *m_frag;
    vec3 m_location, m_rotation, m_scale;
    matrix4 m_modelMatrix;
    AABox m_boundsAA;
    WLDModel *m_complexModel;
    WLDMesh *m_simpleModel;
    ModelType m_type;
    QString m_animName;
    double m_animTime;
    QString m_palName;
    QMap<EquipSlot, ActorEquip> m_equip;
};

class Octree;

class GAME_DLL OctreeIndex
{
public:
    OctreeIndex(AABox bounds, int maxDepth=5);
    Octree * add(WLDActor *actor);
    void findVisible(QVector<WLDActor *> &objects, const Frustum &f, bool cull);
    void findIdealInsertion(AABox bb, int &x, int &y, int &z, int &depth);
    Octree * findBestFittingOctant(int x, int y, int z, int depth);
    
private:
    void findVisible(QVector<WLDActor *> &objects, Octree *octant, const Frustum &f, bool cull);
    
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
