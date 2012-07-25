#ifndef OPENEQ_MATERIAL_H
#define OPENEQ_MATERIAL_H

#include <string>
#include <inttypes.h>
#include <QImage>
#include <QMap>
#include "Vertex.h"

class RenderState;
class VertexGroup;

class Material
{
public:
    enum OriginType
    {
        LowerLeft,
        UpperLeft
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
    
    uint subTexture() const;
    void setSubTexture(uint newID);

    static bool loadTextureDDS(const char *data, size_t size, QImage &img);

private:
    vec4 m_ambient;
    vec4 m_diffuse;
    vec4 m_specular;
    float m_shine;
    QImage m_img;
    OriginType m_origin;
    texture_t m_texture;
    uint m_subTexture;
    bool m_opaque;
};

class MaterialMap
{
public:
    MaterialMap();
    ~MaterialMap();
    const QMap<QString, Material *> & materials() const;
    Material * material(QString name) const;
    void setMaterial(QString name, Material *mat);
    
    texture_t arrayTexture() const;
    bool uploaded() const;
    
    void upload(RenderState *state);
    void uploadArray(RenderState *state);
    void updateTexCoords(VertexGroup *vg);
    void textureArrayInfo(int &maxWidth, int &maxHeight, size_t &totalMem, size_t &usedMem) const;
    
private:
    QMap<QString, Material *> m_materials;
    texture_t m_arrayTexture;
    bool m_uploaded;
};

#endif
