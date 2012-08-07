#ifndef OPENEQ_WLD_ACTOR_H
#define OPENEQ_WLD_ACTOR_H

#include <QObject>
#include <QMap>
#include <QPair>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/Vertex.h"

class PFSArchive;
class ActorFragment;
class WLDModel;
class WLDMesh;
class WLDActor;
class RenderState;

typedef QPair<int, WLDActor *> ActorEquip;

/*!
  \brief Describes an instance of a model (such as an object or a character).
  */
class GAME_DLL WLDActor : public QObject
{
public:
    WLDActor(WLDModel *model, QObject *parent = 0);
    WLDActor(ActorFragment *frag, WLDModel *model, QObject *parent = 0);
    virtual ~WLDActor();

    const vec3 & location() const;

    WLDModel *model() const;
    void setModel(WLDModel *newModel);

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

    bool addEquip(EquipSlot slot, WLDActor *actor);

    void draw(RenderState *state);

private:
    static QString slotName(EquipSlot slot);

    ActorFragment *m_frag;
    vec3 m_location, m_rotation, m_scale;
    WLDModel *m_model;
    QString m_animName;
    double m_animTime;
    QString m_palName;
    QMap<EquipSlot, ActorEquip> m_equip;
};

/*!
  \brief Describes an instance of a zone object.
  */
class GAME_DLL WLDZoneActor
{
public:
    WLDZoneActor(ActorFragment *frag, WLDMesh *mesh);

    vec3 location, rotation, scale;
    AABox boundsAA;
    WLDMesh *mesh;
};

class ActorIndex;

class GAME_DLL ActorIndexNode
{
public:
    ActorIndexNode(const AABox &bounds);
    ~ActorIndexNode();

    QVector<uint16_t> & actors();
    ActorIndexNode ** children();
    const AABox & bounds() const;
    void add(uint16_t index, const vec3 &pos, ActorIndex *tree);
    bool contains(const vec3 &pos) const;

private:
    void split(ActorIndex *tree);
    int locate(const vec3 &pos) const;

    //TODO optimize to: int count, union { ActorIndexNode *, WLDActor * }
    bool m_leaf;
    ActorIndexNode *m_children[8];
    QVector<uint16_t> m_actors;
    AABox m_bounds;
};

class GAME_DLL ActorIndex
{
public:
    ActorIndex();
    virtual ~ActorIndex();

    const QList<WLDZoneActor> & actors() const;
    ActorIndexNode *root() const;

    void add(const WLDZoneActor &actor);
    void clear();
    void findVisible(QVector<const WLDZoneActor *> &objects, const Frustum &f, bool cull);

private:
    void findVisible(QVector<const WLDZoneActor *> &objects, ActorIndexNode *node, const Frustum &f, bool cull);
    
    ActorIndexNode *m_root;
    QList<WLDZoneActor> m_actors;
};

class Octree;

class GAME_DLL OctreeIndex
{
public:
    OctreeIndex(AABox bounds, int maxDepth=5);
    Octree * add(WLDZoneActor *actor);
    void findVisible(QVector<const WLDZoneActor *> &objects, const Frustum &f, bool cull);
    void findIdealInsertion(AABox bb, int &x, int &y, int &z, int &depth);
    Octree * findBestFittingOctant(int x, int y, int z, int depth);
    
private:
    void findVisible(QVector<const WLDZoneActor *> &objects, Octree *octant, const Frustum &f, bool cull);
    
    Octree *m_root;
    int m_maxDepth;
};

class GAME_DLL Octree
{
public:
    Octree(AABox bounds, OctreeIndex *index);
    const AABox & strictBounds() const;
    AABox looseBounds() const;
    const QVector<WLDZoneActor *> & actors() const;
    QVector<WLDZoneActor *> & actors();
    Octree *child(int index) const;
    Octree *createChild(int index);
    
private:
    AABox m_bounds;
    OctreeIndex *m_index;
    Octree *m_children[8];
    QVector<WLDZoneActor *> m_actors;
};

#endif
