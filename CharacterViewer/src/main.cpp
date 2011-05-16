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
    bool ok = v.loadCharacters(QString("%1/%2").arg(PATH).arg("global_chr.s3d"));
    if(ok && veliousTextures)
    {
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global17_amr.s3d"));
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global18_amr.s3d"));
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global19_amr.s3d"));
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global20_amr.s3d"));
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global21_amr.s3d"));
        v.loadCharacters(QString("%1/%2").arg(PATH).arg("global22_amr.s3d"));
    }
    return ok;
}

bool loadOldChars(CharacterViewerWindow &v)
{
    return v.loadCharacters(QString("%1/%2").arg(PATH).arg("chequip.s3d"));
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
    //if(!loadGlobalChars(v, false))
    if(!loadOldChars(v))
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
