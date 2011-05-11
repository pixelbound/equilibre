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
#include "WLDData.h"
#include "WLDFragment.h"
#include "Fragments.h"

const char *PATH = "Data/gfaydark_obj.d/gfaydark_obj.wld";

bool loadResources(RenderState *state)
{
    WLDData *d = WLDData::fromFile(PATH);
    if(d)
    {
        foreach(WLDFragment *f, d->fragments())
        {
            MeshFragment *mf = f->cast<MeshFragment>();
            if(mf)
            {
                VertexGroup *vg = mf->toGroup();
                state->loadMeshFromGroup(mf->name().toStdString(), vg);
                delete vg;
            }
        }
        delete d;
        return true;
    }
    return false;
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
    if(!loadResources(&state))
    {
        QMessageBox::critical(0, "Error", "Could not load the WLD file.");
        return 1;
    }

    // add all mesh names to the combo box
    QComboBox *meshCombo = new QComboBox();
    map<string, Mesh *>::iterator it = state.meshes().begin();
    while(it != state.meshes().end())
    {
        meshCombo->addItem(QString::fromStdString(it->first));
        it++;
    }
    QObject::connect(meshCombo, SIGNAL(activated(QString)),
                     &scene, SLOT(setMeshName(QString)));

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
