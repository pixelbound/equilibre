#ifndef OPENEQ_WLD_ACTOR_H
#define OPENEQ_WLD_ACTOR_H

#include <QObject>
#include "Platform.h"
#include "Vertex.h"

class PFSArchive;
class ActorFragment;
class WLDModel;
class RenderState;

/*!
  \brief Describes an instance of a model (such as an object or a character).
  */
class WLDActor : public QObject
{
public:
    WLDActor(ActorFragment *frag, WLDModel *model, QObject *parent = 0);
    virtual ~WLDActor();

    WLDModel *model() const;
    void setModel(WLDModel *newModel);

    void draw(RenderState *state);

private:
    ActorFragment *m_frag;
    WLDModel *m_model;
};

#endif
