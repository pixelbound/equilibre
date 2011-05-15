#include <QApplication>
#include <QGLFormat>
#include <QMessageBox>
#include "Scene.h"
#include "RenderStateGL2.h"
#include "CharacterViewerWindow.h"

const char *PATH = "Data";
//const char *PATH = "EQ Classic";
const char *ZONE = "gfaydark";

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
    if(!v.loadZone(PATH, ZONE))
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
