#include <GL/glew.h>
#include "ShaderProgramGL2.h"
#include "RenderStateGL2.h"
#include "Material.h"
#include "WLDSkeleton.h"

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
    m_bones = new vec4[MAX_TRANSFORMS * 2];
    m_instanceMvBuffer = 0;
    m_meshData.clear();
}

ShaderProgramGL2::~ShaderProgramGL2()
{
    delete [] m_bones;

    if(m_instanceMvBuffer != 0)
        glDeleteBuffers(1, &m_instanceMvBuffer);

    uint32_t currentProg = 0;
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
    // Create a buffer that can contain model-view matrices for instanced objects.
    if(GLEW_ARB_draw_instanced && GLEW_ARB_instanced_arrays)
    {
        size_t bufferSize = RenderState::MAX_OBJECT_INSTANCES * sizeof(matrix4);
        glGenBuffers(1, &m_instanceMvBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceMvBuffer);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
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
    glUniformMatrix4fv(m_uniform[U_MODELVIEW_MATRIX],
        1, GL_FALSE, (const GLfloat *)modelView.d);
}

void ShaderProgramGL2::setProjectionMatrix(const matrix4 &projection)
{
    glUniformMatrix4fv(m_uniform[U_PROJECTION_MATRIX],
        1, GL_FALSE, (const GLfloat *)projection.d);
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
    //XXX GL_MAX_TEXTURE_IMAGE_UNITS
    glUniform4fv(m_uniform[U_MAT_AMBIENT], 1, (const GLfloat *)&m.ambient());
    glUniform4fv(m_uniform[U_MAT_DIFFUSE], 1, (const GLfloat *)&m.diffuse());
    if(m.texture() != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture());
        glUniform1i(m_uniform[U_MAT_TEXTURE], 0);
        glUniform1i(m_uniform[U_MAT_HAS_TEXTURE], 1);
    }
    else
    {
        glUniform1i(m_uniform[U_MAT_HAS_TEXTURE], 0);
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

void ShaderProgramGL2::uploadVertexAttributes(const VertexGroup *vg)
{
    const VertexData *vd = vg->vertices.constData();
    const uint8_t *posPointer = (const uint8_t *)&vd->position;
    const uint8_t *normalPointer = (const uint8_t *)&vd->normal;
    const uint8_t *texCoordsPointer = (const uint8_t *)&vd->texCoords;
    const uint8_t *bonePointer = (const uint8_t *)&vd->bone;
    if(vg->vertexBuffer.buffer != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vg->vertexBuffer.buffer);
        posPointer = 0;
        normalPointer = posPointer + sizeof(vec3);
        texCoordsPointer = normalPointer + sizeof(vec3);
        bonePointer = texCoordsPointer + sizeof(vec2);
    }
    glVertexAttribPointer(m_attr[A_POSITION], 3, GL_FLOAT, GL_FALSE,
        sizeof(VertexData), posPointer);
    if(m_attr[A_NORMAL] >= 0)
        glVertexAttribPointer(m_attr[A_NORMAL], 3, GL_FLOAT, GL_FALSE,
            sizeof(VertexData), normalPointer);
    if(m_attr[A_TEX_COORDS] >= 0)
        glVertexAttribPointer(m_attr[A_TEX_COORDS], 2, GL_FLOAT, GL_FALSE,
            sizeof(VertexData), texCoordsPointer);
    if(m_attr[A_BONE_INDEX] >= 0)
        glVertexAttribPointer(m_attr[A_BONE_INDEX], 1, GL_INT, GL_FALSE,
            sizeof(VertexData), bonePointer);
    if(vg->vertexBuffer.buffer != 0)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint primitiveToGLMode(VertexGroup::Primitive mode)
{
    switch(mode)
    {
    case VertexGroup::Triangle:
        return GL_TRIANGLES;
    case VertexGroup::Quad:
        return GL_QUADS;
    }
    return 0;
}

void ShaderProgramGL2::beginDrawMesh(const VertexGroup *vg, MaterialMap *materials,
                                     const BoneTransform *bones, int boneCount)
{
    if(m_meshData.pending || !vg)
        return;
    m_meshData.pending = true;
    m_meshData.vg = vg;
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
    if(vg->indexBuffer.buffer != 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vg->indexBuffer.buffer);
        m_meshData.haveIndices = true;
    }
    else if(vg->indices.count() > 0)
    {
        m_meshData.indices = vg->indices.constData();
        m_meshData.haveIndices = true;
    }
    uploadVertexAttributes(vg);
}

void ShaderProgramGL2::drawMesh()
{
    if(!m_meshData.pending)
        return;
    drawMaterialGroups(m_meshData.vg, 1);
}

void ShaderProgramGL2::drawMeshBatch(const matrix4 *mvMatrices, uint32_t instances)
{
    if(!m_meshData.pending)
        return;
    int mvAttr = m_attr[A_MODEL_VIEW_0];
    if((m_instanceMvBuffer == 0) || (mvAttr < 0) ||
        (instances < 2) || (instances > RenderState::MAX_OBJECT_INSTANCES))
    {
        // Naive instancing if the extensions are not supported.
        for(uint32_t i = 0; i < instances; i++)
        {
            setModelViewMatrix(mvMatrices[i]);
            drawMaterialGroups(m_meshData.vg, 1);
        }
    }
    else
    {
        size_t bufferSize = instances * sizeof(matrix4);
        size_t bufferStride = sizeof(vec4) * 4;
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceMvBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, mvMatrices);
        for(int i = 0; i < 4; i++)
        {
            void *ptr = (void*)(sizeof(vec4) * i);
            enableVertexAttribute(A_MODEL_VIEW_0, i);
            glVertexAttribPointer(mvAttr + i, 4, GL_FLOAT, GL_FALSE, bufferStride, ptr);
            glVertexAttribDivisor(mvAttr + i, 1);
        }
        drawMaterialGroups(m_meshData.vg, instances);
        for(int i = 0; i < 4; i++)
            glVertexAttribDivisor(mvAttr + i, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShaderProgramGL2::drawMaterialGroups(const VertexGroup *vg, int instances)
{
    foreach(MaterialGroup mg, vg->matGroups)
        drawMaterialGroup(vg, mg, instances);
}

void ShaderProgramGL2::drawMaterialGroup(const VertexGroup *vg, MaterialGroup &mg, int instances)
{
    // skip meshes that don't have a material
    if(mg.matName.isEmpty())
       return;
    Material *mat = m_meshData.materials ? m_meshData.materials->material(mg.matName) : NULL;
    if(mat)
    {
        // XXX fix rendering non-opaque polygons
        if(!mat->isOpaque())
            return;
        m_state->pushMaterial(*mat);
    }
    GLuint mode = primitiveToGLMode(vg->mode);
    if(m_meshData.haveIndices)
    {
        const uint32_t *indices = m_meshData.indices + vg->indexBuffer.offset + mg.offset;
        if(instances > 1)
            glDrawElementsInstanced(mode, mg.count, GL_UNSIGNED_INT, indices, instances);
        else
            glDrawElements(mode, mg.count, GL_UNSIGNED_INT, indices);
    }
    else
    {
        uint32_t offset = vg->vertexBuffer.offset + mg.offset;
        if(instances > 1)
            glDrawArraysInstanced(mode, offset, mg.count, instances);
        else
            glDrawArrays(mode, offset, mg.count);
    }
    if(mat)
        m_state->popMaterial();
}

void ShaderProgramGL2::endDrawMesh()
{
    if(!m_meshData.pending)
        return;
    const VertexGroup *vg = m_meshData.vg;
    if(vg->indexBuffer.buffer != 0)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    for(int i = 0; i < 4; i++)
        disableVertexAttribute(A_MODEL_VIEW_0, i);
    disableVertexAttribute(A_BONE_INDEX);
    disableVertexAttribute(A_POSITION);
    disableVertexAttribute(A_NORMAL);
    disableVertexAttribute(A_TEX_COORDS);
    m_meshData.clear();
}

/*void ShaderProgramGL2::drawSkinned(const VertexGroup *vg, WLDMaterialPalette *palette)
{
    if(!vg)
        return;
    VertexGroup skinnedVg(vg->mode);
    skinnedVg.vertices.resize(vg->vertices.count());
    skinnedVg.indices = vg->indices;
    skinnedVg.matGroups = vg->matGroups;
    const VertexData *src = vg->vertices.constData();
    VertexData *dst = skinnedVg.vertices.data();
    for(uint32_t i = 0; i < vg->vertices.count(); i++, src++, dst++)
    {
        BoneTransform transform;
        if((int)src->bone < MAX_TRANSFORMS)
            transform = BoneTransform(m_bones[src->bone * 2], m_bones[src->bone * 2 + 1]);
        dst->position = transform.map(src->position);
        dst->normal = src->normal;
        dst->bone = src->bone;
        dst->texCoords = src->texCoords;
    }
    beginDraw(&skinnedVg);
    draw(&skinnedVg, palette);
    endDraw(&skinnedVg);
}*/

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

/*void UniformSkinningProgram::drawSkinned(const VertexGroup *vg, WLDMaterialPalette *palette)
{
    glUniform4fv(m_bonesLoc, MAX_TRANSFORMS * 2, (const GLfloat *)m_bones);
    draw(vg, palette);
}*/

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

/*void TextureSkinningProgram::drawSkinned(const VertexGroup *vg, WLDMaterialPalette *palette)
{
    // upload bone transforms to the transform texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_boneTexture);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 2, MAX_TRANSFORMS,
        GL_RGBA, GL_FLOAT, m_bones);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(m_bonesLoc, 1);
    // draw the mesh and let the shader do the skinning
    draw(vg, palette);
    // restore state
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    glActiveTexture(GL_TEXTURE0);
}*/

////////////////////////////////////////////////////////////////////////////////

void MeshDataGL2::clear()
{
    vg = NULL;
    bones = NULL;
    boneCount = 0;
    materials = NULL;
    haveIndices = false;
    indices = NULL;
    pending = false;
}
