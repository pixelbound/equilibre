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
#include "ZoneViewerWindow.h"

QWidget * showZoneViewer(RenderContext *renderCtx)
{
    ZoneViewerWindow *v = new ZoneViewerWindow(renderCtx);
    QDir assetDir(v->scene()->assetPath());
    QString zoneInfoFile ="zonevars.txt";
    Game *game = v->scene()->game();
    game->loadSky(assetDir.path());
    game->loadBuiltinOjects(assetDir.path());
    game->loadCharacters(assetDir.absoluteFilePath("global_chr.s3d"));
    if(assetDir.exists(zoneInfoFile))
        game->loadZoneInfo(assetDir.filePath(zoneInfoFile));
    return v;
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
    QWidget *v = showZoneViewer(&renderCtx);
    v->setWindowState(Qt::WindowMaximized);
    v->show();
    app.exec();
    
    return 0;
}
