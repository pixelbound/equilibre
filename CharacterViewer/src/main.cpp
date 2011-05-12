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
#include "WLDModel.h"
#include "Fragments.h"
#include "PFSArchive.h"

const char *S3D_PATH = "Data/gfaydark_obj.s3d";
const char *WLD_DIR = "Data/gfaydark_obj.d";
const char *WLD_PATH = "Data/gfaydark_obj.d/gfaydark_obj.wld";

bool loadResources(RenderState *state, Scene *scene)
{
    PFSArchive a(S3D_PATH);
    foreach(QString name, a.files())
        qDebug("%s", name.toLatin1().constData());

    scene->openWLD(WLD_PATH);
    if(scene->wldData())
    {
        foreach(WLDFragment *f, scene->wldData()->fragments())
        {
            MeshFragment *mf = f->cast<MeshFragment>();
            if(mf)
            {
                WLDModel *m = new WLDModel(state, WLD_DIR, scene);
                m->addMesh(mf);
                scene->models().insert(mf->name(), m);
            }
        }
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
    if(!loadResources(&state, &scene))
    {
        QMessageBox::critical(0, "Error", "Could not load the WLD file.");
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
