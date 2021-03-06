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
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"

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

WLDStaticActor::WLDStaticActor(ActorFragment *frag, WLDMesh *simpleModel) : WLDActor(Kind)
{
    m_mesh = simpleModel;
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

WLDMesh * WLDStaticActor::mesh() const
{
    return m_mesh;
}

ActorFragment * WLDStaticActor::frag() const
{
    return m_frag;
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
    m_boundsAA = m_mesh->boundsAA();
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
    if(!m_frag || !m_frag->m_lighting || !m_frag->m_lighting->m_def)
        return;
    const QVector<QRgb> &colors = m_frag->m_lighting->m_def->m_colors;
    m_colorSegment.offset = meshBuf->colors.count();
    m_colorSegment.count = colors.count();
    m_colorSegment.elementSize = sizeof(uint32_t);
    for(int i = 0; i < m_colorSegment.count; i++)
        meshBuf->colors.append(colors[i]);
}

////////////////////////////////////////////////////////////////////////////////

WLDCharActor::WLDCharActor(Game *game) : WLDActor(Kind)
{
    m_game = game;
    m_zone = NULL;
    m_model = NULL;
    m_materialMap = NULL;
    m_animation = NULL;
    m_idleAnim = NULL;
    m_walkingAnim = NULL;
    m_runningAnim = NULL;
    m_jumpingAnim = NULL;
    m_location = vec3(0.0, 0.0, 0.0);
    m_rotation = vec3(0.0, 0.0, 0.0);
    m_scale = vec3(1.0, 1.0, 1.0);
    m_walkSpeed = 10.0f;
    m_runSpeed = 25.0f;
    m_movementStateX = m_movementStateY = 0;
    m_hasCamera = false;
    m_cameraDistance = 0.0f;
    m_lookOrientX = m_lookOrientZ = 0.0f;
    m_moveMode = Running;
    m_wantsToJump = false;
    m_jumping = false;
    m_jumpTime = 0.0f;
    m_animTime = 0.0f;
    m_startAnimationTime = 0.0f;
    m_palName = "00";
    m_shape = NULL;
    m_capsuleHeight = 6.0;
    m_capsuleRadius = 1.0;
}

WLDCharActor::~WLDCharActor()
{
    delete m_materialMap;
    if(m_shape && m_zone->collisionWorld())
    {
        NewtonReleaseCollision(m_zone->collisionWorld(), m_shape);
    }
}

Game * WLDCharActor::game() const
{
    return m_game;
}

Zone * WLDCharActor::zone() const
{
    return m_zone;
}

WLDModel * WLDCharActor::model() const
{
    return m_model;
}

void WLDCharActor::setModel(WLDModel *newModel)
{
    if(newModel != m_model)
    {
        if(newModel && !m_materialMap)
        {
            m_materialMap = new MaterialMap();
        }
        if(newModel)
        {
            WLDMaterialPalette *pal = newModel->mainMesh()->palette();
            m_materialMap->clear();
            m_materialMap->resize(pal->materialSlots().size());
            m_model = newModel;
            m_idleAnim = findAnimation("P01");
            m_walkingAnim = findAnimation("L01");
            m_runningAnim = findAnimation("L02");
            m_jumpingAnim = findAnimation("L03");
        }
        else
        {
            m_model = NULL;
        }
    }
}

float WLDCharActor::capsuleHeight() const
{
    return m_capsuleHeight;
}

float WLDCharActor::capsuleRadius() const
{
    return m_capsuleRadius;
}

NewtonCollision * WLDCharActor::shape() const
{
    return m_shape;
}

WLDCharActor::ActorMoveMode WLDCharActor::moveMode() const
{
    return m_moveMode;
}

void WLDCharActor::setMoveMode(WLDCharActor::ActorMoveMode newMode)
{
    m_moveMode = newMode;
}

float WLDCharActor::speed() const
{
    switch(m_moveMode)
    {
    default:
    case Running:
        return m_runSpeed;
    case Walking:
        return m_walkSpeed;
    case Sitting:
        return 0.0f;
    }
}

bool WLDCharActor::jumping() const
{
    return m_jumping;
}

void WLDCharActor::setJumping(bool isJumping)
{
    m_jumping = isJumping;
}

ActorState & WLDCharActor::currentState()
{
    return m_currentState;
}

ActorState & WLDCharActor::previousState()
{
    return m_previousState;
}

int WLDCharActor::movementX() const
{
    return m_movementStateX;
}

int WLDCharActor::movementY() const
{
    return m_movementStateY;
}

void WLDCharActor::setMovementX(int movementX)
{
    m_movementStateX = movementX;
}

void WLDCharActor::setMovementY(int movementY)
{
    m_movementStateY = movementY;
}

vec3 WLDCharActor::lookOrient() const
{
    return vec3(m_lookOrientX, 0.0f, m_lookOrientZ);
}

void WLDCharActor::setLookOrientX(float newOrientation)
{
    m_lookOrientX = newOrientation;
}

void WLDCharActor::setLookOrientZ(float newOrientation)
{
    m_lookOrientZ = newOrientation;
}

float WLDCharActor::cameraDistance() const
{
    return m_cameraDistance;
}

void WLDCharActor::setCameraDistance(float dist)
{
    m_cameraDistance = dist;
}

bool WLDCharActor::hasCamera() const
{
    return m_hasCamera;
}

void WLDCharActor::setHasCamera(bool camera)
{
    m_hasCamera = camera;
}

MaterialMap * WLDCharActor::materialMap() const
{
    return m_materialMap;
}

WLDAnimation * WLDCharActor::animation() const
{
    return m_animation;   
}

void WLDCharActor::setAnimation(WLDAnimation *newAnim)
{
    m_animation = newAnim;
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

bool WLDCharActor::addEquip(WLDCharActor::EquipSlot slot, WLDMesh *mesh, MaterialArray *materials)
{
    QString name = slotName(slot);
    if(name.isEmpty() || !m_animation)
        return false;
    int trackIndex = m_animation->findTrack(name);
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

void WLDCharActor::setSkin(uint32_t skinID)
{
    m_model->mainMesh()->palette()->makeSkinMap(skinID, m_materialMap);
}

WLDAnimation * WLDCharActor::findAnimation(QString animName)
{
    if(!m_model || !m_model->skeleton())
    {
        return NULL;
    }
    return m_model->skeleton()->animations().value(animName);
}

void WLDCharActor::draw(RenderContext *renderCtx, RenderProgram *prog)
{
    if(!m_model)
        return;
    WLDModelSkin *skin = m_model->skins().value(m_palName);
    if(!skin)
        return;
    QVector<BoneTransform> bones;
    if(m_animation)
        bones = m_animation->transformationsAtTime(m_animTime);

    float offsetZ = (m_capsuleHeight * 0.5f);
    renderCtx->pushMatrix();
    renderCtx->translate(m_location.x, m_location.y, m_location.z + offsetZ);
    renderCtx->rotate(m_rotation.x, 1.0, 0.0, 0.0);
    renderCtx->rotate(m_rotation.y, 0.0, 1.0, 0.0);
    renderCtx->rotate(m_rotation.z, 0.0, 0.0, 1.0);
    // XXX Find a better way to handle the orientation of the player.
    if(m_hasCamera)
        renderCtx->rotate(-m_lookOrientZ + 90.0f, 0.0, 0.0, 1.0);
    renderCtx->scale(m_scale.x, m_scale.y, m_scale.z);
    
    // XXX drawEquip method to allow skinned equipment (e.g. bow, epics)
    skin->draw(prog, bones, m_materialMap);
    foreach(ActorEquip eq, m_equip)
    {
        renderCtx->pushMatrix();
        BoneTransform bone = bones.value(eq.TrackID);
        renderCtx->translate(bone.location.toVector3D());
        renderCtx->rotate(bone.rotation);
        MeshBuffer *meshBuf = eq.Mesh->data()->buffer;
        meshBuf->matGroups.clear();
        meshBuf->addMaterialGroups(eq.Mesh->data());
        prog->beginDrawMesh(meshBuf, eq.Materials);
        prog->drawMesh();
        prog->endDrawMesh();
        renderCtx->popMatrix();
    }
    renderCtx->popMatrix();
}

void WLDCharActor::interpolateState(double alpha)
{
    m_location = (m_currentState.position * alpha) +
        (m_previousState.position * (1.0 - alpha));
}

void WLDCharActor::updatePosition(double dt)
{
    // Perform a step if the player is moving.
    ActorState &state = m_currentState;
    float dist = (speed() * dt);
    float deltaX = dist * m_movementStateX;
    float deltaY = dist * m_movementStateY;
    bool ghost = (m_cameraDistance < m_game->minDistanceToShowCharacter());
    ghost &= m_game->allowMultiJumps();
    matrix4 m;
    if(ghost)
        m = matrix4::rotate(lookOrient().x, 1.0, 0.0, 0.0);
    else
        m.setIdentity();
    m = m * matrix4::rotate(lookOrient().z, 0.0, 0.0, 1.0);
    state.position = state.position + m.map(vec3(-deltaX, deltaY, 0.0));
    
    // Handle jumping.
    const float jumpDuration = 0.2f;
    const float jumpAccelFactor = 2.5f;
    if(m_wantsToJump)
    {
        m_jumpTime = jumpDuration;
        m_jumping = true;
        m_wantsToJump = false;
    }
    double jumpTime = qMin((double)m_jumpTime, dt);
    if(m_jumping && (jumpTime > 0.0))
    {
        double jumpAccel = (-jumpAccelFactor * m_game->gravity().z) * dt;
        state.velocity = state.velocity + vec3(0.0, 0.0, (float)jumpAccel);
        m_jumpTime -= jumpTime;
    }
    
    // Handle gravity.
    if(m_game->applyGravity())
    {
        state.velocity = state.velocity + (m_game->gravity() * dt);
    }
}

void WLDCharActor::jump()
{
    if(!m_jumping || m_game->allowMultiJumps())
    {
        m_wantsToJump = true;
    }
}

void WLDCharActor::calculateViewFrustum(Frustum &frustum) const
{
    vec3 rot = lookOrient();
    matrix4 viewMat = matrix4::rotate(rot.x, 1.0, 0.0, 0.0) *
        matrix4::rotate(rot.y, 0.0, 1.0, 0.0) *
        matrix4::rotate(rot.z, 0.0, 0.0, 1.0);
    vec3 camPos(0.0, -m_cameraDistance, 0.0);
    camPos = viewMat.map(camPos);
    const float eyeLevel = 0.8;
    vec3 eyePos(0.0, 0.0, m_capsuleHeight * eyeLevel);
    vec3 eye = m_location + eyePos + camPos;
    frustum.setEye(eye);
    frustum.setFocus(eye + viewMat.map(vec3(0.0, 1.0, 0.0)));
    frustum.setUp(vec3(0.0, 0.0, 1.0));
    frustum.update();
}

void WLDCharActor::update(double currentTime)
{
    if(m_model)
    {
        WLDAnimation *oldAnimation = m_animation;
        WLDAnimation *newAnimation = oldAnimation;
        if(m_jumping)
        {
            newAnimation = m_jumpingAnim;
        }
        else if(m_movementStateX || m_movementStateY)
        {
            switch(m_moveMode)
            {
            default:
            case Running:
                newAnimation = m_runningAnim;
                break;
            case Walking:
                newAnimation = m_walkingAnim;
                break;
            }
        }
        else
        {
            newAnimation = m_idleAnim;
        }
        
        if(oldAnimation != newAnimation)
        {
            m_animation = newAnimation;
            m_startAnimationTime = currentTime;
        }
        m_animTime = (currentTime - m_startAnimationTime);
    }
}

void WLDCharActor::enteredZone(Zone *newZone, const vec3 &initialPos)
{
    Q_ASSERT(!m_shape && !m_zone);
    m_shape = NewtonCreateCapsule(newZone->collisionWorld(),
                                  m_capsuleRadius, m_capsuleHeight, 0, NULL);
    m_location = initialPos;
    m_hasCamera = true;
    m_currentState.position = m_previousState.position = initialPos;
    m_zone = newZone;
}

void WLDCharActor::leftZone(Zone *oldZone)
{
    if(m_shape)
    {
        NewtonReleaseCollision(oldZone->collisionWorld(), m_shape);
        m_shape = NULL;
    }
    m_hasCamera = false;
    m_zone = NULL;
}

////////////////////////////////////////////////////////////////////////////////

WLDLightActor::WLDLightActor(LightSourceFragment *frag, uint16_t lightID) : WLDActor(Kind)
{
    m_frag = frag;
    m_lightID = lightID;
    if(frag)
    {
        vec3 radius(frag->m_radius, frag->m_radius, frag->m_radius);
        m_boundsAA.low = frag->m_pos - radius;
        m_boundsAA.high = frag->m_pos + radius;
        m_location = frag->m_pos;
        
        LightDefFragment *def = frag->m_ref->m_def;
        m_params.color = vec3(def->m_color.x, def->m_color.y, def->m_color.z);
        m_params.bounds.pos = frag->m_pos;
        m_params.bounds.radius = frag->m_radius;
    }
    else
    {
        m_params.color = vec3(0.0, 0.0, 0.0);
        m_params.bounds.pos = vec3(0.0, 0.0, 0.0);
        m_params.bounds.radius = 0.0;
    }
}

uint16_t WLDLightActor::lightID() const
{
    return m_lightID;
}

const LightParams & WLDLightActor::params() const
{
    return m_params;
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

OctreeIndex::~OctreeIndex()
{
    delete m_root;
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

void OctreeIndex::findVisible(const Frustum &f, OctreeCallback callback, void *user, bool cull)
{
    findVisible(f, m_root, callback, user, cull);
}

void OctreeIndex::findVisible(const Frustum &f, Octree *octant, OctreeCallback callback, void *user, bool cull)
{
    if(!octant)
        return;
    TestResult r = cull ? f.containsAABox(octant->looseBounds()) : INSIDE;
    if(r == OUTSIDE)
        return;
    cull = (r != INSIDE);
    for(int i = 0; i < 8; i++)
        findVisible(f, octant->child(i), callback, user, cull);
    foreach(WLDActor *actor, octant->actors())
    {
        if((r == INSIDE) || (f.containsAABox(actor->boundsAA()) != OUTSIDE))
            (*callback)(actor, user);
    }
}

void OctreeIndex::findVisible(const Sphere &s, OctreeCallback callback, void *user, bool cull)
{
    findVisible(s, m_root, callback, user, cull);
}

void OctreeIndex::findVisible(const Sphere &s, Octree *octant, OctreeCallback callback, void *user, bool cull)
{
    if(!octant)
        return;
    TestResult r = cull ? s.containsAABox(octant->looseBounds()) : INSIDE;
    if(r == OUTSIDE)
        return;
    cull = (r != INSIDE);
    for(int i = 0; i < 8; i++)
        findVisible(s, octant->child(i), callback, user, cull);
    foreach(WLDActor *actor, octant->actors())
    {
        if((r == INSIDE) || (s.containsAABox(actor->boundsAA()) != OUTSIDE))
            (*callback)(actor, user);
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

Octree::~Octree()
{
    for(int i = 0; i < 8; i++)
        delete m_children[i];
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
