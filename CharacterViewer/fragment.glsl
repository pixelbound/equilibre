uniform int u_has_texture;
uniform sampler2D u_material_texture;

varying vec4 v_color;
varying vec2 v_texCoords;

void main()
{
    if(u_has_texture != 0)
        gl_FragColor = texture2D(u_material_texture, v_texCoords);
    else
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    /*gl_FragColor = v_color;
    if(u_has_texture != 0)
        gl_FragColor = gl_FragColor * texture2D(u_material_texture, v_texCoords);*/
    // discard transparent pixels
    // XXX sort objects back to front in renderer
    if(gl_FragColor.w == 0.0)
        discard;
}
