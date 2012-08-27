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
#include "EQuilibre/Render/RenderState.h"
#include "EQuilibre/Render/dds.h"
#include "EQuilibre/Render/dxt.h"

Material::Material()
{
    m_origin = LowerLeft;
    m_texture = 0;
    m_subTexture = 0;
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

QImage Material::image() const
{
    return m_img;
}

void Material::setImage(QImage newImage)
{
    m_img = newImage;
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

MaterialMap::MaterialMap()
{
    m_uploaded = false;
    m_arrayTexture = 0;
}

MaterialMap::~MaterialMap()
{
    foreach(Material *mat, m_materials)
        delete mat;
}

const QMap<uint32_t, Material *> & MaterialMap::materials() const
{
    return m_materials;
}

Material * MaterialMap::material(uint32_t matID) const
{
    return m_materials.value(matID);
}

void MaterialMap::setMaterial(uint32_t matID, Material *mat)
{
    m_materials[matID] = mat;
}

texture_t MaterialMap::arrayTexture() const
{
    return m_arrayTexture;
}

bool MaterialMap::uploaded() const
{
    return m_uploaded;
}

void MaterialMap::textureArrayInfo(int &maxWidth, int &maxHeight,
                                   size_t &totalMem, size_t &usedMem) const
{
    const size_t pixelSize = 4;
    size_t count = 0;
    maxWidth = maxHeight = totalMem = usedMem = 0;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        int width = mat->image().width();
        int height = mat->image().height();
        maxWidth = qMax(maxWidth, width);
        maxHeight = qMax(maxHeight, height);
        usedMem += (width * height * pixelSize);
        //qDebug("Texture %d x %d", width, height);
        count++;
    }
    totalMem = maxWidth * maxHeight * pixelSize * count;
}

void MaterialMap::upload(RenderState *state)
{
    if(m_uploaded)
        return;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        if(!mat->image().isNull() && (mat->texture() == 0))
        {
            QImage img = mat->image();
            if(mat->origin() != Material::LowerLeft)
              img = QGLWidget::convertToGLFormat(img);
            mat->setTexture(state->loadTexture(img));
        }
    }
    m_uploaded = true;
}

void MaterialMap::uploadArray(RenderState *state)
{
    if(m_uploaded)
        return;
    QVector<Material *> uploadMats;
    QVector<QImage> images;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        if(!mat->image().isNull() && (mat->texture() == 0))
        {
            QImage img = mat->image();
            if(mat->origin() != Material::LowerLeft)
              img = QGLWidget::convertToGLFormat(img);
            uploadMats.append(mat);
            images.append(img);
        }
    }
    
    m_arrayTexture = state->loadTextures(images.constData(), images.count());
    for(uint i = 0; i < uploadMats.count(); i++)
    {
        Material *mat = uploadMats[i];
        mat->setTexture(m_arrayTexture);
        mat->setSubTexture(i + 1);
    }
    m_uploaded = true;
}
