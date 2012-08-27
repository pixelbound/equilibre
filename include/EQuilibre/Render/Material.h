#ifndef EQUILIBRE_MATERIAL_H
#define EQUILIBRE_MATERIAL_H

#include <QImage>
#include <QMap>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"

class RenderState;

class RENDER_DLL Material
{
public:
    enum OriginType
    {
        LowerLeft,
        UpperLeft
    };
    
    Material();
    Material(vec4 ambient, vec4 diffuse);

    const vec4 & ambient() const;
    const vec4 & diffuse() const;
    void setAmbient(const vec4 &ambient);
    void setDiffuse(const vec4 &diffuse);

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

class RENDER_DLL MaterialMap
{
public:
    MaterialMap();
    ~MaterialMap();
    const QMap<uint32_t, Material *> &materials() const;
    Material * material(uint32_t matID) const;
    void setMaterial(uint32_t matID, Material *mat);
    
    texture_t arrayTexture() const;
    bool uploaded() const;
    
    void upload(RenderState *state);
    void uploadArray(RenderState *state);
    void textureArrayInfo(int &maxWidth, int &maxHeight, size_t &totalMem, size_t &usedMem) const;
    
private:
    QMap<uint32_t, Material *> m_materials;
    texture_t m_arrayTexture;
    bool m_uploaded;
};

#endif
