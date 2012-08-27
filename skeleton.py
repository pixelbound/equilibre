# Copyright (C) 2012 PiB <pixelbound@gmail.com>
# 
# EQuilibre is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import re
import sys
import os
from wld import WLDData
from s3d import S3DArchive
from euclid import Vector3, Quaternion

def skeletonsFromWld(wld):
    skeletons = {}
    for skelDef in wld.fragmentsByType(0x10):
        if re.match("^\w{3}_HS_DEF$", skelDef.name):
            skeletons[skelDef.name[0:3]] = Skeleton(skelDef)
    for trackRef in wld.fragmentsByType(0x13):
        if re.match("^\w\d{2}.*_TRACK$", trackRef.name):
            animName, skelName = trackRef.name[0:3], trackRef.name[3:6]
            skel = skeletons.get(skelName)
            if skel:
                skel.addTrack(animName, trackRef)
    return skeletons

class Skeleton:
    # Holds information about a model's skeleton, used for animation. """
    def __init__(self, skelDef):
        self.skelDef = skelDef
        self.pose = Animation("POS", [n.trackRef for n in skelDef.tree], self)
        self.animations = {self.pose.name: self.pose}
    
    def addTrack(self, animName, track):
        if not animName in self.animations:
            anim = self.pose.copy(animName)
            self.animations[animName] = anim
        else:
            anim = self.animations[animName]
        anim.replaceTrack(track)

class Animation:
    def __init__(self, name, tracks, skeleton):
        self.name = name
        self.tracks = tracks
        self.skeleton = skeleton
        self.indices = {track.name: i for i, track in enumerate(tracks)}
        self.frameCount = max(len(t.Reference.frames) for t in tracks)
    
    def replaceTrack(self, track):
        index = self.indices.get(track.name[3:], -1)
        if index >= 0:
            self.tracks[index] = track
            self.frameCount = max(self.frameCount, len(track.Reference.frames))
    
    def copy(self, newName):
        return Animation(newName, self.tracks[:], self.skeleton)
    
    def transformations(self, frame=0, tuples=False):
        transforms = {}
        tree = self.skeleton.skelDef.tree
        self.computePieceTransformation(transforms, tree, 0, frame, Vector3(), Quaternion())
        if tuples:
            return {pieceID: (name, trans[:], rot[:]) for pieceID, (name, trans, rot) in transforms.items()}
        else:
            return transforms

    def computePieceTransformation(self, transforms, tree, pieceID, frame, parentLoc, parentRot):
        piece, track = tree[pieceID], self.tracks[pieceID].Reference
        pieceLoc, pieceRot = Vector3(*track.location(frame)), Quaternion(*track.rotation(frame))
        try:
            transLoc = (parentRot * pieceLoc) + parentLoc
        except Exception:
            print(repr(pieceLoc), repr(parentRot))
            raise
        transRot = parentRot * pieceRot
        transforms[pieceID] = (piece.name, transLoc, transRot)
        for childID in piece.children:
            self.computePieceTransformation(transforms, tree, childID, frame, transLoc, transRot)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: %s <.S3D file>" % sys.argv[0])
    path = sys.argv[1]
    with S3DArchive(path) as a:
        wldName = os.path.basename(path).replace(".s3d", ".wld")
        wld = WLDData.fromArchive(a, wldName)
        skeletons = skeletonsFromWld(wld)
        for name, skel in skeletons.items():
            print(name)
            for anim in skel.animations.values():
                print("  %s %d" % (anim.name, anim.frameCount))
                print("    %s" % "\n    ".join(["%s %d" % (f.name, len(f.Reference.frames)) for f in anim.tracks]))
