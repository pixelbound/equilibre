#include <cstdio>
#include "Platform.h"
#include "RenderStateGL2.h"
#include "MeshGL2.h"
#include "WLDSkeleton.h"

const int MAX_TRANSFORMS = 256;

RenderStateGL2::RenderStateGL2() : RenderState()
{
    m_matrixMode = ModelView;
    m_matrix[(int)ModelView].setIdentity();
    m_matrix[(int)Projection].setIdentity();
    m_matrix[(int)Texture].setIdentity();
    m_ambient0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_diffuse0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_specular0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_light0_pos = vec4(0.0, 1.0, 1.0, 0.0);
    m_shaderLoaded = false;
    m_program = new ShaderProgramGL2();
    m_skinningMode = SoftwareSkinning;
    m_boneTexture = 0;
    m_bones = new vec4[MAX_TRANSFORMS * 2];
}

RenderStateGL2::~RenderStateGL2()
{
    delete m_program;
    delete [] m_bones;
    if(m_boneTexture != 0)
        glDeleteTextures(1, &m_boneTexture);
}

ShaderProgramGL2 * RenderStateGL2::program() const
{
    return m_program;
}

Mesh * RenderStateGL2::createMesh()
{
    return new MeshGL2(this);
}

void RenderStateGL2::drawMesh(Mesh *m, const BoneTransform *bones, int boneCount)
{
    if(!m)
        return;
    m_program->setMatrices(m_matrix[(int)ModelView], m_matrix[(int)Projection]);
    m_program->setUniformValue("u_skinningMode", (int)m_skinningMode);
    m->draw(bones, boneCount);
}

RenderStateGL2::SkinningMode RenderStateGL2::skinningMode() const
{
    return m_skinningMode;
}

void RenderStateGL2::setSkinningMode(RenderStateGL2::SkinningMode newMode)
{
    m_skinningMode = newMode;
}

void RenderStateGL2::setBoneTransforms(const BoneTransform *transforms, int count)
{
    for(int i = 0; i < MAX_TRANSFORMS; i++)
    {
        if(transforms && (i < count))
        {
            QVector4D loc = transforms[i].location;
            QQuaternion rot = transforms[i].rotation;
            m_bones[i * 2 + 0] = vec4(loc.x(), loc.y(), loc.z(), 1.0);
            m_bones[i * 2 + 1] = vec4(rot.x(), rot.y(), rot.z(), rot.scalar());
        }
        else
        {
            m_bones[i * 2 + 0] = vec4(0.0, 0.0, 0.0, 1.0);
            m_bones[i * 2 + 1] = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
}

void RenderStateGL2::uploadBoneTransformsUniform()
{
//    for(int i = 0; i < MAX_TRANSFORMS; i++)
//        setUniformValue(QString("u_bone_translation[%1]").arg(i), m_bones[i * 2 + 0]);
//    for(int i = 0; i < MAX_TRANSFORMS; i++)
//        setUniformValue(QString("u_bone_rotation[%1]").arg(i), m_bones[i * 2 + 1]);
}

void RenderStateGL2::uploadBoneTransformsTexture()
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_boneTexture);
    if(m_skinningMode == HardwareSkinning)
    {
        glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 2, MAX_TRANSFORMS,
            GL_RGBA, GL_FLOAT, m_bones);
    }
    glActiveTexture(GL_TEXTURE0);
}

void RenderStateGL2::startSkinning()
{
    uploadBoneTransformsTexture();
    m_program->setUniformValue("u_bones", 1);
}

void RenderStateGL2::stopSkinning()
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    glActiveTexture(GL_TEXTURE0);
}

void RenderStateGL2::setMatrixMode(RenderStateGL2::MatrixMode newMode)
{
    m_matrixMode = newMode;
}

void RenderStateGL2::loadIdentity()
{
    int i = (int)m_matrixMode;
    m_matrix[i].setIdentity();
}

void RenderStateGL2::multiplyMatrix(const matrix4 &m)
{
    int i = (int)m_matrixMode;
    m_matrix[i] = m_matrix[i] * m;
}

void RenderStateGL2::pushMatrix()
{
    int i = (int)m_matrixMode;
    m_matrixStack[i].push_back(m_matrix[i]);
}

void RenderStateGL2::popMatrix()
{
    int i = (int)m_matrixMode;
    m_matrix[i] = m_matrixStack[i].back();
    m_matrixStack[i].pop_back();
}

void RenderStateGL2::translate(float dx, float dy, float dz)
{
    multiplyMatrix(matrix4::translate(dx, dy, dz));
}

void RenderStateGL2::rotate(float angle, float rx, float ry, float rz)
{
    multiplyMatrix(matrix4::rotate(angle, rx, ry, rz));
}

void RenderStateGL2::scale(float sx, float sy, float sz)
{
    multiplyMatrix(matrix4::scale(sx, sy, sz));
}

matrix4 RenderStateGL2::currentMatrix() const
{
    return m_matrix[(int)m_matrixMode];
}

void RenderStateGL2::pushMaterial(const Material &m)
{
    m_materialStack.push_back(m);
    beginApplyMaterial(m);
}

void RenderStateGL2::popMaterial()
{
    Material m = m_materialStack.back();
    m_materialStack.pop_back();
    endApplyMaterial(m);
    if(m_materialStack.size() > 0)
        beginApplyMaterial(m_materialStack.back());
}

void RenderStateGL2::beginApplyMaterial(const Material &m)
{
    m_program->setUniformValue("u_material_ambient", m.ambient());
    //setUniformValue("u_material_diffuse", m.diffuse());
    //setUniformValue("u_material_specular", m.specular());
    //setUniformValue("u_material_shine", m.shine());
    if(m.texture() != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture());
        m_program->setUniformValue("u_material_texture", 0);
        m_program->setUniformValue("u_has_texture", 1);
    }
    else
    {
        m_program->setUniformValue("u_has_texture", 0);
    }
}

void RenderStateGL2::endApplyMaterial(const Material &m)
{
    if(m.texture() != 0)
        glBindTexture(GL_TEXTURE_2D, 0);
}

bool RenderStateGL2::beginFrame(int w, int h)
{
    glPushAttrib(GL_ENABLE_BIT);
    if(m_shaderLoaded)
        m_program->beginFrame();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setupViewport(w, h);
    setMatrixMode(ModelView);
    pushMatrix();
    loadIdentity();
    if(m_shaderLoaded)
        glClearColor(m_bgColor.x, m_bgColor.y, m_bgColor.z, m_bgColor.w);
    else
        glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return m_shaderLoaded;
}

void RenderStateGL2::endFrame()
{
    setMatrixMode(ModelView);
    popMatrix();
    if(m_shaderLoaded)
        m_program->endFrame();
    glPopAttrib();
    glFlush();
}

void RenderStateGL2::setupViewport(int w, int h)
{
    glViewport(0, 0, w, h);
    setMatrixMode(Projection);
    loadIdentity();
    float r = (float)w / (float)h;
    if(m_projection)
        multiplyMatrix(matrix4::perspective(45.0f, r, 0.1f, 100.0f));
    else if (w <= h)
        multiplyMatrix(matrix4::ortho(-1.0, 1.0, -1.0 / r, 1.0 / r, -10.0, 10.0));
    else
        multiplyMatrix(matrix4::ortho(-1.0 * r, 1.0 * r, -1.0, 1.0, -10.0, 10.0));
    setMatrixMode(ModelView);
}

void RenderStateGL2::init()
{
    m_shaderLoaded = loadShaders();
    glGenTextures(1, &m_boneTexture);
    setBoneTransforms(0, 0);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_boneTexture);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, 2, MAX_TRANSFORMS, 0,
                GL_RGBA, GL_FLOAT, m_bones);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
}

bool RenderStateGL2::loadShaders()
{
    return m_program->load("vertex.glsl", "fragment.glsl");
}

////////////////////////////////////////////////////////////////////////////////

ShaderProgramGL2::ShaderProgramGL2()
{
    m_program = 0;
    m_vertexShader = 0;
    m_fragmentShader = 0;
    m_modelViewMatrixLoc = -1;
    m_projMatrixLoc = -1;
    m_positionAttr = -1;
    m_normalAttr = -1;
    m_texCoordsAttr = -1;
    m_boneAttr = -1;
}

ShaderProgramGL2::~ShaderProgramGL2()
{
    if(m_vertexShader != 0)
        glDeleteShader(m_vertexShader);
    if(m_fragmentShader != 0)
        glDeleteShader(m_fragmentShader);
    if(m_program != 0)
        glDeleteProgram(m_program);
}

bool ShaderProgramGL2::loaded() const
{
    return m_program != 0;
}

bool ShaderProgramGL2::load(QString vertexFile, QString fragmentFile)
{
    if(loaded())
        return false;
    return compileProgram(vertexFile, fragmentFile);
}

void ShaderProgramGL2::beginFrame()
{
    glUseProgram(m_program);
}

void ShaderProgramGL2::endFrame()
{
    glUseProgram(0);
}

bool ShaderProgramGL2::compileProgram(QString vertexFile, QString fragmentFile)
{
    uint32_t vertexShader = loadShader(vertexFile, GL_VERTEX_SHADER);
    if(vertexShader == 0)
        return false;
    uint32_t fragmentShader = loadShader(fragmentFile, GL_FRAGMENT_SHADER);
    if(fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return false;
    }
    uint32_t program = glCreateProgram();
    if(program == 0)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(!status)
    {
        GLint log_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size)
        {
            GLchar *log = new GLchar[log_size];
            glGetProgramInfoLog(program, log_size, 0, log);
            fprintf(stderr, "Error linking program: %s\n", log);
            delete [] log;
        }
        else
        {
            fprintf(stderr, "Error linking program.\n");
        }
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    m_vertexShader = vertexShader;
    m_fragmentShader = fragmentShader;
    m_program = program;
    m_modelViewMatrixLoc = glGetUniformLocation(program, "u_modelViewMatrix");
    m_projMatrixLoc = glGetUniformLocation(program, "u_projectionMatrix");
    m_positionAttr = glGetAttribLocation(program, "a_position");
    m_normalAttr = glGetAttribLocation(program, "a_normal");
    m_texCoordsAttr = glGetAttribLocation(program, "a_texCoords");
    m_boneAttr = glGetAttribLocation(program, "a_boneIndex");
    return true;
}

uint32_t ShaderProgramGL2::loadShader(QString path, uint32_t type) const
{
    char *code = loadFileData(path.toStdString());
    if(!code)
        return 0;
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar **)&code, 0);
    freeFileData(code);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size)
        {
            GLchar *log = new GLchar[log_size];
            glGetShaderInfoLog(shader, log_size, 0, log);
            fprintf(stderr, "Error compiling shader: %s\n", log);
            delete [] log;
        }
        else
        {
            fprintf(stderr, "Error compiling shader.\n");
        }
        return 0;
    }
    return shader;
}

void ShaderProgramGL2::setUniformValue(QString name, const vec4 &v)
{
    int location = glGetUniformLocation(m_program, name.toLatin1().constData());
    if(location < 0)
    {
        //fprintf(stderr, "Uniform '%s' is not active\n", name.toLatin1().constData());
        return;
    }
    glUniform4fv(location, 1, (GLfloat *)&v);
}

void ShaderProgramGL2::setUniformValue(QString name, float f)
{
    int location = glGetUniformLocation(m_program, name.toLatin1().constData());
    if(location < 0)
    {
        //fprintf(stderr, "Uniform '%s' is not active\n", name.toLatin1().constData());
        return;
    }
    glUniform1f(location, f);
}

void ShaderProgramGL2::setUniformValue(QString name, int i)
{
    int location = glGetUniformLocation(m_program, name.toLatin1().constData());
    if(location < 0)
    {
        //fprintf(stderr, "Uniform '%s' is not active\n", name.toLatin1().constData());
        return;
    }
    glUniform1i(location, i);
}

void ShaderProgramGL2::setMatrices(const matrix4 &modelView, const matrix4 &projection)
{
    glUniformMatrix4fv(m_modelViewMatrixLoc, 1, GL_FALSE, (const GLfloat *)modelView.d);
    glUniformMatrix4fv(m_projMatrixLoc, 1, GL_FALSE, (const GLfloat *)projection.d);
}

void ShaderProgramGL2::enableVertexAttributes()
{
    glEnableVertexAttribArray(m_positionAttr);
    if(m_normalAttr >= 0)
        glEnableVertexAttribArray(m_normalAttr);
    if(m_texCoordsAttr >= 0)
        glEnableVertexAttribArray(m_texCoordsAttr);
    if(m_boneAttr >= 0)
        glEnableVertexAttribArray(m_boneAttr);
}

void ShaderProgramGL2::disableVertexAttributes()
{
    glDisableVertexAttribArray(m_positionAttr);
    if(m_normalAttr >= 0)
        glDisableVertexAttribArray(m_normalAttr);
    if(m_texCoordsAttr >= 0)
        glDisableVertexAttribArray(m_texCoordsAttr);
    if(m_boneAttr >= 0)
        glDisableVertexAttribArray(m_boneAttr);
}

void ShaderProgramGL2::uploadVertexAttributes(VertexGroup *vg)
{
    glVertexAttribPointer(m_positionAttr, 3, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), &vg->data->position);
    if(m_normalAttr >= 0)
        glVertexAttribPointer(m_normalAttr, 3, GL_FLOAT, GL_FALSE,
            sizeof(VertexData), &vg->data->normal);
    if(m_texCoordsAttr >= 0)
        glVertexAttribPointer(m_texCoordsAttr, 2, GL_FLOAT, GL_FALSE,
            sizeof(VertexData), &vg->data->texCoords);
    if(m_boneAttr >= 0)
        glVertexAttribPointer(m_boneAttr, 1, GL_INT, GL_FALSE,
            sizeof(VertexData), &vg->data->bone);
}
