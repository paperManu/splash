/*
 * Copyright (C) 2014 Emmanuel Durand
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

#include "./config.h"

namespace Splash
{

struct ShaderSources
{
    const std::map<std::string, std::string> INCLUDES{//
        // Project a point wrt a mvp matrix, and check if it is in the view frustum.
        // Returns the distance on X and Y in the distToCenter parameter
        {"projectAndCheckVisibility", R"(
            bool projectAndCheckVisibility(inout vec4 p, in mat4 mvp, in float margin, out vec2 distToCenter)
            {
                vec4 projected = mvp * vec4(p.xyz, 1.0);
                projected /= projected.w;
                p = projected;

                if (projected.z >= 0.0)
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
                weight = pow(max(0.0, min(1.0, weight)), 2.0);

                // d4 (and d3 if pow(x, 1.0))
                //float weight = pow(abs(dist.x / blendDist * dist.y / blendDist), 1.5);
                
                return weight;
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
        {"yuv", R"(
            vec3 yuv2rgb(vec3 c)
            {
                // Input colors are stored with a gamma applied to them
                vec3 yuv = pow(c, vec3(1.0/2.2));
                vec3 rgb = vec3((yuv.r - 16.0/255.0) + 1.403*(yuv.b - 0.5),
                                (yuv.r - 16.0/255.0) - 0.344*(yuv.g - 0.5) - 0.714*(yuv.b - 0.5),
                                (yuv.r - 16.0/255.0) + 1.772*(yuv.g - 0.5));
                rgb = clamp(rgb, vec3(0.0), vec3(1.0));
                rgb = pow(rgb, vec3(2.2));
                return rgb;
            }

            vec3 rgb2yuv(vec3 c)
            {
                // Input colors are stored with a gamma applied to them
                vec3 rgb = pow(c, vec3(1.0/2.2));
                vec3 yuv = vec3(0.299*rgb.r + 0.587*rgb.g + 0.114*rgb.b + 16.0/255.0,
                                -0.16874*rgb.r - 0.33126*rgb.g + 0.5*rgb.b + 0.5,
                                0.5*rgb.r - 0.41869*rgb.g - 0.08131*rgb.b + 0.5);
                yuv = clamp(yuv, vec3(0.0), vec3(1.0));
                yuv = pow(yuv, vec3(2.2));
                return yuv;
            }
        )"}};

/**
 * Version directive, included at the start of all shaders
 */
#if HAVE_OSX
    const std::string VERSION_DIRECTIVE_GL4{R"(
        #version 410 core
    )"};
#else
    const std::string VERSION_DIRECTIVE_GL4{R"(
        #version 430 core
    )"};
#endif

    /**************************/
    // COMPUTE
    /**************************/

    /**
     * Default compute shader
     */
    const std::string COMPUTE_SHADER_DEFAULT{R"(
        #extension GL_ARB_compute_shader : enable
        #extension GL_ARB_shader_storage_buffer_object : enable

        layout(local_size_x = 32, local_size_y = 32) in;

        layout (std430, binding = 0) buffer vertexBuffer
        {
            vec4 vertex[];
        };

        layout (std430, binding = 1) buffer texcoordsBuffer
        {
            vec2 texcoords[];
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

        layout(local_size_x = 128) in;

        layout (std430, binding = 3) buffer annexeBuffer
        {
            vec4 annexe[]; // Holds camera count, blending sum, flag set to true if primitive is visible, primitive ID
        };

        uniform int _vertexNbr;
        uniform int _primitiveIdShift = 0;

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

        layout(local_size_x = 32, local_size_y = 32) in;

        layout(binding = 0) uniform sampler2D imgVisibility;
        layout(std430, binding = 3) buffer annexeBuffer
        {
            vec4 annexe[];
        };

        uniform vec2 _texSize;
        uniform int _idShift = 0;

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
        uniform mat4 _mvp;
        uniform mat4 _mNormal;
        uniform float _blendWidth = 0.1;

        void main(void)
        {
            int globalID = int(gl_GlobalInvocationID.x);
            vec4 screenVertex[3];
            bvec3 vertexVisible;

            if (globalID < _vertexNbr / 3)
            {
                for (int idx = 0; idx < 3; ++idx)
                {
                    int vertexId = globalID * 3 + idx;

                    // If this vertex was marked as non visible, we can return
                    if (annexe[vertexId].z == 0.0)
                        return;

                    vec2 distToCenter;
                    vec4 normalizedSpaceVertex = vertex[vertexId];
                    vertexVisible[idx] = projectAndCheckVisibility(normalizedSpaceVertex, _mvp, 0.005, distToCenter);
                    screenVertex[idx] = normalizedSpaceVertex;
                }

                vec3 projectedNormal = normalVector(screenVertex[0].xyz, screenVertex[1].xyz, screenVertex[2].xyz);
                if (all(vertexVisible) && projectedNormal.z >= 0.0)
                {
                    for (int idx = 0; idx < 3; ++idx)
                    {
                        int vertexId = globalID * 3 + idx;
                        annexe[vertexId].xy += vec2(1.0, getSmoothBlendFromVertex(screenVertex[idx], _blendWidth));
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
        layout (location = 0) in vec4 _vertex;
        layout (location = 1) in vec2 _texcoord;
        layout (location = 2) in vec4 _normal;
        layout (location = 3) in vec4 _annexe;

        uniform mat4 _mvp;
        uniform mat4 _mNormal;

        out VS_OUT
        {
            smooth vec4 vertex;
            smooth vec2 texcoord;
            smooth vec4 normal;
            smooth vec4 annexe;
        } vs_out;

        void main(void)
        {
            vs_out.vertex = _vertex;
            vs_out.texcoord = _texcoord;
            vs_out.normal = _normal;
            vs_out.annexe = _annexe;
        }
    )"};

    /**
     * Default feedback tessellation shader
     */
    const std::string TESS_CTRL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA{R"(
        #include normalVector
        #include projectAndCheckVisibility

        layout (vertices = 3) out;

        in VS_OUT
        {
            smooth vec4 vertex;
            smooth vec2 texcoord;
            smooth vec4 normal;
            smooth vec4 annexe;
        } tcs_in[];

        out TCS_OUT
        {
            vec4 vertex;
            vec2 texcoord;
            vec4 normal;
            vec4 annexe;
        } tcs_out[];

        uniform mat4 _mvp;
        uniform mat4 _mNormal;
        uniform float _blendWidth = 0.1;
        uniform float _blendPrecision = 0.1;

        const float blendDistFactorToSubdiv = 2.0;

        void main(void)
        {
            if (gl_InvocationID == 0)
            {
                bool anyVertexVisible = false;
                vec4 projectedVertices[3];
                float maxDist = 0.0;
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
                        vec2 distToCenter;
                        projectedVertices[i] = tcs_in[i].vertex;
                        if (projectAndCheckVisibility(projectedVertices[i], _mvp, 0.0, distToCenter))
                            anyVertexVisible = true;
                        float localMax = max(distToCenter.x, distToCenter.y);
                        if (localMax > maxDist)
                        {
                            maxDist = localMax;
                            nearestBorder = float(distToCenter.y > distToCenter.x);
                        }
                    }

                    // Also check for the middle of the edges, to improve the handling of larger faces
                    for (int i = 0; i < 3; ++i)
                    {
                        vec2 distToCenter;
                        vec4 middlePoint = (projectedVertices[i] + projectedVertices[(i + 1) % 3]) / 2.0;
                        if (projectAndCheckVisibility(middlePoint, _mvp, 0.0, distToCenter))
                            anyVertexVisible = true;
                    }

                    vec3 projectedNormal = normalVector(projectedVertices[0].xyz, projectedVertices[1].xyz, projectedVertices[2].xyz);
                    if (anyVertexVisible && projectedNormal.z >= 0.0)
                    {
                        if (1.0 - maxDist < _blendWidth * blendDistFactorToSubdiv)
                        {
                            vec2 nearestBorderNormal = nearestBorder * vec2(1.0, 0.0) + (1.0 - nearestBorder) * vec2(0.0, 1.0);
                            float maxTessLevel = 1.0;
                            for (int idx = 0; idx < 3; idx++)
                            {
                                int nextIdx = (idx + 1) % 3;
                                vec2 edge = projectedVertices[nextIdx].xy - projectedVertices[idx].xy;
                                float edgeProjectedLength = abs(dot(edge, nearestBorderNormal));
                                float tessLevel = max(1.0, ((edgeProjectedLength + length(edge)) * 0.5) / _blendPrecision);
                                tessLevel = mix(1.0, tessLevel, smoothstep(1.0 - min(1.0, blendDistFactorToSubdiv * _blendWidth), 1.0, maxDist));
                                maxTessLevel = max(maxTessLevel, tessLevel);
                                gl_TessLevelOuter[(idx + 2) % 3] = tessLevel;
                            }

                            gl_TessLevelInner[0] = maxTessLevel;
                        }
                    }
                }
            }

            tcs_out[gl_InvocationID].vertex = tcs_in[gl_InvocationID].vertex;
            tcs_out[gl_InvocationID].texcoord = tcs_in[gl_InvocationID].texcoord;
            tcs_out[gl_InvocationID].normal = tcs_in[gl_InvocationID].normal;
            tcs_out[gl_InvocationID].annexe = tcs_in[gl_InvocationID].annexe;

            gl_out[gl_InvocationID].gl_Position = tcs_out[gl_InvocationID].vertex;
        }
    )"};

    const std::string TESS_EVAL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA{R"(
        //layout (triangles, fractional_odd_spacing) in;
        layout (triangles) in;

        in TCS_OUT
        {
            vec4 vertex;
            vec2 texcoord;
            vec4 normal;
            vec4 annexe;
        } tes_in[];

        out TES_OUT
        {
            vec4 vertex;
            vec2 texcoord;
            vec4 normal;
            vec4 annexe;
        } tes_out;

        void main(void)
        {
            tes_out.vertex = (gl_TessCoord.x * tes_in[0].vertex) +
                             (gl_TessCoord.y * tes_in[1].vertex) +
                             (gl_TessCoord.z * tes_in[2].vertex);
            tes_out.texcoord = (gl_TessCoord.x * tes_in[0].texcoord) +
                               (gl_TessCoord.y * tes_in[1].texcoord) +
                               (gl_TessCoord.z * tes_in[2].texcoord);
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
        #include normalVector
        #include projectAndCheckVisibility

        in TES_OUT
        {
            vec4 vertex;
            vec2 texcoord;
            vec4 normal;
            vec4 annexe;
        } geom_in[];

        out GEOM_OUT
        {
            vec4 vertex;
            vec2 texcoord;
            vec4 normal;
            vec4 annexe;
        } geom_out;

        layout (triangles) in;
        layout (triangle_strip, max_vertices = 9) out;

        const int cutTable[6*9] = {
            0, 3, 4, 3, 1, 4, 1, 2, 4,
            0, 3, 4, 3, 1, 4, 4, 2, 0,
            0, 1, 3, 3, 4, 0, 3, 2, 4,
            0, 1, 3, 3, 4, 0, 3, 2, 4,
            0, 3, 4, 3, 1, 4, 4, 2, 0,
            0, 3, 4, 3, 1, 4, 1, 2, 4
        };

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
                    geom_out.texcoord = geom_in[i].texcoord;
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
                vec2 texcoords[6];
                vec4 normals[6];
                vec4 annexes[6];
                for (int i = 0; i < 3; ++i)
                {
                    vertices[i] = geom_in[i].vertex;
                    texcoords[i] = geom_in[i].texcoord;
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
                        texcoords[nextVertex] = mix(texcoords[i], texcoords[nextId], ratio);
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
                        geom_out.texcoord = texcoords[currentIndex];
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
     * Default vertex shader
     */
    const std::string VERTEX_SHADER_DEFAULT{R"(
        #include getSmoothBlendFromVertex

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        layout(location = 2) in vec4 _normal;
        layout(location = 3) in vec4 _annexe;

        uniform mat4 _modelViewProjectionMatrix;
        uniform mat4 _normalMatrix;
        uniform vec2 _cameraAttributes = vec2(0.05, 1.0); // blendWidth and brightness

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
            vertexOut.texCoord = _texcoord;
            vertexOut.annexe = _annexe;
        }
    )"};

    /**
     * Filter vertex shader
     */
    const std::string VERTEX_SHADER_FILTER{R"(
        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.xyz, 1.0);
            texCoord = _texcoord;
        }
    )"};

    /**
     * Fragment shader for filters
     */
    const std::string FRAGMENT_SHADER_FILTER{R"(
        #include hsv
        #include yuv

        #define PI 3.14159265359

    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif

        in vec2 texCoord;
        out vec4 fragColor;

        uniform vec2 _tex0_size = vec2(1.0);
        // Texture transformation
        uniform int _tex0_flip = 0;
        uniform int _tex0_flop = 0;
        // Format specific parameters
        uniform int _tex0_YCoCg = 0;
        uniform int _tex0_YUV = 0; // 1 = UYVY, 2 = YUYV

        // Film uniforms
        uniform float _filmDuration = 0.f;
        uniform float _filmRemaining = 0.f;

        // Filter parameters
        uniform float _blackLevel = 0.f;
        uniform float _brightness = 1.f;
        uniform float _contrast = 1.f;
        uniform float _saturation = 1.f;
        uniform int _invertChannels = 0;
        uniform vec2 _colorBalance = vec2(1.f, 1.f);

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

            // If the color is expressed as YCoCg (for HapQ compression), extract RGB color from it
            if (_tex0_YCoCg == 1)
            {
                float scale = (color.z * (255.0 / 8.0)) + 1.0;
                float Co = (color.x - (0.5 * 256.0 / 255.0)) / scale;
                float Cg = (color.y - (0.5 * 256.0 / 255.0)) / scale;
                float Y = color.w;
                color.rgba = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);
                color.rgb = pow(color.rgb, vec3(2.2));
            }

            // If the color format is YUYV
            if (_tex0_YUV > 0)
            {
                // Texture coord rounded to the closer even pixel
                ivec2 yuyvCoords = ivec2((int(realCoords.x * _tex0_size.x) / 2) * 2, int(realCoords.y * _tex0_size.y));
                vec4 yuyv;
                yuyv.rg = texelFetch(_tex0, yuyvCoords, 0).rg;
                yuyv.ba = texelFetch(_tex0, ivec2(yuyvCoords.x + 1, yuyvCoords.y), 0).rg;

                if (_tex0_YUV == 1)
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
            float maxBalanceRatio = max(_colorBalance.r, _colorBalance.g);
            color.r *= _colorBalance.r / maxBalanceRatio;
            color.g *= 1.0 / maxBalanceRatio;
            color.b *= _colorBalance.g / maxBalanceRatio;

            if (_brightness != 1.f || _saturation != 1.f || _contrast != 1.f)
            {
                vec3 hsv = rgb2hsv(color.rgb);
                // Brightness correction
                hsv.z *= _brightness;
                // Saturation
                hsv.y = min(1.0, hsv.y * _saturation);
                // Contrast correction
                hsv.z = (hsv.z - 0.5f) * _contrast + 0.5f;
                hsv.z = min(1.0, hsv.z);
                color.rgb = hsv2rgb(hsv);
            }

            // Black level
            if (_blackLevel != 0.0)
                color.rgb = color.rgb * (1.0 - _blackLevel) + _blackLevel;

            // Color curves
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

            fragColor = color;
        }
    )"};

    /**
     * Warp vertex shader
     */
    const std::string VERTEX_SHADER_WARP{R"(
        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.x, _vertex.y, _vertex.z, 1.0);
            texCoord = _texcoord;
        }
    )"};

    /**
     * Fragment shader for warp
     */
    const std::string FRAGMENT_SHADER_WARP{R"(

        #define PI 3.14159265359

    #ifdef TEXTURE_RECT
        uniform sampler2DRect _tex0;
    #else
        uniform sampler2D _tex0;
    #endif

        in vec2 texCoord;
        out vec4 fragColor;

        uniform vec2 _tex0_size = vec2(1.0);
        // Texture transformation
        uniform int _tex0_flip = 0;
        uniform int _tex0_flop = 0;
        // HapQ specific parameters
        uniform int _tex0_YCoCg = 0;

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

            // If the color is expressed as YCoCg (for HapQ compression), extract RGB color from it
            if (_tex0_YCoCg == 1)
            {
                float scale = (color.z * (255.0 / 8.0)) + 1.0;
                float Co = (color.x - (0.5 * 256.0 / 255.0)) / scale;
                float Cg = (color.y - (0.5 * 256.0 / 255.0)) / scale;
                float Y = color.w;
                color.rgba = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);
                color.rgb = pow(color.rgb, vec3(2.2));
            }

            fragColor = color;
        }
    )"};

    /**
     * Cubemap objects rendering (used by the virtual screens)
     */
    const std::string VERTEX_SHADER_OBJECT_CUBEMAP{R"(
        #include getSmoothBlendFromVertex

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;

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
            vertexOut.texCoord = _texcoord;
        }
    )"};

    const std::string GEOMETRY_SHADER_OBJECT_CUBEMAP{R"(
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
        #ifdef TEXTURE_RECT
            uniform sampler2DRect _tex0;
        #else
            uniform sampler2D _tex0;
        #endif

            uniform vec2 _tex0_size = vec2(1.0);

            in VertexData
            {
                vec4 position;
                vec2 texCoord;
            } vertexIn;

            out vec4 fragColor;

            void main(void)
            {
        #if TEXTURE_RECT
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
        #include getSmoothBlendFromVertex

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        layout(location = 2) in vec4 _normal;
        layout(location = 3) in vec4 _annexe;

        uniform mat4 _modelViewProjectionMatrix;
        uniform mat4 _normalMatrix;
        uniform vec2 _cameraAttributes = vec2(0.05, 1.0); // blendWidth and brightness

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
            vertexOut.texCoord = _texcoord;
            vertexOut.annexe = _annexe;

            vec4 projectedVertex = vertexOut.position / vertexOut.position.w;
            if (projectedVertex.z >= 0.0)
            {
                if (_annexe.y == 0.0)
                    vertexOut.blendingValue = 1.0;
                else
                    vertexOut.blendingValue = min(1.0, getSmoothBlendFromVertex(projectedVertex, _cameraAttributes.x) / _annexe.y);
            }
        }
    )"};

    /**
     * Textured fragment shader
     */
    const std::string FRAGMENT_SHADER_TEXTURE{R"(
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

        uniform vec2 _tex0_size = vec2(1.0);
        uniform vec2 _tex1_size = vec2(1.0);

        uniform int _showCameraCount = 0;
        uniform int _sideness = 0;
        uniform int _textureNbr = 0;
        uniform vec2 _cameraAttributes = vec2(0.05, 1.0); // blendWidth and brightness
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g
        uniform int _isColorLUT = 0;
        uniform vec4 _color = vec4(0.0, 0.0, 0.0, 1.0);
        uniform vec3 _colorLUT[256];
        uniform mat3 _colorMixMatrix = mat3(1.0, 0.0, 0.0,
                                            0.0, 1.0, 0.0,
                                            0.0, 0.0, 1.0);
        uniform float _normalExp = 0.0;

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
            color.rgb = color.rgb * brightness;

            // Color correction through a LUT
            if (_isColorLUT != 0)
            {
                ivec3 icolor = ivec3(round(color.rgb * 255.f));
                color.rgb = vec3(_colorLUT[icolor.r].r, _colorLUT[icolor.g].g, _colorLUT[icolor.b].b);
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
        #define PI 3.14159265359

        uniform int _sideness = 0;
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g
        uniform vec4 _color = vec4(0.0, 1.0, 0.0, 1.0);

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
        #define PI 3.14159265359

        uniform int _sideness = 0;
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g

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
        #define PI 3.14159265359

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec4 normal;
            vec4 annexe;
        } vertexIn;

        out vec4 fragColor;

        uniform vec2 _cameraAttributes = vec2(0.05, 1.0); // blendWidth and brightness
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g

        void main(void)
        {
            int index = int(round(vertexIn.annexe.w));
            ivec2 components = ivec2(index) / ivec2(65025, 255);
            components.y -= components.x * 255;
            fragColor = vec4(float(components.x) / 255.0, float(components.y) / 255.0, float(index % 255) / 255.0, 1.0);
        }
    )"};

    /**
     * Wireframe rendering
     */
    const std::string VERTEX_SHADER_WIREFRAME{R"(
        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        layout(location = 2) in vec4 _normal;

        out VertexData
        {
            vec4 vertex;
            vec2 texcoord;
        } vertexOut;

        void main()
        {
            vertexOut.vertex = _vertex;
            vertexOut.texcoord = _texcoord;
        }
    )"};

    const std::string GEOMETRY_SHADER_WIREFRAME{R"(
        layout(triangles) in;
        layout(triangle_strip, max_vertices = 3) out;
        uniform mat4 _modelViewProjectionMatrix;

        in VertexData
        {
            vec4 vertex;
            vec2 texcoord;
        } vertexIn[];

        out VertexData
        {
            vec2 texcoord;
            vec3 bcoord;
            vec4 position;
        } vertexOut;

        void main()
        {
            vec4 v = _modelViewProjectionMatrix * vec4(vertexIn[0].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[0].texcoord;
            vertexOut.bcoord = vec3(1.0, 0.0, 0.0);
            vertexOut.position = v;
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[1].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[1].texcoord;
            vertexOut.bcoord = vec3(0.0, 1.0, 0.0);
            vertexOut.position = v;
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[2].vertex.xyz, 1.0);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[2].texcoord;
            vertexOut.bcoord = vec3(0.0, 0.0, 1.0);
            vertexOut.position = v;
            EmitVertex();

            EndPrimitive();
        }
    )"};

    const std::string FRAGMENT_SHADER_WIREFRAME{R"(
        #define PI 3.14159265359

        in VertexData
        {
            vec2 texcoord;
            vec3 bcoord;
            vec4 position;
        } vertexIn;

        uniform vec4 _wireframeColor = vec4(1.0, 0.0, 0.0, 1.0);
        uniform int _sideness = 0;
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g
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
        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.xyz, 1.0);
            texCoord = _texcoord;
        }
    )"};

    const std::string FRAGMENT_SHADER_CUBEMAP_PROJECTION{R"(
        #define PI 3.141592653589793
        #define HALFPI 1.5707963268 

        uniform samplerCube _tex0;
        uniform vec2 _tex0_size = vec2(1.0);

        uniform int _projectionType = 1; // 0 = equirectangular, 1 = spherical
        uniform float _sphericalFov = 180.0;

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
        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;

        out VertexData
        {
            vec4 vertex;
            vec2 texcoord;
        } vertexOut;

        void main()
        {
            vertexOut.vertex = _vertex;
            vertexOut.texcoord = _texcoord;
        }
    )"};

    const std::string GEOMETRY_SHADER_WARP_WIREFRAME{R"(
        layout(triangles) in;
        layout(triangle_strip, max_vertices = 3) out;

        in VertexData
        {
            vec4 vertex;
            vec2 texcoord;
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
        #define PI 3.14159265359

        in VertexData
        {
            vec3 bcoord;
        } vertexIn;

        uniform int _sideness = 0;
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g
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
        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        //layout(location = 2) in vec3 _normal;
        //uniform mat4 _modelViewProjectionMatrix;
        out vec2 texCoord;

        void main(void)
        {
            gl_Position = vec4(_vertex.x, _vertex.y, _vertex.z, 1.0);
            texCoord = _texcoord;
        }
    )"};

    const std::string FRAGMENT_SHADER_WINDOW{R"(
        #define PI 3.14159265359

    #ifdef TEX_1
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
        uniform int _textureNbr = 0;
        uniform ivec4 _layout = ivec4(0, 1, 2, 3);
        uniform vec2 _gamma = vec2(1.0, 2.2);
        in vec2 texCoord;
        out vec4 fragColor;

        void main(void)
        {
            float frames = float(_textureNbr);
            for (int i = 0; i < _textureNbr; ++i)
            {
                int value = _layout[i];
                for (int j = i + 1; j < _textureNbr; ++j)
                {
                    if (_layout[j] == value)
                    {
                        frames--;
                        break;
                    }
                }
            }

            fragColor.rgba = vec4(0.0);
    #ifdef TEX_1
            if (_textureNbr > 0 && texCoord.x > float(_layout[0]) / frames && texCoord.x < (float(_layout[0]) + 1.0) / frames)
            {
                fragColor = texture(_tex0, vec2((texCoord.x - float(_layout[0]) / frames) * frames, texCoord.y));
            }
    #ifdef TEX_2
            if (_textureNbr > 1 && texCoord.x > float(_layout[1]) / frames && texCoord.x < (float(_layout[1]) + 1.0) / frames)
            {
                vec4 color = texture(_tex1, vec2((texCoord.x - float(_layout[1]) / frames) * frames, texCoord.y));
                fragColor.rgb = mix(fragColor.rgb, color.rgb, color.a);
                fragColor.a = max(fragColor.a, color.a);
            }
    #ifdef TEX_3
            if (_textureNbr > 2 && texCoord.x > float(_layout[2]) / frames && texCoord.x < (float(_layout[2]) + 1.0) / frames)
            {
                vec4 color = texture(_tex2, vec2((texCoord.x - float(_layout[2]) / frames) * frames, texCoord.y));
                fragColor.rgb = mix(fragColor.rgb, color.rgb, color.a);
                fragColor.a = max(fragColor.a, color.a);
            }
    #ifdef TEX_4
            if (_textureNbr > 3 && texCoord.x > float(_layout[3]) / frames && texCoord.x < (float(_layout[3]) + 1.0) / frames)
            {
                vec4 color = texture(_tex3, vec2((texCoord.x - float(_layout[3]) / frames) * frames, texCoord.y));
                fragColor.rgb = mix(fragColor.rgb, color.rgb, color.a);
                fragColor.a = max(fragColor.a, color.a);
            }
    #endif
    #endif
    #endif
    #endif

            if (_gamma.x != 1.0)
                fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / _gamma.y));
        }
    )"};

} ShaderSources;

} // end of namespace

#endif // SPLASH_SHADERSOURCES_H
