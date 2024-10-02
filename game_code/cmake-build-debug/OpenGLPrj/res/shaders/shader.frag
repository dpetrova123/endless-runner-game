#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoord;

// texture samplers
uniform vec3 MyColor;

uniform bool useTexture;
uniform bool endGame;

uniform sampler2D diffuseTexture;

uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 cameraPos;

void main()
{
        if(!endGame) {
                vec4 texColor;
                if (useTexture) {
                        vec4 diffuseColor = texture(diffuseTexture, TexCoord);

                        vec4 finalColor = vec4(diffuseColor.rgb, 1.0);

                        texColor = finalColor;
                } else {
                        texColor = vec4(MyColor, 1.0f);
                }

                float distance = length(FragPos - cameraPos);

                float fogFactor = (fogEnd - distance) / (fogEnd - fogStart);
                fogFactor = clamp(fogFactor, 0.0, 1.0);

                vec4 foggedColor = mix(vec4(fogColor, 1.0), texColor, fogFactor);

                FragColor = foggedColor;
        }
        else {
                FragColor = vec4(MyColor, 1.0f);
        }
}
