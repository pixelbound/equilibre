#include <QApplication>
#include <QGLFormat>
#include <QMessageBox>
#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include "SceneViewport.h"
#include "Scene.h"
#include "RenderState.h"
#include "RenderStateGL2.h"
#include "WLDModel.h"

//const char *PATH = "Data";
const char *PATH = "EQ Classic";

bool loadResources(Scene *scene)
{
    return scene->openZone(PATH, "gfaydark");
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
    SceneViewport vp(&scene, &state, f);
    vp.makeCurrent();
    if(!loadResources(&scene))
    {
        QMessageBox::critical(0, "Error", "Could not load the zone.");
        return 1;
    }

    // add all mesh names to the combo box
    QComboBox *meshCombo = new QComboBox();
    foreach(QString name, scene.models().keys())
        meshCombo->addItem(name);
    scene.setSelectedModelName(meshCombo->itemText(0));
    QObject::connect(meshCombo, SIGNAL(activated(QString)),
                     &scene, SLOT(setSelectedModelName(QString)));

    QWidget *w = new QWidget();
    w->setWindowState(Qt::WindowMaximized);
    w->setWindowTitle("OpenEQ Character Viewer");

    QVBoxLayout *l = new QVBoxLayout(w);
    l->addWidget(meshCombo);
    l->addWidget(&vp);

    w->show();
    
    // main window loop
    app.exec();
    return 0;
}
