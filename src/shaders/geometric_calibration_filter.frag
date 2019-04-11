#version 450 core

uniform sampler2D _tex0; // black texture
uniform sampler2D _tex1; // grey texture
uniform sampler2D _tex2; // pattern texture

uniform int subdivs = 0;
uniform ivec4 texLayout = ivec4(0);

in vec2 texCoord;
out vec4 fragColor;

void main(void)
{
    float subdivs_f = float(subdivs);
    float width = 1.0 / subdivs_f;

    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    for (int i = 0; i < subdivs; ++i)
    {
        vec2 tc = vec2((texCoord.x - width * float(i)) * subdivs_f, texCoord.y);
        if (tc.x < 0.0)
            continue;

        fragColor = vec4(tc, 0.0, 1.0);
        if (texLayout[i] == 0)
            fragColor = texture(_tex0, tc);
        else if (texLayout[i] == 1)
            fragColor = texture(_tex1, tc);
        else
            fragColor = texture(_tex2, tc);
    }
}
