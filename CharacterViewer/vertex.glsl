attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texCoords;

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

varying vec4 v_color;
varying vec2 v_texCoords;

void main()
{
    gl_Position = u_projectionMatrix * u_modelViewMatrix * vec4(a_position, 1.0);
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
