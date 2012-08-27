#include <GL/glew.h>
#include "EQuilibre/Render/ShaderProgramGL2.h"
#include "EQuilibre/Render/RenderStateGL2.h"
#include "EQuilibre/Render/Material.h"

ShaderProgramGL2::ShaderProgramGL2(RenderStateGL2 *state)
{
    m_state = state;
    m_program = 0;
    m_vertexShader = 0;
    m_fragmentShader = 0;
    for(int i = 0; i <= A_MAX; i++)
        m_attr[i] = -1;
    for(int i = 0; i <= U_MAX; i++)
        m_uniform[i] = -1;
    m_drawCalls = 0;
    m_textureBinds = 0;
    m_bones = new vec4[MAX_TRANSFORMS * 2];
    m_meshData.clear();
}

ShaderProgramGL2::~ShaderProgramGL2()
{
    delete [] m_bones;

    if(current())
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

bool ShaderProgramGL2::current() const
{
    uint32_t currentProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, (int32_t *)&currentProg);
    return (currentProg != 0) && (currentProg == m_program);
}

uint32_t ShaderProgramGL2::program() const
{
    return m_program;
}

int ShaderProgramGL2::drawCalls() const
{
    return m_drawCalls;
}

int ShaderProgramGL2::textureBinds() const
{
    return m_textureBinds;
}

void ShaderProgramGL2::resetFrameStats()
{
    m_drawCalls = 0;
    m_textureBinds = 0;
}

bool ShaderProgramGL2::init()
{
    return true;
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
    m_uniform[U_MODELVIEW_MATRIX] = glGetUniformLocation(program, "u_modelViewMatrix");
    m_uniform[U_PROJECTION_MATRIX] = glGetUniformLocation(program, "u_projectionMatrix");
    m_uniform[U_MAT_AMBIENT] = glGetUniformLocation(program, "u_material_ambient");
    m_uniform[U_MAT_DIFFUSE] = glGetUniformLocation(program, "u_material_diffuse");
    m_uniform[U_MAT_TEXTURE] = glGetUniformLocation(program, "u_material_texture");
    m_uniform[U_MAT_HAS_TEXTURE] = glGetUniformLocation(program, "u_has_texture");
    m_attr[A_POSITION] = glGetAttribLocation(program, "a_position");
    m_attr[A_NORMAL] = glGetAttribLocation(program, "a_normal");
    m_attr[A_TEX_COORDS] = glGetAttribLocation(program, "a_texCoords");
    m_attr[A_BONE_INDEX] = glGetAttribLocation(program, "a_boneIndex");
    m_attr[A_MODEL_VIEW_0] = glGetAttribLocation(program, "a_modelViewMatrix");
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

void ShaderProgramGL2::setModelViewMatrix(const matrix4 &modelView)
{
    //const vec4 *columns = t.columns();
    //for(int i = 0; i < 4; i++)
    //    glVertexAttrib4fv(m_attr[A_MODEL_VIEW_0] + i, (const GLfloat *)&columns[i]);
    glUniformMatrix4fv(m_uniform[U_MODELVIEW_MATRIX],
            1, GL_FALSE, (const GLfloat *)modelView.columns());
}

void ShaderProgramGL2::setProjectionMatrix(const matrix4 &projection)
{
    glUniformMatrix4fv(m_uniform[U_PROJECTION_MATRIX],
        1, GL_FALSE, (const GLfloat *)projection.columns());
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

void ShaderProgramGL2::beginApplyMaterial(MaterialMap *map, Material *m)
{
    //XXX GL_MAX_TEXTURE_IMAGE_UNITS
    //GLuint target = (map->arrayTexture() != 0) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    GLuint target = GL_TEXTURE_2D_ARRAY;
    glUniform4fv(m_uniform[U_MAT_AMBIENT], 1, (const GLfloat *)&m->ambient());
    glUniform4fv(m_uniform[U_MAT_DIFFUSE], 1, (const GLfloat *)&m->diffuse());
    if(m->texture() != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, m->texture());
        glUniform1i(m_uniform[U_MAT_TEXTURE], 0);
        glUniform1i(m_uniform[U_MAT_HAS_TEXTURE], 1);
        m_textureBinds++;
    }
    else
    {
        glUniform1i(m_uniform[U_MAT_HAS_TEXTURE], 0);
    }
}

void ShaderProgramGL2::endApplyMaterial(MaterialMap *map, Material *m)
{
    //GLuint target = (map->arrayTexture() != 0) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    GLuint target = GL_TEXTURE_2D_ARRAY;
    if(m->texture() != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, 0);
    }
}

void ShaderProgramGL2::enableVertexAttribute(int attr, int index)
{
    if(m_attr[attr] >= 0)
        glEnableVertexAttribArray(m_attr[attr] + index);
}

void ShaderProgramGL2::disableVertexAttribute(int attr, int index)
{
    if(m_attr[attr] >= 0)
        glDisableVertexAttribArray(m_attr[attr] + index);
}

void ShaderProgramGL2::uploadVertexAttributes(const MeshBuffer *meshBuf)
{
    const Vertex *vd = meshBuf->vertices.constData();
    const uint8_t *posPointer = (const uint8_t *)&vd->position;
    const uint8_t *normalPointer = (const uint8_t *)&vd->normal;
    const uint8_t *texCoordsPointer = (const uint8_t *)&vd->texCoords;
    const uint8_t *bonePointer = (const uint8_t *)&vd->bone;
    if(meshBuf->vertexBuffer != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, meshBuf->vertexBuffer);
        posPointer = 0;
        normalPointer = posPointer + sizeof(vec3);
        texCoordsPointer = normalPointer + sizeof(vec3);
        bonePointer = texCoordsPointer + sizeof(vec3);
    }
    glVertexAttribPointer(m_attr[A_POSITION], 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), posPointer);
    if(m_attr[A_NORMAL] >= 0)
        glVertexAttribPointer(m_attr[A_NORMAL], 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), normalPointer);
    if(m_attr[A_TEX_COORDS] >= 0)
        glVertexAttribPointer(m_attr[A_TEX_COORDS], 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), texCoordsPointer);
    if(m_attr[A_BONE_INDEX] >= 0)
        glVertexAttribPointer(m_attr[A_BONE_INDEX], 1, GL_INT, GL_FALSE,
            sizeof(Vertex), bonePointer);
    if(meshBuf->vertexBuffer != 0)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShaderProgramGL2::beginDrawMesh(const MeshBuffer *meshBuf, MaterialMap *materials,
                                     const BoneTransform *bones, int boneCount)
{
    if(m_meshData.pending || !meshBuf)
        return;
    // XXX make the caller pass this structure to be reentrant
    m_meshData.pending = true;
    m_meshData.meshBuf = meshBuf;
    m_meshData.materials = materials;
    m_meshData.bones = bones;
    m_meshData.boneCount = boneCount;
    enableVertexAttribute(A_POSITION);
    enableVertexAttribute(A_NORMAL);
    enableVertexAttribute(A_TEX_COORDS);
    if(bones && (boneCount > 0))
    {
        enableVertexAttribute(A_BONE_INDEX);
        setBoneTransforms(bones, boneCount);
    }
    if(meshBuf->indexBuffer != 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuf->indexBuffer);
        m_meshData.haveIndices = true;
    }
    else if(meshBuf->indices.count() > 0)
    {
        m_meshData.indices = meshBuf->indices.constData();
        m_meshData.haveIndices = true;
    }
    if(bones && (boneCount > 0))
        beginSkinMesh();
    uploadVertexAttributes(meshBuf);
}

void ShaderProgramGL2::drawMeshBatch(const matrix4 *mvMatrices, uint32_t instances)
{
    // No material - nothing is drawn.
    if(!m_meshData.pending || !m_meshData.materials)
        return;

    // Get a Material pointer for each material group.
    const MeshBuffer *meshBuf = m_meshData.meshBuf;
    QVector<MaterialGroup> groups;
    QVector<Material *> groupMats;
    for(int i = 0; i < meshBuf->matGroups.count(); i++)
    {
        uint32_t matID = meshBuf->matGroups[i].matID;
        Material *mat = m_meshData.materials->material(matID);
        // skip meshes that don't have a material
        // XXX fix rendering non-opaque polygons
        if(!mat || !mat->isOpaque())
            continue;
        groups.append(meshBuf->matGroups[i]);
        groupMats.append(mat);
    }

    // Check whether all materials use the same texture (array) or not.
    Material *arrayMat = NULL;
    if(groups.count() > 0)
    {
        arrayMat = groupMats[0];
        for(int i = 1; i < groupMats.count(); i++)
        {
            if(arrayMat->texture() != groupMats[i]->texture())
            {
                arrayMat = NULL;
                break;
            }
        }
    }

    if(arrayMat != NULL)
    {
        // If all material groups use the same texture we can render them together.
        // XXX have an uniform array of material state
        beginApplyMaterial(m_meshData.materials, arrayMat);
        for(uint32_t i = 0; i < instances; i++)
        {
            setModelViewMatrix(mvMatrices[i]);

            // Assume groups are sorted by offset and merge as many as possible.
            MaterialGroup merged;
            merged.offset = groups[0].offset;
            merged.count = groups[0].count;
            for(int j = 1; j < groups.count(); j++)
            {
                uint32_t mergedEnd = merged.offset + merged.count;
                if(groups[j].offset == mergedEnd)
                {
                    merged.count += groups[j].count;
                }
                else
                {
                    drawMaterialGroup(merged);
                    merged.offset = groups[j].offset;
                    merged.count = groups[j].count;
                }
            }
            drawMaterialGroup(merged);
        }
        endApplyMaterial(m_meshData.materials, arrayMat);
    }
    else
    {
        // Otherwise we have to change the texture for every material group.
        for(int i = 0; i < groups.count(); i++)
        {
            Material *mat = groupMats[i];
            beginApplyMaterial(m_meshData.materials, mat);
            for(uint32_t j = 0; j < instances; j++)
            {
                setModelViewMatrix(mvMatrices[j]);
                drawMaterialGroup(groups[i]);
            }
            endApplyMaterial(m_meshData.materials, mat);
        }
    }
}

void ShaderProgramGL2::drawMaterialGroup(const MaterialGroup &mg)
{
    const GLuint mode = GL_TRIANGLES;
    if(m_meshData.haveIndices)
        glDrawElements(mode, mg.count, GL_UNSIGNED_INT, m_meshData.indices + mg.offset);
    else
        glDrawArrays(mode, mg.offset, mg.count);
    m_drawCalls++;
}

void ShaderProgramGL2::endDrawMesh()
{
    if(!m_meshData.pending)
        return;
    if(m_meshData.bones && (m_meshData.boneCount > 0))
        endSkinMesh();
    //XXX No unbind / do unbind at the end of the frame.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    for(int i = 0; i < 4; i++)
        disableVertexAttribute(A_MODEL_VIEW_0, i);
    disableVertexAttribute(A_BONE_INDEX);
    disableVertexAttribute(A_POSITION);
    disableVertexAttribute(A_NORMAL);
    disableVertexAttribute(A_TEX_COORDS);
    m_meshData.clear();
}

void ShaderProgramGL2::beginSkinMesh()
{
    // We can only do mesh skinning in software with a VBO as we can't overwrite the Mesh data.
    const MeshBuffer *meshBuf = m_meshData.meshBuf;
    if(meshBuf->vertexBuffer == 0)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, meshBuf->vertexBuffer);
    // Discard the previous data.
    glBufferData(GL_ARRAY_BUFFER, meshBuf->vertexBufferSize, NULL, GL_STREAM_DRAW);
    // Map the VBO in memory and copy skinned vertices directly to it.
    void *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if(!buffer)
        return;
    const Vertex *src = meshBuf->vertices.constData();
    Vertex *dst = (Vertex *)buffer;
    for(uint32_t i = 0; i < meshBuf->vertices.count(); i++, src++, dst++)
    {
        BoneTransform transform;
        if((int)src->bone < MAX_TRANSFORMS)
            transform = BoneTransform(m_bones[src->bone * 2], m_bones[src->bone * 2 + 1]);
        dst->position = transform.map(src->position);
        dst->normal = src->normal;
        dst->bone = src->bone;
        dst->texCoords = src->texCoords;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShaderProgramGL2::endSkinMesh()
{
    // Restore the old mesh data that was overwritten by beginSkinMesh.
    const MeshBuffer *meshBuf = m_meshData.meshBuf;
    if(meshBuf->vertexBuffer == 0)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, meshBuf->vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, meshBuf->vertexBufferSize, meshBuf->vertices.constData());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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

void UniformSkinningProgram::beginSkinMesh()
{
    glUniform4fv(m_bonesLoc, MAX_TRANSFORMS * 2, (const GLfloat *)m_bones);
}

void UniformSkinningProgram::endSkinMesh()
{
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
    else if(!GLEW_ARB_texture_float)
    {
        fprintf(stderr, "error: extension 'ARB_texture_float' is not supported.\n");
        return false;
    }
    glGenTextures(1, &m_boneTexture);
    setBoneTransforms(0, 0);
    glBindTexture(GL_TEXTURE_2D, m_boneTexture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2, MAX_TRANSFORMS, 0,
        GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void TextureSkinningProgram::beginSkinMesh()
{
    // upload bone transforms to the transform texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_boneTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, MAX_TRANSFORMS,
        GL_RGBA, GL_FLOAT, m_bones);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(m_bonesLoc, 1);
    m_textureBinds++;
}

void TextureSkinningProgram::endSkinMesh()
{
    // restore state
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    glActiveTexture(GL_TEXTURE0);
}

////////////////////////////////////////////////////////////////////////////////

void MeshDataGL2::clear()
{
    meshBuf = NULL;
    bones = NULL;
    boneCount = 0;
    materials = NULL;
    haveIndices = false;
    indices = NULL;
    pending = false;
}
