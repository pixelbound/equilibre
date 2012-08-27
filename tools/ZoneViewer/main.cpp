#include <GL/glew.h>
#include <QApplication>
#include <QGLFormat>
#include <QDir>
#include <QMessageBox>
#include "EQuilibre/Render/Scene.h"
#include "EQuilibre/Render/RenderStateGL2.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDSkeleton.h"
#include "ZoneViewerWindow.h"

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
    QWidget *v = showZoneViewer(&state);
    v->setWindowState(Qt::WindowMaximized);
    v->show();
    app.exec();
    return 0;
}
