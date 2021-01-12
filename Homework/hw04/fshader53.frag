/*
File Name: "fshader53.glsl":
           Fragment Shader
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation
              // errors due to different settings of the default GLSL version

in vec4 color;
in vec2 texCoord;
in vec2 latticeCoord;
in float Distance;
out vec4 fColor;

uniform int Fog;      // 0 - no fog; 1 - linear; 2 - exponential; 3 - exp square
uniform int Lattice;  // 0 - no lattice; 1 - lattice effect
uniform int Texture;  // 0 - no texture;
                      // 1 - 2D texture for ground;
                      // 2 - 1D texture for sphere;
                      // 3 - 2D texture for sphere;
uniform sampler1D StripeTexture;
uniform sampler2D GroundTexture;

uniform float FogStart;
uniform float FogEnd;
uniform float FogDensity;

uniform vec4 FogColor;

void main() {
    fColor = color;
    if (Texture == 1) {
        fColor *= texture(GroundTexture, texCoord);
    } else if (Texture == 2) {
        fColor *= texture(StripeTexture, texCoord.s);
    } else if (Texture == 3) {
        vec4 temp_color = texture(GroundTexture, texCoord);
        if (temp_color.r == 0) {
            fColor *= vec4(0.9, 0.1, 0.1, 1.0);
        }
    }

    if (Lattice == 1) {
        if (fract(4 * latticeCoord.s) < 0.35 &&
            fract(4 * latticeCoord.t) < 0.35) {
            discard;
        }
    }

    float FogFactor = 1.0;
    if (Fog == 1) {
        FogFactor = (FogEnd - Distance) / (FogEnd - FogStart);
    } else if (Fog == 2) {
        FogFactor = exp(-FogDensity * Distance);
    } else if (Fog == 3) {
        FogFactor = exp(-pow(FogDensity * Distance, 2));
    }
    FogFactor = clamp(FogFactor, 0.0, 1.0);
    fColor = mix(FogColor, fColor, FogFactor);
}
