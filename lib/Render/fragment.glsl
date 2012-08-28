// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#extension GL_EXT_texture_array : enable

uniform int u_has_texture;
uniform sampler2DArray u_material_texture;
uniform vec4 u_fogColor;

varying vec4 v_color;
varying vec3 v_texCoords;
varying float v_fogFactor;

void main()
{
    gl_FragColor = v_color;
    if(u_has_texture != 0)
        gl_FragColor = gl_FragColor * texture2DArray(u_material_texture,
                                                     vec3(v_texCoords.xy, v_texCoords.z - 1.0), 0.0);
    // discard transparent pixels
    if(gl_FragColor.w == 0.0)
        discard;
    
    gl_FragColor = mix(u_fogColor, gl_FragColor, v_fogFactor);
}
