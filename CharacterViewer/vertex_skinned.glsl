attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texCoords;
attribute float a_boneIndex; // to be compatible with OpenGL < 3.0

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform int u_skinningMode;

uniform vec4 u_bone_rotation[256];
uniform vec4 u_bone_translation[256];

varying vec2 v_texCoords;

// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qquaternion.h
vec4 mult_quat(vec4 q1, vec4 q2)
{
    float ww = (q1.z + q1.x) * (q2.x + q2.y);
    float yy = (q1.w - q1.y) * (q2.w + q2.z);
    float zz = (q1.w + q1.y) * (q2.w - q2.z);
    float xx = ww + yy + zz;
    float qq = 0.5 * (xx + (q1.z - q1.x) * (q2.x - q2.y));

    float w = qq - ww + (q1.z - q1.y) * (q2.y - q2.z);
    float x = qq - xx + (q1.x + q1.w) * (q2.x + q2.w);
    float y = qq - yy + (q1.w - q1.x) * (q2.y + q2.z);
    float z = qq - zz + (q1.z + q1.y) * (q2.w - q2.x);
    return vec4(x, y, z, w);
}

// rotate the point v by the quaternion q
vec3 rotate_by_quat(vec3 v, vec4 q)
{
    return vec3(mult_quat(mult_quat(q, vec4(v, 0.0)), vec4(-q.x, -q.y, -q.z, q.w)));
}

vec4 skin(vec3 pos)
{
    int boneIndex = int(a_boneIndex);
    vec4 rotation = u_bone_rotation[boneIndex];
    vec4 translation = u_bone_translation[boneIndex];
    return vec4(rotate_by_quat(pos, rotation) + translation.xyz, 1.0);
}

vec4 skinDualQuaternion(vec3 pos)
{
    int boneIndex = int(a_boneIndex);
    vec4 d0 = u_bone_rotation[boneIndex];
    vec4 d1 = u_bone_translation[boneIndex];
    vec3 pos2 = pos + 2.0 * cross(d0.xyz, cross(d0.xyz, pos.xyz) + d0.w * pos.xyz);
    return vec4(pos2 + 2.0 * (d0.w * d1.xyz - d1.w * d0.xyz + cross(d0.xyz, d1.xyz)), 1.0);
}

void main()
{
    vec4 skinnedPos;
    if(u_skinningMode == 1)
        skinnedPos = skin(a_position);
    else if(u_skinningMode == 2)
        skinnedPos = skinDualQuaternion(a_position);
    else
        skinnedPos = vec4(a_position, 1.0);
    gl_Position = u_projectionMatrix * u_modelViewMatrix * skinnedPos;
    v_texCoords = a_texCoords;
}
