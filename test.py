import struct
import re
import os
from s3d import S3DArchive, readStruct
from wld import WLDData

data_path = os.path.join(os.getcwd(), "Data")
dir_path = data_path

#wld = WLDData.fromFile(os.path.join(data_path, "gfaydark_chr.d/gfaydark_chr.wld")
#wld = WLDData.fromFile(os.path.join(data_path, "gfaydark_obj.d/gfaydark_obj.wld")
#wld = WLDData.fromFile(os.path.join(data_path, "gfaydark.d/gfaydark.wld")
#wld = WLDData.fromFile(os.path.join(data_path, "gfaydark_chr.d/gfaydark_chr.wld")
#wld = WLDData.fromFile(os.path.join(data_path, "gfaydark.d/objects.wld")
#wld = WLDData.fromFile(os.path.join(data_path, "gfaydark_chr.d/gfaydark_chr.wld")
#wld = WLDData.fromFile(os.path.join(data_path, "sleeper2_chr.s3d.d/sleeper2_chr.wld")
#with S3DArchive(os.path.join(data_path, "gequip.s3d") as a:
#   wld = WLDData.fromArchive(a, "gequip.wld")
zoneObjects = [f for f in os.listdir(dir_path) if f.endswith("_obj.s3d")]
zoneObjects.sort()

count, total = 0, len(zoneObjects)
for obj in zoneObjects:
    obj = obj.replace("_obj","")
    wldName = obj.replace(".s3d", ".wld")
    #wldName = "objects.wld"
    path = os.path.join(dirPath, obj)
    if not os.path.exists(path):
        continue
    with S3DArchive(path) as a:
        wld = WLDData.fromArchive(a, wldName)
        if wld is None:
            continue
        meshes = list(wld.fragmentsByType(0x36))
        if len(meshes) > 0xffff:
            print("assertion failed: %s (len(meshes) == %d)" % (
                obj, len(meshes)))
            count += 1
        #for actorDef in actorDefs:
        #    models = actorDef.listModels()
        #    if len(models) > 0:
        #        print("assertion failed: %s/%s@%d (len(hierModels) == %d)" % (
        #            obj, actorDef.name, actorDef.ID, len(models)))
        #        count += 1
        #        break
print("Count: %d / %d (%f %%)" % (count, total, count / total * 100.0))
#counts = {}
#frags = {}
#for k in set(f.type for f in wld.fragments.values()):
#    counts["0x%x" % k] = sum(1 for f in wld.fragments.values() if f.type == k)
#    frags[k] = list(wld.fragmentsByType(k))
