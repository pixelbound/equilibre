#include <GL/glew.h>
#include <QApplication>
#include <QGLFormat>
#include <QMessageBox>
#include "Scene.h"
#include "Zone.h"
#include "WLDActor.h"
#include "WLDModel.h"
#include "WLDSkeleton.h"
#include "RenderStateGL2.h"
#include "CharacterViewerWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // define OpenGL options
    QGLFormat f;
    f.setAlpha(true);
    f.setSampleBuffers(true);
    f.setSwapInterval(0);
    QGLFormat::setDefaultFormat(f);

    // create the scene
    RenderStateGL2 state;
    Scene scene(&state);
    Zone *z = scene.zone();
    z->loadCharacters("../Data/global_chr.s3d");
    z->loadCharacters("../Data/gequip.s3d");

//    WLDActor *weaponActor = z->charModels().value("IT106");
//    WLDActor *weaponActor2 = z->charModels().value("IT113");
    WLDActor *charActor = z->charModels().value("BAF");
    WLDActor *skelActor = z->charModels().value("ELF");
//    if(charActor && weaponActor)
//        charActor->addEquip(WLDActor::Right, weaponActor);
//    if(charActor && weaponActor2)
//        charActor->addEquip(WLDActor::Left, weaponActor2);
    if(charActor && skelActor)
    {
        charActor->model()->skeleton()->copyAnimationsFrom(skelActor->model()->skeleton());
        charActor->setAnimName("P01");
        charActor->setPaletteName("03");
    }

    // create viewport for rendering the scene
    CharacterViewerWindow v(&scene, &state);
    v.setWindowState(Qt::WindowMaximized);
    v.show();
    
    // main window loop
    app.exec();
    return 0;
}
