#version 320 es

precision mediump float;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
uniform mat4 _modelViewProjectionMatrix;

in VertexData
{
    vec4 position;
    vec2 texCoord;
    vec4 normal;
    vec4 annexe;
    float blendingValue;
} vertexIn[];

out VertexData
{
    vec4 position;
    vec2 texCoord;
    vec3 bcoord;
}
vertexOut;

void main()
{
    vertexOut.position = vertexIn[0].position;
    vertexOut.texCoord = vertexIn[0].texCoord;
    vertexOut.bcoord = vec3(1.0, 0.0, 0.0);
    gl_Position = vertexIn[0].position;
    EmitVertex();

    vertexOut.position = vertexIn[1].position;
    vertexOut.texCoord = vertexIn[1].texCoord;
    vertexOut.bcoord = vec3(0.0, 1.0, 0.0);
    gl_Position = vertexIn[1].position;
    EmitVertex();

    vertexOut.position = vertexIn[2].position;
    vertexOut.texCoord = vertexIn[2].texCoord;
    vertexOut.bcoord = vec3(0.0, 0.0, 1.0);
    gl_Position = vertexIn[2].position;
    EmitVertex();

    EndPrimitive();
}
