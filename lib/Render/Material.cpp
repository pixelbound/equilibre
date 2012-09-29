// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <QImage>
#include <QGLWidget>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/dds.h"
#include "EQuilibre/Render/dxt.h"

Material::Material()
{
    m_origin = LowerLeft;
    m_texture = 0;
    m_subTexture = 0;
    m_subTextureCount = 0;
    m_duration = 0;
    m_opaque = true;
}

bool Material::isOpaque() const
{
    return m_opaque;
}

void Material::setOpaque(bool opaque)
{
    m_opaque = opaque;
}

int Material::width() const
{
    return m_images.size() ? m_images.at(0).width() : 0;
}

int Material::height() const
{
    return m_images.size() ? m_images.at(0).height() : 0;
}

const QVector<QImage> & Material::images() const
{
    return m_images;
}

void Material::setImages(const QVector<QImage> &newImages)
{
    m_images = newImages;
}

Material::OriginType Material::origin() const
{
    return m_origin;
}

void Material::setOrigin(Material::OriginType newOrigin)
{
    m_origin = newOrigin;
}

texture_t Material::texture() const
{
    return m_texture;
}

void Material::setTexture(texture_t texture)
{
    m_texture = texture;
}

uint Material::subTexture() const
{
    return m_subTexture;
}

void Material::setSubTexture(uint newID)
{
    m_subTexture = newID;
}

uint32_t Material::subTextureCount() const
{
    return m_subTextureCount;
}

void Material::setSubTextureCount(uint32_t count)
{
    m_subTextureCount = count;
}

uint32_t Material::duration() const
{
    return m_duration;
}

void Material::setDuration(uint32_t durationMs)
{
    m_duration = durationMs;
}

bool Material::loadTextureDDS(const char *data, size_t size, QImage &img)
{
    dds_header_t hdr;
    dds_pixel_format_t pf;
    const char *d = data;
    size_t left = size;
    if(left < sizeof(hdr))
        return false;
    memcpy(&hdr, d, sizeof(hdr));
    left -= sizeof(hdr);
    d += sizeof(hdr);

    if(memcmp(hdr.magic, "DDS ", 4) || (hdr.size != 124) ||
        !(hdr.flags & DDSD_PIXELFORMAT) || !(hdr.flags & DDSD_CAPS) )
        return false;
    if(!(hdr.flags & DDSD_LINEARSIZE) || (hdr.pitch_or_linsize > left))
        return false;

    pf = hdr.pixelfmt;
    if((pf.flags & DDPF_FOURCC))
    {
        int format = DDS_COMPRESS_NONE;
        if(memcmp(pf.fourcc, "DXT1", 4) == 0)
            format = DDS_COMPRESS_BC1;
        else if(memcmp(pf.fourcc, "DXT5", 4) == 0)
            format = DDS_COMPRESS_BC3;
        if(format != DDS_COMPRESS_NONE)
        {
            img = QImage(hdr.width, hdr.height, QImage::Format_ARGB32);
            dxt_decompress(img.bits(), (unsigned char *)d, format, left, hdr.width, hdr.height, 4);
            return true;
        }
    }
    qDebug("DDS format not supported / not implemented");
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MaterialArray::MaterialArray()
{
    m_uploaded = false;
    m_arrayTexture = 0;
    m_maxWidth = m_maxHeight = 0;
}

MaterialArray::~MaterialArray()
{
    foreach(Material *mat, m_materials)
        delete mat;
}

const QVector<Material *> & MaterialArray::materials() const
{
    return m_materials;
}

Material * MaterialArray::material(uint32_t matID) const
{
    return (matID < m_materials.size()) ? m_materials.value(matID) : NULL;
}

void MaterialArray::setMaterial(uint32_t matID, Material *mat)
{
    if(matID >= m_materials.size())
        m_materials.resize(matID + 1);
    m_materials[matID] = mat;
}

texture_t MaterialArray::arrayTexture() const
{
    return m_arrayTexture;
}

bool MaterialArray::uploaded() const
{
    return m_uploaded;
}

int MaterialArray::maxWidth() const
{
    return m_maxWidth;
}

int MaterialArray::maxHeight() const
{
    return m_maxHeight;
}

void MaterialArray::textureArrayInfo(int &maxWidth, int &maxHeight,
                                   size_t &totalMem, size_t &usedMem) const
{
    const size_t pixelSize = 4;
    size_t count = 0;
    maxWidth = maxHeight = totalMem = usedMem = 0;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        foreach(QImage img, mat->images())
        {
            int width = img.width();
            int height = img.height();
            maxWidth = qMax(maxWidth, width);
            maxHeight = qMax(maxHeight, height);
            usedMem += (width * height * pixelSize);
            //qDebug("Texture %d x %d", width, height);
            count++;
        }
    }
    totalMem = maxWidth * maxHeight * pixelSize * count;
}

void MaterialArray::upload(RenderContext *renderCtx)
{
    if(m_uploaded)
        return;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        foreach(QImage img, mat->images())
        {
            if(!img.isNull() && (mat->texture() == 0))
            {
                texture_t tex = 0;
                if(mat->origin() != Material::LowerLeft)
                    tex = renderCtx->loadTexture(QGLWidget::convertToGLFormat(img));
                else
                    tex = renderCtx->loadTexture(img);
                mat->setTexture(tex);
            }
        }
    }
    m_uploaded = true;
    
    size_t totalMem, usedMem;
    textureArrayInfo(m_maxWidth, m_maxHeight, totalMem, usedMem);
}

void MaterialArray::uploadArray(RenderContext *renderCtx)
{
    if(m_uploaded)
        return;
    QVector<Material *> uploadMats;
    QVector<QImage> images;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        uint32_t subTextures = 0;
        foreach(QImage img, mat->images())
        {
            if(!img.isNull() && (mat->texture() == 0))
            {
                if(mat->origin() != Material::LowerLeft)
                    img = QGLWidget::convertToGLFormat(img);
                images.append(img);
                subTextures++;
            }
        }
        mat->setSubTextureCount(subTextures);
        uploadMats.append(mat);
    }
    
    m_arrayTexture = renderCtx->loadTextures(images.constData(), images.count());
    uint32_t subTex = 1;
    for(uint i = 0; i < uploadMats.count(); i++)
    {
        Material *mat = uploadMats[i];
        mat->setTexture(m_arrayTexture);
        mat->setSubTexture(subTex);
        subTex += mat->subTextureCount();
    }
    m_uploaded = true;
    
    size_t totalMem, usedMem;
    textureArrayInfo(m_maxWidth, m_maxHeight, totalMem, usedMem);
}

//////////////////////////////////////////////////////////////////////////////

size_t MaterialMap::count() const
{
    return m_mappings.count();
}

uint32_t * MaterialMap::mappings()
{
    return m_mappings.data();
}

const uint32_t * MaterialMap::mappings() const
{
    return m_mappings.constData();
}

uint32_t * MaterialMap::offsets()
{
    return m_offsets.data();
}

const uint32_t * MaterialMap::offsets() const
{
    return m_offsets.constData();
}

uint32_t MaterialMap::mappingAt(size_t index) const
{
    return (index < m_mappings.count()) ? m_mappings[index] : index;
}

void MaterialMap::setMappingAt(size_t index, uint32_t mapping)
{
    m_mappings[index] = mapping;
}

uint32_t MaterialMap::offsetAt(size_t index) const
{
    return (index < m_offsets.count()) ? m_offsets[index] : 0;
}

void MaterialMap::setOffsetAt(size_t index, uint32_t offset)
{
    m_offsets[index] = offset;
}

void MaterialMap::resize(size_t newCount)
{
    size_t oldCount = count();
    m_mappings.resize(newCount);
    for(size_t i = oldCount; i < newCount; i++)
        m_mappings[i] = i;
    m_offsets.resize(newCount);
    for(size_t i = oldCount; i < newCount; i++)
        m_offsets[i] = 0;
}

void MaterialMap::clear()
{
    m_mappings.clear();
    m_offsets.clear();
}

void MaterialMap::fillTextureMap(MaterialArray *materials, vec3 *textureMap, size_t count) const
{
    int maxTexWidth = materials ? materials->maxWidth() : 0;
    int maxTexHeight = materials ? materials->maxHeight() : 0;
    for(size_t i = 0; i < count; i++)
    {
        uint32_t texID = (uint32_t)(i + 1);
        float matScalingX = 1.0, matScalingY = 1.0;
        if(i < count)
        {
            uint32_t matID = mappingAt(i);
            uint32_t texOffset = offsetAt(i);
            Material *mat = materials ? materials->material(matID) : NULL;
            if(mat)
            {
                matScalingX = (float)mat->width() / (float)maxTexWidth;
                matScalingY = (float)mat->height() / (float)maxTexHeight;
                texID = mat->subTexture() + texOffset;
            }
            else
            {
                texID = matID + texOffset;
            }
        }
        textureMap[i] = vec3(matScalingX, matScalingY, (float)texID);
    }
}
