#include <GL/glew.h>
#include <QApplication>
#include <QGLFormat>
#include <QDir>
#include <QMessageBox>
#include "OpenEQ/Render/Scene.h"
#include "OpenEQ/Render/RenderStateGL2.h"
#include "OpenEQ/Game/Zone.h"
#include "OpenEQ/Game/WLDActor.h"
#include "OpenEQ/Game/WLDModel.h"
#include "OpenEQ/Game/WLDSkeleton.h"
#include "CharacterViewerWindow.h"

QWidget * showCharViewer(RenderState *state)
{
    // create viewport for rendering the scene
    CharacterViewerWindow *v = new CharacterViewerWindow(state);
    CharacterScene *scene = v->scene();
    Zone *z = scene->zone();
    QDir assetDir(scene->assetPath());
    
    CharacterPack *charPack = z->loadCharacters(assetDir.absoluteFilePath("global_chr.s3d"));
    if(!charPack)
        return v;
    CharacterPack *itemPack = z->loadCharacters(assetDir.absoluteFilePath("gequip.s3d"));
    if(!itemPack)
        return v;
    
    WLDActor *charActor = charPack->models().value("BAF");
    WLDActor *skelActor = charPack->models().value("ELF");
    WLDActor *weaponActor = itemPack->models().value("IT106");
    WLDActor *weaponActor2 = itemPack->models().value("IT113");
    if(charActor && weaponActor)
        charActor->addEquip(WLDActor::Right, weaponActor);
    if(charActor && weaponActor2)
        charActor->addEquip(WLDActor::Left, weaponActor2);
    if(charActor && skelActor)
    {
        charActor->complexModel()->skeleton()->copyAnimationsFrom(skelActor->complexModel()->skeleton());
        charActor->setAnimName("P01");
        charActor->setPaletteName("03");
    }
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
    RenderStateGL2 state;

    // main window loop
    QWidget *v = showCharViewer(&state);
    v->setWindowState(Qt::WindowMaximized);
    v->show();
    app.exec();
    return 0;
}
