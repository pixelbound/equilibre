import os
import struct
from s3d import S3DArchive, readStruct
from wld import WLDData
import bpy

def importZone(path, zoneName, importTextures):
    importZoneGeometry(path, zoneName, importTextures)
    importZoneObjects(path, zoneName, importTextures)

def importZoneGeometry(path, zoneName, importTextures):
    zonePath = os.path.join(path, "%s.s3d" % zoneName)
    with S3DArchive(zonePath) as zoneArchive:
        wldZone = WLDData.fromArchive(zoneArchive, "%s.wld" % zoneName)
        zoneMeshFrags = list(wldZone.fragmentsByType(0x36))
        zoneMeshes = importWldMeshes(zoneArchive, zoneMeshFrags, importTextures)
    for frag, mesh in zip(zoneMeshFrags, zoneMeshes):
        obj = bpy.data.objects.new(frag.name, mesh)
        obj.location = (frag.CenterX, frag.CenterY, frag.CenterZ)
        bpy.context.scene.objects.link(obj)

def importZoneObjects(path, zoneName, importTextures):
    objPath = os.path.join(path, "%s_obj.s3d" % zoneName)
    with S3DArchive(objPath) as objArchive:
        wldObjMeshes = WLDData.fromArchive(objArchive, "%s_obj.wld" % zoneName)
        # load meshes in blender
        objectMeshes = {}
        for meshFrag in wldObjMeshes.fragmentsByType(0x36):
            objectMeshes[meshFrag.ID] = importWldMesh(objArchive, meshFrag, importTextures)
        actorMap = {f.name: f for f in wldObjMeshes.fragmentsByType(0x14)}

    # place objects that use the meshes
    zonePath = os.path.join(path, "%s.s3d" % zoneName)
    with S3DArchive(zonePath) as zoneArchive:
        wldObjDefs = WLDData.fromArchive(zoneArchive, "objects.wld")
    for objectDef in wldObjDefs.fragmentsByType(0x15):
        actor = actorMap.get(objectDef.Reference)
        if actor:
            for meshRef in actor.Fragment3:
                mesh = objectMeshes.get(meshRef.Mesh.ID)
                if mesh:
                    obj = bpy.data.objects.new(actor.name, mesh)
                    obj.location = (objectDef.X, objectDef.Y, objectDef.Z)
                    #obj.rotation_euler = (objectDef.RotateX, objectDef.RotateY, objectDef.RotateZ)
                    obj.scale = (objectDef.ScaleX, objectDef.ScaleY, 1)
                    bpy.context.scene.objects.link(obj)
        else:
            print("Actor '%s' not found" % objectDef.name)

def importWldMeshes(archive, fragments, importTextures=True):
    return [importWldMesh(archive, f, importTextures) for f in fragments]

def importWldMesh(archive, frag, importTextures=True):
    if frag.type != 0x36:
        return
    polys = [p[1:] for p in frag.polygons]
    mesh = bpy.data.meshes.new(frag.name)
    mesh.from_pydata(frag.vertices, [], polys)
    if importTextures and frag.Fragment1 is not None:
        # create materials
        materials, images = loadTextureMaterials(archive, frag.Fragment1)
        for mat in materials:
            mesh.materials.append(mat)
        # assign textures and texture coords to faces
        pos = 0
        texture = mesh.uv_textures.new()
        for count, texID in frag.polygonsByTex:
            texImage = images[texID]
            if texImage:
                for polyID in range(pos, pos + count):
                    td = texture.data[polyID]
                    td.image = texImage.image
                    td.use_image = True
                    mesh.faces[polyID].material_index = texID
                    poly = frag.polygons[polyID]
                    td.uv1 = normalizeTexCoords(frag.texCoords[poly[1]])
                    td.uv2 = normalizeTexCoords(frag.texCoords[poly[2]])
                    td.uv3 = normalizeTexCoords(frag.texCoords[poly[3]])
            pos += count
    # update mesh to allow proper display
    mesh.validate()
    mesh.update()
    return mesh

def normalizeTexCoords(tc):
    return tc[0] / 256.0, tc[1] / 256.0

def loadTextureMaterials(archive, frag):
    materials, images = [], []
    for texFrag in frag.Textures:
        material, texture = loadTextureMaterial(archive, texFrag)
        materials.append(material)
        images.append(texture)
    return materials, images

def loadTextureMaterial(archive, texFrag):
    if texFrag.type != 0x30:
        raise Exception("Expected fragment type 0x30, got %x" % texFrag.type)
    elif texFrag.Param1 == 0:
        return None, None
    material = bpy.data.materials.new(name=texFrag.name)
    texture = bpy.data.textures.new(name=texFrag.name, type='IMAGE')
    imgRefFrag = texFrag.Reference
    if imgRefFrag is None:
        raise Exception("Fragment %s has no reference" % repr(texFrag))
    imgFrag = imgRefFrag.Reference
    if imgFrag is None:
        raise Exception("Fragment %s has no reference" % repr(imgRefFrag))
    if len(imgFrag.Files) == 0:
        return None, None
    fileName = imgFrag.Files[0].FileName.lower()
    texture.image = loadImage(fileName, archive, fileName)
    matTex = material.texture_slots.add()
    matTex.texture = texture
    matTex.texture_coords = 'UV'
    return material, texture

def loadImage(name, archive, fileName):
    with archive.openFile(fileName) as f:
        width, height, pixels = loadIndexedBitmap(f)
    if pixels is None:
        return None
    image = bpy.data.images.new(name, width, height)
    image.pixels = pixels
    image.update()
    return image

def loadIndexedBitmap(stream):
    # read file header
    fileHeader = readStruct(stream, "<2sIHHI")
    if fileHeader[0] != b"BM":
        return 0, 0, None
    bmpHeaderSize = readStruct(stream, "I")[0]
    bmpHeader = stream.read(bmpHeaderSize - 4)
    (biWidth, biHeight, biPlanes, biBitCount, biCompression, 
        biSizeImage, biXPelsPerMeter, biYPelsPerMeter, biClrUsed, 
        biClrImportant) = struct.unpack("iiHHIIiiII", bmpHeader[0:36])
    if biBitCount != 8:
        return 0, 0, None
    
    # read color table
    if biClrUsed == 0:
        biClrUsed = 256
    colorTableData = stream.read(biClrUsed * 4)
    colors = []
    for i in range(0, biClrUsed * 4, 4):
        b, g, r, x = colorTableData[i : i + 4]
        colors.append((r / 255.0, g / 255.0, b / 255.0, 1.0))
    
    # read pixels
    pixelCount = biWidth * biHeight
    pixelData = stream.read(pixelCount)
    pixels = []
    for i in range(0, pixelCount):
        pixels.extend(colors[pixelData[i]])
    return biWidth, biHeight, pixels
