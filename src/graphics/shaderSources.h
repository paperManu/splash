/*
 * Copyright (C) 2014 Splash authors
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @shaderSources.h
 * Contains the sources for all shaders used in Splash
 */

#ifndef SPLASH_SHADERSOURCES_H
#define SPLASH_SHADERSOURCES_H

#include <map>
#include <string>

#include "./core/constants.h"

namespace Splash::ShaderSources
{
    const std::map<std::string, std::string> INCLUDES{//
        // Color encoding IDs, must match the IDs defined in
        // graphics/texture_image.h
        {"colorEncoding", R"(
            #define COLOR_RGB 0
            #define COLOR_BGR 1
            #define COLOR_UYVY 2
            #define COLOR_YUYV 3
            #define COLOR_YCoCg 4
        )"},
        // Project a point wrt a mvp matrix, and check if it is in the view frustum.
        // Returns the distance on X and Y in the distToCenter parameter
        {"projectAndCheckVisibility", R"(
            bool projectAndCheckVisibility(inout vec4 p, in mat4 mvp, in float margin, out vec2 distToCenter)
            {
                vec4 projected = mvp * vec4(p.xyz, 1.0);
                projected /= projected.w;
                p = projected;

                if (projected.z >= -1.0)
                {
                    projected = abs(projected);
                    distToCenter = projected.xy;
                    bvec4 isVisible = lessThanEqual(projected, vec4(1.0 + margin));
                    if (all(isVisible.xyz))
                        return true;
                }

                return false;
            }
        )"},
        //
        // Compute a normal vector from three points
        {"normalVector", R"(
            uniform int _sideness;

            vec3 normalVector(vec3 u, vec3 v, vec3 w)
            {
                vec3 n = normalize(cross(v - u, w - u));

                if (_sideness == 0)
                    n.z = 0.0;
                else if (_sideness == 2)
                    n.z = -n.z;

                return n;
            }
        )"},
        //
        // Compute a smooth blending from a projected point
        {"getSmoothBlendFromVertex", R"(
            float getSmoothBlendFromVertex(vec4 v, float blendDist)
            {
                vec2 screenPos = v.xy * 0.5 + vec2(0.5);
                vec2 dist = vec2(min(screenPos.x, 1.0 - screenPos.x), min(screenPos.y, 1.0 - screenPos.y));

                // See Lancelle et al. 2011 for explanations about the various weighting functions
                // d1
                //float weight = min(dist.x / blendDist, dist.y / blendDist);

                // d2
                dist = clamp(dist / blendDist, vec2(0.0), vec2(1.0));
                float weight = 2.0 / (1.0 / dist.x + 1.0 / dist.y);
                weight = max(0.0, min(1.0, weight));
                weight = weight * weight;

                // d4 (and d3 if pow(x, 1.0))
                //float weight = pow(abs(dist.x / blendDist * dist.y / blendDist), 1.5);
                
                return weight;
            }
        )"},
        //
        // sRGB to RGB and RGB to sRGB
        {"srgb", R"(
            vec3 srgb2rgb(vec3 srgb)
            {
                vec3 linearPart = srgb / 12.92;
                vec3 powPart = pow((srgb + vec3(0.055)) / 1.055, vec3(2.4));
                vec3 part = vec3(lessThan(srgb, vec3(0.04045)));
                return mix(powPart, linearPart, part);
            }

            vec3 rgb2srgb(vec3 rgb)
            {
                vec3 linearPart = rgb * 12.92;
                vec3 powPart = pow(rgb, vec3(1.0 / 2.4)) * 1.055 - 0.055;
                vec3 part = vec3(lessThan(rgb, vec3(0.0031308)));
                return mix(powPart, linearPart, part);
            }
        )"},
        //
        // RGB to HSV and HSV to RGB
        {"hsv", R"(
            vec3 rgb2hsv(vec3 c)
            {
                vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
                vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
                vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

                float d = q.x - min(q.w, q.y);
                float e = 1.0e-10;
                return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
            }

            vec3 hsv2rgb(vec3 c)
            {
                vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
                return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
            }
        )"},
        //
        // YUV to RGB and RGB to YUV
        // Formulas are taken from https://www.fourcc.org/fccyvrgb.php
        // The resulting colors do not match exactly VLC or Gstreamer,
        // Mostly, color and saturation are a bit higher.
        {"yuv", R"(
            vec3 yuv2rgb(vec3 c)
            {
                vec3 yuv = c - vec3(16.0 / 255.0, 0.5, 0.5);
                vec3 srgb = vec3(1.164 * yuv.r + 2.018 * yuv.b,
                                1.164 * yuv.r - 0.813 * yuv.g - 0.391 * yuv.b,
                                1.164 * yuv.r + 1.596 * yuv.g);
                // Computed color is in the sRGB space.
                // We must convert it back to RGB
                srgb = clamp(srgb, vec3(0.0), vec3(1.0));
                vec3 rgb = srgb2rgb(srgb);
                return rgb;
            }

            vec3 rgb2yuv(vec3 c)
            {
                // Input colors are stored with a gamma applied to them
                vec3 rgb = pow(c, vec3(1.0/2.2));
                vec3 yuv = vec3(0.257*rgb.r + 0.5004*rgb.g + 0.098*rgb.b + 16.0/255.0,
                                0.439*rgb.r - 0.368*rgb.g + 0.071*rgb.b + 0.5,
                                -0.1148*rgb.r - 0.291*rgb.g + 0.439*rgb.b + 0.5);
                yuv = clamp(yuv, vec3(0.0), vec3(1.0));
                yuv = pow(yuv, vec3(2.2));
                return yuv;
            }
        )"},
        //
        // Color correction: brightness, saturation and contrast
        // hsv also needs to be included
        {"correctColor", R"( 
            vec4 correctColor(in vec4 color, in float brightness, in float saturation, in float contrast)
            {
                if (brightness != 1.f || saturation != 1.f || contrast != 1.f)
                {
                    vec3 hsv = rgb2hsv(color.rgb);
                    // Brightness correction
                    hsv.z *= brightness;
                    // Saturation
                    hsv.y = min(1.0, hsv.y * saturation);
                    // Contrast correction
                    hsv.z = (hsv.z - 0.5f) * contrast + 0.5f;
                    hsv.z = min(1.0, hsv.z);
                    color.rgb = hsv2rgb(hsv);
                }
                return color;
            }
        )"}};

    /**
     * Version directives, included at the start of all shaders
     */
    const std::string VERSION_DIRECTIVE_GL4{R"(
        #version 450 core
    )"};

    const std::string VERSION_DIRECTIVE_GL32_ES{R"(
        #version 320 es
    )"};

    /**************************/
    // COMPUTE
    /**************************/

    /**
     * Default compute shader
     */
    const std::string COMPUTE_SHADER_DEFAULT{R"(
        #extension GL_ARB_compute_shader : enable
        #extension GL_ARB_shader_storage_buffer_object : enable
        precision mediump float;

        layout(local_size_x = 32, local_size_y = 32) in;

        layout (std430, binding = 0) buffer vertexBuffer
        {
            vec4 vertex[];
        };

        layout (std430, binding = 1) buffer texCoordsBuffer
        {
            vec2 texCoords[];
        };

        layout (std430, binding = 2) buffer normalBuffer
        {
            vec4 normal[];
        };

        layout (std430, binding = 3) buffer annexeBuffer
        {
            vec4 annexe[];
        };

        uniform int _vertexNbr;

        void main(void)
        {
            uvec3 pos = gl_GlobalInvocationID;
            int globalID = int(gl_WorkGroupID.x * 32 * 32 + gl_LocalInvocationIndex);

            if (globalID < _vertexNbr)
            {
                vertex[globalID].x += 0.001;
                vertex[globalID].y += 0.001;
                vertex[globalID].z += 0.001;
            }
        }
    )"};

    /**
     * Compute shader to reset all camera visibility attributes
     */
    const std::string COMPUTE_SHADER_RESET_VISIBILITY{R"(
        #extension GL_ARB_compute_shader : enable
        #extension GL_ARB_shader_storage_buffer_object : enable
        precision mediump float;

        layout(local_size_x = 128) in;

        layout (std430, binding = 3) buffer annexeBuffer
        {
            vec4 annexe[]; // Holds camera count, blending sum, flag set to true if primitive is visible, primitive ID
        };

        uniform int _vertexNbr;
        uniform int _primitiveIdShift;

        void main(void)
        {
            int globalID = int(gl_GlobalInvocationID.x);

            if (globalID < _vertexNbr / 3)
            {
                for (int idx = 0; idx < 3; ++idx)
                {
                    int vertexId = globalID * 3 + idx;
                    // the W coordinates holds the primitive ID, for use in the first visibility test
                    annexe[vertexId].zw = vec2(0.0, float(globalID + _primitiveIdShift));
                }
            }
        }
    )"};

    /**
     * Compute shader to reset all camera contribution to zero
     */
    const std::string COMPUTE_SHADER_RESET_BLENDING{R"(
        #extension GL_ARB_compute_shader : enable
        #extension GL_ARB_shader_storage_buffer_object : enable
        precision mediump float;

        layout(local_size_x = 128) in;

        layout (std430, binding = 3) buffer annexeBuffer
        {
            vec4 annexe[];
        };

        uniform int _vertexNbr;

        void main(void)
        {
            int globalID = int(gl_GlobalInvocationID.x);

            if (globalID < _vertexNbr / 3)
            {
                for (int idx = 0; idx < 3; ++idx)
                {
                    int vertexId = globalID * 3 + idx;
                    // The x coordinate holds the number of camera, y the blending value
                    annexe[vertexId].xy = vec2(0.0, 0.0);
                }
            }
        }
    )"};

    /**
     * Compute shader to transfer the visibility from the GL_TEXTURE0 to the vertices attributes
     */
    const std::string COMPUTE_SHADER_TRANSFER_VISIBILITY_TO_ATTR{R"(
        #extension GL_ARB_compute_shader : enable
        #extension GL_ARB_shader_storage_buffer_object : enable
        precision mediump float;

        layout(local_size_x = 32, local_size_y = 32) in;

        layout(binding = 0) uniform sampler2D imgVisibility;
        layout(std430, binding = 3) buffer annexeBuffer
        {
            vec4 annexe[];
        };

        uniform vec2 _texSize;
        uniform int _idShift;

        void main(void)
        {
            vec2 pixCoords = vec2(gl_GlobalInvocationID.xy);
            vec2 texCoords = pixCoords / _texSize;

            if (all(lessThan(pixCoords.xy, _texSize.xy)))
            {
                ivec4 visibility = ivec4(round(texelFetch(imgVisibility, ivec2(pixCoords), 0) * 255.0));
                int primitiveID = visibility.r * 65025 + visibility.g * 255 + visibility.b - _idShift;
                // Mark the primitive found as visible
                for (int idx = primitiveID * 3; idx < primitiveID * 3 + 3; ++idx)
                    annexe[idx].z = 1.0;
            }
        }
    )"};

    /**
     * Compute shader to compute the contribution of a specific camera
     */
    const std::string COMPUTE_SHADER_COMPUTE_CAMERA_CONTRIBUTION{R"(
        #extension GL_ARB_compute_shader : enable
        #extension GL_ARB_shader_storage_buffer_object : enable
        precision mediump float;

        #include getSmoothBlendFromVertex
        #include normalVector
        #include projectAndCheckVisibility

        layout(local_size_x = 128) in;

        layout (std430, binding = 0) buffer vertexBuffer
        {
            vec4 vertex[];
        };

        layout (std430, binding = 2) buffer normalBuffer
        {
            vec4 normal[];
        };

        layout (std430, binding = 3) buffer annexeBuffer
        {
            vec4 annexe[];
        };

        uniform int _vertexNbr;
        uniform mat4 _mv; // Model View matrix
        uniform mat4 _mvp; // Model View Projection matrix
        uniform mat4 _mNormal; // Normal matrix
        uniform float _blendWidth;

        void main(void)
        {
            // After this pass, the annexe buffer holds:
            // x: number of cameras seeing this vertex
            // y: the blending value for the current camera
            // z: the distance to the given vertex, in normalized space
            
            int globalID = int(gl_GlobalInvocationID.x);
            vec4 screenVertex[3];
            vec4 cameraSpaceVertex[3];
            bvec3 vertexVisible;

            if (globalID < _vertexNbr / 3)
            {
                for (int idx = 0; idx < 3; ++idx)
                {
                    int vertexId = globalID * 3 + idx;

                    // If this vertex was marked as non visible, we can return
                    if (annexe[vertexId].z == 0.0)
                    {
                        // We set the w coordinate to 0
                        for (int idx = 0; idx < 3; ++idx)
                        {
                            int vertexId = globalID * 3 + idx;
                            annexe[vertexId].w = 0.0;
                        }
                        return;
                     }

                    vec2 distToCenter;
                    vec4 normalizedSpaceVertex = vertex[vertexId];
                    vertexVisible[idx] = projectAndCheckVisibility(normalizedSpaceVertex, _mvp, 0.005, distToCenter);
                    screenVertex[idx] = normalizedSpaceVertex;
                    cameraSpaceVertex[idx] = _mv * vec4(vertex[vertexId].xyz, 1.0);
                }

                vec3 projectedNormal = normalVector(screenVertex[0].xyz, screenVertex[1].xyz, screenVertex[2].xyz);
                if (all(vertexVisible) && projectedNormal.z >= 0.0)
                {
                    for (int idx = 0; idx < 3; ++idx)
                    {
                        int vertexId = globalID * 3 + idx;
                        annexe[vertexId].xy += vec2(1.0, getSmoothBlendFromVertex(screenVertex[idx], _blendWidth));
                        annexe[vertexId].w = cameraSpaceVertex[vertexId].z;
                    }
                }
            }
        }
    )"};

    /**************************/
    // FEEDBACK
    /**************************/

    /**
     * Default vertex shader with feedback
     */
    const std::string VERTEX_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA{R"(
        precision mediump float;

        layout (location = 0) in vec4 _vertex;
        layout (location = 1) in vec2 _texCoord;
        layout (location = 2) in vec4 _normal;
        layout (location = 3) in vec4 _annexe;

        uniform mat4 _mvp;
        uniform mat4 _mNormal;

        out VS_OUT
        {
            smooth vec4 vertex;
            smooth vec2 texCoord;
            smooth vec4 normal;
            smooth vec4 annexe;
        } vs_out;

        void main(void)
        {
            vs_out.vertex = _vertex;
            vs_out.texCoord = _texCoord;
            vs_out.normal = _normal;
            vs_out.annexe = _annexe;
        }
    )"};

    /**
     * Default feedback tessellation shader
     */
    const std::string TESS_CTRL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA{R"(
        precision mediump float;

        #include normalVector
        #include projectAndCheckVisibility

        layout (vertices = 3) out;

        in VS_OUT
        {
            smooth vec4 vertex;
            smooth vec2 texCoord;
            smooth vec4 normal;
            smooth vec4 annexe;
        } tcs_in[];

        out TCS_OUT
        {
            vec4 vertex;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } tcs_out[];

        uniform mat4 _mvp;
        uniform mat4 _mNormal;
        uniform float _blendWidth;
        uniform float _blendPrecision;

        const float blendDistFactorToSubdiv = 2.0;

        void main(void)
        {
            if (gl_InvocationID == 0)
            {
                bool anyVertexVisible = false;
                vec4 projectedVertices[3];
                float maxDist = 0.0;
                vec2 distToCenter = vec2(0.0); // Distance to horizontal and vertical sides
                float nearestBorder = 0.0; // 0 is nearest border is horizontal, 1 otherwise

                gl_TessLevelInner[0] = 1.0;
                gl_TessLevelOuter[0] = 1.0;
                gl_TessLevelOuter[1] = 1.0;
                gl_TessLevelOuter[2] = 1.0;

                if (tcs_in[0].annexe.z > 0.0)
                {
                    // Check whether the vertices are visible, and their distances to the borders
                    for (int i = 0; i < 3; ++i)
                    {
                        vec2 localDistToCenter;
                        projectedVertices[i] = tcs_in[i].vertex;
                        if (projectAndCheckVisibility(projectedVertices[i], _mvp, 0.0, localDistToCenter))
                            anyVertexVisible = true;
                        float localMax = max(localDistToCenter.x, localDistToCenter.y);
                        if (localMax > maxDist)
                        {
                            maxDist = localMax;
                            distToCenter = localDistToCenter;
                        }
                    }

                    // Also check for the middle of the edges, to improve the handling of larger faces
                    for (int i = 0; i < 3; ++i)
                    {
                        vec2 distToCenter;
                        vec4 middlePoint = (tcs_in[i].vertex + tcs_in[(i + 1) % 3].vertex) / 2.0;
                        if (projectAndCheckVisibility(middlePoint, _mvp, 0.0, distToCenter))
                            anyVertexVisible = true;
                    }

                    vec3 projectedNormal = normalVector(projectedVertices[0].xyz, projectedVertices[1].xyz, projectedVertices[2].xyz);
                    if (anyVertexVisible && projectedNormal.z >= 0.0)
                    {
                        if (1.0 - maxDist < _blendWidth * blendDistFactorToSubdiv)
                        {
                            const vec2 borderNormals[2] = vec2[2](
                                vec2(0.0, 1.0),
                                vec2(1.0, 0.0)
                            );

                            float maxTessLevel = 1.0;
                            float tessLevelOuter[3] = float[3](1.0, 1.0, 1.0);

                            for (int borderId = 0; borderId < 2; ++borderId)
                            {
                                for (int idx = 0; idx < 3; ++idx)
                                {
                                    int nextIdx = (idx + 1) % 3;
                                    vec2 edge = projectedVertices[nextIdx].xy - projectedVertices[idx].xy;
                                    float edgeProjectedLength = abs(dot(edge, borderNormals[borderId]));
                                    float tessLevel = max(1.0, edgeProjectedLength / _blendPrecision);
                                    tessLevel = mix(1.0, tessLevel, smoothstep(1.0 - min(1.0, blendDistFactorToSubdiv * _blendWidth), 1.0, maxDist));
                                    maxTessLevel = max(maxTessLevel, tessLevel);

                                    int edgeId = (idx + 2) % 3;
                                    tessLevelOuter[edgeId] = max(tessLevelOuter[edgeId], tessLevel);
                                }
                            }

                            gl_TessLevelOuter[0] = tessLevelOuter[0];
                            gl_TessLevelOuter[1] = tessLevelOuter[1];
                            gl_TessLevelOuter[2] = tessLevelOuter[2];
                            gl_TessLevelInner[0] = maxTessLevel;
                        }
                    }
                }
            }

            tcs_out[gl_InvocationID].vertex = tcs_in[gl_InvocationID].vertex;
            tcs_out[gl_InvocationID].texCoord = tcs_in[gl_InvocationID].texCoord;
            tcs_out[gl_InvocationID].normal = tcs_in[gl_InvocationID].normal;
            tcs_out[gl_InvocationID].annexe = tcs_in[gl_InvocationID].annexe;

            gl_out[gl_InvocationID].gl_Position = tcs_out[gl_InvocationID].vertex;
        }
    )"};

    const std::string TESS_EVAL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA{R"(
        precision mediump float;

        //layout (triangles, fractional_odd_spacing) in;
        layout (triangles) in;

        in TCS_OUT
        {
            vec4 vertex;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } tes_in[];

        out TES_OUT
        {
            vec4 vertex;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } tes_out;

        void main(void)
        {
            tes_out.vertex = (gl_TessCoord.x * tes_in[0].vertex) +
                             (gl_TessCoord.y * tes_in[1].vertex) +
                             (gl_TessCoord.z * tes_in[2].vertex);
            tes_out.texCoord = (gl_TessCoord.x * tes_in[0].texCoord) +
                               (gl_TessCoord.y * tes_in[1].texCoord) +
                               (gl_TessCoord.z * tes_in[2].texCoord);
            tes_out.normal = (gl_TessCoord.x * tes_in[0].normal) +
                             (gl_TessCoord.y * tes_in[1].normal) +
                             (gl_TessCoord.z * tes_in[2].normal);
            tes_out.annexe = (gl_TessCoord.x * tes_in[0].annexe) +
                             (gl_TessCoord.y * tes_in[1].annexe) +
                             (gl_TessCoord.z * tes_in[2].annexe);

            gl_Position = tes_out.vertex;
        }
    )"};

    /**
     * Feedback geometry shader for handling camera borders
     */
    const std::string GEOMETRY_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA{R"(
        precision mediump float;

        #include normalVector
        #include projectAndCheckVisibility

        in TES_OUT
        {
            vec4 vertex;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } geom_in[];

        out GEOM_OUT
        {
            vec4 vertex;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } geom_out;

        layout (triangles) in;
        layout (triangle_strip, max_vertices = 9) out;

        const int cutTable[6*9] = int[6*9](
            0, 3, 4, 3, 1, 4, 1, 2, 4,
            0, 3, 4, 3, 1, 4, 4, 2, 0,
            0, 1, 3, 3, 4, 0, 3, 2, 4,
            0, 1, 3, 3, 4, 0, 3, 2, 4,
            0, 3, 4, 3, 1, 4, 4, 2, 0,
            0, 3, 4, 3, 1, 4, 1, 2, 4
        );

        uniform vec2 _fov;
        uniform mat4 _mv; // Model-view matrix
        uniform mat4 _mvp; // Model-view-projection matrix
        uniform mat4 _ip; // Inverse of the projection matrix

        vec4 pointToCameraBase(in vec4 p)
        {
            vec4 coords = _mv * vec4(p.xyz, 1.0);
            coords /= coords.w;
            return coords;
        }

        // Compute the ratio of the camera border projected onto the [pq] segment
        vec2 computeRatios(in vec4 p, in vec4 q, in vec4 borderPointProjectionSpace)
        {
            vec2 r = vec2(0.5, 0.5);
            for (int dir = 0; dir < 2; ++dir)
            {
                //vec4 borderPoint = vec4(1.0, 1.0, 0.5, 1.0);
                vec4 borderPoint = _ip * borderPointProjectionSpace;
                borderPoint /= borderPoint.w;

                vec2 mm = vec2(abs(borderPoint[dir]), borderPoint.z);
                vec2 p1 = vec2(abs(p[dir]), p.z);
                vec2 p2 = vec2(abs(q[dir]), q.z);
                vec2 D = normalize(mm);
                vec2 d = normalize(p2 - p1);
                
                vec2 m;
                if (abs(p1.y - p2.y) < 0.0001)
                {
                    m.y = p1.y;
                    m.x = m.y / D.y * D.x;
                }
                else
                {
                    m.y = (-p1.y * d.x / d.y + p1.x) / (D.x / D.y - d.x / d.y);
                    m.x = m.y / D.y * D.x;
                }
                r[dir] = length(m - p1) / length(p2 - p1);
            }

            return max(vec2(0.0), min(vec2(1.0), r));
        }

        void main(void)
        {
            vec4 projectedVertices[3];
            bvec3 side; // true = inside, false = outside
            vec2 distToBoundary[3];
            vec4 pointsCameraBase[3];
            int cutCase = 0;
            for (int i = 0; i < 3; ++i)
            {
                vec2 distToCenter;
                projectedVertices[i] = geom_in[i].vertex;
                bool isVisible = projectAndCheckVisibility(projectedVertices[i], _mvp, 0.0, distToCenter);
                side[i] = isVisible;
                distToBoundary[i] = distToCenter - vec2(1.0);
                pointsCameraBase[i] = pointToCameraBase(geom_in[i].vertex);
                if (side[i])
                    cutCase += 1 << i;
            }
            cutCase -= 1; // The table starts at 0...

            vec3 normal = normalVector(projectedVertices[0].xyz, projectedVertices[1].xyz, projectedVertices[2].xyz);
            // If all vertices are on the same side, and if the face is correctly oriented
            if (all(side) || all(not(side)) || normal.z < 0.0)
            {
                for (int i = 0; i < 3; ++i)
                {
                    gl_Position = geom_in[i].vertex;
                    geom_out.vertex = geom_in[i].vertex;
                    geom_out.texCoord = geom_in[i].texCoord;
                    geom_out.normal = geom_in[i].normal;
                    geom_out.annexe = geom_in[i].annexe;
                    EmitVertex();
                }

                EndPrimitive();
            }
            // ... if not
            else
            {
                vec4 vertices[6];
                vec2 texCoords[6];
                vec4 normals[6];
                vec4 annexes[6];
                for (int i = 0; i < 3; ++i)
                {
                    vertices[i] = geom_in[i].vertex;
                    texCoords[i] = geom_in[i].texCoord;
                    normals[i] = geom_in[i].normal;
                    annexes[i] = geom_in[i].annexe;
                }
                    
                // Create the additional points
                int nextVertex = 3;
                for (int i = 0; i < 3; ++i)
                {
                    int nextId = (i + 1) % 3;
                    if (side[i] != side[nextId])
                    {
                        // Check on which side the cut should be
                        vec4 borderPoint = vec4(0.0, 0.0, 0.5, 1.0);
                        int exteriorVertexId = !side[i] ? i : nextId;
                        if (!side[exteriorVertexId])
                        {
                            borderPoint.x = projectedVertices[exteriorVertexId].x < 0.0 ? -1.0 : 1.0;
                            borderPoint.y = projectedVertices[exteriorVertexId].y < 0.0 ? -1.0 : 1.0;
                        }

                        // We need to find the ratio in projected space, to find the cut direction
                        vec2 ratios = computeRatios(pointsCameraBase[i], pointsCameraBase[nextId], borderPoint);

                        vec2 signs[2];
                        signs[0] = sign(distToBoundary[i]);
                        signs[1] = sign(distToBoundary[nextId]);

                        float ratio;
                        // Corner case: a point is above both edges
                        if (signs[0].x != signs[1].x && signs[0].y != signs[1].y)
                            ratio = side[i] ? min(ratios[0], ratios[1]) : max(ratios[0], ratios[1]);
                        // First edge case: a point is above the vertical edges
                        else if (signs[0].x != signs[1].x)
                            ratio = ratios[0];
                        // Second edge case: a point is above the horizontal edges
                        else
                            ratio = ratios[1];

                        i = i % 3;
                        vertices[nextVertex] = mix(vertices[i], vertices[nextId], ratio);
                        texCoords[nextVertex] = mix(texCoords[i], texCoords[nextId], ratio);
                        normals[nextVertex] = mix(normals[i], normals[nextId], ratio);
                        nextVertex++;
                    }
                }

                // Create the triangles from the cut case
                for (int t = 0; t < 3; ++t)
                {
                    for (int v = 0; v < 3; ++v)
                    {
                        int currentIndex = cutTable[cutCase * 9 + t * 3 + v];
                        gl_Position = vertices[currentIndex];
                        geom_out.vertex = vertices[currentIndex];
                        geom_out.texCoord = texCoords[currentIndex];
                        geom_out.normal = normals[currentIndex];
                        EmitVertex();
                    }

                    EndPrimitive();
                }
            }
        }
    )"};

    /**************************/
    // GRAPHICS
    /**************************/

    /**
     * Vertex shader which transmits the vertex attributes as-is
     */
    const std::string VERTEX_SHADER_DEFAULT{R"(
        precision mediump float;

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        layout(location = 2) in vec4 _normal;

        out VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
        } vertexOut;

        void main(void)
        {
            vertexOut.position = vec4(_vertex.xyz, 1.0);
            gl_Position = vertexOut.position;
            vertexOut.normal = _normal;
            vertexOut.texCoord = _texCoord;
        }
    )"};

    /**
     * Vertex shader which projects the vertices using the modelview matrix
     */
    const std::string VERTEX_SHADER_MODELVIEW{R"(
        precision mediump float;

        #include getSmoothBlendFromVertex

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        layout(location = 2) in vec4 _normal;
        layout(location = 3) in vec4 _annexe;

        uniform mat4 _modelViewProjectionMatrix;
        uniform mat4 _normalMatrix;
        // TODO: OpenGL ES doesn't allow this initialization, make sure it doesn't break anything.
        uniform vec4 _cameraAttributes; // blendWidth, brightness, saturation, contrast

        out VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } vertexOut;

        void main(void)
        {
            vertexOut.position = vec4(_vertex.xyz, 1.0);
            vertexOut.position = _modelViewProjectionMatrix * vertexOut.position;
            gl_Position = vertexOut.position;
            vertexOut.normal = normalize(_normalMatrix * _normal);
            vertexOut.texCoord = _texCoord;
            vertexOut.annexe = _annexe;
        }
    )"};

    /**
     * Filter vertex shader
     */
    const std::string VERTEX_SHADER_FILTER{R"(
        precision mediump float;

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.xyz, 1.0);
            texCoord = _texCoord;
        }
    )"};

    /**
     * Default fragment shader for filters
     * Does not do much except for applying the intput texture
     */
    const std::string FRAGMENT_SHADER_DEFAULT_FILTER{R"(
        precision mediump float;

    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif

        in vec2 texCoord;
        out vec4 fragColor;

        uniform vec2 _tex0_size;

        uniform float _blackLevel;

        void main()
        {
    #ifdef TEXTURE_RECT
            vec4 color = texture(_tex0, texCoord * _tex0_size);
    #else
            vec4 color = texture(_tex0, texCoord);
    #endif

            fragColor.rgb = color.rgb;
        }
    )"};

    /**
     * Image fragment shader for filters
     * This filter applies various color corrections, and is
     * also able to convert from YUYV to RGB
     */
    const std::string FRAGMENT_SHADER_IMAGE_FILTER{R"(
        precision mediump float;

        #include colorEncoding
        #include srgb
        #include hsv
        #include correctColor
        #include yuv

        #define PI 3.14159265359

    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif

        in vec2 texCoord;
        out vec4 fragColor;

        uniform vec2 _tex0_size;
        // Texture transformation
        uniform int _tex0_flip;
        uniform int _tex0_flop;
        // Format specific parameters
        uniform int _tex0_encoding;

        // Film uniforms
        /* uniform float _filmDuration; */
        /* uniform float _filmRemaining; */

        // Filter parameters
        uniform float _brightness;
        uniform float _contrast;
        uniform float _saturation;
        uniform int _invertChannels;
        uniform vec2 _colorBalance;
        uniform vec2 _scale;

        void main(void)
        {
            // Compute the real texture coordinates, according to flip / flop
            vec2 realCoords;
            if (_tex0_flip == 1 && _tex0_flop == 0)
                realCoords = vec2(texCoord.x, 1.0 - texCoord.y);
            else if (_tex0_flip == 0 && _tex0_flop == 1)
                realCoords = vec2(1.0 - texCoord.x, texCoord.y);
            else if (_tex0_flip == 1 && _tex0_flop == 1)
                realCoords = vec2(1.0 - texCoord.x, 1.0 - texCoord.y);
            else
                realCoords = texCoord;

            realCoords = fma((realCoords - vec2(0.5)), vec2(1.0) / _scale, vec2(0.5));

    #ifdef TEXTURE_RECT
            vec4 color = texture(_tex0, realCoords * _tex0_size);
    #else
            vec4 color = texture(_tex0, realCoords);
    #endif

            // If the color encoding is BGR, we need to invert red and blue channels
            if (_tex0_encoding == COLOR_BGR)
            {
                color.rgb = color.bgr;
            }
            // If the color is expressed as YCoCg (for HapQ compression), extract RGB color from it
            else if (_tex0_encoding == COLOR_YCoCg)
            {
                float scale = (color.z * (255.0 / 8.0)) + 1.0;
                float Co = (color.x - (0.5 * 256.0 / 255.0)) / scale;
                float Cg = (color.y - (0.5 * 256.0 / 255.0)) / scale;
                float Y = color.w;
                color.rgba = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);
                color.rgb = pow(color.rgb, vec3(2.2));
            }
            // If the color format is YUYV
            else if (_tex0_encoding == COLOR_UYVY || _tex0_encoding == COLOR_YUYV)
            {
                // Texture coord rounded to the closer even pixel
                ivec2 yuyvCoords = ivec2((int(realCoords.x * _tex0_size.x) / 2) * 2, int(realCoords.y * _tex0_size.y));
                vec4 yuyv;
                yuyv.rg = texelFetch(_tex0, yuyvCoords, 0).rg;
                yuyv.ba = texelFetch(_tex0, ivec2(yuyvCoords.x + 1, yuyvCoords.y), 0).rg;

                if (_tex0_encoding == COLOR_UYVY)
                    yuyv = yuyv.grab;

                if (int(realCoords.x * _tex0_size.x) - (yuyvCoords.x / 2) * 2 == 0) // Even pixel
                    color.rgb = yuv2rgb(yuyv.rga);
                else // Odd pixel
                    color.rgb = yuv2rgb(yuyv.bga);
            }
            
            // Invert channels
            if (_invertChannels == 1)
                color.rgb = color.bgr;

            // Color balance
        
        vec2 colorBalance = _colorBalance;

        if(colorBalance.x == 0.0f && colorBalance.y == 0.0f) {
        colorBalance = vec2(1.0f, 1.0f);
        }

            float maxBalanceRatio = max(colorBalance.r, _colorBalance.g);
            color.r *= colorBalance.r / maxBalanceRatio;
            color.g *= 1.0 / maxBalanceRatio;
            color.b *= colorBalance.g / maxBalanceRatio;

            color = correctColor(color, _brightness, _saturation, _contrast);
            fragColor = color;
        }
    )"};

    /**
     * Black level fragment shader for filters
     */
    const std::string FRAGMENT_SHADER_BLACKLEVEL_FILTER{R"(
        precision mediump float;

    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif

        in vec2 texCoord;
        out vec4 fragColor;

        uniform vec2 _tex0_size;

        uniform float _blackLevel;

        void main()
        {
    #ifdef TEXTURE_RECT
            vec4 color = texture(_tex0, texCoord * _tex0_size);
    #else
            vec4 color = texture(_tex0, texCoord);
    #endif

            fragColor.rgb = color.rgb * (1.0 - _blackLevel) + _blackLevel;
        }
    )"};

    /**
     * Color curves fragment shader for filters
     * This filter applies a transformation curve to RGB colors
     */
    const std::string FRAGMENT_SHADER_COLOR_CURVES_FILTER{R"(
        precision mediump float;

    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif

        in vec2 texCoord;
        out vec4 fragColor;

        uniform vec2 _tex0_size;

    #ifdef COLOR_CURVE_COUNT
        // This is set if Filter::_colorCurves is not empty, by Filter::updateShaderParameters
        uniform vec3 _colorCurves[COLOR_CURVE_COUNT];
    #endif

        int factorial(int n)
        {
            if (n == 0 || n == 1)
                return 1;
            int res = 1;
            for (int i = 2; i <= n; ++i)
                res *= i;
            return res;
        }

        float binomialCoeff(int n, int i)
        {
            if (n < i)
                return 0.f;
            return float(factorial(n) / (factorial(i) * factorial(n - i)));
        }

        void main()
        {
    #ifdef TEXTURE_RECT
            vec4 color = texture(_tex0, texCoord * _tex0_size);
    #else
            vec4 color = texture(_tex0, texCoord);
    #endif

    #ifdef COLOR_CURVE_COUNT
            color = clamp(color, vec4(0.0), vec4(1.0));
            float factors[COLOR_CURVE_COUNT];
            for (int i = 0; i < COLOR_CURVE_COUNT; ++i)
                factors[i] = binomialCoeff(COLOR_CURVE_COUNT - 1, i);

            vec3 curvedColor = vec3(0.0);
            for (int i = 0; i < COLOR_CURVE_COUNT; ++i)
            {
                // We use 0.9999 and not 1.0 because of imprecision
                vec3 factor = factors[i] * pow(color.rgb, vec3(float(i))) * pow(vec3(0.9999) - color.rgb, vec3(float(COLOR_CURVE_COUNT) - 1.0 - float(i)));
                curvedColor += factor * _colorCurves[i];
            }
            color.rgb = curvedColor.rgb;
    #endif

            fragColor.rgb = color.rgb;
        }
    )"};

    /**
     * Warp vertex shader
     */
    const std::string VERTEX_SHADER_WARP{R"(
        precision mediump float;

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.x, _vertex.y, _vertex.z, 1.0);
            texCoord = _texCoord;
        }
    )"};

    /**
     * Fragment shader for warp
     */
    const std::string FRAGMENT_SHADER_WARP{R"(
        precision mediump float;
        #define PI 3.14159265359

    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif

        in vec2 texCoord;
        out vec4 fragColor;

        uniform vec2 _tex0_size;
        // Texture transformation
        uniform int _tex0_flip;
        uniform int _tex0_flop;

        void main(void)
        {
            // Compute the real texture coordinates, according to flip / flop
            vec2 realCoords;
            if (_tex0_flip == 1 && _tex0_flop == 0)
                realCoords = vec2(texCoord.x, 1.0 - texCoord.y);
            else if (_tex0_flip == 0 && _tex0_flop == 1)
                realCoords = vec2(1.0 - texCoord.x, texCoord.y);
            else if (_tex0_flip == 1 && _tex0_flop == 1)
                realCoords = vec2(1.0 - texCoord.x, 1.0 - texCoord.y);
            else
                realCoords = texCoord;

    #ifdef TEXTURE_RECT
            vec4 color = texture(_tex0, realCoords * _tex0_size);
    #else
            vec4 color = texture(_tex0, realCoords);
    #endif

            fragColor = color;
        }
    )"};

    /**
     * Cubemap objects rendering (used by the virtual screens)
     */
    const std::string VERTEX_SHADER_OBJECT_CUBEMAP{R"(
        precision mediump float;

        #include getSmoothBlendFromVertex

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;

        out VertexData
        {
            vec4 position;
            vec2 texCoord;
        } vertexOut;

        uniform mat4 _modelViewMatrix;

        void main(void)
        {
            vertexOut.position = vec4(_vertex.xyz, 1.0);
            vertexOut.position = _modelViewMatrix * vertexOut.position;
            gl_Position = vertexOut.position;
            vertexOut.texCoord = _texCoord;
        }
    )"};

    const std::string GEOMETRY_SHADER_OBJECT_CUBEMAP{R"(
        precision mediump float;

        layout(triangles) in;
        layout(triangle_strip, max_vertices = 18) out;

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
        } vertexIn[];

        out VertexData
        {
            vec4 position;
            vec2 texCoord;
        } vertexOut;

        uniform mat4 _projectionMatrix;

        const mat4 cubemapMat[6] = mat4[](
            mat4(1.0, 0.0, 0.0, 0.0,
                 0.0, 1.0, 0.0, 0.0,
                 0.0, 0.0, 1.0, 0.0,
                 0.0, 0.0, 0.0, 1.0),
            mat4(-1.0, 0.0, 0.0, 0.0,
                 0.0, 1.0, 0.0, 0.0,
                 0.0, 0.0, -1.0, 0.0,
                 0.0, 0.0, 0.0, 1.0),
            mat4(0.0, -1.0, 0.0, 0.0,
                 0.0, 0.0, 1.0, 0.0,
                 -1.0, 0.0, 0.0, 0.0,
                 0.0, 0.0, 0.0, 1.0),
            mat4(0.0, 1.0, 0.0, 0.0,
                 0.0, 0.0, -1.0, 0.0,
                 -1.0, 0.0, 0.0, 0.0,
                 0.0, 0.0, 0.0, 1.0),
            mat4(0.0, 0.0, 1.0, 0.0,
                 0.0, 1.0, 0.0, 0.0,
                 -1.0, 0.0, .0, 0.0,
                 0.0, 0.0, 0.0, 1.0),
            mat4(0.0, 0.0, -1.0, 0.0,
                 0.0, 1.0, 0.0, 0.0,
                 1.0, 0.0, 0.0, 0.0,
                 0.0, 0.0, 0.0, 1.0)
            );

        void main() 
        {
            for (int face = 0; face < 6; ++face)
            {
                gl_Layer = face;

                for (int i = 0; i < 3; ++i)
                {
                    vertexOut.position = _projectionMatrix * cubemapMat[face] * vertexIn[i].position;
                    gl_Position = vertexOut.position;
                    vertexOut.texCoord = vertexIn[i].texCoord;
                    EmitVertex();
                }
                EndPrimitive();
            }
        }
    )"};

    const std::string FRAGMENT_SHADER_OBJECT_CUBEMAP{R"(
        precision mediump float;

        #ifdef TEXTURE_RECT
            uniform sampler2DRect _tex0;
        #else
            uniform sampler2D _tex0;
        #endif

            uniform vec2 _tex0_size;

            in VertexData
            {
                vec4 position;
                vec2 texCoord;
            } vertexIn;

            out vec4 fragColor;

            void main(void)
            {
        #ifdef TEXTURE_RECT
                vec4 color = texture(_tex0, vertexIn.texCoord * _tex0_size);
        #else
                vec4 color = texture(_tex0, vertexIn.texCoord);
        #endif
                
                fragColor = color;
            }
    )"};

    /**
     * Vertex shader for textured rendering
     */
    const std::string VERTEX_SHADER_TEXTURE{R"(
        precision mediump float;

        #include getSmoothBlendFromVertex

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        layout(location = 2) in vec4 _normal;
        layout(location = 3) in vec4 _annexe;

        uniform mat4 _modelViewProjectionMatrix;
        uniform mat4 _modelViewMatrix;
        uniform mat4 _normalMatrix;
        uniform vec4 _cameraAttributes; // blendWidth, brightness, saturation, contrast

    #ifdef VERTEXBLENDING
        uniform float _farthestVertex;
    #endif

        out VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
            float blendingValue;
        } vertexOut;

        void main(void)
        {
            vertexOut.position = vec4(_vertex.xyz, 1.0);
            vertexOut.position = _modelViewProjectionMatrix * vertexOut.position;
            gl_Position = vertexOut.position;
            vertexOut.normal = normalize(_normalMatrix * _normal);
            vertexOut.texCoord = _texCoord;
            vertexOut.annexe = _annexe;

            vec4 projectedVertex = vertexOut.position / vertexOut.position.w;
            if (projectedVertex.z >= 0.0)
            {
    #ifdef VERTEXBLENDING
                // Compute the distance to the camera, then compare it to the farthest
                // vertex distance and adjust blending value based on this
                float vertexDistToCam = abs(_modelViewMatrix * vec4(_vertex.xyz, 1.0)).z;
                // The luminance diminishes with the square of the distance
                // luminanceRatio should always be less than 1.0, as
                // _farthestVertex is by definition the highest possible distance
                float luminanceRatio = 1.0;
                if (_farthestVertex != 0.0)
                {
                    luminanceRatio = vertexDistToCam / _farthestVertex;
                    luminanceRatio = luminanceRatio * luminanceRatio;
                }
                vertexOut.blendingValue = luminanceRatio * min(1.0, getSmoothBlendFromVertex(projectedVertex, _cameraAttributes.x) / _annexe.y);
    #else
                vertexOut.blendingValue = 1.0;
    #endif
            }
        }
    )"};

    /**
     * Textured fragment shader
     */
    const std::string FRAGMENT_SHADER_TEXTURE{R"(
        precision mediump float;

        #include hsv
        #include correctColor

        #define PI 3.14159265359

    #ifdef TEX_1
    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif
    #endif

    #ifdef TEX_2
        uniform sampler2D _tex1;
    #endif

        // TODO: Make sure the removed initializations don't break anything.
        uniform vec2 _tex0_size;
        uniform vec2 _tex1_size;

        uniform int _showCameraCount;
        uniform int _sideness;
        
        // blendWidth, brightness, saturation, contrast
        uniform vec4 _cameraAttributes;

        // fovX and fovY, r/g and b/g
        uniform vec4 _fovAndColorBalance;

        uniform int _isColorLUT;
        uniform vec4 _color;
        uniform int _colorLUTSize;
        uniform vec3 _colorLUT[256];
        uniform mat3 _colorMixMatrix;
        uniform float _normalExp;

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
            float blendingValue;
        } vertexIn;

        out vec4 fragColor;

        void main(void)
        {
            float blendWidth = _cameraAttributes.x;
            float brightness = _cameraAttributes.y;
            float saturation = _cameraAttributes.z;
            float contrast = _cameraAttributes.w;

            vec4 position = vertexIn.position;
            vec2 texCoord = vertexIn.texCoord;
            vec4 normal = vertexIn.normal;

            vec2 screenPos = vec2(position.x / position.w, position.y / position.w);

        #ifdef TEX_1
        #ifdef TEXTURE_RECT
            vec4 color = texture(_tex0, texCoord * _tex0_size);
        #else
            vec4 color = texture(_tex0, texCoord);
        #endif
        #else
            vec4 color = _color;
        #endif

        #ifdef TEX_2
        #ifdef TEXTURE_RECT
            vec4 maskColor = texture(_tex1, texCoord * _tex1_size);
        #else
            vec4 maskColor = texture(_tex1, texCoord);
        #endif
            color.rgb = mix(color.rgb, maskColor.rgb, maskColor.a);
        #endif

            float maxBalanceRatio = max(_fovAndColorBalance.z, _fovAndColorBalance.w);
            color.r *= _fovAndColorBalance.z / maxBalanceRatio;
            color.g *= 1.0 / maxBalanceRatio;
            color.b *= _fovAndColorBalance.w / maxBalanceRatio;

        #ifdef VERTEXBLENDING
            color.rgb = color.rgb * vertexIn.blendingValue;
        #endif

            // Brightness correction
            color = correctColor(color, brightness, saturation, contrast);

            // Color correction through a LUT
            if (_isColorLUT != 0)
            {
                vec3 maxValue = vec3(_colorLUTSize - 1);
                vec3 fcolor = vec3(floor(color.rgb * maxValue));
                vec3 ccolor = vec3(ceil(color.rgb * maxValue));
                vec3 alpha = color.rgb * maxValue - fcolor.rgb;
                // Linear interpolation
                color.r = _colorLUT[int(fcolor.r)].r * (1.f - alpha.r) + _colorLUT[int(ccolor.r)].r * alpha.r; 
                color.g = _colorLUT[int(fcolor.g)].g * (1.f - alpha.g) + _colorLUT[int(ccolor.g)].g * alpha.g;
                color.b = _colorLUT[int(fcolor.b)].b * (1.f - alpha.b) + _colorLUT[int(ccolor.b)].b * alpha.b;
                //color.rgb = clamp(_colorMixMatrix * color.rgb, vec3(0.0), vec3(1.0));
            }

            fragColor.rgb = color.rgb;

            if (_normalExp != 0.0)
            {
                float normFactor = abs(dot(normal.xyz, vec3(0.0, 0.0, 1.0)));
                fragColor.rgb *= pow(normFactor, _normalExp);
            }
            
            fragColor.a = 1.0;

            if (_showCameraCount > 0)
            {
                int count = int(round(vertexIn.annexe.x));
                int r = count - (count / 2) * 2;
                count -= r;
                int g = count - (count / 4) * 4;
                count -= r * 2;
                int b = count - (count / 8) * 8;
                fragColor.rgba = vec4(float(r), float(g), float(b), 1.0);
            }
        }
    )"};

    /**
     * Single color fragment shader
     */
    const std::string FRAGMENT_SHADER_COLOR{R"(
        precision mediump float;

        #define PI 3.14159265359

        uniform vec4 _color;

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } vertexIn;

        out vec4 fragColor;

        void main(void)
        {
            vec4 position = vertexIn.position;
            vec2 texCoord = vertexIn.texCoord;
            vec4 normal = vertexIn.normal;

            fragColor.rgb = _color.rgb * vec3(0.3 + 0.7 * dot(normal.xyz, normalize(vec3(1.0, 1.0, 1.0))));
            fragColor.a = _color.a;
        }
    )"};

    /**
     * UV drawing fragment shader
     * UV coordinates are encoded on 2 channels each, to get 16bits precision
     */
    const std::string FRAGMENT_SHADER_UV{R"(
        precision mediump float;

        #define PI 3.14159265359

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } vertexIn;

        out vec4 fragColor;

        void main(void)
        {
            vec4 position = vertexIn.position;
            vec2 texCoord = vertexIn.texCoord;
            vec4 normal = vertexIn.normal;

            float U = texCoord.x * 65536.0;
            float V = texCoord.y * 65536.0;

            fragColor.rg = vec2(floor(U / 256.0) / 256.0, (U - floor(U / 256.0) * 256.0) / 256.0);
            fragColor.ba = vec2(floor(V / 256.0) / 256.0, (V - floor(V / 256.0) * 256.0) / 256.0);
        }
    )"};

    /**
     * Draws the primitive ID
     * This shader has to be used after a pass of COMPUTE_SHADER_RESET_VISIBILITY
     */
    const std::string FRAGMENT_SHADER_PRIMITIVEID{R"(
        precision mediump float;

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } vertexIn;

        out vec4 fragColor;

        void main(void)
        {
            float index = round(vertexIn.annexe.w);
            float thirdOrder = floor(index / 65025.0);
            float secondOrder = floor(fma(thirdOrder, -65025.0, index) / 255.0);
            float firstOrder = fma(secondOrder, -255.0, fma(thirdOrder, -65025.0, index));
            fragColor = vec4(thirdOrder, secondOrder, firstOrder, 255.0) / 255.0;
        }
    )"};

    /**
     * Wireframe rendering
     */
    const std::string VERTEX_SHADER_WIREFRAME{R"(
        precision mediump float;

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        layout(location = 2) in vec4 _normal;

        out VertexData
        {
            vec4 vertex;
            vec2 texCoord;
        } vertexOut;

        void main()
        {
            vertexOut.vertex = _vertex;
            vertexOut.texCoord = _texCoord;
        }
    )"};

    const std::string GEOMETRY_SHADER_WIREFRAME{R"(
        precision mediump float;

        layout(triangles) in;
        layout(triangle_strip, max_vertices = 3) out;
        uniform mat4 _modelViewProjectionMatrix;

        in VertexData
        {
            vec4 vertex;
            vec2 texCoord;
        } vertexIn[];

        out VertexData
        {
            vec2 texCoord;
            vec3 bcoord;
            vec4 position;
        } vertexOut;

        void main()
        {
            vec4 v = _modelViewProjectionMatrix * vec4(vertexIn[0].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.texCoord = vertexIn[0].texCoord;
            vertexOut.bcoord = vec3(1.0, 0.0, 0.0);
            vertexOut.position = v;
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[1].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.texCoord = vertexIn[1].texCoord;
            vertexOut.bcoord = vec3(0.0, 1.0, 0.0);
            vertexOut.position = v;
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[2].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.texCoord = vertexIn[2].texCoord;
            vertexOut.bcoord = vec3(0.0, 0.0, 1.0);
            vertexOut.position = v;
            EmitVertex();

            EndPrimitive();
        }
    )"};

    const std::string FRAGMENT_SHADER_WIREFRAME{R"(
        precision mediump float;

        #define PI 3.14159265359

        in VertexData
        {
            vec2 texCoord;
            vec3 bcoord;
            vec4 position;
        } vertexIn;

        uniform vec4 _wireframeColor;
        out vec4 fragColor;

        float edgeFactor()
        {
            vec3 d = fwidth(vertexIn.bcoord);
            vec3 a3 = smoothstep(vec3(0.0), d * 1.5, vertexIn.bcoord);
            return min(min(a3.x, a3.y), a3.z);
        }

        void main(void)
        {
            vec4 position = vertexIn.position;

            vec3 b = vertexIn.bcoord;
            float minDist = min(min(b[0], b[1]), b[2]);
            vec4 matColor = vec4(0.3, 0.3, 0.3, 1.0);
            fragColor.rgba = mix(_wireframeColor, matColor, edgeFactor());
        }
    )"};

    /**
     * Cubemap projection to spherical, equirectangular, or "single layer cubemap"
     */
    const std::string VERTEX_SHADER_CUBEMAP_PROJECTION{R"(
        precision mediump float;

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.xyz, 1.0);
            texCoord = _texCoord;
        }
    )"};

    const std::string FRAGMENT_SHADER_CUBEMAP_PROJECTION{R"(
        precision mediump float;

        #define PI 3.141592653589793
        #define HALFPI 1.5707963268 

        uniform samplerCube _tex0;
        uniform vec2 _tex0_size;

        uniform int _projectionType; // 0 = equirectangular, 1 = spherical
        uniform float _sphericalFov;

        in vec2 texCoord;
        out vec4 fragColor;

        void main(void)
        {
            vec2 cuv = vec2(texCoord.s, texCoord.t) * 2.0; // Centered UVs

            if (_projectionType == 0)
            {
                float alpha = cuv.x * PI;// + PI / 4.0;
                float phi = PI - cuv.y * HALFPI;

                vec3 direction = vec3(0.0, 0.0, 1.0);
                mat3 rotationZ = mat3(cos(alpha), sin(alpha), 0.0,
                                      -sin(alpha), cos(alpha), 0.0,
                                      0.0, 0.0, 1.0);
                mat3 rotationY = mat3(cos(phi), 0.0, -sin(phi),
                                      0.0, 1.0, 0.0,
                                      sin(phi), 0.0, cos(phi));
                direction = rotationZ * rotationY * direction;
                fragColor = texture(_tex0, direction);
            }
            else if (_projectionType == 1)
            {
                vec2 cuv = (vec2(texCoord.x, texCoord.y) - 0.5) * 2.0; // Centered UVs
                if (length(cuv) > 1.0)
                {
                    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
                    return;
                }
    
                float phi = length(cuv) * (_sphericalFov / 360.0) * PI;
                vec2 dir = normalize(cuv);
                float alpha = atan(dir.y, dir.x) + PI;

                vec3 direction = vec3(0.0, 0.0, 1.0);
                mat3 rotationZ = mat3(cos(alpha), sin(alpha), 0.0,
                                      -sin(alpha), cos(alpha), 0.0,
                                      0.0, 0.0, 1.0);
                mat3 rotationY = mat3(cos(phi), 0.0, -sin(phi),
                                      0.0, 1.0, 0.0,
                                      sin(phi), 0.0, cos(phi));
                direction = rotationZ * rotationY * direction;
                fragColor = texture(_tex0, direction);
            }
            else
            {
                fragColor = vec4(cuv.st, 0.0, 1.0);
            }
        }
    )"};

    /**
     * Wireframe rendering for Warps
     */
    const std::string VERTEX_SHADER_WARP_WIREFRAME{R"(
        precision mediump float;

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;

        out VertexData
        {
            vec4 vertex;
            vec2 texCoord;
        } vertexOut;

        void main()
        {
            vertexOut.vertex = _vertex;
            vertexOut.texCoord = _texCoord;
        }
    )"};

    const std::string GEOMETRY_SHADER_WARP_WIREFRAME{R"(
        precision mediump float;

        layout(triangles) in;
        layout(triangle_strip, max_vertices = 3) out;

        in VertexData
        {
            vec4 vertex;
            vec2 texCoord;
        } vertexIn[];

        out VertexData
        {
            vec3 bcoord;
        } vertexOut;

        void main()
        {
            vec4 v = vec4(vertexIn[0].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.bcoord = vec3(1.0, 0.0, 0.0);
            EmitVertex();

            v = vec4(vertexIn[1].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.bcoord = vec3(0.0, 1.0, 0.0);
            EmitVertex();

            v = vec4(vertexIn[2].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.bcoord = vec3(0.0, 0.0, 1.0);
            EmitVertex();

            EndPrimitive();
        }
    )"};

    const std::string FRAGMENT_SHADER_WARP_WIREFRAME{R"(
        precision mediump float;

        #define PI 3.14159265359

        in VertexData
        {
            vec3 bcoord;
        } vertexIn;

        uniform int _sideness;
        uniform vec4 _fovAndColorBalance; // fovX and fovY, r/g and b/g
        out vec4 fragColor;

        void main(void)
        {
            vec3 b = vertexIn.bcoord;
            float minDist = min(b[1], b[2]);
            vec4 matColor = vec4(0.3, 0.3, 0.3, 1.0);
            if (minDist < 0.005)
                fragColor.rgba = mix(vec4(vec3(0.5), 1.0), matColor, (minDist - 0.0025) / 0.0025);
            else
                discard;
        }
    )"};

    /**
     * Rendering of the output windows
     */
    const std::string VERTEX_SHADER_WINDOW{R"(
        precision mediump float;

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texCoord;
        //layout(location = 2) in vec3 _normal;
        //uniform mat4 _modelViewProjectionMatrix;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.x, _vertex.y, _vertex.z, 1.0);
            texCoord = _texCoord;
        }
    )"};

    const std::string FRAGMENT_SHADER_WINDOW{R"(
        precision mediump float;

        #define PI 3.14159265359

    #ifdef TEX_1
        uniform ivec4 _layout;

        uniform sampler2D _tex0;
    #ifdef TEX_2
        uniform sampler2D _tex1;
    #ifdef TEX_3
        uniform sampler2D _tex2;
    #ifdef TEX_4
        uniform sampler2D _tex3;
    #endif
    #endif
    #endif
    #endif
        uniform vec2 _gamma;
        in vec2 texCoord;
        out vec4 fragColor;

        const float texcount = float(TEXCOUNT);
        const float width = 1.0 / float(TEXCOUNT);

        void main(void)
        {
            fragColor = vec4(0.0);

            for (int i = 0; i < TEXCOUNT; ++i)
            {
                vec2 tc = vec2((texCoord.x - width * float(i)) * texcount, texCoord.y);
                if (tc.x < 0.0)
                    continue;
                #ifdef TEX_1
                if (_layout[i] == 0)
                    fragColor = texture(_tex0, tc);
                #ifdef TEX_2
                else if (_layout[i] == 1)
                    fragColor = texture(_tex1, tc);
                #ifdef TEX_3
                else if (_layout[i] == 2)
                    fragColor = texture(_tex2, tc);
                #ifdef TEX_4
                else if (_layout[i] == 3)
                    fragColor = texture(_tex3, tc);
                #endif
                #endif
                #endif
                #endif
            }

            if(_gamma.x == 0.0) {
                float gamma = clamp(_gamma.y, 1.0 , 3.0);
                fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / gamma));
            }
        }
    )"};

    const std::string FRAGMENT_SHADER_EMPTY = "void main() {}";

} // namespace Splash

#endif // SPLASH_SHADERSOURCES_H
