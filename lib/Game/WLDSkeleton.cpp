// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <cmath>
#include "EQuilibre/Game/WLDSkeleton.h"
#include "EQuilibre/Game/Fragments.h"

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

void WLDSkeleton::copyAnimationsFrom(WLDSkeleton *skel)
{
    foreach(QString animName, skel->animations().keys())
    {
        if(!animations().contains(animName))
            copyFrom(skel, animName);
    }
}

WLDAnimation * WLDSkeleton::copyFrom(WLDSkeleton *skel, QString animName)
{
    if(!skel || skel == this)
        return 0;
    WLDAnimation *anim = skel->animations().value(animName);
    if(!anim)
        return 0;
    WLDAnimation *anim2 = m_pose->copy(animName, this);
    foreach(TrackDefFragment *trackDef, anim->tracks())
        anim2->replaceTrack(trackDef);
    m_animations.insert(anim2->name(), anim2);
    return anim2;
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

const QVector<TrackDefFragment *> & WLDAnimation::tracks() const
{
    return m_tracks;
}

WLDSkeleton * WLDAnimation::skeleton() const
{
    return m_skel;
}

int WLDAnimation::findTrack(QString name) const
{
    for(int i = 0; i < m_tracks.count(); i++)
    {
        QString trackName = m_tracks[i]->name();
        if(trackName.contains(name))
            return i;
    }
    return -1;
}

void WLDAnimation::replaceTrack(TrackDefFragment *track)
{
    // strip animation name and character name from track name
    QString trackName = track->name().mid(6);
    for(int i = 0; i < m_tracks.count(); i++)
    {
        // strip character name from track name
        if(m_tracks[i]->name().mid(3) == trackName)
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
