#ifndef OPENEQ_RENDER_STATE_H
#define OPENEQ_RENDER_STATE_H

#include <map>
#include <string>
#include "Mesh.h"
#include "Material.h"
#include "Vertex.h"

using namespace std;

class RenderState
{
public:
    RenderState();
    virtual ~RenderState();

    virtual void init();

    virtual void toggleWireframe();
    virtual void toggleProjection();

    virtual void reset();

    // mesh operations
    virtual void drawMesh(Mesh *m) = 0;
    virtual void drawMesh(string name);

    virtual map<string, Mesh *> & meshes();
    virtual const map<string, Mesh *> & meshes() const;

    virtual Mesh * createMesh() const = 0;
    virtual Mesh * loadMeshFromGroup(string name, VertexGroup *vg);
    virtual void freeMeshes();

    virtual uint32_t loadTextureFromFile(string name, string path, bool mipmaps = false);
    virtual uint32_t loadTextureFromData(string name, const char *data, size_t size, bool mipmaps = false);
    virtual uint32_t texture(string name) const;
    virtual void freeTextures() = 0;

    // matrix operations

    enum MatrixMode
    {
        ModelView,
        Projection,
        Texture
    };

    virtual void setMatrixMode(MatrixMode newMode) = 0;

    virtual void loadIdentity() = 0;
    virtual void multiplyMatrix(const matrix4 &m) = 0;
    virtual void pushMatrix() = 0;
    virtual void popMatrix() = 0;

    virtual void translate(float dx, float dy, float dz) = 0;
    virtual void rotate(float angle, float rx, float ry, float rz) = 0;
    virtual void scale(float sx, float sy, float sz) = 0;

    virtual matrix4 currentMatrix() const = 0;

    // general state operations
    virtual void beginFrame(int width, int heigth) = 0;
    virtual void setupViewport(int width, int heigth) = 0;
    virtual void endFrame() = 0;

    // material operations

    virtual void pushMaterial(const Material &m) = 0;
    virtual void popMaterial() = 0;

protected:
    bool m_projection;
    bool m_wireframe;
    vec4 m_bgColor;
    Mesh *m_meshOutput;
    map<string, uint32_t> m_textures;
    map<string, Mesh *> m_meshes;
};

class StateObject
{
public:
    StateObject(RenderState *s);

    void loadIdentity();
    void pushMatrix();
    void popMatrix();

    void translate(float dx, float dy, float dz);
    void rotate(float angle, float rx, float ry, float rz);
    void scale(float sx, float sy, float sz);

    void drawMesh(Mesh *m);
    void drawMesh(string name);

    void pushMaterial(const Material &m);
    void popMaterial();

protected:
    RenderState *m_state;
};

#endif
