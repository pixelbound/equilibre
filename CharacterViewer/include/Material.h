#ifndef OPENEQ_MATERIAL_H
#define OPENEQ_MATERIAL_H

#include <string>
#include <inttypes.h>
#include "Vertex.h"

using namespace std;

class QImage;

class Material
{
public:
    Material();
    Material(vec4 ambient, vec4 diffuse, vec4 specular, float shine);

    const vec4 & ambient() const;
    const vec4 & diffuse() const;
    const vec4 & specular() const;
    float shine() const;
    void setAmbient(const vec4 &ambient);
    void setDiffuse(const vec4 &diffuse);
    void setSpecular(const vec4 &specular);
    void setShine(float shine);

    uint32_t texture() const;
    void setTexture(uint32_t texture);
    void freeTexture();

    void loadTexture(QImage &img, bool mipmaps = false);
    void loadTexture(string path, bool mipmaps = false);
    void loadTexture(const char *data, size_t size, bool mipmaps = false);
    static uint32_t textureFromImage(QImage &img, bool mipmaps = false);
    static uint32_t textureFromImage(string path, bool mipmaps = false);
    static uint32_t textureFromImage(const char *data, size_t size, bool mipmaps = false);

private:
    vec4 m_ambient;
    vec4 m_diffuse;
    vec4 m_specular;
    float m_shine;
    uint32_t m_texture;
};

#endif
