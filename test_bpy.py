import sys
import os
sys.path.append(os.getcwd())
import wld_bpy
import bpy
from wld import WLDData
from s3d import S3DArchive
path = os.path.join(os.getcwd(), "Data")
#zoneName = "global"
zoneName = "gfaydark"
#zoneName = "crushbone"
#wld_bpy.importZone(path, zoneName, False)
wld_bpy.importZoneGeometry(path, zoneName, False)
wld_bpy.importZoneObjects(path, zoneName, False)
#actors = ['BAT_ACTORDEF', 'BRF_ACTORDEF', 'BRM_ACTORDEF', 'DRI_ACTORDEF', 'FAF_ACTORDEF', 'FEM_ACTORDEF', 'GOL_ACTORDEF', #'GFF_ACTORDEF', 'GFM_ACTORDEF', 'INN_ACTORDEF', 'ORC_ACTORDEF', 'PIF_ACTORDEF', 'SPI_ACTORDEF', 'TRE_ACTORDEF', 'UNI_ACTORDEF', #'WAS_ACTORDEF', 'WIL_ACTORDEF', 'WOL_ACTORDEF']
#for a in actors:
#    wld_bpy.importZoneCharacter(path, zoneName, a, True)
#wld_bpy.importZoneCharacter(path, zoneName, 'WOL_ACTORDEF', "C10", 0, True)
#bpy.data.objects["Camera"].location = (4.3722453117370605, -0.21526014804840088, 0.2857252061367035)
#bpy.data.objects["Camera"].rotation_mode = "QUATERNION"
#bpy.data.objects["Camera"].rotation_quaternion = (0.6363058090209961, 0.4534217417240143, 0.41559749841690063, 0.4656204283237457)
#bpy.data.objects["Lamp"].location = (4.3722453117370605, -0.21526014804840088, 0.2857252061367035)
