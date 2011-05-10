uniform vec4 u_light_ambient;
uniform vec4 u_light_diffuse;
uniform vec4 u_light_specular;

uniform vec4 u_material_ambient;
uniform vec4 u_material_diffuse;
uniform vec4 u_material_specular;
uniform float u_material_shine;

uniform int u_has_texture;
uniform sampler2D u_material_texture;

varying vec3 normal, lightDir, halfVector;

void main()
{
    vec4 diffuse, ambient, specular;
    ambient = u_material_ambient * u_light_ambient;
    diffuse = max(dot(normal, lightDir), 0.0) * u_material_diffuse * u_light_diffuse;
    specular = pow(max(dot(normal, halfVector), 0.0), u_material_shine)
        * u_material_specular * u_light_specular;

    gl_FragColor = ambient + diffuse + specular;
    if(u_has_texture != 0)
        gl_FragColor = gl_FragColor * texture2D(u_material_texture, gl_TexCoord[0].st);
}
