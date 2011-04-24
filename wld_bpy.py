import os
import struct
from wld import WLDData
import bpy

def importZone(path, zoneName, importTextures):
    importZoneGeometry(path, zoneName, importTextures)
    importZoneObjects(path, zoneName, importTextures)

def importZoneGeometry(path, zoneName, importTextures):
    wldZonePath = os.path.join(path, "%s.d/%s.wld" % (zoneName, zoneName))
    wldZone = WLDData.fromFile(wldZonePath)
    zoneMeshFrags = list(wldZone.fragmentsByType(0x36))
    zoneMeshes = importWldMeshes(wldZone, zoneMeshFrags, importTextures)
    for frag, mesh in zip(zoneMeshFrags, zoneMeshes):
        obj = bpy.data.objects.new(frag.name, mesh)
        obj.location = (frag.CenterX, frag.CenterY, frag.CenterZ)
        bpy.context.scene.objects.link(obj)

def importZoneObjects(path, zoneName, importTextures):
    wldObjDefsPath = os.path.join(path, "%s.d/objects.wld" % zoneName)
    wldObjMeshesPath = os.path.join(path, "%s_obj.d/%s_obj.wld" % (zoneName, zoneName))
    wldObjDefs = WLDData.fromFile(wldObjDefsPath)
    wldObjMeshes = WLDData.fromFile(wldObjMeshesPath)
    # load meshes in blender
    objectMeshes = {}
    for meshFrag in wldObjMeshes.fragmentsByType(0x36):
        objectMeshes[meshFrag.ID] = importWldMesh(wldObjMeshes, meshFrag, importTextures)

    # place objects that use the meshes
    actorMap = {f.name: f for f in wldObjMeshes.fragmentsByType(0x14)}
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

def importWldMeshes(wld, fragments, importTextures=True):
    return [importWldMesh(wld, f, importTextures) for f in fragments]

def importWldMesh(wld, frag, importTextures=True):
    if frag.type != 0x36:
        return
    polys = [p[1:] for p in frag.polygons]
    mesh = bpy.data.meshes.new(frag.name)
    mesh.from_pydata(frag.vertices, [], polys)
    if importTextures and frag.Fragment1 is not None:
        # create materials
        materials, images = loadTextureMaterials(wld, frag.Fragment1)
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

def loadTextureMaterials(wld, frag):
    materials, images = [], []
    for texFrag in frag.Textures:
        material, texture = loadTextureMaterial(wld, texFrag)
        materials.append(material)
        images.append(texture)
    return materials, images

def loadTextureMaterial(wld, texFrag):
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
    texture.image = loadImage(fileName, os.path.join(wld.path, fileName))
    matTex = material.texture_slots.add()
    matTex.texture = texture
    matTex.texture_coords = 'UV'
    return material, texture

def loadImage(name, path):
    width, height, pixels = loadIndexedBitmap(path)
    if pixels is None:
        return bpy.images.load(path)
    image = bpy.data.images.new(name, width, height)
    image.pixels = pixels
    image.update()
    return image

def loadIndexedBitmap(path):
    with open(path, "rb") as f:
        # read file header
        fileHeader = struct.unpack("<ccIHHI", f.read(14))
        if (fileHeader[0] + fileHeader[1]) != b"BM":
            return 0, 0, None
        bmpHeaderSize = struct.unpack("I", f.read(4))[0]
        bmpHeader = f.read(bmpHeaderSize - 4)
        (biWidth, biHeight, biPlanes, biBitCount, biCompression, 
            biSizeImage, biXPelsPerMeter, biYPelsPerMeter, biClrUsed, 
            biClrImportant) = struct.unpack("iiHHIIiiII", bmpHeader[0:36])
        if biBitCount != 8:
            return 0, 0, None
        
        # read color table
        if biClrUsed == 0:
            biClrUsed = 256
        colorTableData = f.read(biClrUsed * 4)
        colors = []
        for i in range(0, biClrUsed * 4, 4):
            b, g, r, x = colorTableData[i : i + 4]
            colors.append((r / 255.0, g / 255.0, b / 255.0, 1.0))
        
        # read pixels
        pixelCount = biWidth * biHeight
        pixelData = f.read(pixelCount)
        pixels = []
        for i in range(0, pixelCount):
            pixels.extend(colors[pixelData[i]])
        return biWidth, biHeight, pixels
