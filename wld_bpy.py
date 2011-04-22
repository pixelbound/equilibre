import os
import load_wld
import bpy
import io_utils

DefaultPath = "Data/gfaydark_obj.d"

def importWldMesh(wld, frag):
    if frag.type != 0x36:
        return
    polys = [p[1:] for p in frag.polygons]
    mesh = bpy.data.meshes.new(frag.name)
    mesh.from_pydata(frag.vertices, [], polys)
    if frag.Fragment1 is not None:
        # create materials
        materials, images = loadTextureMaterials(wld, frag.Fragment1)
        for mat in materials:
            mesh.materials.append(mat)
        # assign textures and texture coords to faces
        pos = 0
        texture = mesh.uv_textures.new()
        for count, texID in frag.polygonsByTex:
            for polyID in range(pos, pos + count):
                td = texture.data[polyID]
                td.image = images[texID].image
                td.use_image = True
                mesh.faces[polyID].material_index = texID
                poly = frag.polygons[polyID]
                td.uv1 = normalizeTexCoords(frag.texCoords[poly[1]])
                td.uv2 = normalizeTexCoords(frag.texCoords[poly[2]])
                td.uv3 = normalizeTexCoords(frag.texCoords[poly[3]])
            pos += count
    ob = bpy.data.objects.new(frag.name, mesh)
    bpy.context.scene.objects.link(ob)

    # update mesh to allow proper display
    mesh.validate()
    mesh.update()

def normalizeTexCoords(tc):
    return tc[0] / 256.0, tc[1] / 256.0

def loadTextureMaterials(wld, frag):
    materials, images = [], []
    for texFrag in frag.Textures:
        material = bpy.data.materials.new(name=texFrag.name)
        texture = bpy.data.textures.new(name=texFrag.name, type='IMAGE')
        fileName = texFrag.Reference.Reference.Files[0].FileName
        texture.image = io_utils.load_image(fileName, DefaultPath)
        matTex = material.texture_slots.add()
        matTex.texture = texture
        matTex.texture_coords = 'UV'
        materials.append(material)
        images.append(texture)
    return materials, images
