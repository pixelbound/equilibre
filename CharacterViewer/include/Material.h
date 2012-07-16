#ifndef OPENEQ_MATERIAL_H
#define OPENEQ_MATERIAL_H

#include <string>
#include <inttypes.h>
#include <QImage>
#include <QMap>
#include "Vertex.h"

class RenderState;

class Material
{
public:
    enum OriginType
    {
        OpenGL,
        Qt
    };
    
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

    bool isOpaque() const;
    void setOpaque(bool opaque);
    
    QImage image() const;
    void setImage(QImage newImage);
    
    OriginType origin() const;
    void setOrigin(OriginType newOrigin);

    texture_t texture() const;
    void setTexture(texture_t texture);

    static bool loadTextureDDS(const char *data, size_t size, QImage &img);

private:
    vec4 m_ambient;
    vec4 m_diffuse;
    vec4 m_specular;
    float m_shine;
    QImage m_img;
    OriginType m_origin;
    texture_t m_texture;
    bool m_opaque;
};

class MaterialMap
{
public:
    MaterialMap();
    ~MaterialMap();
    Material * material(QString name) const;
    void setMaterial(QString name, Material *mat);
    void upload(RenderState *state);
    
private:
    QMap<QString, Material *> m_materials;
    bool m_uploaded;
};

#endif
