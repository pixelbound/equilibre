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

#include <math.h>
#include <QImage>
#include <QRegExp>
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Render/Material.h"

using namespace std;

WLDMaterialPalette::WLDMaterialPalette(PFSArchive *archive)
{
    m_archive = archive;
    m_def = NULL;
    m_array = NULL;
    m_arrayOffset = 0;
    m_map = NULL;
}

WLDMaterialPalette::~WLDMaterialPalette()
{
    std::vector<WLDMaterialSlot *>::iterator i, e = m_materialSlots.end();
    for(i = m_materialSlots.begin(); i != e; i++)
        delete *i;
    delete m_array;
    delete m_map;
}

MaterialPaletteFragment * WLDMaterialPalette::def() const
{
    return m_def;
}

void WLDMaterialPalette::setDef(MaterialPaletteFragment *newDef)
{
    m_def = newDef;
}

MaterialArray * WLDMaterialPalette::array() const
{
    return m_array;
}

MaterialMap * WLDMaterialPalette::map() const
{
    return m_map;   
}

uint32_t WLDMaterialPalette::arrayOffset() const
{
    return m_arrayOffset;
}

void WLDMaterialPalette::setArrayOffset(uint32_t offset)
{
    m_arrayOffset = offset;
}

std::vector<WLDMaterialSlot *> & WLDMaterialPalette::materialSlots()
{
    return m_materialSlots;
}

void WLDMaterialPalette::createSlots(bool addMatDefs)
{
    if(!m_def)
        return;
    for(uint32_t i = 0; i < m_def->m_materials.size(); i++)
    {
        MaterialDefFragment *matDef = m_def->m_materials[i];
        WLDMaterialSlot *slot = new WLDMaterialSlot(matDef->name());
        if(addMatDefs)
            slot->addSkinMaterial(0, matDef);
        slot->visible = (matDef->m_renderMode != 0);
        m_materialSlots.push_back(slot);
    }
}

WLDMaterialSlot * WLDMaterialPalette::slotByName(const QString &name) const
{
    for(uint32_t i = 0; i < m_materialSlots.size(); i++)
    {
        WLDMaterialSlot *slot = m_materialSlots[i];
        if(slot->slotName == name)
            return slot;
    }
    return NULL;
}

void WLDMaterialPalette::addMeshMaterials(MeshDefFragment *meshDef, uint32_t skinID)
{
    QVector<vec2us> &texMap = meshDef->m_polygonsByTex;
    for(uint32_t i = 0; i < texMap.size(); i++)
    {
        uint32_t slotID = texMap[i].second;
        WLDMaterialSlot *slot = m_materialSlots[slotID];
        slot->addSkinMaterial(skinID, m_def->m_materials[slotID]);
    }
}

bool WLDMaterialPalette::explodeName(QString defName, QString &charName,
                        QString &palName, QString &partName)
{
    // e.g. defName == 'ORCCH0201_MDF'
    // 'ORC' : character
    // 'CH' : piece (part 1)
    // '02' : palette ID
    // '01' : piece (part 2)
    static QRegExp r("^\\w{5}\\d{4}_MDF$");
    if(r.exactMatch(defName))
    {
        charName = defName.left(3);
        palName = defName.mid(5, 2);
        partName = defName.mid(3, 2) + defName.mid(7, 2);
        return true;
    }
    return false;
}

bool WLDMaterialPalette::explodeName(MaterialDefFragment *def, QString &charName,
                        QString &palName, QString &partName)
{
    return explodeName(def->name(), charName, palName, partName);
}

QString WLDMaterialPalette::materialName(QString defName)
{
    QString charName, palName, partName;
    if(explodeName(defName, charName, palName, partName))
        return charName + "00" + partName;
    else
        return defName.replace("_MDF", "");
}

QString WLDMaterialPalette::materialName(MaterialDefFragment *def)
{
    return materialName(def->name());
}

uint32_t WLDMaterialPalette::materialHash(QString matName)
{
    uint32_t hash = 2166136261u;
    for(int i = 0; i < matName.length(); i++)
    {
        hash ^= matName[i].toLatin1();
        hash *= 16777619u;
    }
    return hash;
}

bool WLDMaterialPalette::exportMaterial(WLDMaterial &wldMat, MaterialArray *array, uint32_t &pos)
{
    // Don't export invisible materials.
    MaterialDefFragment *matDef = wldMat.def();
    Material *mat = NULL;
    if(matDef && (matDef->m_renderMode != 0))
        mat = loadMaterial(matDef);
    wldMat.setMaterial(mat ? mat : NULL);
    wldMat.setIndex(mat ? pos : WLDMaterial::INVALID_INDEX);
    array->setMaterial(pos, mat);
    pos++;
    return (mat != NULL);
}

static uint32_t findSkinIndex(WLDMaterialSlot *slot, uint32_t skinID)
{
    // See if the slot has a material with the specified skin ID.
    const WLDMaterial *mat = slot->material(skinID);
    if(mat && mat->isValid())
        return mat->index();
    
    // Otherwise find the first valid material in the slot.
    skinID = 0;
    mat = slot->material(skinID);
    while(mat)
    {
        if(mat && mat->isValid())
            return mat->index();
        mat = slot->material(skinID++);
    }
    return -1;
}

void WLDMaterialPalette::makeSkinMap(uint32_t skinID, MaterialMap *materialMap) const
{
    size_t endSkinID = qMin(m_materialSlots.size(), materialMap->count());
    for(size_t i = 0; i < endSkinID; i++)
    {
        WLDMaterialSlot *slot = m_materialSlots[i];
        materialMap->setMappingAt(i, findSkinIndex(slot, skinID));
    }
}

void WLDMaterialPalette::makeSkinMap(const vector<uint32_t> &skinIDs, vector<uint32_t> &slotIndices) const
{
    size_t endSkinID = qMin(m_materialSlots.size(), skinIDs.size());
    Q_ASSERT(slotIndices.size() >= skinIDs.size());
    for(size_t i = 0; i < endSkinID; i++)
    {
        uint32_t skinID = skinIDs[i];
        WLDMaterialSlot *slot = m_materialSlots[i];
        slotIndices[i] = findSkinIndex(slot, skinID);
    }
}

void WLDMaterialPalette::exportTo(MaterialArray *array)
{
    // Materials are exported to the array in skin-major order. This mean we
    // export every slot of the base skin, then every slot of the first
    // alternative skin and so on.

    // Keep track of the index of the first material of this palette into the array.
    uint32_t pos = array->materials().size();
    m_arrayOffset = pos;

    // Export the base skin's materials.
    uint32_t maxSkinID = 0;
    for(uint32_t j = 0; j < m_materialSlots.size(); j++)
    {
        WLDMaterialSlot *slot = m_materialSlots[j];
        exportMaterial(slot->baseMat, array, pos);
        maxSkinID = qMax(maxSkinID, (uint32_t)slot->skinMats.size());
    }

    // Export other skins' materials.
    for(uint32_t i = 0; i < maxSkinID; i++)
    {
        for(uint32_t j = 0; j < m_materialSlots.size(); j++)
        {
            WLDMaterialSlot *slot = m_materialSlots[j];
            if(i < slot->skinMats.size())
                exportMaterial(slot->skinMats[i], array, pos);
        }
    }
}

MaterialArray * WLDMaterialPalette::createArray()
{
    if(!m_array)
    {
        m_array = new MaterialArray();
        exportTo(m_array);
    }
    return m_array;
}

MaterialMap * WLDMaterialPalette::createMap()
{
    if(!m_map)
    {
        m_map = new MaterialMap();
        m_map->resize(m_materialSlots.size());
    }
    return m_map;
}

void WLDMaterialPalette::animate(double currentTime)
{
    if(m_array && m_map)
    {
        size_t count = m_map->count();
        for(size_t i = 0; i < count; i++)
        {
            Material *mat = m_array->material(i);
            uint32_t offset = 0;
            if(mat && (mat->duration() > 0))
            {
                // Animated texture.
                uint32_t frames = mat->subTextureCount();
                if(frames)
                {
                    double totalSec = (1.0 / mat->duration()) * frames * 50.0;
                    double animTime = fmod(currentTime, totalSec) / totalSec;
                    offset = qMin((uint32_t)floor(animTime * frames), frames - 1);
                }
            }
            m_map->setOffsetAt(i, offset);
        }
    }
}

Material * WLDMaterialPalette::loadMaterial(MaterialDefFragment *frag)
{
    if(!frag)
        return 0;
    SpriteFragment *sprite = frag->m_sprite;
    if(!sprite)
        return 0;
    SpriteDefFragment *spriteDef = sprite->m_def;
    if(!spriteDef || !spriteDef->m_bitmaps.size())
        return 0;

    bool opaque = true;
    bool dds = false;
    QVector<QImage> images;
    if(m_archive)
    {
        foreach(BitmapNameFragment *bmp, spriteDef->m_bitmaps)
        {
            // XXX case-insensitive lookup
            QByteArray data = m_archive->unpackFile(bmp->m_fileName.toLower());
            QImage img;
            if(!img.loadFromData(data))
            {
                if(Material::loadTextureDDS(data.constData(), data.length(), img))
                {
                    dds = true;
                    images.append(img);
                }
            }
            else
            {
                images.append(img);
            }
        }
    }
    
    if(frag->m_renderMode & MaterialDefFragment::USER_DEFINED)
    {
        uint32_t renderMode = (frag->m_renderMode & ~MaterialDefFragment::USER_DEFINED);
        if(renderMode == 0x01)
        {
            // normal rendering
        }
        else if(renderMode == 0xb)
        {
            // this is used for ghost materials (USERDEFINED 12)
        }
        else if(renderMode == 0x12)
        {
            // this is used by SCAHE0004_MDF (USERDEFINED 19)
        }
        else if(renderMode == 0x13)
        {
            // masked texture (USERDEFINED 20)
            for(size_t i = 0; i < images.size(); i++)
            {
                QImage &img = images[i];
                if(img.colorCount() > 0)
                {
                    // replace the mask color by a transparent color in the table
                    QVector<QRgb> colors = img.colorTable();
                    uint8_t maskColor = 0;
                    colors[maskColor] = qRgba(0, 0, 0, 0);
                    img.setColorTable(colors);
                }
            }
            opaque = false;
        }
        else if(renderMode == 0x14)
        {
            // USERDEFINED 21
        }
        else if(renderMode == 0x17)
        {
            // semi-transparent (e.g. the sleeper, wasp, bixie)
            // depends on how dark/light the color is
            for(size_t i = 0; i < images.size(); i++)
            {
                QImage &img = images[i];
                if(img.colorCount() > 0)
                {
                    QVector<QRgb> colors = img.colorTable();
                    for(int i = 0; i < colors.count(); i++)
                    {
                        QRgb c = colors[i];
                        int alpha = (qRed(c) + qGreen(c) + qBlue(c)) / 3;
                        colors[i] = qRgba(qRed(c), qGreen(c), qBlue(c), alpha);
                    }
                    img.setColorTable(colors);
                }
            }
            opaque = false;
        }
        else if(renderMode == 0x05)
        {
            // semi-transparent (water elemental, air elemental, ghost wolf)
            for(size_t i = 0; i < images.size(); i++)
            {
                QImage &img = images[i];
                if(img.colorCount() > 0)
                {
                    QVector<QRgb> colors = img.colorTable();
                    int alpha = 127; // arbitrary value XXX find the real value
                    for(int i = 0; i < colors.count(); i++)
                    {
                        QRgb c = colors[i];
                        colors[i] = qRgba(qRed(c), qGreen(c), qBlue(c), alpha);
                    }
                    img.setColorTable(colors);
                }
            }
            opaque = false;
        }
        else
        {
            qDebug("Unknown render mode %x", renderMode);
        }
    }

    Material *mat = new Material();
    mat->setOpaque(opaque);
    mat->setImages(images);
    mat->setOrigin(dds ? Material::LowerLeft : Material::UpperLeft);
    mat->setDuration(spriteDef->m_duration);
    return mat;
}

///////////////////////////////////////////////////////////////////////////////

WLDMaterial::WLDMaterial()
{
    m_mat = NULL;
    m_def = NULL;
    m_index = INVALID_INDEX;
}

bool WLDMaterial::isValid() const
{
    return m_def && (m_index != INVALID_INDEX);
}

uint32_t WLDMaterial::index() const
{
    return m_index;
}

void WLDMaterial::setIndex(uint32_t index)
{
    m_index = index;
}

MaterialDefFragment * WLDMaterial::def()
{
    return m_def;
}

void WLDMaterial::setDef(MaterialDefFragment *matDef)
{
    if(matDef->handled())
        return;
    if((m_def != NULL) && (m_def != matDef))
        qDebug("warning: duplicated material definitions '%s' (fragments %d and %d)",
               m_def->name().toLatin1().constData(), m_def->ID(), matDef->ID());
    m_def = matDef;
    matDef->setHandled(true);
}

Material * WLDMaterial::material() const
{
    return m_mat;
}

void WLDMaterial::setMaterial(Material *material)
{
    m_mat = material;
}

///////////////////////////////////////////////////////////////////////////////

WLDMaterialSlot::WLDMaterialSlot(QString matName)
{
    QString charName, palName;
    if(!WLDMaterialPalette::explodeName(matName, charName, palName, slotName))
        slotName = matName.replace("_MDF", "");
    offset = 0;
    visible = false;
}

const WLDMaterial * WLDMaterialSlot::material(uint32_t skinID) const
{
    if(skinID == 0)
        return &baseMat;
    else if(skinID <= skinMats.size())
        return &skinMats[skinID - 1];
    else
        return NULL;
}

void WLDMaterialSlot::addSkinMaterial(uint32_t skinID, MaterialDefFragment *matDef)
{
    if(skinID == 0)
    {
        // Skin zero is the base skin.
        baseMat.setDef(matDef);
    }
    else
    {
        if(skinID > skinMats.size())
            skinMats.resize(skinID);
        skinMats[skinID-1].setDef(matDef);
    }
}
