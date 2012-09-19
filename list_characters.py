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
def explodeMaterialName(defName):
    # e.g. defName == 'ORCCH0201_MDF'
    # 'ORC' : character
    # 'CH' : piece (part 1)
    # '02' : palette ID
    # '01' : piece (part 2)
    if matNameExp.match(defName):
        return defName[0:3], defName[5:7], defName[3:5] + defName[7:9]
    else:
        return None, None, None

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
        print("%s (%d skins)" % (actorName, len(actor.skins)))
        for palName in sorted(actor.skins):
            skin = actor.skins[palName]
            print("+-- %s (%d materials)" % (palName, len(skin.materials)))
            for matName in sorted(skin.materials):
                mat = skin.materials[matName]
                print("|   +-- %s" % matName)

def importCharacters(wld, actors):
    for actorDef in wld.fragmentsByType(0x14):
        actorName = actorDef.name.replace("_ACTORDEF", "")
        actor = WLDActor(actorName)
        actors[actorName] = actor
        skin = actor.skin
        for model in actorDef.listModels():
            mesh = model.Mesh
            skin.parts.append(mesh)
            if hasattr(mesh, "Fragment1"):
                palette = mesh.Fragment1
                for matDef in palette.Textures:
                    skin.materials[matDef.name] = matDef

def importCharacterPalettes(wld, actors):
    for matDef in wld.fragmentsByType(0x30):
        actorName, palName, pieceName = explodeMaterialName(matDef.name)
        if not actorName in actors:
            continue
        actor = actors[actorName]
        try:
            skin = actor.skins[palName]
        except KeyError:
            skin = actor.newSkin(palName)
        skin.materials[matDef.name] = matDef
            
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: %s <.S3D file> [WLD name]" % sys.argv[0])
    s3dPath = sys.argv[1]
    if len(sys.argv) < 3:
        wldName = os.path.basename(s3dPath).replace(".s3d", ".wld")
    else:
        wldName = sys.argv[2]
    listCharacters(s3dPath, wldName)
