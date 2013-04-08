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

#include <GL/glew.h>
#include <QApplication>
#include <QGLFormat>
#include <QDir>
#include <QMessageBox>
#include "EQuilibre/Render/Scene.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDSkeleton.h"
#include "CharacterViewerWindow.h"

void initCharViewer(CharacterViewerWindow *v)
{
    /*CharacterScene *scene = v->scene();
    Game *game = scene->game();
    QDir assetDir(scene->assetPath());
    
    CharacterPack *charPack = game->loadCharacters(assetDir.absoluteFilePath("global_chr.s3d"));
    if(!charPack)
        return v;
    ObjectPack *itemPack = game->loadObjects(assetDir.absoluteFilePath("gequip.s3d"));
    if(!itemPack)
        return v;
    
    WLDCharActor *charActor = charPack->models().value("BAF");
    WLDCharActor *skelActor = charPack->models().value("ELF");
    WLDMesh *weaponActor = itemPack->models().value("IT106");
    WLDMesh *weaponActor2 = itemPack->models().value("IT113");
    if(charActor && weaponActor)
        charActor->addEquip(WLDCharActor::Right, weaponActor, itemPack->materials());
    if(charActor && weaponActor2)
        charActor->addEquip(WLDCharActor::Left, weaponActor2, itemPack->materials());
    if(charActor && skelActor)
    {
        WLDModel *charModel = charActor->model();
        charModel->skeleton()->copyAnimationsFrom(skelActor->model()->skeleton());
        charActor->setAnimName("P01");
        //charActor->setPaletteName("03");
    }*/
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // define OpenGL options
    QGLFormat f;
    f.setAlpha(true);
    f.setSampleBuffers(true);
    f.setSwapInterval(0);
    QGLFormat::setDefaultFormat(f);
    RenderContext renderCtx;

    // main window loop
    CharacterViewerWindow v(&renderCtx);
    initCharViewer(&v);
    v.setWindowState(Qt::WindowMaximized);
    v.show();
    app.exec();
    
    return 0;
}
