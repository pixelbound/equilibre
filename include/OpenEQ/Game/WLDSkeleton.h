#ifndef OPENEQ_WLD_SKELETON_H
#define OPENEQ_WLD_SKELETON_H

#include <QObject>
#include <QMap>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/Vertex.h"
#include "WLDFragment.h"

class HierSpriteDefFragment;
class TrackDefFragment;
class TrackFragment;
class MeshFragment;
class WLDAnimation;

class SkeletonNode
{
public:
    WLDFragmentRef name;
    uint32_t flags;
    TrackFragment *track;
    MeshFragment *mesh;
    QVector<uint32_t> children;
};

/*!
  \brief Holds information about a model's skeleton, used for animation.
  */
class GAME_DLL WLDSkeleton : public QObject
{
public:
    WLDSkeleton(HierSpriteDefFragment *def, QObject *parent = 0);

    WLDAnimation *pose() const;
    const QMap<QString, WLDAnimation *> & animations() const;
    const QVector<SkeletonNode> &tree() const;

    void addTrack(QString animName, TrackDefFragment *track);
    void copyAnimationsFrom(WLDSkeleton *skel);
    WLDAnimation * copyFrom(WLDSkeleton *skel, QString animName);

private:
    HierSpriteDefFragment *m_def;
    QMap<QString, WLDAnimation *> m_animations;
    WLDAnimation *m_pose;
};

/*!
  \brief Describes one way of animating a model's skeleton.
  */
class GAME_DLL WLDAnimation : public QObject
{
public:
    WLDAnimation(QString name, QVector<TrackDefFragment *> tracks, WLDSkeleton *skel,
                 QObject *parent = 0);

    QString name() const;
    const QVector<TrackDefFragment *> & tracks() const;
    WLDSkeleton * skeleton() const;

    int findTrack(QString name) const;
    void replaceTrack(TrackDefFragment *track);
    WLDAnimation * copy(QString newName, QObject *parent = 0) const;
    QVector<BoneTransform> transformationsAtTime(double t) const;
    QVector<BoneTransform> transformationsAtFrame(double f) const;

private:
    void transformPiece(QVector<BoneTransform> &transforms, const QVector<SkeletonNode> &tree,
        uint32_t pieceID, double f, BoneTransform parentTrans) const;
    BoneTransform interpolate(TrackDefFragment *track, double f) const;

    QString m_name;
    QVector<TrackDefFragment *> m_tracks;
    WLDSkeleton *m_skel;
    uint32_t m_frameCount;
};

#endif
