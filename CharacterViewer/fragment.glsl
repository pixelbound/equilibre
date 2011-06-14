uniform int u_has_texture;
uniform sampler2D u_material_texture;
uniform vec4 u_material_ambient;

varying vec2 v_texCoords;

void main()
{
    gl_FragColor = u_material_ambient;
    if(u_has_texture != 0)
        gl_FragColor = gl_FragColor * texture2D(u_material_texture, v_texCoords);
    // discard transparent pixels
    // XXX sort objects back to front in renderer
    if(gl_FragColor.w == 0.0)
        discard;
}
