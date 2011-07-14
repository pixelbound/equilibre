#include <GL/glew.h>
#include <QApplication>
#include <QGLFormat>
#include <QDir>
#include <QMessageBox>
#include "Scene.h"
#include "Zone.h"
#include "WLDActor.h"
#include "WLDModel.h"
#include "WLDSkeleton.h"
#include "RenderStateGL2.h"
#include "CharacterViewerWindow.h"
#include "ZoneViewerWindow.h"

QWidget * showCharViewer(RenderState *state)
{
    // create viewport for rendering the scene
    CharacterViewerWindow *v = new CharacterViewerWindow(state);
    CharacterScene *scene = v->scene();
    Zone *z = scene->zone();
    QDir assetDir(scene->assetPath());
    z->loadCharacters(assetDir.absoluteFilePath("global_chr.s3d"));
    z->loadCharacters(assetDir.absoluteFilePath("gequip.s3d"));

    WLDActor *weaponActor = z->charModels().value("IT106");
    WLDActor *weaponActor2 = z->charModels().value("IT113");
    WLDActor *charActor = z->charModels().value("BAF");
    WLDActor *skelActor = z->charModels().value("ELF");
    if(charActor && weaponActor)
        charActor->addEquip(WLDActor::Right, weaponActor);
    if(charActor && weaponActor2)
        charActor->addEquip(WLDActor::Left, weaponActor2);
    if(charActor && skelActor)
    {
        charActor->model()->skeleton()->copyAnimationsFrom(skelActor->model()->skeleton());
        charActor->setAnimName("P01");
        charActor->setPaletteName("03");
    }
    return v;
}

QWidget * showZoneViewer(RenderState *state)
{
    //ZoneViewerWindow *v = new ZoneViewerWindow(state);
    //v->scene()->zone()->load(v->scene()->assetPath(), "gfaydark");
    return new ZoneViewerWindow(state);
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
    //QWidget *v = showCharViewer(&state);
    QWidget *v = showZoneViewer(&state);
    v->setWindowState(Qt::WindowMaximized);
    v->show();
    app.exec();
    return 0;
}
