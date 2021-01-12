/*
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation
              // errors due to different settings of the default GLSL version

in vec4 vPosition;
in vec4 vColor;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 color;
out float Distance;
out vec2 texCoord;
out vec2 latticeCoord;

uniform vec4 GlobalAmbient;
uniform vec4 DistantAmbient, DistantDiffuse, DistantSpecular;
uniform vec4 PositionalAmbient, PositionalDiffuse, PositionalSpecular;
uniform vec4 MaterialAmbient, MaterialDiffuse, MaterialSpecular;
uniform vec4 LightPosition, DistantLightDirection, SpotlightDestination;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat3 Normal_Matrix;

uniform float Shininess;
uniform float SpotlightExponent;

uniform float ConstAtt;   // Constant Attenuation
uniform float LinearAtt;  // Linear Attenuation
uniform float QuadAtt;    // Quadratic Attenuation

uniform int Lighting;       // Lighting Flag
uniform int SmoothShading;  // Smooth Shading Flag
uniform int Spotlight;      // Spotlight Flag

uniform int ObjSpace;  // Object Space Flag
uniform int Vertical;  // Vertical / Slanted Flag
uniform int Upright;   // Upright / Titled Flag
uniform int Texture;   // 0 - no texture;
                       // 1 - 2D texture for ground;
                       // 2 - 1D texture for sphere;
                       // 3 - 2D texture for sphere;

void main() {
    if (Lighting == 1) {
        // Transform vertex position into eye coordinates
        vec3 pos = (ModelView * vPosition).xyz;
        float dist = length(LightPosition.xyz - pos);

        vec3 L_p = normalize(LightPosition.xyz - pos);
        vec3 L_d = normalize(-DistantLightDirection.xyz);
        vec3 E = normalize(-pos);
        vec3 H_p = normalize(L_p + E);
        vec3 H_d = normalize(L_d + E);

        // Transform vertex normal into eye coordinates
        vec3 N;
        if (SmoothShading == 1) {
            N = normalize(pos - (ModelView * vec4(0, 0, 0, 1)).xyz);
        } else {
            N = normalize(Normal_Matrix * vNormal);
        }

        // YJC Note: N must use the one pointing *toward* the viewer
        //     ==> If (N dot E) < 0 then N must be changed to -N
        //
        if (dot(N, E) < 0) N = -N;

        // Compute attenuation
        float attenuation =
            1.0 / (ConstAtt + dist * LinearAtt + dist * dist * QuadAtt);
        if (Spotlight == 1) {
            vec3 L_f = normalize(SpotlightDestination - LightPosition).xyz;
            vec3 P = normalize(pos - LightPosition.xyz);
            float theta = acos(dot(L_f, P));
            if (theta > radians(20)) {
                attenuation = 0.0;
            } else {
                attenuation *= pow(dot(L_f, P), SpotlightExponent);
            }
        }

        // Compute terms in the illumination equation
        vec4 global = GlobalAmbient * MaterialAmbient;
        vec4 distant_ambient = DistantAmbient * MaterialAmbient;
        vec4 positional_ambient = PositionalAmbient * MaterialAmbient;

        float d_distant = max(dot(L_d, N), 0.0);
        vec4 distant_diffuse = d_distant * DistantDiffuse * MaterialDiffuse;
        float d_postional = max(dot(L_p, N), 0.0);
        vec4 positional_diffuse =
            d_postional * PositionalDiffuse * MaterialDiffuse;

        float s_distant = pow(max(dot(N, H_d), 0.0), Shininess);
        vec4 distant_specular = s_distant * DistantSpecular * MaterialSpecular;
        float s_positional = pow(max(dot(N, H_p), 0.0), Shininess);
        vec4 positional_specular =
            s_positional * PositionalSpecular * MaterialSpecular;

        if (dot(L_d, N) < 0.0) {
            distant_specular = vec4(0.0, 0.0, 0.0, 1.0);
        }
        if (dot(L_p, N) < 0.0) {
            positional_specular = vec4(0.0, 0.0, 0.0, 1.0);
        }

        /*--- attenuation below must be computed properly ---*/
        color = global + distant_ambient + distant_diffuse + distant_specular +
                attenuation * (positional_ambient + positional_diffuse +
                               positional_specular);
    } else {
        color = vColor;
    }

    gl_Position = Projection * ModelView * vPosition;

    vec4 posForTexture = vPosition;
    if (ObjSpace == 0) posForTexture = ModelView * posForTexture;

    if (Texture == 2) {
        if (Vertical == 1) {
            texCoord = vec2(2.5 * posForTexture.x, 0);
        } else {
            texCoord = vec2(
                1.5 * (posForTexture.x + posForTexture.y + posForTexture.z), 0);
        }
    } else if (Texture == 3) {
        if (Vertical == 1) {
            texCoord = vec2(0.75 * (posForTexture.x + 1),
                            0.75 * (posForTexture.y + 1));
        } else {
            texCoord = vec2(
                0.45 * (posForTexture.x + posForTexture.y + posForTexture.z),
                0.45 * (posForTexture.x - posForTexture.y + posForTexture.z));
        }
    } else {
        texCoord = vTexCoord;
    }

    if (Upright == 1) {
        latticeCoord = vec2(0.5 * (vPosition.x + 1), 0.5 * (vPosition.y + 1));
    } else {
        latticeCoord = vec2(0.3 * (vPosition.x + vPosition.y + vPosition.z),
                            0.3 * (vPosition.x - vPosition.y + vPosition.z));
    }

    Distance = length(ModelView * vPosition);
}
