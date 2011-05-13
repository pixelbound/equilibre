#ifndef OPENEQ_SCENE_H
#define OPENEQ_SCENE_H

#include <QObject>
#include <QMap>
#include "RenderState.h"
#include "Vertex.h"

class WLDData;
class WLDModel;
class MeshFragment;
class PFSArchive;

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

    WLDData * wldData() const;
    QMap<QString, WLDModel *> & models();
    WLDModel * selectedModel() const;
    QString selectedModelName() const;

    WLDModel * createModelFromMesh(MeshFragment *frag);

    void draw();

    void selectNext();
    void selectPrevious();

    void topView();
    void sideView();
    void frontView();

    void reset();
    void animate();

    void openWLD(QString archivePath, QString wldName);

public slots:
    void setSelectedModelName(QString name);

private:
    double m_started;
    int m_selected;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    QString m_meshName;
    PFSArchive *m_archive;
    WLDData *m_wldData;
    QMap<QString, WLDModel *> m_models;
};

#endif
