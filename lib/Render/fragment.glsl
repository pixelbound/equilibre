#extension GL_EXT_texture_array : enable

uniform int u_has_texture;
uniform sampler2DArray u_material_texture;

varying vec4 v_color;
varying vec3 v_texCoords;

void main()
{
    gl_FragColor = v_color;
    if(u_has_texture != 0)
        gl_FragColor = gl_FragColor * texture2DArray(u_material_texture,
                                                     vec3(v_texCoords.xy, v_texCoords.z - 1.0), 0.0);
    // discard transparent pixels
    // XXX sort objects back to front in renderer
    if(gl_FragColor.w == 0.0)
        discard;
}
