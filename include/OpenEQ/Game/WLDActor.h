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
class WLDActor : public QObject
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
class WLDZoneActor
{
public:
    WLDZoneActor(ActorFragment *frag, WLDMesh *mesh);

    vec3 location, rotation, scale;
    AABox boundsAA;
    WLDMesh *mesh;
};

class ActorIndex;

class ActorIndexNode
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
    int locate(const vec3 &pos) const;

    //TODO optimize to: int count, union { ActorIndexNode *, WLDActor * }
    bool m_leaf;
    ActorIndexNode *m_children[8];
    QVector<uint16_t> m_actors;
    AABox m_bounds;
};

class ActorIndex
{
public:
    ActorIndex();
    virtual ~ActorIndex();

    const QList<WLDZoneActor> & actors() const;
    ActorIndexNode *root() const;

    void add(const WLDZoneActor &actor);
    void clear();

private:
    ActorIndexNode *m_root;
    QList<WLDZoneActor> m_actors;
};

#endif
