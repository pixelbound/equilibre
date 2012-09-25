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

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec3 a_texCoords;
attribute vec4 a_color;
attribute float a_boneIndex; // to be compatible with OpenGL < 3.0

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform int u_mapMaterials;
uniform vec3 u_materialMap[64];

uniform float u_fogStart;
uniform float u_fogEnd;
uniform float u_fogDensity;

uniform vec4 u_bones[512];

varying vec3 v_color;
varying float v_texFactor;
varying vec3 v_texCoords;
varying float v_fogFactor;

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
    int i = int(a_boneIndex) * 2;
    vec4 trans = u_bones[i];
    vec4 rot = u_bones[i + 1];
    return vec4(rotate_by_quat(pos, rot) + trans.xyz, 1.0);
}

void main()
{
    vec4 viewPos = u_modelViewMatrix * skin(a_position);
    gl_Position = u_projectionMatrix * viewPos;
    
    // Transform texture coordinates if using the material map.
    float baseTex = a_texCoords.z - 1.0;
    vec2 baseTexCoords = a_texCoords.xy;
    vec3 matInfo = u_materialMap[int(baseTex)];
    vec2 mappedTexCoords = baseTexCoords * matInfo.xy;
    float mappedTex = matInfo.z - 1.0;
    float finalTex = (u_mapMaterials > 0) ? mappedTex : baseTex;
    vec2 finalTexCoords = (u_mapMaterials > 0) ? mappedTexCoords : baseTexCoords;
    v_texCoords = vec3(finalTexCoords, finalTex);
    
    v_color = vec3(0.0, 0.0, 0.0);
    v_texFactor = 1.0;
    
    // Compute the fog factor.
    const float LOG2 = 1.442695;
    float fogDist = max(length(viewPos) - u_fogStart, 0.0);
    v_fogFactor = clamp(exp2(-pow(u_fogDensity * fogDist, 2.0) * LOG2), 0.0, 1.0);
}
