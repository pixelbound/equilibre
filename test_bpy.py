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

import sys
import os
from optparse import OptionParser
sys.path.append(os.getcwd())
import wld_bpy
import bpy
from wld import WLDData
from s3d import S3DArchive

def importZone(path, options):
    if options.import_terrain:
        wld_bpy.importZoneGeometry(path, options.zone, options.import_textures)
    if options.import_objects:
        wld_bpy.importZoneObjects(path, options.zone, options.import_textures)

def importWolf():
    path = os.path.join(os.getcwd(), "Data")
    zoneName = "gfaydark"
    wld_bpy.importZoneCharacter(path, zoneName, 'WOL_ACTORDEF', "C10", 0, True)
    bpy.data.objects["Camera"].location = (4.3722453117370605, -0.21526014804840088, 0.2857252061367035)
    bpy.data.objects["Camera"].rotation_mode = "QUATERNION"
    bpy.data.objects["Camera"].rotation_quaternion = (0.6363058090209961, 0.4534217417240143, 0.41559749841690063, 0.4656204283237457)
    bpy.data.objects["Lamp"].location = (4.3722453117370605, -0.21526014804840088, 0.2857252061367035)

if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-P", dest="python_script", help="Blender's -P argument")
    parser.add_option("-z", "--zone", dest="zone", help="Name of the zone to import")
    parser.add_option("--import-terrain", dest="import_terrain", action="store_true", default=True)
    parser.add_option("--import-objects", dest="import_objects", action="store_true", default=False)
    parser.add_option("--import-textures", dest="import_textures", action="store_true", default=False)
    try:
        pos = sys.argv.index("--")
        sys.argv = sys.argv[pos:]
    except ValueError:
        pass
    print(sys.argv)
    (options, args) = parser.parse_args()
    print(options, args)
    if args:
        path = args[0]
    else:
        path = os.path.join(os.getcwd(), "Data")
    importZone(path, options)
