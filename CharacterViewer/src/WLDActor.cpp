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
