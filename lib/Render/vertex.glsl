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
attribute vec4 a_color;

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform vec4 u_ambientLight;

const int MAX_LIGHTS = 8;
uniform vec3 u_lightPos[MAX_LIGHTS];
uniform vec3 u_lightColor[MAX_LIGHTS];
uniform float u_lightRadius[MAX_LIGHTS];

const int NO_LIGHTING = 0;
const int BAKED_LIGHTING = 1;
const int DEBUG_VERTEX_COLOR = 2;
const int DEBUG_TEX_FACTOR = 3;
const int DEBUG_DIFFUSE = 4;
uniform int u_lightingMode;

uniform float u_fogStart;
uniform float u_fogEnd;
uniform float u_fogDensity;

varying vec3 v_color;
varying float v_texFactor;
varying vec3 v_texCoords;
varying float v_fogFactor;

vec4 lightDiffuseValue(int index, vec4 eyeVertex, vec3 eyeNormal)
{
    vec4 lightPos = vec4(u_lightPos[index], 1.0);
    float lightRadius = u_lightRadius[index];
    vec4 eyeLightPos = u_modelViewMatrix * lightPos;
    vec3 lightDir = vec3(eyeLightPos - eyeVertex);
    float lightDist = length(lightDir);
    lightDir = normalize(lightDir);
    float lightIntensity = (lightRadius > 0.0) ? clamp(1.0 - (lightDist / lightRadius), 0.0, 1.0) : 0.0;
    vec4 lightColor = vec4(u_lightColor[index] * lightIntensity, 1.0);
    vec4 lightContrib = max(dot(eyeNormal, lightDir), 0.0) * lightColor;
    return (lightDist < lightRadius) ? lightContrib : vec4(0.0, 0.0, 0.0, 1.0);
}

void main()
{
    vec4 viewPos = u_modelViewMatrix * vec4(a_position, 1.0);
    gl_Position = u_projectionMatrix * viewPos;
    v_texCoords = a_texCoords;
    
    if((u_lightingMode == BAKED_LIGHTING) || (u_lightingMode == DEBUG_DIFFUSE))
    {
        mat3 normalMatrix;
        normalMatrix[0] = vec3(u_modelViewMatrix[0]);
        normalMatrix[1] = vec3(u_modelViewMatrix[1]);
        normalMatrix[2] = vec3(u_modelViewMatrix[2]);
        vec3 normal = normalize(normalMatrix * a_normal);
      
        vec4 diffuse = vec4(0.0, 0.0, 0.0, 1.0);
        for(int i = 0; i < MAX_LIGHTS; i++)
            diffuse += a_color * lightDiffuseValue(i, viewPos, normal);
        if(u_lightingMode == BAKED_LIGHTING)
        {
            v_color = vec3((u_ambientLight * a_color.w) + diffuse);
            v_texFactor = 0.75;
        }
        else
        {
            v_color = vec3(diffuse);
            v_texFactor = 0.0;
        }
    }
    else if(u_lightingMode == DEBUG_VERTEX_COLOR)
    {
        v_color = a_color.xyz;
        v_texFactor = 0.0;
    }
    else if(u_lightingMode == DEBUG_TEX_FACTOR)
    {
        // Show extreme texture factors as solid red/blue, or grayscale otherwise.
        if(a_color.w == 0.0)
            v_color = vec3(1.0, 0.0, 0.0);
        else if(a_color.w == 1.0)
            v_color = vec3(0.0, 0.0, 1.0);
        else
            v_color = a_color.www;
        v_texFactor = 0.0;
    }
    else
    {
        v_color = vec3(0.0, 0.0, 0.0);
        v_texFactor = 1.0;
    }
    
    // Compute the fog factor.
    const float LOG2 = 1.442695;
    float fogDist = max(length(viewPos) - u_fogStart, 0.0);
    v_fogFactor = clamp(exp2(-pow(u_fogDensity * fogDist, 2.0) * LOG2), 0.0, 1.0);
}
