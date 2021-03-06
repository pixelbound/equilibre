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

import os
import struct
from s3d import S3DArchive, readStruct
from wld import WLDData
import skeleton
import bpy
from mathutils import Vector, Matrix, Quaternion
import euclid
import math

#TODO: actor class
#           references meshes, skeletons (all animations)

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
            for meshRef in actor.models:
                #XXX: this can be a skeleton instead of a mesh. Okay, maybe not for placeable objects.
                mesh = objectMeshes.get(meshRef.Mesh.ID)
                if mesh:
                    obj = bpy.data.objects.new(actor.name, mesh)
                    obj.location = objectDef.location
                    #obj.rotation_euler = objectDef.rotation
                    obj.scale = objectDef.scale
                    bpy.context.scene.objects.link(obj)
        else:
            print("Actor '%s' not found" % objectDef.name)

def importZoneCharacter(path, zoneName, actorName, animName, frame, importTextures):
    chrPath = os.path.join(path, "%s_chr.s3d" % zoneName)
    with S3DArchive(chrPath) as chrArchive:
        wldChrMeshes = WLDData.fromArchive(chrArchive, "%s_chr.wld" % zoneName)
        actorFrag = wldChrMeshes.findFragment(0x14, actorName)
        if not actorFrag:
            raise Exception("Actor not found '%s' in archive" % actorName)
        meshes = []
        skeletons = skeleton.skeletonsFromWld(wldChrMeshes)
        for modelFrag in actorFrag.models:
            if (modelFrag.type == 0x11) and modelFrag.Reference:
                skelFrag = modelFrag.Reference
                skel = skeletons.get(skelFrag.name[0:3])
                if skel:
                    meshes.extend(importCharacterFromSkeletonDef(chrArchive, skel, animName, frame, importTextures))
            elif (modelFrag.type == 0x2d) and modelFrag.Mesh:
                meshFrag = modelFrag.Mesh
                meshes.append((meshFrag, importWldMesh(chrArchive, meshFrag, importTextures)))
    actorObj = bpy.data.objects.new(actorName, None)
    bpy.context.scene.objects.link(actorObj)
    for meshFrag, mesh in meshes:
        obj = bpy.data.objects.new(meshFrag.name, mesh)
        obj.parent = actorObj
        bpy.context.scene.objects.link(obj)

def importCharacterFromSkeletonDef(archive, skel, animName, frame, importTextures):
    transforms = skel.animations[animName].transformations(frame)
    meshes = []
    for meshRef in skel.skelDef.Fragments:
        if (meshRef.type == 0x2d) and meshRef.Mesh:
            meshFrag = meshRef.Mesh
            mesh = importWldMesh(archive, meshFrag, importTextures)
            skinMesh(meshFrag, mesh, transforms)
            meshes.append((meshFrag, mesh))
    return meshes

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
                    #mesh.polygons[polyID].material_index = texID
                    mesh.faces[polyID].material_index = texID
                    poly = frag.polygons[polyID]
                    polyTexCoords = [frag.texCoords[poly[i]] for i in range(1, 4)]
                    td.uv1, td.uv2, td.uv3 = normalizeTexCoords(polyTexCoords)
            pos += count
        # Blender 2.63+ (really slow and doesn't seem to work).
        #uv_layer = mesh.uv_layers[0]
        #for i in range(0, len(frag.texCoords)):
        #    uv_layer.data[i].uv = frag.texCoords[i]
    # update mesh to allow proper display
    mesh.validate()
    mesh.update()
    return mesh

def normalizeTexCoords(tc):
    minU, dU = min(uv[0] for uv in tc) / 256.0, 0.0
    minV, dV = min(uv[1] for uv in tc) / 256.0, 0.0
    dU, dV = minU - math.modf(minU)[0], minV - math.modf(minV)[0]
    return [((uv[0] / 256.0) + dU, (uv[1] / 256.0) + dV) for uv in tc]

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
    masked = (texFrag.Param1 & 0b11) == 0b11
    texture.image = loadImage(fileName, archive, fileName, masked)
    matTex = material.texture_slots.add()
    matTex.texture = texture
    matTex.texture_coords = 'UV'
    return material, texture

def loadImage(name, archive, fileName, masked):
    #print(fileName)
    with archive.openFile(fileName) as f:
        width, height, pixels = loadIndexedBitmap(f, masked)
    if pixels is None:
        return None
    image = bpy.data.images.new(name, width, height)
    image.pixels = pixels
    image.update()
    return image

def loadIndexedBitmap(stream, masked):
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
    transparent = (0.0, 0.0, 0.0, 0.0)
    for i in range(0, biClrUsed * 4, 4):
        b, g, r, x = colorTableData[i : i + 4]
        colors.append((r / 255.0, g / 255.0, b / 255.0, 1.0))
    #if masked:
    #    colors[0] = (1.0, 1.0, 1.0, 0.0)
    
    # read pixels
    pixelCount = biWidth * biHeight
    pixelData = stream.read(pixelCount)
    pixels = []
    if masked:
        colors[pixelData[0]] = transparent
    for i in range(0, pixelCount):
        try:
            p = pixelData[i]
            pixels.extend(colors[p])
        except IndexError as e:
            pixels.extend(transparent)
    return biWidth, biHeight, pixels

def importSkeleton(frag):
    if frag.type != 0x10:
        raise Exception("Expected fragment type 0x10, got %x" % frag.type)
    origin = Vector((0, 0, 0))
    bpy.ops.object.add(type='ARMATURE', enter_editmode=True, location=origin)
    obj = bpy.context.object
    obj.show_x_ray = True
    obj.name = frag.name
    skel = obj.data
    skel.name = frag.name + '_ARMATURE'
    skel.show_axes = True
    bpy.ops.object.mode_set(mode='EDIT')
    importSkeletonPiece(skel, frag.tree, frag.tree[0], None)
    bpy.ops.object.mode_set(mode='OBJECT')
    return obj

def importSkeletonPiece(skel, tree, piece, parentName, depth=0):
    bone = skel.edit_bones.new(piece.name)
    track = piece.trackRef.Reference
    trans = Vector(track.translation())
    rot = Quaternion(track.rotation())
    mTrans = Matrix.Translation(trans)
    mRot = rot.to_matrix()
    mRot.resize_4x4()
    if parentName:
        parent = skel.edit_bones[parentName]
        bone.parent = parent
        bone.head = parent.tail
        bone.use_connect = False
        m = parent.matrix.copy()
    else:
        bone.head = (0, 0, 0)
        m = Matrix()
    m = m * mTrans
    m = m * mRot
    bone.tail = transform(m, bone.head)
    # recursively import the children bones
    for childID in piece.children:
        importSkeletonPiece(skel, tree, tree[childID], piece.name, depth+1)

def skinMesh(meshFrag, mesh, transforms):
    pos = 0
    vertices = mesh.vertices
    for count, pieceID in meshFrag.vertexPieces:
        pieceName, trans, rot = transforms[pieceID]
        for i in range(pos, pos + count):
            v = euclid.Vector3(*vertices[i].co)
            v = (rot * v) + trans
            vertices[i].co = v[0:3]
        pos += count

def transform(m, v):
    " Transform the vector v using the matrix m and return the resulting vector. "
    x = (m[0][0] * v[0]) + (m[1][0] * v[1]) + (m[2][0] * v[2]) + m[3][0]
    y = (m[0][1] * v[0]) + (m[1][1] * v[1]) + (m[2][1] * v[2]) + m[3][1]
    z = (m[0][2] * v[0]) + (m[1][2] * v[1]) + (m[2][2] * v[2]) + m[3][2]
    return x, y, z
