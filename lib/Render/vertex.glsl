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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.attribute vec3 a_position;

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec3 a_texCoords;

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

const vec4 u_light_ambient = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 u_light_diffuse = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 u_light_specular = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 u_light_pos = vec4(0.0, 1.0, 1.0, 0.0);

uniform vec4 u_material_ambient;
uniform vec4 u_material_diffuse;

uniform float u_fogStart;
uniform float u_fogEnd;
uniform float u_fogDensity;

varying vec4 v_color;
varying vec3 v_texCoords;
varying float v_fogFactor;

void main()
{
    vec4 viewPos = u_modelViewMatrix * vec4(a_position, 1.0);
    gl_Position = u_projectionMatrix * viewPos;
    v_texCoords = a_texCoords;

    vec3 normal, lightDir, halfVector;
    vec4 diffuse, ambient, specular;
    mat3 normalMatrix;
    normalMatrix[0] = vec3(u_modelViewMatrix[0]);
    normalMatrix[1] = vec3(u_modelViewMatrix[1]);
    normalMatrix[2] = vec3(u_modelViewMatrix[2]);

    normal = normalize(normalMatrix * a_normal);
    lightDir = normalize(u_light_pos.xyz);

    ambient = u_material_ambient * u_light_ambient;
    diffuse = max(dot(normal, lightDir), 0.0) * u_material_diffuse * u_light_diffuse;

    v_color = ambient + diffuse;
    
    // Compute the fog factor.
    const float LOG2 = 1.442695;
    float fogDist = max(length(viewPos) - u_fogStart, 0.0);
    v_fogFactor = clamp(exp2(-pow(u_fogDensity * fogDist, 2.0) * LOG2), 0.0, 1.0);
}
