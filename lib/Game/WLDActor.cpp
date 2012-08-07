#include <math.h>
#include "OpenEQ/Game/WLDActor.h"
#include "OpenEQ/Game/WLDModel.h"
#include "OpenEQ/Game/Fragments.h"
#include "OpenEQ/Render/RenderState.h"

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
    // XXX drawEquip method to minimize program changes
    if(bones.count() > 0)
        state->setRenderMode(RenderState::Skinning);
    else
        state->setRenderMode(RenderState::Basic);
    skin->draw(state, bones.constData(), (uint32_t)bones.count());
    state->setRenderMode(RenderState::Basic);
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

WLDZoneActor::WLDZoneActor(ActorFragment *frag, WLDMesh *mesh)
{
    this->mesh = mesh;
    this->boundsAA = mesh->boundsAA();
    if(frag)
    {
        this->location = frag->m_location;
        this->rotation = frag->m_rotation;
        this->scale = frag->m_scale;
        this->boundsAA.scale(this->scale);
        this->boundsAA.rotate(this->rotation);
        this->boundsAA.translate(this->location);
    }
    else
    {
        this->location = vec3(0.0, 0.0, 0.0);
        this->rotation = vec3(0.0, 0.0, 0.0);
        this->scale = vec3(1.0, 1.0, 1.0);
    }
}

////////////////////////////////////////////////////////////////////////////////

ActorIndex::ActorIndex()
{
    m_root = 0;
}

ActorIndex::~ActorIndex()
{
    clear();
}

const QList<WLDZoneActor> & ActorIndex::actors() const
{
    return m_actors;
}

ActorIndexNode *ActorIndex::root() const
{
    return m_root;
}

void ActorIndex::add(const WLDZoneActor &actor)
{
    if(!m_root)
    {
        //TODO provide boundaries somewhere
        vec3 low(-1e4, -1e4, -1e4), high(1e4, 1e4, 1e4);
        m_root = new ActorIndexNode(AABox(low, high));
    }
    uint16_t index = (uint16_t)m_actors.count();
    m_actors.append(actor);
    m_root->add(index, actor.location, this);
}

void ActorIndex::clear()
{
    delete m_root;
    m_root = 0;
    m_actors.clear();
}

void ActorIndex::findVisible(QVector<const WLDZoneActor *> &objects, const Frustum &f, bool cull)
{
    findVisible(objects, m_root, f, cull);
}

void ActorIndex::findVisible(QVector<const WLDZoneActor *> &objects, ActorIndexNode *node, const Frustum &f, bool cull)
{
    if(!node)
        return;
    Frustum::TestResult r = cull ? f.containsAABox(node->bounds()) : Frustum::INSIDE;
    if(r == Frustum::OUTSIDE)
        return;
    cull = (r != Frustum::INSIDE);
    for(int i = 0; i < 8; i++)
        findVisible(objects, node->children()[i], f, cull);
    foreach(int actorIndex, node->actors())
        objects.append(&m_actors[actorIndex]);
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

QVector<uint16_t> & ActorIndexNode::actors()
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

void ActorIndexNode::add(uint16_t index, const vec3 &pos, ActorIndex *tree)
{
    if(!contains(pos))
        return;
    else if(!m_leaf)
    {
        int childIndex = locate(pos);
        m_children[childIndex]->add(index, pos, tree);
    }
    else if(m_actors.count() <= 20)
    {
        m_actors.append(index);
    }
    else
    {
        split(tree);
        add(index, pos, tree);
    }
}

void ActorIndexNode::split(ActorIndex *tree)
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
    const QList<WLDZoneActor> &actors = tree->actors();
    foreach(int actorIndex, m_actors)
    {
        vec3 pos = actors[actorIndex].location;
        int childIndex = locate(pos);
        m_children[childIndex]->add(actorIndex, pos, tree);
    }
    m_actors.clear();
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
    return m_bounds.contains(pos);
}

////////////////////////////////////////////////////////////////////////////////

Octree::Octree(AABox bounds, Octree *root)
{
    m_bounds = bounds;
    m_root = root ? root : this;
    for(int i = 0; i < 8; i++)
        m_children[i] = NULL;
}

const AABox & Octree::strictBounds() const
{
    return m_bounds;
}

AABox Octree::looseBounds() const
{
    AABox loose = m_bounds;
    loose.scaleCenter(2.0f);
    return loose;
}

void Octree::add(WLDZoneActor *actor)
{
    int x = 0, y = 0, z = 0, depth = 0;
    m_root->findIdealInsertion(actor->boundsAA, x, y, z, depth);
    Octree *octant = findBestFittingOctant(m_root, x, y, z, depth);
    octant->addInternal(actor);
}

void Octree::addInternal(WLDZoneActor *actor)
{
    m_actors.append(actor);
    if(!m_children[0] && (m_actors.count() >= 20))
        split();
}

void Octree::split()
{
    // Create children octants.
    vec3 l = m_bounds.low, c = m_bounds.center(), h = m_bounds.high;
    m_children[0] = new Octree(AABox(vec3(l.x, l.y, l.z), vec3(c.x, c.y, c.z)), m_root);
    m_children[1] = new Octree(AABox(vec3(l.x, l.y, c.z), vec3(c.x, c.y, h.z)), m_root);
    m_children[2] = new Octree(AABox(vec3(l.x, c.y, l.z), vec3(c.x, h.y, c.z)), m_root);
    m_children[3] = new Octree(AABox(vec3(l.x, c.y, c.z), vec3(c.x, h.y, h.z)), m_root);
    m_children[4] = new Octree(AABox(vec3(c.x, l.y, l.z), vec3(h.x, c.y, c.z)), m_root);
    m_children[5] = new Octree(AABox(vec3(c.x, l.y, c.z), vec3(h.x, c.y, h.z)), m_root);
    m_children[6] = new Octree(AABox(vec3(c.x, c.y, l.z), vec3(h.x, h.y, c.z)), m_root);
    m_children[7] = new Octree(AABox(vec3(c.x, c.y, c.z), vec3(h.x, h.y, h.z)), m_root);
    
    // Try to insert actors in children octants.
    for(int i = m_actors.count() - 1; i >= 0; i--)
    {
        WLDZoneActor *actor = m_actors[i];
        int x = 0, y = 0, z = 0, depth = 0;
        m_root->findIdealInsertion(actor->boundsAA, x, y, z, depth);
        Octree *octant = findBestFittingOctant(m_root, x, y, z, depth);
        if(octant != this)
        {
            octant->add(actor);
            m_actors.remove(i);
        }
    }
}

void Octree::findIdealInsertion(AABox bb, int &x, int &y, int &z, int &depth)
{
    // Determine the maximum depth at which the bounds fit the octant.
    AABox sb = strictBounds();
    float sbRadius = (sb.high.x - sb.low.x);
    vec3 bbExtent = (bb.high - bb.low) * 0.5f;
    float bbRadius = qMax(bbExtent.x, qMax(bbExtent.y, bbExtent.z));
    depth = 0;
    while(true)
    {
        sbRadius *= 0.5f;
        if(sbRadius < bbRadius)
            break;
        depth++;
    }
    depth--;
    sbRadius *= 2.0f;
    
    // Get the index of the node at this depth.
    vec3 bbCenter = bb.center() - sb.low;
    vec3 sbSize = (sb.high - sb.low);
    int scale = 1 << depth;
    x = (int)floor((scale * bbCenter.x) / sbSize.x);
    y = (int)floor((scale * bbCenter.y) / sbSize.y);
    z = (int)floor((scale * bbCenter.z) / sbSize.z);
    Q_ASSERT(x >= 0 && x < scale);
    Q_ASSERT(y >= 0 && y < scale);
    Q_ASSERT(z >= 0 && z < scale);
}

Octree * Octree::findBestFittingOctant(Octree *root, int x, int y, int z, int depth)
{
    Octree *octant = root;
    int highBit = 1 << (depth - 1);
    for(int i = 0; i < depth; i++)
    {
        if(!octant->m_children[0])
        {
            // Octant not split.
            return octant;
        }
        else
        {
            // Determine the octant at this depth using the index's current high bit.
            int localX = (x & highBit) > 0;
            int localY = (y & highBit) > 0;
            int localZ = (z & highBit) > 0;
            int childIndex = localX + (localY << 1) + (localZ << 2);
            Q_ASSERT(childIndex >= 0 && childIndex <= 7);
            octant = octant->m_children[childIndex];
            highBit >>= 1;
        }
    }
    return octant;
}

void Octree::findVisible(QVector<const WLDZoneActor *> &objects, const Frustum &f, bool cull)
{
    findVisible(objects, m_root, f, cull);
}

void Octree::findVisible(QVector<const WLDZoneActor *> &objects, Octree *octant, const Frustum &f, bool cull)
{
    if(!octant)
        return;
    Frustum::TestResult r = cull ? f.containsAABox(octant->looseBounds()) : Frustum::INSIDE;
    if(r == Frustum::OUTSIDE)
        return;
    cull = (r != Frustum::INSIDE);
    if(octant->m_children[0])
    {
        for(int i = 0; i < 8; i++)
            findVisible(objects, octant->m_children[i], f, cull);
    }
    foreach(WLDZoneActor *actor, octant->m_actors)
        objects.append(actor);
}
