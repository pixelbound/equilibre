#include "ShaderProgramGL2.h"
#include "RenderStateGL2.h"
#include "Material.h"
#include "WLDSkeleton.h"
#include "WLDModel.h"

const int MAX_TRANSFORMS = 256;

ShaderProgramGL2::ShaderProgramGL2(RenderStateGL2 *state)
{
    m_state = state;
    m_program = 0;
    m_vertexShader = 0;
    m_fragmentShader = 0;
    m_modelViewMatrixLoc = -1;
    m_projMatrixLoc = -1;
    m_matAmbientLoc = -1;
    m_matHasTextureLoc = -1;
    m_matTextureLoc = -1;
    m_positionAttr = -1;
    m_normalAttr = -1;
    m_texCoordsAttr = -1;
    m_boneAttr = -1;
    m_bones = new vec4[MAX_TRANSFORMS * 2];
}

ShaderProgramGL2::~ShaderProgramGL2()
{
    delete [] m_bones;

    uint32_t currentProg;
    glGetIntegerv(GL_CURRENT_PROGRAM, (int32_t *)&currentProg);
    if(currentProg == m_program)
        glUseProgram(0);
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
    else if(!compileProgram(vertexFile, fragmentFile))
        return false;
    else if(!init())
        return false;
    else
        return true;
}

bool ShaderProgramGL2::init()
{
    return true;
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
    m_matAmbientLoc = glGetUniformLocation(program, "u_material_ambient");
    m_matTextureLoc = glGetUniformLocation(program, "u_material_texture");
    m_matHasTextureLoc = glGetUniformLocation(program, "u_has_texture");
    m_positionAttr = glGetAttribLocation(program, "a_position");
    m_normalAttr = glGetAttribLocation(program, "a_normal");
    m_texCoordsAttr = glGetAttribLocation(program, "a_texCoords");
    m_boneAttr = glGetAttribLocation(program, "a_boneIndex");
    return true;
}

uint32_t ShaderProgramGL2::loadShader(QString path, uint32_t type)
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

void ShaderProgramGL2::setMatrices(const matrix4 &modelView, const matrix4 &projection)
{
    glUniformMatrix4fv(m_modelViewMatrixLoc, 1, GL_FALSE, (const GLfloat *)modelView.d);
    glUniformMatrix4fv(m_projMatrixLoc, 1, GL_FALSE, (const GLfloat *)projection.d);
}

void ShaderProgramGL2::setBoneTransforms(const BoneTransform *transforms, int count)
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

void ShaderProgramGL2::beginApplyMaterial(const Material &m)
{
    glUniform4fv(m_matAmbientLoc, 1, (const GLfloat *)&m.ambient());
    if(m.texture() != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture());
        glUniform1i(m_matTextureLoc, 0);
        glUniform1i(m_matHasTextureLoc, 1);
    }
    else
    {
        glUniform1i(m_matHasTextureLoc, 0);
    }
}

void ShaderProgramGL2::endApplyMaterial(const Material &m)
{
    if(m.texture() != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
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

void ShaderProgramGL2::drawSkinned(VertexGroup *vg)
{
    if(!vg)
        return;
    enableVertexAttributes();
    VertexGroup skinnedVg(vg->mode, vg->count);
    skinnedVg.indices = vg->indices;
    skinnedVg.matGroups = vg->matGroups;
    VertexData *src = vg->data, *dst = skinnedVg.data;
    for(uint32_t i = 0; i < vg->count; i++, src++, dst++)
    {
        BoneTransform transform;
        if((int)src->bone < MAX_TRANSFORMS)
            transform = BoneTransform(m_bones[src->bone * 2], m_bones[src->bone * 2 + 1]);
        dst->position = transform.map(src->position);
        dst->normal = src->normal;
        dst->bone = src->bone;
        dst->texCoords = src->texCoords;
    }
    drawArray(&skinnedVg);
    disableVertexAttributes();
}

void ShaderProgramGL2::drawArray(VertexGroup *vg)
{
    Material *mat = 0;
    uploadVertexAttributes(vg);
    if(vg->indices.count() > 0)
    {
        const uint16_t *indices = vg->indices.constData();
        foreach(MaterialGroup mg, vg->matGroups)
        {
            if(mg.palette)
                mat = mg.palette->material(mg.matName);
            if(mat)
                m_state->pushMaterial(*mat);
            glDrawElements(vg->mode, mg.count, GL_UNSIGNED_SHORT, indices + mg.offset);
            if(mat)
                m_state->popMaterial();
        }
    }
    else
    {
        foreach(MaterialGroup mg, vg->matGroups)
        {
            if(mg.palette)
                mat = mg.palette->material(mg.matName);
            if(mat)
                m_state->pushMaterial(*mat);
            glDrawArrays(vg->mode, mg.offset, mg.count);
            if(mat)
                m_state->popMaterial();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

UniformSkinningProgram::UniformSkinningProgram(RenderStateGL2 *state) : ShaderProgramGL2(state)
{
    m_bonesLoc = -1;
}

bool UniformSkinningProgram::init()
{
    m_bonesLoc = glGetUniformLocation(m_program, "u_bones");
    return m_bonesLoc >= 0;
}

void UniformSkinningProgram::drawSkinned(VertexGroup *vg)
{
    if(!vg)
        return;
    enableVertexAttributes();
    glUniform4fv(m_bonesLoc, MAX_TRANSFORMS * 2, (const GLfloat *)m_bones);
    drawArray(vg);
    disableVertexAttributes();
}

////////////////////////////////////////////////////////////////////////////////

TextureSkinningProgram::TextureSkinningProgram(RenderStateGL2 *state) : ShaderProgramGL2(state)
{
    m_boneTexture = 0;
    m_bonesLoc = -1;
}

TextureSkinningProgram::~TextureSkinningProgram()
{
    if(m_boneTexture != 0)
        glDeleteTextures(1, &m_boneTexture);
}

bool TextureSkinningProgram::init()
{
    int32_t texUnits = 0;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &texUnits);
    m_bonesLoc = glGetUniformLocation(m_program, "u_bones");
    if(m_bonesLoc < 0)
    {
        fprintf(stderr, "error: uniform 'u_bones' is inactive.\n");
        return false;
    }
    else if(texUnits < 2)
    {
        fprintf(stderr, "error: vertex texture fetch is not supported.\n");
        return false;
    }
    else if(!GLEW_ARB_texture_rectangle)
    {
        fprintf(stderr, "error: extension 'ARB_texture_rectangle' is not supported.\n");
        return false;
    }
    glGenTextures(1, &m_boneTexture);
    setBoneTransforms(0, 0);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_boneTexture);
    glTexParameterf(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, 2, MAX_TRANSFORMS, 0,
        GL_RGBA, GL_FLOAT, m_bones);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    return true;
}

void TextureSkinningProgram::drawSkinned(VertexGroup *vg)
{
    if(!vg)
        return;
    enableVertexAttributes();
    // upload bone transforms to the transform texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_boneTexture);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 2, MAX_TRANSFORMS,
        GL_RGBA, GL_FLOAT, m_bones);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(m_bonesLoc, 1);
    // draw the mesh and let the shader do the skinning
    drawArray(vg);
    // restore state
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    glActiveTexture(GL_TEXTURE0);
    disableVertexAttributes();
}
