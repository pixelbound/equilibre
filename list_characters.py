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

import struct
import sys
import os
import re
from s3d import S3DArchive
from wld import WLDData

class WLDActor(object):
    DefaultSkin = "00"
    def __init__(self, name, palette):
        self.name = name
        self.skins = {}
        self.pieces = []
        self.palette = palette
    
    def newSkin(self, palName):
        skin = WLDModelSkin(palName)
        self.skins[palName] = skin
        if palName != WLDActor.DefaultSkin:
            defaultSkin = self.skins["00"]
            for part in defaultSkin.parts:
                skin.parts.append(part)
        return skin

class WLDModelPiece(object):
    def __init__(self, pieceID, pieceName):
        self.ID = pieceID
        self.name = pieceName
        self.skins = []

class WLDModelSkin(object):
    def __init__(self, palName):
        self.palName = palName
        self.parts = []
        self.materials = {}
    
    def replacePart(self, meshDef, actorName, partName):
        for i in range(0, len(self.parts)):
            part = self.parts[i]
            oldActorName, oldPartName, oldPalName = explodeMeshName(part.name)
            if partName == oldPartName:
                self.parts[i] = meshDef
                return
        # No base part found, just append it.
        self.parts.append(meshDef)

matNameExp = re.compile("^\w{5}\d{4}_MDF$");
meshNameExp = re.compile("^(\w{3})(.*)(\d{2})_DMSPRITEDEF$");
def explodeMaterialName(defName):
    # e.g. defName == 'ORCCH0201_MDF'
    # 'ORC' : character
    # 'CH' : piece (part 1)
    # '02' : palette ID
    # '01' : piece (part 2)
    if matNameExp.match(defName):
        return defName[0:3], defName[3:5] + defName[7:9], defName[5:7]
    else:
        return None, None, None

def explodeMeshName(defName):
    # e.g. defName == 'ELEHE00_DMSPRITEDEF'
    # 'ELE' : character
    # 'HE' : mesh
    # '00' : skin ID
    m = meshNameExp.match(defName)
    if m:
        return m.group(1), m.group(2), m.group(3)
    else:
        return None, None, None

def combineMaterialName(actorName, palName, pieceName):
    return actorName + pieceName[0:2] + palName + pieceName[2:4] + "_MDF"

def listCharacters(s3dPath, wldName):
    with S3DArchive(s3dPath) as a:
        wld = WLDData.fromArchive(a, wldName)
        actors = {}
        importCharacters(wld, actors)
        importCharacterPalettes(wld, actors)
        dumpCharacters(wld, actors)

def dumpCharacters(wld, actors):
    for actorName in sorted(actors):
        actor = actors[actorName]
        print("%s (%d pieces, %d skins)" % (actorName, len(actor.pieces), len(actor.skins)))
        for piece in actor.pieces:
            matName = actor.palette.Textures[piece.ID].name
            actorName, pieceName, palName = explodeMaterialName(matName)
            palNames = []
            if actorName:
                for skinName in sorted(actor.skins):
                    skin = actor.skins[skinName]
                    matNameForSkin = combineMaterialName(actorName, skinName, pieceName)
                    if matNameForSkin in skin.materials:
                        palNames.append(skinName)
            print("+-- Piece %02d -> %s [%s]" % (piece.ID, piece.name, ", ".join(palNames)))
        for skinName in sorted(actor.skins):
            skin = actor.skins[skinName]
            meshNames = sorted(mesh.name.replace("_DMSPRITEDEF", "") for mesh in skin.parts)
            print("+-- Skin %s -> [%s]" % (skinName, ", ".join(meshNames)))

def findMainMesh(actorDef, actorName):
    if not actorDef.models:
        raise Exception("Actor '%s' has no mesh." % actorName)
    mainMeshName = "%s_DMSPRITEDEF" % actorName
    for meshRef in actorDef.listModels():
        mesh = meshRef.Mesh
        if mesh and (mesh.name == mainMeshName) and hasattr(mesh, "Fragment1"):
            palette = mesh.Fragment1
            if not palette:
                raise Exception("Mesh '%s' has no palette." % firstMesh.name)
            return mesh, palette
    return None, None

def importCharacters(wld, actors):
    for actorDef in wld.fragmentsByType(0x14):
        actorName = actorDef.name.replace("_ACTORDEF", "")
        mainMesh, mainPalette = findMainMesh(actorDef, actorName)
        if not mainMesh:
            print("Warning: could not find main mesh for actor '%s'." % actorName)
            continue
        actor = WLDActor(actorName, mainPalette)
        actors[actorName] = actor
        for i, matDef in enumerate(mainPalette.Textures):
            actorName2, pieceName, palName = explodeMaterialName(matDef.name)
            if not actorName2:
                print("Warning: could not split material name '%s' of actor %s, piece %d" % (matDef.name, actorName, i))
                pieceName = matDef.name.replace("_MDF","")
            actor.pieces.append(WLDModelPiece(i, pieceName))
        skin = actor.newSkin(WLDActor.DefaultSkin)
        for model in actorDef.listModels():
            skin.parts.append(model.Mesh)

def importCharacterPalettes(wld, actors):
    # look for alternate materials
    for matDef in wld.fragmentsByType(0x30):
        # XXX This is not always possible for all materials.
        # Should instead enumerate materials through meshes.
        actorName, pieceName, palName = explodeMaterialName(matDef.name)
        if not actorName in actors:
            continue
        actor = actors[actorName]
        try:
            skin = actor.skins[palName]
        except KeyError:
            skin = actor.newSkin(palName)
        skin.materials[matDef.name] = matDef

    # look for alternate meshes (e.g. heads)
    for meshDef in wld.fragmentsByType(0x36):
        actorName, partName, palName = explodeMeshName(meshDef.name)
        if not actorName in actors:
            continue
        actor = actors[actorName]
        try:
            skin = actor.skins[palName]
        except KeyError:
            skin = actor.newSkin(palName)
        skin.replacePart(meshDef, actorName, partName)
            
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: %s <.S3D file> [WLD name]" % sys.argv[0])
    s3dPath = sys.argv[1]
    if len(sys.argv) < 3:
        wldName = os.path.basename(s3dPath).replace(".s3d", ".wld")
    else:
        wldName = sys.argv[2]
    listCharacters(s3dPath, wldName)
