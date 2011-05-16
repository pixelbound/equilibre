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
    WLDActor(WLDModel *model, QObject *parent = 0);
    WLDActor(ActorFragment *frag, WLDModel *model, QObject *parent = 0);
    virtual ~WLDActor();

    WLDModel *model() const;
    void setModel(WLDModel *newModel);

    QString animName() const;
    void setAnimName(QString name);

    double animTime() const;
    void setAnimTime(double newTime);

    QString paletteName() const;
    void setPaletteName(QString palName);

    void draw(RenderState *state);

private:
    ActorFragment *m_frag;
    vec3 m_location, m_rotation, m_scale;
    WLDModel *m_model;
    QString m_animName;
    double m_animTime;
    QString m_palName;
};

#endif
