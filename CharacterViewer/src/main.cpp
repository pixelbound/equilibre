#include <QApplication>
#include <QGLFormat>
#include <QMessageBox>
#include "SceneViewport.h"
#include "Scene.h"
#include "RenderState.h"
#include "RenderStateGL2.h"

bool loadResources(RenderState *state)
{
    return true;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // define OpenGL options
    QGLFormat f;
    f.setAlpha(true);
    f.setSampleBuffers(true);
    f.setSwapInterval(0);

    // create the scene
    RenderStateGL2 state;
    Scene scene(&state);

    // create viewport for rendering the scene
    SceneViewport w(&scene, &state, f);
    w.setWindowState(Qt::WindowMaximized);
    w.setWindowTitle("Dragons Demo");
    w.makeCurrent();
    if(!loadResources(&state))
    {
        QMessageBox::critical(0, "Error", "Could not load the mesh.");
        return 1;
    }
    w.show();
    
    // main window loop
    app.exec();
    return 0;
}
