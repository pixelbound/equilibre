#include <QImage>
#include <QGLWidget>
#include <cstring>
#include <cstdio>
#include "Platform.h"
#include "Material.h"

Material::Material()
{
    m_ambient = vec4(0.0, 0.0, 0.0, 0.0);
    m_diffuse = vec4(0.0, 0.0, 0.0, 0.0);
    m_specular = vec4(0.0, 0.0, 0.0, 0.0);
    m_shine = 0.0;
    m_texture = 0;
}

Material::Material(vec4 ambient, vec4 diffuse, vec4 specular, float shine)
{
    m_ambient = ambient;
    m_diffuse = diffuse;
    m_specular = specular;
    m_shine = shine;
    m_texture = 0;
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

void Material::loadTexture(QImage &img, bool mipmaps)
{
    m_texture = textureFromImage(img, mipmaps);
}

void Material::loadTexture(string path, bool mipmaps)
{
    m_texture = textureFromImage(path, mipmaps);
}

void Material::loadTexture(const char *data, size_t size, bool mipmaps)
{
    m_texture = textureFromImage(data, size, mipmaps);
}

uint32_t Material::textureFromImage(QImage &img, bool mipmaps)
{
    QImage img2 = QGLWidget::convertToGLFormat(img);
    uint32_t texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img2.width(), img2.height(), 0,
        GL_RGBA, GL_UNSIGNED_BYTE, img2.bits());
    setTextureParams(GL_TEXTURE_2D, false);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

uint32_t Material::textureFromImage(string path, bool mipmaps)
{
    QImage img(QString::fromStdString(path));
    return textureFromImage(img, mipmaps);
}

uint32_t Material::textureFromImage(const char *data, size_t size, bool mipmaps)
{
    return 0;
}
