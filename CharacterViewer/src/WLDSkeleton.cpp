#include <cmath>
#include "WLDSkeleton.h"
#include "Fragments.h"

WLDSkeleton::WLDSkeleton(HierSpriteDefFragment *def, QObject *parent) : QObject(parent)
{
    m_def = def;
    QVector<TrackDefFragment *> tracks;
    foreach(SkeletonNode node, def->m_tree)
        tracks.append(node.track->m_def);
    m_pose = new WLDAnimation("POS", tracks, this, this);
    m_animations.insert(m_pose->name(), m_pose);
}

WLDAnimation *WLDSkeleton::pose() const
{
    return m_pose;
}

const QVector<SkeletonNode> & WLDSkeleton::tree() const
{
    return m_def->m_tree;
}

const QMap<QString, WLDAnimation *> & WLDSkeleton::animations() const
{
    return m_animations;
}

void WLDSkeleton::addTrack(QString animName, TrackDefFragment *track)
{
    WLDAnimation *anim = m_animations.value(animName);
    if(!anim)
    {
        anim = m_pose->copy(animName, this);
        m_animations.insert(animName, anim);
    }
    anim->replaceTrack(track);
}

////////////////////////////////////////////////////////////////////////////////

WLDAnimation::WLDAnimation(QString name, QVector<TrackDefFragment *> tracks,
                           WLDSkeleton *skel, QObject *parent) : QObject(parent)
{
    m_name = name;
    m_tracks = tracks;
    m_skel = skel;
    m_frameCount = 0;
    foreach(TrackDefFragment *track, tracks)
        m_frameCount = std::max(m_frameCount, (uint32_t)track->m_frames.count());
}

QString WLDAnimation::name() const
{
    return m_name;
}

void WLDAnimation::replaceTrack(TrackDefFragment *track)
{
    QString trackName = track->name().mid(3);
    for(int i = 0; i < m_tracks.count(); i++)
    {
        if(m_tracks[i]->name() == trackName)
        {
            m_tracks[i] = track;
            m_frameCount = std::max(m_frameCount, (uint32_t)track->m_frames.count());
            break;
        }
    }
}

WLDAnimation * WLDAnimation::copy(QString newName, QObject *parent) const
{
    return new WLDAnimation(newName, m_tracks, m_skel, parent);
}

QVector<BoneTransform> WLDAnimation::transformations(double t) const
{
    const double fps = 2.0;
    double dur = m_frameCount / fps;
    uint32_t frameIndex = qRound(fmod(t, dur) * fps) % m_frameCount;
    return transformations(frameIndex);
}

QVector<BoneTransform> WLDAnimation::transformations(uint32_t frameIndex) const
{
    BoneTransform initTrans;
    initTrans.location = QVector3D();
    initTrans.rotation = QQuaternion();
    QVector<BoneTransform> trans;
    trans.resize(m_tracks.count());
    transformPiece(trans, m_skel->tree(), 0, frameIndex, initTrans);
    return trans;
}

void WLDAnimation::transformPiece(QVector<BoneTransform> &transforms, const QVector<SkeletonNode> &tree,
    uint32_t pieceID, uint32_t frameIndex, BoneTransform parentTrans) const
{
    SkeletonNode piece = tree.value(pieceID);
    TrackDefFragment *track = m_tracks[pieceID];
    BoneTransform pieceTrans = track->frame(frameIndex), effTrans;
    effTrans.location = parentTrans.map(pieceTrans.location);
    effTrans.rotation = parentTrans.rotation * pieceTrans.rotation;
    transforms[pieceID] = effTrans;
    foreach(uint32_t childID, piece.children)
        transformPiece(transforms, tree, childID, frameIndex, effTrans);
}
