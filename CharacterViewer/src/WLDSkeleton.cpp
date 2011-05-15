#include <cmath>
#include "WLDSkeleton.h"
#include "Fragments.h"

vec3 BoneTransform::map(const vec3 &v)
{
    QVector3D v2(v.x, v.y, v.z);
    v2 = rotation.rotatedVector(v2) + location;
    return vec3(v2.x(), v2.y(), v2.z());
}

QVector3D BoneTransform::map(const QVector3D &v)
{
    return rotation.rotatedVector(v) + location;
}

BoneTransform BoneTransform::interpolate(BoneTransform a, BoneTransform b, double f)
{
    BoneTransform c;
    c.rotation = QQuaternion::slerp(a.rotation, b.rotation, f);
    c.location = (a.location * (1.0 - f)) + (b.location * f);
    return c;
}

////////////////////////////////////////////////////////////////////////////////

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

QVector<BoneTransform> WLDAnimation::transformationsAtTime(double t) const
{
    const double fps = 10.0;
    double dur = m_frameCount / fps;
    return transformationsAtFrame(fmod(fmod(t, dur) * fps, m_frameCount));
}

QVector<BoneTransform> WLDAnimation::transformationsAtFrame(double f) const
{
    BoneTransform initTrans;
    initTrans.location = QVector3D();
    initTrans.rotation = QQuaternion();
    QVector<BoneTransform> trans;
    trans.resize(m_tracks.count());
    transformPiece(trans, m_skel->tree(), 0, f, initTrans);
    return trans;
}

void WLDAnimation::transformPiece(QVector<BoneTransform> &transforms, const QVector<SkeletonNode> &tree,
    uint32_t pieceID, double f, BoneTransform parentTrans) const
{
    SkeletonNode piece = tree.value(pieceID);
    TrackDefFragment *track = m_tracks[pieceID];
    BoneTransform pieceTrans = interpolate(track, f);
    BoneTransform effTrans;
    effTrans.location = parentTrans.map(pieceTrans.location);
    effTrans.rotation = parentTrans.rotation * pieceTrans.rotation;
    transforms[pieceID] = effTrans;
    foreach(uint32_t childID, piece.children)
        transformPiece(transforms, tree, childID, f, effTrans);
}

BoneTransform WLDAnimation::interpolate(TrackDefFragment *t, double f) const
{
    int i = qRound(floor(f));
    //int next = prev + 1;
    return BoneTransform::interpolate(t->frame(i), t->frame(i + 1), f - i);
}
