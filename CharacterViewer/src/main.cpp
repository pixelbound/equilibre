#include <QApplication>
#include <QGLFormat>
#include <QMessageBox>
#include "Scene.h"
#include "RenderStateGL2.h"
#include "CharacterViewerWindow.h"

const char *PATH = "Data";
//const char *PATH = "EQ Classic";
const char *ZONE = "gfaydark";

bool loadGlobalChars(CharacterViewerWindow &v, bool veliousTextures)
{
    if(veliousTextures)
    {
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global17_amr.s3d"), "global17_amr.wld");
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global18_amr.s3d"), "global18_amr.wld");
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global19_amr.s3d"), "global19_amr.wld");
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global20_amr.s3d"), "global20_amr.wld");
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global21_amr.s3d"), "global21_amr.wld");
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global22_amr.s3d"), "global22_amr.wld");
    }
    return v.loadCharacters(QString("%1/%2").arg(PATH).arg("global_chr.s3d"), "global_chr.wld");
}

bool loadOldChars(CharacterViewerWindow &v)
{
    return v.loadCharacters(QString("%1/%2").arg(PATH).arg("chequip.s3d"), "chequip.wld");
}

bool loadZone(CharacterViewerWindow &v)
{
    return v.loadZone(PATH, ZONE);
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

    // create the scene
    RenderStateGL2 state;
    Scene scene(&state);

    // create viewport for rendering the scene
    CharacterViewerWindow v(&scene, &state);
    if(!loadGlobalChars(v, false))
    {
        QMessageBox::critical(0, "Error", "Could not load the zone.");
        return 1;
    }
    v.setWindowState(Qt::WindowMaximized);
    v.show();
    
    // main window loop
    app.exec();
    return 0;
}
