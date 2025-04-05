#version 320 es

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
    vec3 bcoord;
} vertexIn;

out vec4 fragColor;

float edgeFactor()
{
    vec3 d = fwidth(vertexIn.bcoord);
    vec3 a3 = smoothstep(vec3(0.0), d * 1.5, vertexIn.bcoord);
    return min(min(a3.x, a3.y), a3.z);
}

vec4 wireframeColor = vec4(1.0);

void main(void)
{
    vec4 position = vertexIn.position;
    vec2 texCoord = vertexIn.texCoord;

#ifdef TEXTURE_RECT
    vec4 color = texture(_tex0, texCoord * _tex0_size);
#else
    vec4 color = texture(_tex0, texCoord);
#endif

    fragColor.rgb = color.rgb;
    fragColor.a = 1.0;

    vec3 b = vertexIn.bcoord;
    float minDist = min(min(b[0], b[1]), b[2]);
    vec4 matColor = vec4(0.3, 0.3, 0.3, 1.0);
    fragColor.rgba = mix(wireframeColor, fragColor, edgeFactor());
}
