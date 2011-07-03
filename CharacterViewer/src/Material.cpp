#include <QImage>
#include <cstring>
#include <cstdio>
#include "Platform.h"
#include <QGLWidget>
#include "Material.h"
#include "dds.h"
#include "dxt.h"

Material::Material()
{
    m_ambient = vec4(0.0, 0.0, 0.0, 0.0);
    m_diffuse = vec4(0.0, 0.0, 0.0, 0.0);
    m_specular = vec4(0.0, 0.0, 0.0, 0.0);
    m_shine = 0.0;
    m_texture = 0;
    m_opaque = true;
}

Material::Material(vec4 ambient, vec4 diffuse, vec4 specular, float shine)
{
    m_ambient = ambient;
    m_diffuse = diffuse;
    m_specular = specular;
    m_shine = shine;
    m_texture = 0;
    m_opaque = true;
}

const vec4 & Material::ambient() const
{
    return m_ambient;
}

const vec4 & Material::diffuse() const
{
    return m_diffuse;
}

const vec4 & Material::specular() const
{
    return m_specular;
}

float Material::shine() const
{
    return m_shine;
}

void Material::setAmbient(const vec4 &ambient)
{
    m_ambient = ambient;
}

void Material::setDiffuse(const vec4 &diffuse)
{
    m_diffuse = diffuse;
}

void Material::setSpecular(const vec4 &specular)
{
    m_specular = specular;
}

void Material::setShine(float shine)
{
    m_shine = shine;
}

bool Material::isOpaque() const
{
    return m_opaque;
}

void Material::setOpaque(bool opaque)
{
    m_opaque = opaque;
}

uint32_t Material::texture() const
{
    return m_texture;
}

void Material::setTexture(uint32_t texture)
{
    m_texture = texture;
}

void Material::freeTexture()
{
    if(m_texture != 0)
    {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
}

void setTextureParams(uint32_t target, bool mipmaps)
{
    if(mipmaps)
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Material::loadTexture(QImage &img, bool mipmaps, bool convertToGL)
{
    m_texture = textureFromImage(img, mipmaps, convertToGL);
}

void Material::loadTexture(string path, bool mipmaps, bool convertToGL)
{
    m_texture = textureFromImage(path, mipmaps, convertToGL);
}

uint32_t Material::textureFromImage(QImage &img, bool mipmaps, bool convertToGL)
{
    QImage img2;
    if(convertToGL)
        img2 = QGLWidget::convertToGLFormat(img);
    else
        img2 = img;
    uint32_t texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    if(mipmaps)
    {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, img2.width(), img2.height(),
            GL_RGBA, GL_UNSIGNED_BYTE, img2.bits());
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img2.width(), img2.height(), 0,
            GL_RGBA, GL_UNSIGNED_BYTE, img2.bits());
    }
    setTextureParams(GL_TEXTURE_2D, false);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

uint32_t Material::textureFromImage(string path, bool mipmaps, bool convertToGL)
{
    QImage img(QString::fromStdString(path));
    return textureFromImage(img, mipmaps, convertToGL);
}

////////////////////////////////////////////////////////////////////////////////

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
    if((!hdr.flags & DDSD_LINEARSIZE) || (hdr.pitch_or_linsize > left))
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
