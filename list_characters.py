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
    def __init__(self, name):
        self.name = name
        self.skin = WLDModelSkin("00")
        self.skins = {self.skin.palName : self.skin}
    
    def newSkin(self, palName):
        skin = WLDModelSkin(palName)
        self.skins[palName] = skin
        return skin

class WLDModelSkin(object):
    def __init__(self, palName):
        self.palName = palName
        self.parts = []
        self.materials = {}

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
        mesh = actor.skin.parts[0]
        matDefs = mesh.Fragment1.Textures
        slots = [mesh.polygonsByTex[i][1] for i in range(0, mesh.PolygonTexCount)]
        print("%s (%d slots, %d skins)" % (actorName, len(slots), len(actor.skins)))
        for slotID in sorted(slots):
            matName = matDefs[slotID].name
            actorName, pieceName, palName = explodeMaterialName(matName)
            palNames = []
            for skinName in sorted(actor.skins):
                skin = actor.skins[skinName]
                matNameForSkin = combineMaterialName(actorName, skinName, pieceName)
                if matNameForSkin in skin.materials:
                    palNames.append(skinName)
            print("+-- %02d -> %s [%s]" % (slotID, pieceName, ", ".join(palNames)))

def importCharacters(wld, actors):
    for actorDef in wld.fragmentsByType(0x14):
        actorName = actorDef.name.replace("_ACTORDEF", "")
        actor = WLDActor(actorName)
        actors[actorName] = actor
        skin = actor.skin
        for model in actorDef.listModels():
            mesh = model.Mesh
            if skin.parts:
                continue # XXX deal with heads
            skin.parts.append(mesh)
            if hasattr(mesh, "Fragment1"):
                palette = mesh.Fragment1
                for matDef in palette.Textures:
                    skin.materials[matDef.name] = matDef

def importCharacterPalettes(wld, actors):
    # look for alternate materials
    for matDef in wld.fragmentsByType(0x30):
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
        mesh = meshDef
        matDefs = mesh.Fragment1.Textures
        slots = [mesh.polygonsByTex[i][1] for i in range(0, mesh.PolygonTexCount)]
        print("%s %s (%d slots)" % (actorName+partName, palName, len(slots)))
        for slotID in sorted(slots):
            matName = matDefs[slotID].name
            actorName, pieceName, palName = explodeMaterialName(matName)
            print("+-- %02d -> %s" % (slotID, pieceName))
            
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: %s <.S3D file> [WLD name]" % sys.argv[0])
    s3dPath = sys.argv[1]
    if len(sys.argv) < 3:
        wldName = os.path.basename(s3dPath).replace(".s3d", ".wld")
    else:
        wldName = sys.argv[2]
    listCharacters(s3dPath, wldName)
