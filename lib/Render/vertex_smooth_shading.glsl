uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform vec4 u_light_pos;

varying vec3 normal, lightDir, halfVector;

void main()
{
    mat3 normalMatrix;
    normalMatrix[0] = vec3(u_modelViewMatrix[0]);
    normalMatrix[1] = vec3(u_modelViewMatrix[1]);
    normalMatrix[2] = vec3(u_modelViewMatrix[2]);

    normal = normalize(normalMatrix * gl_Normal);
    lightDir = normalize(u_light_pos.xyz);
    halfVector = normalize(lightDir + vec3(0, 0, 1));

    gl_Position = u_projectionMatrix * u_modelViewMatrix * gl_Vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
}
