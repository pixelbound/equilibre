#ifndef OPENEQ_SCENE_H
#define OPENEQ_SCENE_H

#include <QObject>
#include <QMap>
#include "RenderState.h"
#include "Vertex.h"

class WLDModel;
class WLDActor;
class Zone;

class Scene : public QObject, StateObject
{
    Q_OBJECT

public:
    Scene(RenderState *state);
    virtual ~Scene();

    void init();

    vec3 & theta();
    float & sigma();
    vec3 & delta();

    Zone * zone() const;
    const QMap<QString, WLDModel *> & objModels() const;
    const QMap<QString, WLDActor *> & charModels() const;
    WLDModel * selectedObject() const;
    WLDActor * selectedCharacter() const;
    QString selectedModelName() const;

    void draw();

    void selectNext();
    void selectPrevious();

    void topView();
    void sideView();
    void frontView();

    void reset();

    enum Mode
    {
        CharacterViewer,
        ObjectViewer,
        ZoneViewer
    };

    Mode mode() const;
    void setMode(Mode mode);

public slots:
    void setSelectedModelName(QString name);

private:
    double m_started;
    int m_selected;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    QString m_meshName;
    Zone *m_zone;
    Mode m_mode;
};

#endif
