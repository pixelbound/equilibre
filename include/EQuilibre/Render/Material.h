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

#ifndef EQUILIBRE_MATERIAL_H
#define EQUILIBRE_MATERIAL_H

#include <QImage>
#include <QMap>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"

class RenderContext;

class RENDER_DLL Material
{
public:
    enum OriginType
    {
        LowerLeft,
        UpperLeft
    };
    
    Material();

    bool isOpaque() const;
    void setOpaque(bool opaque);
    
    const QImage & image() const;
    void setImage(const QImage &newImage);
    
    OriginType origin() const;
    void setOrigin(OriginType newOrigin);

    texture_t texture() const;
    void setTexture(texture_t texture);
    
    uint subTexture() const;
    void setSubTexture(uint newID);

    static bool loadTextureDDS(const char *data, size_t size, QImage &img);

private:
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
    const QVector<Material *> &materials() const;
    Material * material(uint32_t matID) const;
    void setMaterial(uint32_t matID, Material *mat);
    
    texture_t arrayTexture() const;
    bool uploaded() const;
    
    void upload(RenderContext *renderCtx);
    void uploadArray(RenderContext *renderCtx);
    void textureArrayInfo(int &maxWidth, int &maxHeight, size_t &totalMem, size_t &usedMem) const;
    
private:
    QVector<Material *> m_materials;
    texture_t m_arrayTexture;
    bool m_uploaded;
};

#endif
