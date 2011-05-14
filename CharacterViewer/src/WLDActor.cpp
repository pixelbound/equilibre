#include "WLDActor.h"
#include "Fragments.h"

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
}
