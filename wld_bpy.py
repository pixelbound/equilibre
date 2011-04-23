import os
import struct
import load_wld
import bpy

DefaultPath = "Data/gfaydark.d"

def importWldMeshes(wld, fragments):
    objects = []
    try:
        for f in fragments:
            objects.append(importWldMesh(wld, f))
    except Exception:
        for obj in objects:
            bpy.context.scene.objects.link(obj)
        raise

def importWldMesh(wld, frag):
    if frag.type != 0x36:
        return
    polys = [p[1:] for p in frag.polygons]
    mesh = bpy.data.meshes.new(frag.name)
    mesh.from_pydata(frag.vertices, [], polys)
    if False and frag.Fragment1 is not None:
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
    object = bpy.data.objects.new(frag.name, mesh)
    object.location = (frag.CenterX, frag.CenterY, frag.CenterZ)
    bpy.context.scene.objects.link(object)
    return object

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
    texture.image = loadImage(fileName, os.path.join(DefaultPath, fileName))
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
