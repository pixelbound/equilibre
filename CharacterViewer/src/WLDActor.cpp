#include "WLDActor.h"
#include "WLDModel.h"
#include "Fragments.h"
#include "RenderState.h"

WLDActor::WLDActor(ActorFragment *frag, WLDModel *model, QObject *parent) : QObject(parent)
{
    m_frag = frag;
    m_model = model;
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

void WLDActor::draw(RenderState *state)
{
    state->pushMatrix();
    state->translate(m_frag->m_location);
    //state->rotate(m_frag->m_rotation.x, 1.0, 0.0, 0.0);
    //state->rotate(m_frag->m_rotation.y, 0.0, 1.0, 0.0);
    //state->rotate(m_frag->m_rotation.z, 0.0, 0.0, 1.0);
    state->scale(m_frag->m_scale);
    m_model->draw(state);
    state->popMatrix();
}
