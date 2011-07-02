attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texCoords;
attribute float a_boneIndex; // to be compatible with OpenGL < 3.0

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform sampler2DRect u_bones;

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
    vec4 rotation = texture2DRect(u_bones, vec2(0, a_boneIndex));
    vec4 translation = texture2DRect(u_bones, vec2(1, a_boneIndex));
    //vec4 rotation = vec4(0.0, 0.0, 0.0, 1.0);
    //vec4 translation = vec4(0.0, 0.0, 0.0, 1.0);
    return vec4(rotate_by_quat(pos, rotation) + translation.xyz, 1.0);
}

void main()
{
    gl_Position = u_projectionMatrix * u_modelViewMatrix * skin(a_position);
    v_texCoords = a_texCoords;
}
