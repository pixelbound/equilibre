attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texCoords;
attribute float a_boneIndex; // to be compatible with OpenGL < 3.0

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

const vec4 u_light_ambient = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 u_light_diffuse = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 u_light_specular = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 u_light_pos = vec4(0.0, 1.0, 1.0, 0.0);

uniform vec4 u_material_ambient;
uniform vec4 u_material_diffuse;
//uniform vec4 u_material_specular;
//uniform float u_material_shine;

uniform sampler2DRect u_bones;

varying vec4 v_color;
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

    vec3 normal, lightDir, halfVector;
    vec4 diffuse, ambient, specular;
    mat3 normalMatrix;
    normalMatrix[0] = vec3(u_modelViewMatrix[0]);
    normalMatrix[1] = vec3(u_modelViewMatrix[1]);
    normalMatrix[2] = vec3(u_modelViewMatrix[2]);

    normal = normalize(normalMatrix * a_normal);
    lightDir = normalize(u_light_pos.xyz);
    //halfVector = normalize(lightDir + vec3(0, 0, 1));

    ambient = u_material_ambient * u_light_ambient;
    diffuse = max(dot(normal, lightDir), 0.0) * u_material_diffuse * u_light_diffuse;
    //specular = pow(max(dot(normal, halfVector), 0.0), u_material_shine)
    //    * u_material_specular * u_light_specular;

    v_color = ambient + diffuse;
}
