#include "WLDActor.h"
#include "WLDModel.h"
#include "Fragments.h"
#include "RenderState.h"

WLDActor::WLDActor(WLDModel *model, QObject *parent) : QObject(parent)
{
    m_model = model;
    m_location = vec3(0.0, 0.0, 0.0);
    m_rotation = vec3(0.0, 0.0, 0.0);
    m_scale = vec3(1.0, 1.0, 1.0);
    m_animName = "POS";
    m_animTime = 0;
    m_palName = "00";
}

WLDActor::WLDActor(ActorFragment *frag, WLDModel *model, QObject *parent) : QObject(parent)
{
    m_model = model;
    if(frag)
    {
        m_location = frag->m_location;
        m_rotation = frag->m_rotation;
        m_scale = frag->m_scale;
    }
    else
    {
        m_location = vec3(0.0, 0.0, 0.0);
        m_rotation = vec3(0.0, 0.0, 0.0);
        m_scale = vec3(1.0, 1.0, 1.0);
    }
    m_animName = "POS";
    m_animTime = 0;
    m_palName = "00";
}

WLDActor::~WLDActor()
{
}

const vec3 & WLDActor::location() const
{
    return m_location;
}

WLDModel *WLDActor::model() const
{
    return m_model;
}

void WLDActor::setModel(WLDModel *newModel)
{
    m_model = newModel;
}

QString WLDActor::animName() const
{
    return m_animName;
}

void WLDActor::setAnimName(QString name)
{
    m_animName = name;
}

double WLDActor::animTime() const
{
    return m_animTime;
}

void WLDActor::setAnimTime(double newTime)
{
    m_animTime = newTime;
}

QString WLDActor::paletteName() const
{
    return m_palName;
}

void WLDActor::setPaletteName(QString palName)
{
    m_palName = palName;
}

bool WLDActor::addEquip(WLDActor::EquipSlot slot, WLDActor *actor)
{
    QString name = slotName(slot);
    if(name.isEmpty() || !m_model->skeleton())
        return false;
    WLDAnimation *anim = m_model->skeleton()->animations().value(m_animName);
    if(!anim)
        return false;
    int trackIndex = anim->findTrack(name);
    if(trackIndex < 0)
        return false;
    m_equip[slot] = QPair<int, WLDActor *>(trackIndex, actor);
    return true;
}

QString WLDActor::slotName(EquipSlot slot)
{
    switch(slot)
    {
    case Head:
        return "HEAD_POINT";
    case Guild:
        return "GUILD_POINT";
    case Left:
        return "L_POINT";
    case Right:
        return "R_POINT";
    case Shield:
        return "SHIELD_POINT";
    }
    return QString::null;
}

void WLDActor::draw(RenderState *state)
{
    WLDModelSkin *skin = m_model->skins().value(m_palName);
    if(!skin)
        return;
    QVector<BoneTransform> bones;
    if(m_model->skeleton())
    {
        WLDAnimation *anim = m_model->skeleton()->animations().value(m_animName);
        if(anim)
            bones = anim->transformationsAtTime(m_animTime);
    }
    state->pushMatrix();
    state->translate(m_location);
    state->rotate(m_rotation.x, 1.0, 0.0, 0.0);
    state->rotate(m_rotation.y, 0.0, 1.0, 0.0);
    state->rotate(m_rotation.z, 0.0, 0.0, 1.0);
    state->scale(m_scale);
    skin->draw(state, bones.constData(), (uint32_t)bones.count());
    foreach(ActorEquip eq, m_equip)
    {
        BoneTransform bone = bones.value(eq.first);
        state->pushMatrix();
        state->translate(bone.location.toVector3D());
        state->rotate(bone.rotation);
        eq.second->draw(state);
        state->popMatrix();
    }
    state->popMatrix();
}

////////////////////////////////////////////////////////////////////////////////

bool boxContains(const vec3 &p, const AABox &b)
{
    return (b.low.x <= p.x) && (p.x <= b.high.x)
        && (b.low.y <= p.y) && (p.y <= b.high.y)
        && (b.low.z <= p.z) && (p.z <= b.high.z);
}

ActorIndex::ActorIndex()
{
    m_root = 0;
}

ActorIndex::~ActorIndex()
{
    delete m_root;
}

ActorIndexNode *ActorIndex::root() const
{
    return m_root;
}

void ActorIndex::add(WLDActor *actor)
{
    if(!m_root)
    {
        //TODO provide boundaries somewhere
        vec3 low(-1e4, -1e4, -1e4), high(1e4, 1e4, 1e4);
        m_root = new ActorIndexNode(AABox(low, high));
    }
    m_root->add(actor);
}

////////////////////////////////////////////////////////////////////////////////

ActorIndexNode::ActorIndexNode(const AABox &bounds)
{
    for(int i = 0; i < 8; i++)
        m_children[i] = 0;
    m_leaf = true;
    m_bounds = bounds;
}

ActorIndexNode::~ActorIndexNode()
{
    if(!m_leaf)
    {
        for(int i = 0; i < 8; i++)
            delete m_children[i];
    }
}

QVector<WLDActor *> & ActorIndexNode::actors()
{
    return m_actors;
}

ActorIndexNode ** ActorIndexNode::children()
{
    return m_children;
}

const AABox & ActorIndexNode::bounds() const
{
    return m_bounds;
}

void ActorIndexNode::add(WLDActor *actor)
{
    if(!contains(actor->location()))
        return;
    else if(!m_leaf)
    {
        int childIndex = locate(actor->location());
        m_children[childIndex]->add(actor);
    }
    else if(m_actors.count() <= 20)
    {
        m_actors.append(actor);
    }
    else
    {
        // split node into 8 children
        vec3 l = m_bounds.low, c = m_bounds.center(), h = m_bounds.high;
        m_children[0] = new ActorIndexNode(AABox(vec3(l.x, l.y, l.z), vec3(c.x, c.y, c.z)));
        m_children[1] = new ActorIndexNode(AABox(vec3(l.x, l.y, c.z), vec3(c.x, c.y, h.z)));
        m_children[2] = new ActorIndexNode(AABox(vec3(l.x, c.y, l.z), vec3(c.x, h.y, c.z)));
        m_children[3] = new ActorIndexNode(AABox(vec3(l.x, c.y, c.z), vec3(c.x, h.y, h.z)));
        m_children[4] = new ActorIndexNode(AABox(vec3(c.x, l.y, l.z), vec3(h.x, c.y, c.z)));
        m_children[5] = new ActorIndexNode(AABox(vec3(c.x, l.y, c.z), vec3(h.x, c.y, h.z)));
        m_children[6] = new ActorIndexNode(AABox(vec3(c.x, c.y, l.z), vec3(h.x, h.y, c.z)));
        m_children[7] = new ActorIndexNode(AABox(vec3(c.x, c.y, c.z), vec3(h.x, h.y, h.z)));
        m_leaf = false;

        // add actors to the children nodes
        foreach(WLDActor *a, m_actors)
        {
            int childIndex = locate(a->location());
            m_children[childIndex]->add(a);
        }
        m_actors.clear();
        add(actor);
    }
}

int ActorIndexNode::locate(const vec3 &pos) const
{
    vec3 center = m_bounds.center();
    if(pos.x < center.x)
    {
        if(pos.y < center.y)
            return (pos.z < center.z) ? 0 : 1;
        else
            return (pos.z < center.z) ? 2 : 3;
    }
    else
    {
        if(pos.y < center.y)
            return (pos.z < center.z) ? 4 : 5;
        else
            return (pos.z < center.z) ? 6 : 7;
    }
}

bool ActorIndexNode::contains(const vec3 &pos) const
{
    return boxContains(pos, m_bounds);
}
