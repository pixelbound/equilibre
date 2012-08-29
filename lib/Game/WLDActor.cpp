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

#include <math.h>
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Render/RenderState.h"

WLDActor::WLDActor(ActorType type)
{
    m_type = type;
}

WLDActor::~WLDActor()
{
}

const vec3 & WLDActor::location() const
{
    return m_location;
}

const AABox & WLDActor::boundsAA() const
{
    return m_boundsAA;
}

WLDActor::ActorType WLDActor::type() const
{
    return m_type;
}

////////////////////////////////////////////////////////////////////////////////

WLDStaticActor::WLDStaticActor(ActorFragment *frag, WLDMesh *simpleModel) : WLDActor(Static)
{
    m_simpleModel = simpleModel;
    m_frag = frag;
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
    update();
}

WLDStaticActor::~WLDStaticActor()
{
}

WLDMesh * WLDStaticActor::simpleModel() const
{
    return m_simpleModel;
}

const matrix4 & WLDStaticActor::modelMatrix() const
{
    return m_modelMatrix;
}

const BufferSegment & WLDStaticActor::colorSegment() const
{
    return m_colorSegment;
}

void WLDStaticActor::update()
{
    m_boundsAA = m_simpleModel->boundsAA();
    m_boundsAA.scale(m_scale);
    m_boundsAA.rotate(m_rotation);
    m_boundsAA.translate(m_location);
    
    m_modelMatrix.setIdentity();
    m_modelMatrix = m_modelMatrix
            * matrix4::translate(m_location.x, m_location.y, m_location.z)
            * matrix4::rotate(m_rotation.x, 1.0, 0.0, 0.0)
            * matrix4::rotate(m_rotation.y, 0.0, 1.0, 0.0)
            * matrix4::rotate(m_rotation.z, 0.0, 0.0, 1.0)
            * matrix4::scale(m_scale.x, m_scale.y, m_scale.z);
}

void WLDStaticActor::importColorData(MeshBuffer *meshBuf)
{
    // XXX handle complex models here.
    if(!!m_frag || !m_frag->m_lighting || !m_frag->m_lighting->m_def)
        return;
    const QVector<QRgb> &colors = m_frag->m_lighting->m_def->m_colors;
    m_colorSegment.offset = meshBuf->colors.count();
    m_colorSegment.count = colors.count();
    m_colorSegment.elementSize = sizeof(uint32_t);
    for(int i = 0; i < m_colorSegment.count; i++)
        meshBuf->colors.append(colors[i]);
}

////////////////////////////////////////////////////////////////////////////////

WLDCharActor::WLDCharActor(WLDModel *model) : WLDActor(Character)
{
    m_complexModel = model;
    m_frag = NULL;
    m_location = vec3(0.0, 0.0, 0.0);
    m_rotation = vec3(0.0, 0.0, 0.0);
    m_scale = vec3(1.0, 1.0, 1.0);
    m_animName = "POS";
    m_animTime = 0;
    m_palName = "00";
}

WLDCharActor::WLDCharActor(ActorFragment *frag, WLDModel *model) : WLDActor(Character)
{
    m_complexModel = model;
    m_frag = frag;
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

WLDCharActor::~WLDCharActor()
{
}

WLDModel * WLDCharActor::complexModel() const
{
    return m_complexModel;
}

QString WLDCharActor::animName() const
{
    return m_animName;
}

void WLDCharActor::setAnimName(QString name)
{
    m_animName = name;
}

double WLDCharActor::animTime() const
{
    return m_animTime;
}

void WLDCharActor::setAnimTime(double newTime)
{
    m_animTime = newTime;
}

QString WLDCharActor::paletteName() const
{
    return m_palName;
}

void WLDCharActor::setPaletteName(QString palName)
{
    m_palName = palName;
}

bool WLDCharActor::addEquip(WLDCharActor::EquipSlot slot, WLDMesh *mesh, MaterialMap *materials)
{
    QString name = slotName(slot);
    if(name.isEmpty() || !m_complexModel->skeleton())
        return false;
    WLDAnimation *anim = m_complexModel->skeleton()->animations().value(m_animName);
    if(!anim)
        return false;
    int trackIndex = anim->findTrack(name);
    if(trackIndex < 0)
        return false;
    ActorEquip eq;
    eq.TrackID = trackIndex;
    eq.Mesh = mesh;
    eq.Materials = materials;
    m_equip[slot] = eq;
    return true;
}

QString WLDCharActor::slotName(EquipSlot slot)
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

void WLDCharActor::draw(RenderState *state)
{
    if(!m_complexModel)
        return;
    WLDModelSkin *skin = m_complexModel->skins().value(m_palName);
    if(!skin)
        return;
    QVector<BoneTransform> bones;
    if(m_complexModel->skeleton())
    {
        WLDAnimation *anim = m_complexModel->skeleton()->animations().value(m_animName);
        if(anim)
            bones = anim->transformationsAtTime(m_animTime);
    }
    
    state->pushMatrix();
    state->multiplyMatrix(m_modelMatrix);
    // XXX drawEquip method to minimize program changes
    if(bones.count() > 0)
        state->setRenderMode(RenderState::Skinning);
    else
        state->setRenderMode(RenderState::Basic);
    skin->draw(state, bones.constData(), (uint32_t)bones.count());
    state->setRenderMode(RenderState::Basic);
    foreach(ActorEquip eq, m_equip)
    {
        state->pushMatrix();
        BoneTransform bone = bones.value(eq.TrackID);
        state->translate(bone.location.toVector3D());
        state->rotate(bone.rotation);
        MeshBuffer *meshBuf = eq.Mesh->data()->buffer;
        meshBuf->matGroups.clear();
        meshBuf->addMaterialGroups(eq.Mesh->data());
        state->beginDrawMesh(meshBuf, eq.Materials);
        state->drawMesh();
        state->endDrawMesh();
        state->popMatrix();
    }
    state->popMatrix();
}

////////////////////////////////////////////////////////////////////////////////

OctreeIndex::OctreeIndex(AABox bounds, int maxDepth)
{
    // Convert the bounds to a cube.
    float cubeLow = qMin(bounds.low.x, qMin(bounds.low.y, bounds.low.z));
    float cubeHigh = qMax(bounds.high.x, qMax(bounds.high.y, bounds.high.z));
    AABox cubeBounds(vec3(cubeLow, cubeLow, cubeLow), vec3(cubeHigh, cubeHigh, cubeHigh));
    m_root = new Octree(cubeBounds, this);
    m_maxDepth = maxDepth;
}

Octree * OctreeIndex::add(WLDActor *actor)
{
    AABox actorBounds = actor->boundsAA();
    int x = 0, y = 0, z = 0, depth = 0;
    findIdealInsertion(actorBounds, x, y, z, depth);
    Octree *octant = findBestFittingOctant(x, y, z, depth);
#ifndef NDEBUG
    vec3 actorCenter = actorBounds.center();
    AABox octantSBounds = octant->strictBounds();
    AABox octantLBounds = octant->looseBounds();
    Q_ASSERT(octantSBounds.contains(actorCenter));
    Q_ASSERT(octantLBounds.contains(actorBounds));
#endif
    octant->actors().append(actor);
    return octant;
}

void OctreeIndex::findVisible(QVector<WLDActor *> &objects, const Frustum &f, bool cull)
{
    findVisible(objects, m_root, f, cull);
}

void OctreeIndex::findVisible(QVector<WLDActor *> &objects, Octree *octant, const Frustum &f, bool cull)
{
    if(!octant)
        return;
    Frustum::TestResult r = cull ? f.containsAABox(octant->looseBounds()) : Frustum::INSIDE;
    if(r == Frustum::OUTSIDE)
        return;
    cull = (r != Frustum::INSIDE);
    for(int i = 0; i < 8; i++)
        findVisible(objects, octant->child(i), f, cull);
    foreach(WLDActor *actor, octant->actors())
    {
        if((r == Frustum::INSIDE) || (f.containsAABox(actor->boundsAA()) != Frustum::OUTSIDE))
            objects.append(actor);
    }
}

void OctreeIndex::findIdealInsertion(AABox bb, int &x, int &y, int &z, int &depth)
{
    // Determine the maximum depth at which the bounds fit the octant.
    AABox sb = m_root->strictBounds();
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

Octree * OctreeIndex::findBestFittingOctant(int x, int y, int z, int depth)
{
    Octree *octant = m_root;
    int highBit = 1 << (depth - 1);
    for(int i = 0; i < qMin(depth, m_maxDepth); i++)
    {
        // Determine the octant at this depth using the index's current high bit.
        int localX = (x & highBit) > 0;
        int localY = (y & highBit) > 0;
        int localZ = (z & highBit) > 0;
        int childIndex = localX + (localY << 1) + (localZ << 2);
        Q_ASSERT(childIndex >= 0 && childIndex <= 7);
        Octree *newOctant = octant->child(childIndex);
        if(!newOctant)
        {
            // Create children octants as needed.
            newOctant = octant->createChild(childIndex);
            Q_ASSERT(newOctant);
        }
        octant = newOctant;
        highBit >>= 1;
    }
    return octant;
}

////////////////////////////////////////////////////////////////////////////////

Octree::Octree(AABox bounds, OctreeIndex *index)
{
    m_bounds = bounds;
    m_index = index;
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

const QVector<WLDActor *> & Octree::actors() const
{
    return m_actors;
}

QVector<WLDActor *> & Octree::actors()
{
    return m_actors;
}

Octree *Octree::child(int index) const
{
    return ((index >= 0) && (index < 8)) ? m_children[index] : NULL;
}

Octree * Octree::createChild(int index)
{
    Octree *octant = NULL;
    vec3 l = m_bounds.low, c = m_bounds.center(), h = m_bounds.high;
    switch(index)
    {
    case 0: // x = 0, y = 0, z = 0
        octant = new Octree(AABox(vec3(l.x, l.y, l.z), vec3(c.x, c.y, c.z)), m_index);
        break;
    case 1: // x = 1, y = 0, z = 0
        octant = new Octree(AABox(vec3(c.x, l.y, l.z), vec3(h.x, c.y, c.z)), m_index);
        break;
    case 2: // x = 0, y = 1, z = 0
        octant = new Octree(AABox(vec3(l.x, c.y, l.z), vec3(c.x, h.y, c.z)), m_index);
        break;
    case 3: // x = 1, y = 1, z = 0
        octant = new Octree(AABox(vec3(c.x, c.y, l.z), vec3(h.x, h.y, c.z)), m_index);
        break;
    case 4: // x = 0, y = 0, z = 1
        octant = new Octree(AABox(vec3(l.x, l.y, c.z), vec3(c.x, c.y, h.z)), m_index);
        break;
    case 5: // x = 1, y = 0, z = 1
        octant = new Octree(AABox(vec3(c.x, l.y, c.z), vec3(h.x, c.y, h.z)), m_index);
        break;
    case 6: // x = 0, y = 1, z = 1
        octant = new Octree(AABox(vec3(l.x, c.y, c.z), vec3(c.x, h.y, h.z)), m_index);
        break;
    case 7: // x = 1, y = 1, z = 1
        octant = new Octree(AABox(vec3(c.x, c.y, c.z), vec3(h.x, h.y, h.z)), m_index);
        break;
    }
    if(octant)
        m_children[index] = octant;
    return octant;
}
